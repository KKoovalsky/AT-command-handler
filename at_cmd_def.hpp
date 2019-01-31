/**
 * @file	at_cmd_def.hpp
 * @brief	Defines enumeration with all of the AT commands used within the library.
 * @author	Kacper Kowalski - kacper.s.kowalski@gmail.com
 */

#ifndef AT_CMD_DEF_HPP
#define AT_CMD_DEF_HPP

#include "at_cmd_config.hpp"

//! Commands that have AT prefix always (doesn't matter whether they can be solicited or unsolicited messages).
enum class at_cmd
{
    at = 0,
    AT_COMMANDS_NOT_EXTENDED,
    AT_COMMANDS_EXTENDED,
    number_of_commands,
    none
};

//! Commands which doesn't have AT prefix and can only be sent by the chip which is controlled using AT commands.
enum class at_unsolicited_msg
{
    AT_UNSOLICITED_MESSAGES_NAMES,
    number_of_msgs,
    none
};

#endif /* AT_CMD_DEF_HPP */
