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

namespace jungles {

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
    template <typename T>
    void register_unsolicited_handler(T unsolicited_command, std::function<bool(std::unique_ptr<std::string>)> handler);

    template <typename T> void register_asynch_msg_handler(AsynchMsgs asynch_msg, std::function<bool()> handler);

  private:
    const std::array<std::string_view, CmdsNotExtendedNum> &m_cmds_not_extended;
    const std::array<std::string_view, CmdsExtendedNum> &m_cmds_extended;
    const std::array<std::string_view, AsynchMsgsNum> &m_asynch_msgs;

    static constexpr std::string_view at_prefix{"AT"};
    static constexpr std::string_view cme_error_str{"+CME ERROR"};

    template <typename T> constexpr bool is_extended_cmd(T cmd) const noexcept;
    template <typename T> constexpr auto get_at_array_size() const noexcept;
    template <typename T> constexpr const auto &get_ref_to_at_array() const noexcept;
};

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

    if (awaited_cmd == CmdsExtended::none)
	{
		return at_err::error;
	}
	else
	{
		return at_err::ok;
	}
}

}; // namespace jungles

#endif /* AT_CMD_HANDLER_IMPL_HPP */
