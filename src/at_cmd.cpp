/**
 * @file	at_cmd.cpp
 * @brief	Implements AT commands handling on the RTOS level.
 * @author	Kacper Kowalski - kacper.s.kowalski@gmail.com
 */
#include "at_cmd.hpp"
#include "FreeRTOS.h"
#include "at_cmd_def.hpp"
#include "at_cmd_handler.hpp"
#include "hw_at.h"
#include "neither/neither.hpp"
#include "os.h"
#include "os_lockguard.hpp"
#include "os_queue.hpp"
#include "semphr.h"
#include "string_buf_rx.hpp"
#include "string_buf_tx.hpp"
#include "task.h"

// --------------------------------------------------------------------------------------------------------------------
// DEFINITIONS OF STRUCTURES, DATA TYPES, ...
// --------------------------------------------------------------------------------------------------------------------

#define CTRL_Z_STR "\x1A"

struct at_work_result_struct
{
    at_cmd command;
    at_err result;
    std::string payload;

    at_work_result_struct(at_cmd command, at_err result, std::string &&payload)
        : command(command), result(result), payload(std::move(payload))
    {
    }
};

struct at_prompt_msg_struct
{
    at_prompt_end_policy policy;
    std::string prompt_message;
    bool valid = false;

    void set(at_prompt_end_policy prompt_end_policy, std::string &&message)
    {
        policy = prompt_end_policy;
        prompt_message = std::move(message);
        valid = true;
    }
};

// --------------------------------------------------------------------------------------------------------------------
// DECLARATION OF PRIVATE FUNCTIONS AND VARIABLES
// --------------------------------------------------------------------------------------------------------------------
static at_cmd_handler cmd_handler;

constexpr size_t rx_buf_len = AT_CMD_HANDLER_RX_BUFLEN;

#ifdef AT_CMD_HANDLER_NO_NEWLINE_AFTER_PROMPT
static string_buf_rx<rx_buf_len> rx_buf(">");
#elif /* AT_CMD_HANDLER_NO_NEWLINE_AFTER_PROMPT */
static string_buf_rx<rx_buf_len> rx_buf;
#endif /* AT_CMD_HANDLER_NO_NEWLINE_AFTER_PROMPT */
static string_buf_tx tx_buf;

static TaskHandle_t at_rx_task_handle;

//! Used to handle a single command at once.
static SemaphoreHandle_t at_mux;

//! Used to guard acces to at_cmd_handler.
static SemaphoreHandle_t at_cmd_handler_mux;

static os_queue<at_cmd, 1> at_depute_work_queue;
static os_queue<at_work_result_struct, 1> at_work_result_queue;

static at_prompt_msg_struct at_prompt_data;

static void at_rx_task(void *);

template <typename... TransmitCommandArgs>
static at_err at_send_and_get_response(at_cmd command,
                                       std::string &response_payload,
                                       TickType_t ticks_to_wait,
                                       TransmitCommandArgs... transmit_command_args);
static void transmit_command(std::string &&prefix);
static void transmit_command(std::string &&prefix, std::string &&payload);
static void handle_received_response(std::unique_ptr<std::string> response);
template <typename... T> static void register_unsolicited_handler(T &&... args);
static void handle_prompt_request();

// --------------------------------------------------------------------------------------------------------------------
// DEFINITION OF PUBLIC FUNCTIONS AND VARIABLES
// --------------------------------------------------------------------------------------------------------------------
extern "C" void init_at();
void init_at()
{
    xTaskCreate(at_rx_task, "at_rx", 1024, NULL, 1, &at_rx_task_handle);
    at_mux = xSemaphoreCreateMutex();
    at_cmd_handler_mux = xSemaphoreCreateMutex();
}

extern "C" void deinit_at();
void deinit_at()
{
    vTaskDelete(at_rx_task_handle);
    vSemaphoreDelete(at_mux);
    vSemaphoreDelete(at_cmd_handler_mux);
}

at_err at_send(at_cmd command, std::string &&payload, TickType_t ticks_to_wait, std::string &response_payload)
{
    auto command_prefix = at_cmd_handler::prepare_cmd_prefix_to_transmit(command, at_cmd_type::write);
    return at_send_and_get_response(
        command, response_payload, ticks_to_wait, std::move(command_prefix), std::move(payload));
}

at_err at_send(at_cmd command, std::string &&payload, TickType_t ticks_to_wait)
{
    std::string dummy_pload;
    auto command_prefix = at_cmd_handler::prepare_cmd_prefix_to_transmit(command, at_cmd_type::write);
    return at_send_and_get_response(command, dummy_pload, ticks_to_wait, std::move(command_prefix), std::move(payload));
}

at_err at_send(at_cmd command, at_cmd_type command_type, TickType_t ticks_to_wait, std::string &response_payload)
{
    auto command_prefix = at_cmd_handler::prepare_cmd_prefix_to_transmit(command, command_type);
    return at_send_and_get_response(command, response_payload, ticks_to_wait, std::move(command_prefix));
}

at_err at_send(at_cmd command, at_cmd_type command_type, TickType_t ticks_to_wait)
{
    std::string dummy_pload;
    auto command_prefix = at_cmd_handler::prepare_cmd_prefix_to_transmit(command, command_type);
    return at_send_and_get_response(command, dummy_pload, ticks_to_wait, std::move(command_prefix));
}

at_err at_send_prompted(at_cmd command,
                        std::string payload,
                        std::string prompt_message,
                        at_prompt_end_policy policy,
                        TickType_t ticks_to_wait)
{
    std::string dummy_pload;
    auto command_prefix = at_cmd_handler::prepare_cmd_prefix_to_transmit(command, at_cmd_type::write);
    at_prompt_data.set(policy, std::move(prompt_message));
    return at_send_and_get_response(command, dummy_pload, ticks_to_wait, std::move(command_prefix), std::move(payload));
}

void at_register_unsolicited_handler(at_cmd command,
                                     std::function<bool(std::unique_ptr<std::string> response_payload)> handler)
{
    register_unsolicited_handler(command, handler);
}

void at_register_unsolicited_handler(at_unsolicited_msg unsolicited_msg, std::function<bool(void)> handler)
{
    register_unsolicited_handler(unsolicited_msg, handler);
}

extern "C" void it_handle_at_byte_rx(char c);
void it_handle_at_byte_rx(char c)
{
    // Notify the receiver task on the command end.
    if (rx_buf.push_byte_and_is_string_end(c))
        notify_from_isr(at_rx_task_handle);
}

extern "C" void it_handle_at_byte_tx();
void it_handle_at_byte_tx()
{
    if (tx_buf.is_empty())
        hw_at_disable_tx_it();
    else
        hw_at_send_byte(tx_buf.pop_byte());
}

// --------------------------------------------------------------------------------------------------------------------
// DEFINITION OF PRIVATE FUNCTIONS
// --------------------------------------------------------------------------------------------------------------------
static void at_rx_task(void *)
{
    hw_at_enable_rx_it();
    for (;;)
    {
        auto notif_num = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        while (notif_num--)
        {
            auto response = rx_buf.pop_string();
            if (response->length() == 0)
                continue;

            handle_received_response(std::move(response));
        }
    }
}

static void handle_received_response(std::unique_ptr<std::string> response)
{
    static std::string response_payload;
    static at_cmd awaited_cmd = at_cmd::none;

    neither::Either<at_cmd, bool> work_input = at_depute_work_queue.receive(0);
    if (work_input.isLeft)
    {
        // If a new command is being awaited then this string must be cleared in case of that the previous
        // command hasn't been handled on time.
        response_payload.clear();
        // The task which deputes handling a new command passes a command name as a work input.
        awaited_cmd = work_input.leftValue;
    }

    at_err res = at_err::unknown;
    {
        os_lockguard guard(at_cmd_handler_mux);
        res = cmd_handler.handle_received_response(std::move(response), awaited_cmd, response_payload);
    }

    if (res == at_err::ok || res == at_err::error || res == at_err::cme_error)
    {
        // Pass the work result.
        at_work_result_queue.overwrite(awaited_cmd, res, std::move(response_payload));
        awaited_cmd = at_cmd::none;
    }
    else if (res == at_err::prompt_request)
        handle_prompt_request();
}

template <typename... TransmitCommandArgs>
static at_err at_send_and_get_response(at_cmd command,
                                       std::string &response_payload,
                                       TickType_t ticks_to_wait,
                                       TransmitCommandArgs... transmit_command_args)
{
    // Only single command at a time can be handled so take this mutex.
    os_lockguard guard(at_mux);
    at_depute_work_queue.overwrite(command);
    
    // Clean the buffer before transmission
    tx_buf.clean();

    transmit_command(std::forward<TransmitCommandArgs>(transmit_command_args)...);

    while (true)
    {
        auto work_output = at_work_result_queue.receive(ticks_to_wait);
        if (work_output.isLeft)
        {
            auto p = work_output.leftValue;
            if (p.command != command)
            {
                // When the received command is not the awaited command then wait for the other result.
                continue;
            }
            else
            {
                response_payload = std::move(p.payload);
                return p.result;
            }
        }
        else
        {
            return at_err::timeout;
        }
    }
}

static void transmit_command(std::string &&prefix)
{
    prefix += "\r\n";
    tx_buf.push_string(std::make_unique<std::string>(std::move(prefix)));
    hw_at_enable_tx_it();
}

static void transmit_command(std::string &&prefix, std::string &&payload)
{
    tx_buf.push_string(std::make_unique<std::string>(std::move(prefix)));
    tx_buf.push_string(std::make_unique<std::string>(std::move(payload)));
    tx_buf.push_string(std::make_unique<std::string>("\r\n"));
    hw_at_enable_tx_it();
}

template <typename... T> static void register_unsolicited_handler(T &&... args)
{
    // We want to enable the registration of the unsolicited handlers before the scheduler is running but we can't
    // use a mutex before the scheduler is running so this check is musthave.
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
    {
        os_lockguard g(at_cmd_handler_mux);
        cmd_handler.register_unsolicited_handler(std::forward<T>(args)...);
    }
    else
        cmd_handler.register_unsolicited_handler(std::forward<T>(args)...);
}

static void handle_prompt_request()
{
    if (!at_prompt_data.valid)
        return;

    std::string suffix;
    if (at_prompt_data.policy == at_prompt_end_policy::ctrl_z)
        suffix = CTRL_Z_STR "\r\n";
    else if (at_prompt_data.policy == at_prompt_end_policy::crlf)
        suffix = "\r\n";

    transmit_command(std::move(at_prompt_data.prompt_message), std::move(suffix));
    at_prompt_data.valid = false;
}
