/**
 * @file	at_cmd_handler.h
 * @brief	Implements a class which handles AT commands.
 * @author	Kacper Kowalski - kacper.s.kowalski@gmail.com
 */

#ifndef AT_CMD_HANDLER_HPP
#define AT_CMD_HANDLER_HPP

#include "at_cmd_def.hpp"
#include <functional>
#include <list>
#include <memory>
#include <string>

enum class at_cmd;

enum class at_err
{
    ok,
    error,
    cme_error,
    handling_cmd,
    prompt_request,
    unknown,
    timeout
};

enum class at_cmd_type
{
    exec,
    write,
    read,
    test
};

struct at_unsolicited_cmd_record
{
    std::function<bool(std::unique_ptr<std::string>)> handler;
    at_cmd command;

    at_unsolicited_cmd_record(std::function<bool(std::unique_ptr<std::string>)> handler, at_cmd command)
        : handler(handler), command(command)
    {
    }
};

struct at_unsolicited_msg_record
{
    std::function<bool(void)> handler;
    at_unsolicited_msg message;

    at_unsolicited_msg_record(std::function<bool(void)> handler, at_unsolicited_msg message)
        : handler(handler), message(message)
    {
    }
};

/**
 * \brief Handles received AT commands' responses, handles unsolicited commands, composes commands to transmit.
 *
 * This is not thread safe. Each access to a non-static method must be thread safe between each other.
 */
class at_cmd_handler
{
  public:
    //! Returns a string with AT command prefix ready to be sent to a device which handles AT commands.
    static std::string prepare_cmd_prefix_to_transmit(at_cmd command, at_cmd_type command_type);

    at_err handle_received_response(std::unique_ptr<std::string> response,
                                    at_cmd awaited_command,
                                    std::string &response_payload);
    void register_unsolicited_handler(at_cmd unsolicited_command,
                                      std::function<bool(std::unique_ptr<std::string>)> handler);
    void register_unsolicited_handler(at_unsolicited_msg unsolicited_msg, std::function<bool(void)> handler);

  private:
    std::list<at_unsolicited_cmd_record> unsolicited_cmd_handlers;
    std::list<at_unsolicited_msg_record> unsolicited_msg_handlers;
    void handle_unsolicited_cmd(std::unique_ptr<std::string> response);
};

const char *at_err_to_string(at_err e);

#endif /* AT_CMD_HANDLER_HPP */
