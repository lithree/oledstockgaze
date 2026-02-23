#ifndef __USB_ROUTINE_H 
#define __USB_ROUTINE_H

#include <stdint.h>

// Custom message type for combined ticker and price (must match Handler.c)
#define MSG_TYPE_TICKER_AND_PRICE 0x03

// Define fixed size for ticker string within the combined payload (must match Handler.c)
#define TICKER_FIXED_LEN 8
#define COMBINED_PAYLOAD_LEN (TICKER_FIXED_LEN + sizeof(float))

// Protocol Constants (copied from Handler.c for consistency)
#define FRAME_SOF 0x0A
#define HEADER_SIZE 5 // SOF(2) + Type(1) + Len(2)
#define FOOTER_SIZE 1 // Checksum(1)
#define OVERHEAD (HEADER_SIZE + FOOTER_SIZE)


extern uint8_t ret;
extern uint8_t LED_flag;
extern uint8_t LED_status;

extern uint8_t  Dat_Up_Buf[1024];
extern uint8_t  Bulk_Out_Buf[1024];
extern volatile uint16_t Bulk_Out_Len;

// External declaration for USBHS_EP3_Rx_Buffer and USBHS_EP3_Rx_Len
// These are typically defined in a USBHS device driver file (e.g., ch32v30x_usbhs_device.c)
extern __attribute__ ((aligned(4))) uint8_t  USBHS_EP3_Rx_Buffer[64]; // Assuming endpoint 3 max packet size is 64
extern __attribute__ ((aligned(4))) uint8_t  USBHS_EP4_Tx_Buffer[64]; // Assuming endpoint 4 max packet size is 64
extern volatile uint16_t USBHS_EP3_Rx_Len;


/******************************************************************************/
/* external functions */
void USB_command_check(void);
void USB_bulk_data_handler(void);

#endif
