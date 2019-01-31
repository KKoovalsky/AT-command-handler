/**
 * @file	at_cmd_handler.cpp
 * @brief	Implements the handler of AT commands.
 * @author	Kacper Kowalski - kacper.s.kowalski@gmail.com
 */
#include "at_cmd_handler.hpp"
#include "at_cmd_config.hpp"
#include "at_cmd_def.hpp"
#include "at_cmd_gen.hpp"
#include <string_view>

// --------------------------------------------------------------------------------------------------------------------
// COMPILE-TIME GENERATION OF THE ARRAYS WITH THE AT COMMANDS DEFINED BY THE USER AND OTHER COMPILE-TIME UTILITIES
// --------------------------------------------------------------------------------------------------------------------

// Get the number of the not-extended AT commands
#define AT_COMMANDS_NOT_EXTENDED_STRING TO_STRING(AT_COMMANDS_NOT_EXTENDED)
static constexpr char at_cmd_not_extended_str[] = AT_COMMANDS_NOT_EXTENDED_STRING;
static constexpr auto at_not_extended_cmds_num{count(at_cmd_not_extended_str, ',') + 1};

/*
 * Generate the list of AT commands as an array of string_views with the commands in uppercase mapped
 * by enum class at_cmd.
 */
#define AT_COMMANDS_ALL AT_COMMANDS_NOT_EXTENDED, AT_COMMANDS_EXTENDED
#define AT_COMMANDS_ALL_STRING TO_STRING(AT_COMMANDS_ALL)
static constexpr std::array<char, sizeof(AT_COMMANDS_ALL_STRING)> at_commands_all{AT_COMMANDS_ALL_STRING};
static constexpr auto at_commands_all_uppercase{to_upper(at_commands_all)};
static constexpr auto at_cmd_str{
    make_array_with_at_commands<to_u_type(at_cmd::number_of_commands)>(at_commands_all_uppercase)};

static constexpr std::array<std::string_view, to_u_type(at_unsolicited_msg::number_of_msgs)> at_unsolicited_msg_str{
    AT_UNSOLICITED_MESSAGES};

static constexpr std::string_view at_prefix{"AT"};
static constexpr std::string_view cme_error_str{"+CME ERROR"};

static constexpr bool is_extended_at_cmd(at_cmd cmd)
{
    return to_u_type(cmd) > at_not_extended_cmds_num;
}

// --------------------------------------------------------------------------------------------------------------------
// DECLARATION OF STATIC FUNCTIONS AND VARIABLES
// --------------------------------------------------------------------------------------------------------------------
static at_err response_to_at_err(const std::string &message, at_cmd awaited_command);
static bool is_response_to_command(const std::string &response, at_cmd command);
static bool is_response_containing_command_name(const std::string &response);
static bool is_response_to_specific_extended_command(const std::string &response, at_cmd command);
static void remove_prefix_from_response(std::string &response, size_t chars_to_erase_at_beginning);
static size_t calc_prefix_len_in_response_on_extended_cmd(const std::string &response, at_cmd command);
static bool is_echo(const std::string &response);
static bool is_specific_unsolicited_msg(const std::string &message, at_unsolicited_msg unsolicited_msg);
static void append_string_and_if_nonempty_add_newline(std::string &&src, std::string &dst);
template <typename StrTypeLeft, typename StrTypeRight>
static bool string_starts_with(const StrTypeLeft &input, const StrTypeRight &searched);

static const char *at_err_str[] = {"ok", "error", "cme_error", "handling_cmd", "prompt_request", "unknown", "timeout"};

// --------------------------------------------------------------------------------------------------------------------
// DEFINITION OF PUBLIC MEMBER FUNCTIONS
// --------------------------------------------------------------------------------------------------------------------
std::string at_cmd_handler::prepare_cmd_prefix_to_transmit(at_cmd command, at_cmd_type type)
{
    auto is_extended = is_extended_at_cmd(command);

    // Calculate the command length to know how much memory to reserve to make it
    size_t len = at_prefix.length() + at_cmd_str[to_u_type(command)].length() + (is_extended ? 1 : 0);
    if (type == at_cmd_type::read || type == at_cmd_type::write)
        len += 1; // One character for '?' or '='
    else if (type == at_cmd_type::test)
        len += 2; // One character for '=' and the second for '?'.

    std::string msg;

    // Reserve the needed length and also some characters more if one wanted to put a CRLF into it.
    msg.reserve(len + 4);

    msg += at_prefix;
    if (is_extended)
        msg += '+';
    msg += at_cmd_str[to_u_type(command)];

    if (type == at_cmd_type::read)
        msg += '?';
    else if (type == at_cmd_type::test)
        msg += "=?";
    else if (type == at_cmd_type::write)
        msg += '=';

    return msg;
}

at_err at_cmd_handler::handle_received_response(std::unique_ptr<std::string> response,
                                                at_cmd awaited_command,
                                                std::string &response_payload)
{
    if (awaited_command == at_cmd::none)
    {
        handle_unsolicited_cmd(std::move(response));
        return at_err::unknown;
    }

    if (is_echo(*response))
        return at_err::unknown;

    auto response_meaning = response_to_at_err(*response, awaited_command);

    if (response_meaning == at_err::cme_error)
    {
        remove_prefix_from_response(*response, cme_error_str.length());
        append_string_and_if_nonempty_add_newline(std::move(*response), response_payload);
    }
    else if (response_meaning == at_err::handling_cmd)
    {
        if (is_response_containing_command_name(*response))
        {
            auto prefix_len = calc_prefix_len_in_response_on_extended_cmd(*response, awaited_command);
            remove_prefix_from_response(*response, prefix_len);
        }
        append_string_and_if_nonempty_add_newline(std::move(*response), response_payload);
    }
    else if (response_meaning == at_err::unknown)
        handle_unsolicited_cmd(std::move(response));

    return response_meaning;
}

void at_cmd_handler::register_unsolicited_handler(at_cmd unsolicited_command,
                                                  std::function<bool(std::unique_ptr<std::string>)> handler)
{
    unsolicited_cmd_handlers.emplace_back(handler, unsolicited_command);
}

void at_cmd_handler::register_unsolicited_handler(at_unsolicited_msg unsolicited_msg, std::function<bool()> handler)
{
    unsolicited_msg_handlers.emplace_back(handler, unsolicited_msg);
}

// --------------------------------------------------------------------------------------------------------------------
// DEFINITION OF PUBLIC FUNCTIONS
// --------------------------------------------------------------------------------------------------------------------
const char *at_err_to_string(at_err e)
{
    return at_err_str[to_u_type(e)];
}

// --------------------------------------------------------------------------------------------------------------------
// DEFINITION OF PRIVATE MEMBER FUNCTIONS
// --------------------------------------------------------------------------------------------------------------------
void at_cmd_handler::handle_unsolicited_cmd(std::unique_ptr<std::string> response)
{
    bool is_handled = false;
    for (auto it = unsolicited_cmd_handlers.begin(); it != unsolicited_cmd_handlers.end();)
    {
        if (is_response_to_specific_extended_command(*response, it->command))
        {
            auto prefix_len = calc_prefix_len_in_response_on_extended_cmd(*response, it->command);
            remove_prefix_from_response(*response, prefix_len);
            is_handled = true;
            // When the handler returns true then the unsolicited handler won't be invoked anymore.
            // This allows to control easily how many times should the handler be invoked.
            if (it->handler(std::move(response)))
                unsolicited_cmd_handlers.erase(it);
            // After moving the unique_ptr it is invalidated so we must return to not to cause a segmentation fault.
            return;
        }
        it++;
    }
    if (is_handled)
        return;
    // This loop is analogous like the one above, and we break DRY there (TODO change this)
    for (auto it = unsolicited_msg_handlers.begin(); it != unsolicited_msg_handlers.end();)
    {
        if (is_specific_unsolicited_msg(*response, it->message))
        {
            if (it->handler())
                unsolicited_msg_handlers.erase(it);
            return;
        }
        it++;
    }
}

// --------------------------------------------------------------------------------------------------------------------
// DEFINITION OF STATIC FUNCTIONS
// --------------------------------------------------------------------------------------------------------------------
static at_err response_to_at_err(const std::string &response, at_cmd awaited_command)
{
    at_err result = at_err::unknown;
    if (response == "OK")
        result = at_err::ok;
    else if (response == "ERROR")
        result = at_err::error;
    else if (response == ">")
        result = at_err::prompt_request;
    else if (string_starts_with(response, cme_error_str))
        result = at_err::cme_error;
    else if (is_response_to_command(response, awaited_command))
        result = at_err::handling_cmd;
    return result;
}

static bool is_response_to_command(const std::string &response, at_cmd command)
{
    // Do not handle not extended AT commands as they are not used commonly.
    if (!is_extended_at_cmd(command))
        return false;

    // When the response doesn't contain a prefix (e.g. :"+CREG:...") then automatically mark it as a response.
    // This should be changed, because sometimes there are unsolicited messages which doesn't contain the command's
    // name (like e.g. "RING") which will cause this implementation to be buggy.
    if (!is_response_containing_command_name(response))
        return true;

    // After '+' character there is the command name put. We can check then whether the command's name correspons
    // to the awaited command.
    return is_response_to_specific_extended_command(response, command);
}

static bool is_response_containing_command_name(const std::string &response)
{
    return response[0] == '+';
}

static bool is_response_to_specific_extended_command(const std::string &response, at_cmd command)
{
    const auto &cmd_name = at_cmd_str[to_u_type(command)];
    return response.compare(1, cmd_name.length(), cmd_name) == 0;
}

static void remove_prefix_from_response(std::string &response, size_t chars_to_erase_at_beginning)
{
    response.erase(response.begin(), response.begin() + chars_to_erase_at_beginning);
}

static size_t calc_prefix_len_in_response_on_extended_cmd(const std::string &response, at_cmd command)
{
    const auto &cmd_name = at_cmd_str[to_u_type(command)];
    size_t sz = sizeof(char('+')) + cmd_name.length() + sizeof(char(':'));
    // Check whether there is space after the colon
    if (response[sz] == ' ')
        return sz + 1;
    else
        return sz;
}

static bool is_echo(const std::string &response)
{
    return string_starts_with(response, at_prefix);
}

static bool is_specific_unsolicited_msg(const std::string &message, at_unsolicited_msg unsolicited_msg)
{
    return string_starts_with(message, at_unsolicited_msg_str[to_u_type(unsolicited_msg)]);
}

static void append_string_and_if_nonempty_add_newline(std::string &&src, std::string &dst)
{
    if (dst.empty())
    {
        dst = std::move(src);
    }
    else
    {
        dst += "\r\n";
        dst += std::move(src);
    }
}

template <typename StrTypeLeft, typename StrTypeRight>
static bool string_starts_with(const StrTypeLeft &input, const StrTypeRight &searched)
{
    return input.compare(0, searched.length(), searched) == 0;
}

