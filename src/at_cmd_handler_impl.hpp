/**
 * @file	at_cmd_handler.h
 * @brief	Defines the template for creating AT commands and parsing received responses on AT commands.
 * @author	Kacper Kowalski - kacper.s.kowalski@gmail.com
 */

#ifndef AT_CMD_HANDLER_IMPL_HPP
#define AT_CMD_HANDLER_IMPL_HPP

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace jungles {

enum class at_err
{
    ok,
    error,
    cme_error,
    handling_cmd,
    prompt_request,
    handled_unsolicited,
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

enum class at_handler_policy
{
    keep,
    remove
};

template <typename E> constexpr static auto to_u_type(E e) noexcept
{
    return static_cast<std::underlying_type_t<E>>(e);
}

template <typename CmdsNotExtended,
          typename CmdsExtended,
          typename AsynchMsgs,
          size_t CmdsNotExtendedNum,
          size_t CmdsExtendedNum,
          size_t AsynchMsgsNum>
class at_cmd_handler
{
  public:
    constexpr at_cmd_handler(const std::array<std::string_view, CmdsNotExtendedNum> &cmds_not_extended,
                             const std::array<std::string_view, CmdsExtendedNum> &cmds_extended,
                             const std::array<std::string_view, AsynchMsgsNum> &asynch_msgs)
        : m_cmds_not_extended(cmds_not_extended), m_cmds_extended(cmds_extended), m_asynch_msgs(asynch_msgs)
    {
    }

    //! Make an AT command. The T type must be CmdsExtended or CmdsNotExtended.
    template <typename T> std::string make_cmd_prefix(T cmd, at_cmd_type cmd_type) const;

    //! Handle a received response on an AT command. The T type must be CmdsExtended or CmdsNotExtended.
    template <typename T>
    at_err handle_rcvd_response(std::unique_ptr<std::string> response, T awaited_cmd, std::string &response_payload);

    //! Register a handler called on a received unsolicited message. The T type must be CmdsExtended or CmdsNotExtended.
    void register_unsolicited_handler(CmdsExtended unsolicited_command,
                                      std::function<at_handler_policy(std::unique_ptr<std::string>)> handler);

    void register_asynch_msg_handler(AsynchMsgs asynch_msg, std::function<at_handler_policy()> handler);

  private:
    // ----------------------------------------------------------------------------------------------------------------
    // Private variables
    // ----------------------------------------------------------------------------------------------------------------
    const std::array<std::string_view, CmdsNotExtendedNum> &m_cmds_not_extended;
    const std::array<std::string_view, CmdsExtendedNum> &m_cmds_extended;
    const std::array<std::string_view, AsynchMsgsNum> &m_asynch_msgs;

    std::list < std::pair<std::function<at_handler_policy(std::unique_ptr<std::string>)>, CmdsExtended>
                    unsolicited_cmd_handlers;
    std::list < std::pair<std::function<at_handler_policy(void)>, AsynchMsgs> asynch_msgs_handlers;

    static constexpr std::string_view at_prefix{"AT"};
    static constexpr std::string_view cme_error_str{"+CME ERROR"};

    // ----------------------------------------------------------------------------------------------------------------
    // Private methods
    // ----------------------------------------------------------------------------------------------------------------
    template <typename T> constexpr bool is_extended_cmd(T cmd) const noexcept;
    template <typename T> constexpr auto get_at_array_size() const noexcept;
    template <typename T> constexpr const auto &get_ref_to_at_array() const noexcept;

    at_err handle_unsolicited_cmd(std::unique_ptr<std::string> response);

    bool is_echo(const std::string &response);
    bool is_response_to_specific_extended_command(const std::string &response, CmdsExtended command);
    bool is_specific_asynch_msg(const std::string &message, AsynchMsgs asynch_msg) size_t
        calc_prefix_len_in_response_on_extended_cmd(const std::string &response, CmdsExtended command);
    void remove_prefix_from_response(std::string &response, size_t chars_to_erase_at_beginning);
    at_err resolve_is_control_message(const std::string &response);
    template <typename T> at_err resolve_is_response_to_command(const std::string &response, T command);
};

namespace {

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

} // namespace

}; // namespace jungles

template <typename T1, typename T2, typename T3, size_t S1, size_t S2, size_t S3, typename... Ts>
constexpr auto make_at_cmd_handler(const std::array<std::string_view, S1> &cmds_not_extended,
                                   const std::array<std::string_view, S2> &cmds_extended,
                                   const std::array<std::string_view, S3> &asynch_msgs)
{
    return at_cmd_handler<T1, T2, T3, S1, S2, S3>(cmds_not_extended, cmds_extended, asynch_msgs);
}

template <typename T1, typename T2, typename T3, size_t S1, size_t S2, size_t S3>
template <typename T>
std::string at_cmd_handler<T1, T2, T3, S1, S2, S3>::make_cmd_prefix(T cmd, at_cmd_type cmd_type) const
{
    const auto is_extended = is_extended_cmd(cmd);
    const auto &at_cmd_str = get_ref_to_at_array<T>();

    // Calculate the command length to know how much memory to reserve to make it
    size_t len = at_prefix.length() + at_cmd_str[to_u_type(cmd)].length() + (is_extended ? 1 : 0);
    if (cmd_type == at_cmd_type::read || cmd_type == at_cmd_type::write)
        len += 1; // One character for '?' or '='
    else if (cmd_type == at_cmd_type::test)
        len += 2; // One character for '=' and the second for '?'.

    std::string msg;

    // Reserve the needed length and also some characters more if one wanted to put a CRLF into it.
    msg.reserve(len + 4);

    msg += at_prefix;
    if (is_extended)
        msg += '+';
    msg += at_cmd_str[to_u_type(cmd)];

    if (cmd_type == at_cmd_type::read)
        msg += '?';
    else if (cmd_type == at_cmd_type::test)
        msg += "=?";
    else if (cmd_type == at_cmd_type::write)
        msg += '=';

    return msg;
}

template <typename CmdsNotExtended, typename CmdsExtended, typename T3, size_t S1, size_t S2, size_t S3>
template <typename T>
constexpr bool at_cmd_handler<CmdsNotExtended, CmdsExtended, T3, S1, S2, S3>::is_extended_cmd(T cmd) const noexcept
{
    static_assert(std::is_same_v<T, CmdsNotExtended> || std::is_same_v<T, CmdsExtended>,
                  "Function can only take as a paramater an enum class which defines AT commands");
    if constexpr (std::is_same_v<T, CmdsNotExtended>)
        return false;
    else
        return true;
}

template <typename CmdsNotExtended,
          typename CmdsExtended,
          typename T3,
          size_t CmdsNotExtendedNum,
          size_t CmdsExtendedNum,
          size_t S3>
template <typename T>
constexpr auto
at_cmd_handler<CmdsNotExtended, CmdsExtended, T3, CmdsNotExtendedNum, CmdsExtendedNum, S3>::get_at_array_size() const
    noexcept
{
    static_assert(std::is_same_v<T, CmdsNotExtended> || std::is_same_v<T, CmdsExtended>,
                  "Function can only take as a paramater an enum class which defines AT commands");
    if constexpr (std::is_same_v<T, CmdsNotExtended>)
        return CmdsNotExtendedNum;
    else
        return CmdsExtendedNum;
}

template <typename CmdsNotExtended,
          typename CmdsExtended,
          typename T3,
          size_t CmdsNotExtendedNum,
          size_t CmdsExtendedNum,
          size_t S3>
template <typename T>
constexpr const auto &
at_cmd_handler<CmdsNotExtended, CmdsExtended, T3, CmdsNotExtendedNum, CmdsExtendedNum, S3>::get_ref_to_at_array() const
    noexcept
{
    static_assert(std::is_same_v<T, CmdsNotExtended> || std::is_same_v<T, CmdsExtended>,
                  "Function can only take as a paramater an enum class which defines AT commands");
    if constexpr (std::is_same_v<T, CmdsNotExtended>)
        return m_cmds_not_extended;
    else
        return m_cmds_extended;
}

template <typename CmdsNotExtended,
          typename CmdsExtended,
          typename T3,
          size_t CmdsNotExtendedNum,
          size_t CmdsExtendedNum,
          size_t S3>
template <typename T>
at_err at_cmd_handler<CmdsNotExtended, CmdsExtended, T3, CmdsNotExtendedNum, CmdsExtendedNum, S3>::handle_rcvd_response(
    std::unique_ptr<std::string> response, T awaited_cmd, std::string &response_payload)
{
    static_assert(std::is_same_v<T, CmdsNotExtended> || std::is_same_v<T, CmdsExtended>,
                  "Function can only take as a paramater an enum class which defines AT commands");

    if (awaited_cmd == T::none)
        return handle_unsolicited_cmd(std::move(response));
    if (is_echo(*response))
        return at_err::unknown;

    auto response_meaning = resolve_is_control_message(*response);
    if (response_meaning == at_err::unknown)
	{
		if constexpr(std::is_same_v<T, CmdsExtended>)
		{
			if(is_response_to_specific_extended_command(*response, awaited_cmd))
		}
	}
        response_meaning = resolve_is_response_to_command(*response, awaited_cmd);

    if (response_meaning == at_err::cme_error)
    {
        remove_prefix_from_response(*response, cme_error_str.length());
        append_string_and_if_nonempty_add_newline(std::move(*response), response_payload);
    }
    else if (response_meaning == at_err::handling_cmd)
    {
        if (is_response_containing_command_name(*response))
        {
            auto prefix_len = calc_prefix_len_in_response_on_extended_cmd(*response, awaited_cmd);
            remove_prefix_from_response(*response, prefix_len);
        }
        append_string_and_if_nonempty_add_newline(std::move(*response), response_payload);
    }
    else if (response_meaning == at_err::unknown)
        return handle_unsolicited_cmd(std::move(response));

    return response_meaning;
}

template <typename T1, typename T2, typename T3, size_t S1, size_t S2, size_t S3>
at_err at_cmd_handler<T1, T2, T3, S1, S2, S3>::handle_unsolicited_cmd(std::unique_ptr<std::string> response)
{
    for (auto it = unsolicited_cmd_handlers.begin(); it != unsolicited_cmd_handlers.end(); ++it)
    {
        const auto &[handler, command] = *it;
        if (is_response_to_specific_extended_command(*response, command))
        {
            auto prefix_len = calc_prefix_len_in_response_on_extended_cmd(*response, command);
            remove_prefix_from_response(*response, prefix_len);
            // When the handler returns ::remove then the unsolicited handler won't be invoked anymore.
            // This allows to control easily how many times should the handler be invoked.
            if (handler(std::move(response)) == at_handler_policy::remove)
                unsolicited_cmd_handlers.erase(it);
            // After moving the unique_ptr it is invalidated so we must return to not to cause a segmentation fault.
            return at_err::handled_unsolicited;
        }
    }
    for (auto it = asynch_msgs_handlers.begin(); it != asynch_msgs_handlers.end(); ++it)
    {
        const auto &[handler, message] = *it;
        if (is_specific_asynch_msg(*response, message))
        {
            if (handler() == at_handler_policy::remove)
                asynch_msgs_handlers.erase(it);
            return at_err::handled_unsolicited;
        }
    }
    return at_err::unknown;
}

template <typename T1, typename CmdsExtended, typename T3, size_t S1, size_t S2, size_t S3>
bool at_cmd_handler<T1, CmdsExtended, T3, S1, S2, S3>::is_response_to_specific_extended_command(
    const std::string &response, CmdsExtended command)
{
    const auto &cmd_name = cmds_extended[to_u_type(command)];
    return response.compare(1, cmd_name.length(), cmd_name) == 0;
}

template <typename T1, typename CmdsExtended, typename T3, size_t S1, size_t S2, size_t S3>
size_t at_cmd_handler<T1, CmdsExtended, T3, S1, S2, S3>::calc_prefix_len_in_response_on_extended_cmd(
    const std::string &response, CmdsExtended command)
{
    const auto &cmd_name = cmds_extended[to_u_type(command)];
    const size_t sz = sizeof(char('+')) + cmd_name.length() + sizeof(char(':'));
    // Check whether there is space after the colon
    if (response[sz] == ' ')
        return sz + 1;
    else
        return sz;
}

template <typename T1, typename T2, typename T3, size_t S1, size_t S2, size_t S3>
void at_cmd_handler<T1, T2, T3, S1, S2, S3>::remove_prefix_from_response(std::string &response,
                                                                         size_t chars_to_erase_at_beginning)
{
    response.erase(response.begin(), response.begin() + chars_to_erase_at_beginning);
}

template <typename T1, typename T2, typename AsynchMsgs, size_t S1, size_t S2, size_t S3>
bool at_cmd_handler<T1, T2, AsynchMsgs, S1, S2, S3>::is_specific_asynch_msg(const std::string &message,
                                                                            AsynchMsgs asynch_msg)
{
    return string_starts_with(message, m_asynch_msgs[to_u_type(asynch_msg)]);
}

template <typename T1, typename T2, typename T3, size_t S1, size_t S2, size_t S3>
bool at_cmd_handler<T1, T2, T3, S1, S2, S3>::is_echo(const std::string &response)
{
    return string_starts_with(response, at_prefix);
}

template <typename T1, typename T2, typename T3, size_t S1, size_t S2, size_t S3>
at_err at_cmd_handler<T1, T2, T3, S1, S2, S3>::resolve_is_control_message(const std::string &response)
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
    return result;
}

template <typename CmdsNotExtended, typename CmdsNotExtended, typename T3, size_t S1, size_t S2, size_t S3>
template <typename T>
at_err at_cmd_handler<CmdsNotExtended, CmdsExtended, T3, S1, S2, S3>::resolve_is_response_to_command(
    const std::string &response, T command)
{
}

}
; // namespace jungles

#endif /* AT_CMD_HANDLER_IMPL_HPP */
