/**
 * @file	hw_at.h
 * @brief	Declares functions needed to communicate with hardware modules.
 * @author	Kacper Kowalski - kacper.s.kowalski@gmail.com
 */

#ifndef HW_AT_H
#define HW_AT_H

#ifdef __cplusplus
extern "C" {
#endif

//! Enables the interrupt to transmit AT commands.
void hw_at_enable_tx_it();

//! Disables the interrupt to transmit AT commands.
void hw_at_disable_tx_it();

//! Enables the interrupt to receive AT commands/responses.
void hw_at_enable_rx_it();

//! Disable the interrupt to receive AT commands/responses.
void hw_at_disable_rx_it();

//! Transmits byte over the AT TX line.
void hw_at_send_byte(char c);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HW_AT_H */
