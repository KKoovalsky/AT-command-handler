/**
 * @file	at_test.cpp
 * @brief	Tests of AT commands handling.
 * @author	Kacper Kowalski - kacper.s.kowalski@gmail.com
 */
#include "at_cmd.hpp"
#include "unity.h"
#include <csignal>
#include <iostream>
#include <list>
#include <string>

// --------------------------------------------------------------------------------------------------------------------
// DECLARATION OF THE TEST CASES
// --------------------------------------------------------------------------------------------------------------------
static void GIVEN_prepared_response_WHEN_at_sent_THEN_response_populated_to_caller_task();
static void GIVEN_sent_command_WHEN_response_not_received_THEN_timeout_error_received();
static void GIVEN_first_command_fails_WHEN_second_successful_THEN_received_proper_response();

// --------------------------------------------------------------------------------------------------------------------
// DECLARATION OF PRIVATE FUNCTIONS AND VARIABLES
// --------------------------------------------------------------------------------------------------------------------

constexpr TickType_t max_wait_time_ticks = pdMS_TO_TICKS(15 * 1000);

static std::list<std::string> mock_responses_on_at_commands;

static bool is_tx_interrupt_enabled;

static void simulated_rx_interrupt(int sig);
static void simulated_tx_interrupt(int sig);

// --------------------------------------------------------------------------------------------------------------------
// EXTERNAL DEPENDENCIES DECLARATION
// --------------------------------------------------------------------------------------------------------------------

#define SIMULATED_RX_INTERRUPT_SIGNAL SIGRTMIN + 3
#define SIMULATED_TX_INTERRUPT_SIGNAL SIGRTMIN + 4

extern "C" void hw_at_enable_tx_it();
extern "C" void hw_at_disable_tx_it();
extern "C" void hw_at_enable_rx_it();
extern "C" void hw_at_disable_rx_it();
extern "C" void hw_at_send_byte(char c);
extern "C" void it_handle_at_byte_rx(char c);
extern "C" void it_handle_at_byte_tx();
extern "C" void init_at();
extern "C" void deinit_at();

// --------------------------------------------------------------------------------------------------------------------
// DEFINITION OF THE TEST CASES
// --------------------------------------------------------------------------------------------------------------------
static void GIVEN_prepared_response_WHEN_at_sent_THEN_response_populated_to_caller_task()
{
    // Given
    mock_responses_on_at_commands.push_back("+FIRST: 0,1\r\n");
    mock_responses_on_at_commands.push_back("OK\r\n");

    // When
    std::string pload;
    auto res = at_send(at_cmd::first, at_cmd_type::read, max_wait_time_ticks, pload);

    // Then
    TEST_ASSERT(res == at_err::ok);
    TEST_ASSERT_EQUAL_STRING("0,1", pload.c_str());
}

static void GIVEN_sent_command_WHEN_response_not_received_THEN_timeout_error_received()
{
    auto res = at_send(at_cmd::first, at_cmd_type::exec, 0);
    TEST_ASSERT(res == at_err::timeout);
}

static void GIVEN_first_command_fails_WHEN_second_successful_THEN_received_proper_response()
{
    // Given
    at_send(at_cmd::second, at_cmd_type::exec, 0);

    // When
    mock_responses_on_at_commands.push_back("OK\r\n");
    auto result = at_send(at_cmd::third, "THIS IS SOME DUMMY PLOAD", max_wait_time_ticks);

    // Then
    TEST_ASSERT(result == at_err::ok);
}

// --------------------------------------------------------------------------------------------------------------------
// EXECUTION OF THE TESTS
// --------------------------------------------------------------------------------------------------------------------
void test_at()
{
    // Register the signal handlers used to simulate the interrupts.
    std::signal(SIMULATED_RX_INTERRUPT_SIGNAL, simulated_rx_interrupt);
    std::signal(SIMULATED_TX_INTERRUPT_SIGNAL, simulated_tx_interrupt);

    init_at();

    RUN_TEST(GIVEN_prepared_response_WHEN_at_sent_THEN_response_populated_to_caller_task);
    RUN_TEST(GIVEN_sent_command_WHEN_response_not_received_THEN_timeout_error_received);
    RUN_TEST(GIVEN_first_command_fails_WHEN_second_successful_THEN_received_proper_response);

    deinit_at();
    // Unregister the signal handlers used to simulate the interrupts.
    std::signal(SIMULATED_RX_INTERRUPT_SIGNAL, SIG_DFL);
    std::signal(SIMULATED_TX_INTERRUPT_SIGNAL, SIG_DFL);
}

// --------------------------------------------------------------------------------------------------------------------
// EXTERNAL DEPENDENCIES DEFINITION
// --------------------------------------------------------------------------------------------------------------------
void hw_at_enable_tx_it()
{
    is_tx_interrupt_enabled = true;
    std::raise(SIMULATED_TX_INTERRUPT_SIGNAL);
}

void hw_at_disable_tx_it()
{
    is_tx_interrupt_enabled = false;
}

void hw_at_enable_rx_it()
{
}

void hw_at_disable_rx_it()
{
}

void hw_at_send_byte(char c)
{
    (void)c;
}

// --------------------------------------------------------------------------------------------------------------------
// DEFINITION OF PRIVATE FUNCTIONS
// --------------------------------------------------------------------------------------------------------------------

static void simulated_rx_interrupt(int sig)
{
    while (mock_responses_on_at_commands.size() > 0)
    {
        // Pop the mocked response
        auto message = mock_responses_on_at_commands.front();
        mock_responses_on_at_commands.pop_front();
        // Invoke the interrupt handler for each byte in the response.
        for (auto c : message)
            it_handle_at_byte_rx(c);
    }
}

static void simulated_tx_interrupt(int sig)
{
    it_handle_at_byte_tx();
    // This interrupt will be enabled until last byte has been sent. Then hw_at_disable_tx_it() will be called which
    // will clear this flag.
    if (is_tx_interrupt_enabled)
        std::raise(SIMULATED_TX_INTERRUPT_SIGNAL);
    // When all bytes has been sent then switch to transmission of a response.
    else
        std::raise(SIMULATED_RX_INTERRUPT_SIGNAL);
}
