/**
 * @file	at_cmd.hpp
 * @brief	Declarations of functions which allow to handle AT commands on the RTOS level.
 * @author	Kacper Kowalski - kacper.s.kowalski@gmail.com
 */

#ifndef AT_CMD_HPP
#define AT_CMD_HPP

#include "FreeRTOS.h"
#include "at_cmd_def.hpp"
#include "at_cmd_handler.hpp"

//! Determines how to terminate the prompted message (for AT commands which demand a message after receiving '>')
enum class at_prompt_end_policy
{
    //! Terminate with CTRL-Z character
    ctrl_z,

    //! Terminate normally with CRLF after sending the whole message.
    crlf
};

/**
 * \brief Send WRITE(SET) AT command and get the payload of the response.
 *
 * The payload of the response is the part after the ':' character, e.g. when the response for the AT command:
 * "AT+MAKAPAKA?" is "+MAKAPAKA: FUNNY HUEHUE" then the payload is "FUNNY HUEHUE". In general, the response payload
 * is the payload of the response and contains only the "effective" data.
 *
 * The payload of the command is the part after '=' character, e.g. when write AT command like that
 * "AT+MAKAPAKA=FUNNY HUEHUE" is sent then the payload is "FUNNY HUEHUE".
 *
 * \param[in] command           The command to be sent.
 * \param[in] payload           The payload of the write AT command.
 * \param[out] response_payload The payload of the received response.
 * \param[in] ticks_to_wait     Max number of ticks this call can block the caller task.
 * \returns result of the operation. \see at_err
 */
at_err at_send(at_cmd command, std::string &&payload, TickType_t ticks_to_wait, std::string &response_payload);

//! Overload of at_send() when you don't need the response's payload for WRITE command types.
at_err at_send(at_cmd command, std::string &&payload, TickType_t ticks_to_wait);

/**
 * \brief Send EXEC, READ or TEST AT command and get the payload of the response.
 *
 * This function can't be called for the WRITE(SET) command. \see Other overloads of at_send().
 *
 * \param[in] command           The command to be sent.
 * \param[in] command_type      The type of the command (at_cmd_type::exec, at_cmd_type::read or at_cmd_type::test).
 * \param[in] ticks_to_wait     Max number of ticks this call can block the caller task.
 * \param[out] response_payload The payload of the received response.
 * \returns result of the operation. \see at_err
 */
at_err at_send(at_cmd command, at_cmd_type command_type, TickType_t ticks_to_wait, std::string &response_payload);

//! Overload of at_send() when you don't need the response's payload.
at_err at_send(at_cmd command, at_cmd_type command_type, TickType_t ticks_to_wait);

/**
 * \brief Send a WRITE command which needs also a second message after receiving the prompt character ('>')
 *
 * \param[in] command           The command to be sent.
 * \param[in] payload           The payload of the write AT command.
 * \param[in] prompt_message    The message sent after receiving the prompt character.
 * \param[in] policy	        The policy used for termination of the prompted message. \see at_prompt_end_policy
 * \param[in] ticks_to_wait     Max number of ticks this call can block the caller task.
 * \returns result of the operation. \see at_err
 */
at_err at_send_prompted(at_cmd command,
                        std::string payload,
                        std::string prompt_message,
                        at_prompt_end_policy policy,
                        TickType_t ticks_to_wait);

/**
 * \brief Register a handler for the specific unsolicited command.
 *
 * The handler is invoked when the unsolicited command arrives. The handler can't use any blocking OS function.
 *
 * \param[in] command   The unsolicited command for which the handler will be invoked.
 * \param[in] handler   The handler takes as a parameter the response payload as an rvalue. It returns true when it
 *                      shall be removed from the list of unsolicited handlers. This gives control over how many
 *                      times should the handler be called on the unsolicited command arrival. When one wants to
 *                      keep the unsolicited handler be called to the end of the world then the handler shall always
 *                      return false. If one wants to implement a one shot handler then this function shall return
 *                      true on the first invocation.
 */
void at_register_unsolicited_handler(at_cmd command,
                                     std::function<bool(std::unique_ptr<std::string> response_payload)> handler);

//! Second overload which accepts unsolicited messages instead of commands (e.g. "RING", "NO CARRIER")
void at_register_unsolicited_handler(at_unsolicited_msg unsolicited_msg, std::function<bool(void)> handler);

#endif /* AT_CMD_HPP */
