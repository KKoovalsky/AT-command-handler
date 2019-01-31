# AT command handler

## Dependencies

External dependencies:
* FreeRTOS
* [LoopPerfect/neither](https://github.com/LoopPerfect/neither/)
* my data structures
* my FreeRTOS structures

## Configuration

See [at_cmd_config.hpp.example](at_cmd_config.hpp.example). Rename the file to `at_cmd_config.hpp` and define the
options you need.

## How to use it 

See [at_cmd.hpp](at_cmd.hpp) for the interface.

This implementation works on top of FreeRTOS and my data structures repository. It is implemented in C++14.

To make this implementation work you need to install internal ISR handlers. The handlers are:
```
extern "C" void it_handle_at_byte_rx(char c);   // 1
extern "C" void it_handle_at_byte_tx();         // 2
```

For STM32 call this functions from `stm32*xx_it.c` file, from the specific ISR handler. This function doesn't handle
the hardware specific flag clearing/setting. You must do it on your own. Only TX ISR is controlled from the AT command
handler, because it knows when it can disable the TX interrupt.

The first one must be called from the UART RX interrupt and one must pass the received byte to the function. The second
one must be called from the UART TX interrupt.

This implementation uses a hardware abstraction layer which must be implemented by the user. The functions are:
* `void hw_at_enable_rx_it(void)` - Enables the UART RX interrupt
* `void hw_at_disable_tx_it(void)` - Disables the UART RX interrupt
* `void hw_at_enable_tx_it(void)` - Enables the UART TX interrupt
* `void hw_at_disable_tx_it(void)` - Disables the UART RX interrupt
* `void hw_at_send_byte(char c)` - Sends byte over UART TX line
