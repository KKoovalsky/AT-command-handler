/**
 * @file	at_cmd_handler_test.cpp
 * @brief	Contains unit tests and behaviour driven tests of AT command handler class.
 * @author	Kacper Kowalski - kacper.s.kowalski@gmail.com
 */
#include "at_cmd_handler.hpp"
#include "unity.h"

// --------------------------------------------------------------------------------------------------------------------
// DECLARATION OF THE TEST CASES
// --------------------------------------------------------------------------------------------------------------------
static void GIVEN_at_cmd_handler_WHEN_single_line_response_received_without_prefix_THEN_payload_obtained();
static void GIVEN_at_cmd_handler_WHEN_single_line_response_received_with_prefix_THEN_payload_obtained();

static void UNIT_TEST_at_prepare_cmd_write();
static void UNIT_TEST_at_prepare_cmd_test();
static void UNIT_TEST_at_prepare_cmd_exec();
static void UNIT_TEST_at_prepare_cmd_read();

// TODO: implement handling command which uses a prompt.
// static void UNIT_TEST_at_send_prompted();

static void UNIT_TEST_at_handle_unsolicited_one_shot();
static void UNIT_TEST_at_handle_unsolicited_multiple_times();
static void UNIT_TEST_at_handle_unsolicited_msg();
static void UNIT_TEST_at_handle_unsolicited_no_space_after_colon();

static void
GIVEN_awaited_solicited_WHEN_unsolicited_sent_AND_WHEN_solicited_response_sent_THEN_solicited_pload_obtained();
static void
GIVEN_awaited_solic_and_unsolic_WHEN_unsolicited_sent_AND_WHEN_solicited_resp_sent_THEN_both_ploads_obtained();
static void GIVEN_awaited_solicited_command_WHEN_multiline_response_received_without_prefix_THEN_all_lines_obtained();
static void GIVEN_awaited_solicited_command_WHEN_multiline_response_received_with_prefix_THEN_all_lines_obtained();
static void GIVEN_awaited_solicited_WHEN_multiline_response_received_mixed_with_unsolicited_THEN_all_lines_obtained();
static void GIVEN_awaited_same_solicited_and_unsolicited_WHEN_solicited_response_THEN_only_solicited_handled();
static void GIVEN_awaited_two_unsolicited_WHEN_first_arrives_THEN_the_first_handled();

static void UNIT_TEST_at_ignore_echo();
static void UNIT_TEST_at_handle_response_no_space_after_colon();

// --------------------------------------------------------------------------------------------------------------------
// DECLARATION OF PRIVATE MACROS, FUNCTIONS AND VARIABLES
// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------
// EXECUTION OF THE TESTS
// --------------------------------------------------------------------------------------------------------------------
void test_at_cmd_handler()
{
    RUN_TEST(GIVEN_at_cmd_handler_WHEN_single_line_response_received_without_prefix_THEN_payload_obtained);
    RUN_TEST(GIVEN_at_cmd_handler_WHEN_single_line_response_received_with_prefix_THEN_payload_obtained);

    RUN_TEST(UNIT_TEST_at_prepare_cmd_write);
    RUN_TEST(UNIT_TEST_at_prepare_cmd_test);
    RUN_TEST(UNIT_TEST_at_prepare_cmd_exec);
    RUN_TEST(UNIT_TEST_at_prepare_cmd_read);

    RUN_TEST(UNIT_TEST_at_handle_unsolicited_one_shot);
    RUN_TEST(UNIT_TEST_at_handle_unsolicited_multiple_times);
    RUN_TEST(UNIT_TEST_at_handle_unsolicited_msg);
    RUN_TEST(UNIT_TEST_at_handle_unsolicited_no_space_after_colon);

    RUN_TEST(
        GIVEN_awaited_solicited_WHEN_unsolicited_sent_AND_WHEN_solicited_response_sent_THEN_solicited_pload_obtained);
    RUN_TEST(
        GIVEN_awaited_solic_and_unsolic_WHEN_unsolicited_sent_AND_WHEN_solicited_resp_sent_THEN_both_ploads_obtained);
    RUN_TEST(GIVEN_awaited_solicited_command_WHEN_multiline_response_received_without_prefix_THEN_all_lines_obtained);
    RUN_TEST(GIVEN_awaited_solicited_command_WHEN_multiline_response_received_with_prefix_THEN_all_lines_obtained);
    RUN_TEST(GIVEN_awaited_solicited_WHEN_multiline_response_received_mixed_with_unsolicited_THEN_all_lines_obtained);
    RUN_TEST(GIVEN_awaited_same_solicited_and_unsolicited_WHEN_solicited_response_THEN_only_solicited_handled);
    RUN_TEST(GIVEN_awaited_two_unsolicited_WHEN_first_arrives_THEN_the_first_handled);

    RUN_TEST(UNIT_TEST_at_ignore_echo);
    RUN_TEST(UNIT_TEST_at_handle_response_no_space_after_colon);
}

// --------------------------------------------------------------------------------------------------------------------
// DEFINITION OF THE TEST CASES
// --------------------------------------------------------------------------------------------------------------------
static void GIVEN_at_cmd_handler_WHEN_single_line_response_received_without_prefix_THEN_payload_obtained()
{
    // GIVEN
    at_cmd_handler at_handler;
    std::string pload;
    std::string mock_response("Some single line data without prefix");
    std::string expected(mock_response);

    // WHEN
    auto awaited_cmd = at_cmd::third;
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>(std::move(mock_response)),
                                                    awaited_cmd,
                                                    pload) == at_err::handling_cmd);
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>("OK"), awaited_cmd, pload) ==
                at_err::ok);

    // THEN
    TEST_ASSERT_EQUAL_STRING(expected.c_str(), pload.c_str());
}

static void GIVEN_at_cmd_handler_WHEN_single_line_response_received_with_prefix_THEN_payload_obtained()
{
    // GIVEN
    at_cmd_handler at_handler;
    std::string pload;
    std::string mock_response("Some single line data");
    std::string expected(mock_response);

    // WHEN
    auto awaited_cmd = at_cmd::first;
    TEST_ASSERT(at_handler.handle_received_response(
                    std::make_unique<std::string>("+FIRST: " + std::move(mock_response)), awaited_cmd, pload) ==
                at_err::handling_cmd);

    // THEN
    TEST_ASSERT_EQUAL_STRING(expected.c_str(), pload.c_str());
}

static void UNIT_TEST_at_prepare_cmd_write()
{
    TEST_ASSERT_EQUAL_STRING(
        "AT+FOURTH=", at_cmd_handler::prepare_cmd_prefix_to_transmit(at_cmd::fourth, at_cmd_type::write).c_str());
}

static void UNIT_TEST_at_prepare_cmd_test()
{
    TEST_ASSERT_EQUAL_STRING("AT+TENTH=?",
                             at_cmd_handler::prepare_cmd_prefix_to_transmit(at_cmd::tenth, at_cmd_type::test).c_str());
}

static void UNIT_TEST_at_prepare_cmd_exec()
{
    TEST_ASSERT_EQUAL_STRING("AT+NINTH",
                             at_cmd_handler::prepare_cmd_prefix_to_transmit(at_cmd::ninth, at_cmd_type::exec).c_str());
}

static void UNIT_TEST_at_prepare_cmd_read()
{
    TEST_ASSERT_EQUAL_STRING("AT+EIGHTH?",
                             at_cmd_handler::prepare_cmd_prefix_to_transmit(at_cmd::eighth, at_cmd_type::read).c_str());
}

static void UNIT_TEST_at_handle_unsolicited_one_shot()
{
    at_cmd_handler h;
    int test_var = 0;

    h.register_unsolicited_handler(at_cmd::third, [&test_var](std::unique_ptr<std::string>) {
        test_var++;
        // Unregister the handler after the first call.
        return true;
    });

    std::string response_payload;

    // After the first call the variable should be incremented.
    h.handle_received_response(
        std::make_unique<std::string>("+THIRD: first unused payload"), at_cmd::none, response_payload);
    TEST_ASSERT_EQUAL(1, test_var);

    // After the second call the lambda should not be called.
    h.handle_received_response(
        std::make_unique<std::string>("+THIRD: second unused payload"), at_cmd::none, response_payload);
    TEST_ASSERT_EQUAL(1, test_var);
}

static void UNIT_TEST_at_handle_unsolicited_multiple_times()
{
    at_cmd_handler h;
    int test_var = 0;

    h.register_unsolicited_handler(at_cmd::third, [&test_var](std::unique_ptr<std::string>) {
        static int cnt = 0;
        test_var++;
        cnt++;
        // Unregister the handler after the third call.
        if (cnt == 3)
            return true;
        else
            return false;
    });

    std::string response_payload;

    h.handle_received_response(
        std::make_unique<std::string>("+THIRD: first unused payload"), at_cmd::none, response_payload);
    TEST_ASSERT_EQUAL(1, test_var);
    h.handle_received_response(
        std::make_unique<std::string>("+THIRD: second unused payload"), at_cmd::none, response_payload);
    TEST_ASSERT_EQUAL(2, test_var);
    h.handle_received_response(
        std::make_unique<std::string>("+THIRD: third unused payload"), at_cmd::none, response_payload);
    TEST_ASSERT_EQUAL(3, test_var);
    h.handle_received_response(
        std::make_unique<std::string>("+THIRD: fourth unused payload"), at_cmd::none, response_payload);
    TEST_ASSERT_EQUAL(3, test_var);
}

static void UNIT_TEST_at_handle_unsolicited_msg()
{
    at_cmd_handler h;
    int test_var = 0;
    h.register_unsolicited_handler(at_unsolicited_msg::neul, [&test_var]() {
        test_var = 1;
        return false;
    });

    std::string response_payload;
    h.handle_received_response(std::make_unique<std::string>("Neul"), at_cmd::none, response_payload);
    TEST_ASSERT_EQUAL(1, test_var);
}

static void UNIT_TEST_at_handle_unsolicited_no_space_after_colon()
{
    at_cmd_handler at_handler;
    std::string pload("INIT THAT SHEET");
    std::string expected_pload("BLINK MOTHERF");
    at_handler.register_unsolicited_handler(at_cmd::seventh, [&pload](std::unique_ptr<std::string> response) {
        pload = std::move(*response);
        return true;
    });

    // WHEN
    std::string dummy_pload;
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>("+SEVENTH:" + expected_pload),
                                                    at_cmd::none,
                                                    dummy_pload) == at_err::unknown);

    TEST_ASSERT_EQUAL_STRING(expected_pload.c_str(), pload.c_str());
}

static void
GIVEN_awaited_solicited_WHEN_unsolicited_sent_AND_WHEN_solicited_response_sent_THEN_solicited_pload_obtained()
{
    // GIVEN
    at_cmd_handler at_handler;
    std::string pload;
    std::string mock_solicited_payload("Some awesome solicited data");
    std::string mock_unsolicited_payload("Some awesome unsolicited data");
    std::string expected(mock_solicited_payload);

    // WHEN
    auto awaited_cmd = at_cmd::second;
    TEST_ASSERT(at_handler.handle_received_response(
                    std::make_unique<std::string>("+FIRST: " + mock_unsolicited_payload), awaited_cmd, pload) ==
                at_err::unknown);

    // AND WHEN
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>("+SECOND: " + mock_solicited_payload),
                                                    awaited_cmd,
                                                    pload) == at_err::handling_cmd);
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>("OK"), awaited_cmd, pload) ==
                at_err::ok);

    // THEN
    TEST_ASSERT_EQUAL_STRING(expected.c_str(), pload.c_str());
}

static void
GIVEN_awaited_solic_and_unsolic_WHEN_unsolicited_sent_AND_WHEN_solicited_resp_sent_THEN_both_ploads_obtained()
{
    // GIVEN
    at_cmd_handler at_handler;
    std::string pload_solicited, pload_unsolicited;
    std::string mock_solicited_payload("Some pretty cool solicited data");
    std::string mock_unsolicited_payload("Some pretty bad unsolicited data");
    std::string expected_solicited(mock_solicited_payload), expected_unsolicited(mock_unsolicited_payload);
    at_handler.register_unsolicited_handler(at_cmd::first, [&pload_unsolicited](std::unique_ptr<std::string> response) {
        pload_unsolicited = std::move(*response);
        return true;
    });

    // WHEN
    auto awaited_cmd = at_cmd::second;
    TEST_ASSERT(
        at_handler.handle_received_response(std::make_unique<std::string>("+FIRST: " + mock_unsolicited_payload),
                                            awaited_cmd,
                                            pload_solicited) == at_err::unknown);

    // AND WHEN
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>("+SECOND: " + mock_solicited_payload),
                                                    awaited_cmd,
                                                    pload_solicited) == at_err::handling_cmd);
    TEST_ASSERT(at_handler.handle_received_response(
                    std::make_unique<std::string>("OK"), awaited_cmd, pload_solicited) == at_err::ok);

    // THEN
    TEST_ASSERT_EQUAL_STRING(expected_solicited.c_str(), pload_solicited.c_str());
    TEST_ASSERT_EQUAL_STRING(expected_unsolicited.c_str(), pload_unsolicited.c_str());
}

static void GIVEN_awaited_solicited_command_WHEN_multiline_response_received_without_prefix_THEN_all_lines_obtained()
{
    // GIVEN
    at_cmd_handler at_handler;
    std::string pload;
    std::string first_line("First line baybies");
    std::string second_line("Second line baybies");
    std::string third_line("Third line baybies");
    std::string fourth_line("Fourth line baybies");

    // WHEN
    auto awaited_cmd = at_cmd::fifth;
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>(first_line), awaited_cmd, pload) ==
                at_err::handling_cmd);
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>(second_line), awaited_cmd, pload) ==
                at_err::handling_cmd);
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>(third_line), awaited_cmd, pload) ==
                at_err::handling_cmd);
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>(fourth_line), awaited_cmd, pload) ==
                at_err::handling_cmd);
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>("OK"), awaited_cmd, pload) ==
                at_err::ok);

    // THEN
    TEST_ASSERT_EQUAL_STRING(
        std::string(first_line + "\r\n" + second_line + "\r\n" + third_line + "\r\n" + fourth_line).c_str(),
        pload.c_str());
}

static void GIVEN_awaited_solicited_command_WHEN_multiline_response_received_with_prefix_THEN_all_lines_obtained()
{
    // GIVEN
    at_cmd_handler at_handler;
    std::string pload;
    std::string first_line("First line groundhogs");
    std::string second_line("Second line groundhogs");
    std::string third_line("Third line groundhogs");
    std::string fourth_line("Fourth line groundhogs");

    // WHEN
    auto awaited_cmd = at_cmd::sixth;
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>("+SIXTH: " + first_line),
                                                    awaited_cmd,
                                                    pload) == at_err::handling_cmd);
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>("+SIXTH: " + second_line),
                                                    awaited_cmd,
                                                    pload) == at_err::handling_cmd);
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>("+SIXTH: " + third_line),
                                                    awaited_cmd,
                                                    pload) == at_err::handling_cmd);
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>("+SIXTH: " + fourth_line),
                                                    awaited_cmd,
                                                    pload) == at_err::handling_cmd);
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>("OK"), awaited_cmd, pload) ==
                at_err::ok);

    // THEN
    TEST_ASSERT_EQUAL_STRING(
        std::string(first_line + "\r\n" + second_line + "\r\n" + third_line + "\r\n" + fourth_line).c_str(),
        pload.c_str());
}

static void GIVEN_awaited_solicited_WHEN_multiline_response_received_mixed_with_unsolicited_THEN_all_lines_obtained()
{
    // GIVEN
    at_cmd_handler at_handler;
    std::string pload;
    std::string first_line("First coconut line");
    std::string second_line("Second coconut line");
    std::string third_line("Third coconut line");

    // WHEN
    auto awaited_cmd = at_cmd::seventh;
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>("+SEVENTH: " + first_line),
                                                    awaited_cmd,
                                                    pload) == at_err::handling_cmd);
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>("+THIRD: totally transparent"),
                                                    awaited_cmd,
                                                    pload) == at_err::unknown);
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>("+SEVENTH: " + second_line),
                                                    awaited_cmd,
                                                    pload) == at_err::handling_cmd);
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>("+SEVENTH: " + third_line),
                                                    awaited_cmd,
                                                    pload) == at_err::handling_cmd);
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>("OK"), awaited_cmd, pload) ==
                at_err::ok);

    // THEN
    TEST_ASSERT_EQUAL_STRING(std::string(first_line + "\r\n" + second_line + "\r\n" + third_line).c_str(),
                             pload.c_str());
}

static void GIVEN_awaited_two_unsolicited_WHEN_first_arrives_THEN_the_first_handled()
{
    // GIVEN
    at_cmd_handler at_handler;
    std::string pload;
    std::string pload_solicited, pload_unsolicited("Primary content hasn't change");
    std::string mock_solicited_payload("Some pretty neat solicited data");
    std::string mock_unsolicited_payload("Some puce unsolicited data");
    std::string expected_solicited(mock_solicited_payload), expected_unsolicited(pload_unsolicited);

    auto awaited_cmd = at_cmd::first;
    at_handler.register_unsolicited_handler(awaited_cmd, [&pload_unsolicited](std::unique_ptr<std::string> response) {
        pload_unsolicited = std::move(*response);
        return true;
    });

    // WHEN
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>("+FIRST: " + mock_solicited_payload),
                                                    awaited_cmd,
                                                    pload_solicited) == at_err::handling_cmd);
    TEST_ASSERT(at_handler.handle_received_response(
                    std::make_unique<std::string>("OK"), awaited_cmd, pload_solicited) == at_err::ok);

    // THEN
    TEST_ASSERT_EQUAL_STRING(expected_solicited.c_str(), pload_solicited.c_str());
    TEST_ASSERT_EQUAL_STRING(expected_unsolicited.c_str(), pload_unsolicited.c_str());
}

static void GIVEN_awaited_same_solicited_and_unsolicited_WHEN_solicited_response_THEN_only_solicited_handled()
{
    // GIVEN
    at_cmd_handler at_handler;
    std::string pload, dummy_pload, expected_pload("SIEMANDERO MORDECZKI");

    at_handler.register_unsolicited_handler(at_cmd::first, [&pload](std::unique_ptr<std::string> response) {
        pload = std::move(*response);
        return true;
    });
    at_handler.register_unsolicited_handler(at_cmd::second, [](std::unique_ptr<std::string>) { return true; });

    // WHEN
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>("+FIRST: " + expected_pload),
                                                    at_cmd::none,
                                                    dummy_pload) == at_err::unknown);

    // THEN
    TEST_ASSERT_EQUAL_STRING(expected_pload.c_str(), pload.c_str());
}

static void UNIT_TEST_at_ignore_echo()
{
    at_cmd_handler at_handler;
    std::string pload;
    std::string expected_pload("ARGENTINA");
    auto awaited_cmd = at_cmd::fourth;

    TEST_ASSERT(at_handler.handle_received_response(
                    std::make_unique<std::string>("AT+FOURTH=MEXICO"), awaited_cmd, pload) == at_err::unknown);
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>("+FOURTH: " + expected_pload),
                                                    awaited_cmd,
                                                    pload) == at_err::handling_cmd);
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>("OK"), awaited_cmd, pload) ==
                at_err::ok);
    TEST_ASSERT_EQUAL_STRING(expected_pload.c_str(), pload.c_str());
}

static void UNIT_TEST_at_handle_response_no_space_after_colon()
{
    at_cmd_handler at_handler;
    std::string pload;
    std::string expected_pload("MAKARENA");

    auto awaited_cmd = at_cmd::ninth;
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>("+NINTH:" + expected_pload),
                                                    awaited_cmd,
                                                    pload) == at_err::handling_cmd);
    TEST_ASSERT(at_handler.handle_received_response(std::make_unique<std::string>("OK"), awaited_cmd, pload) ==
                at_err::ok);

    TEST_ASSERT_EQUAL_STRING(expected_pload.c_str(), pload.c_str());
}
