#ifndef __USB_ROUTINE_H 
#define __USB_ROUTINE_H

extern uint8_t ret;
extern uint8_t LED_flag;
extern uint8_t LED_status;

extern uint8_t  Dat_Up_Buf[1024];
extern uint8_t  Bulk_Out_Buf[1024];
extern volatile uint16_t Bulk_Out_Len;


/******************************************************************************/
/* external functions */
void USB_command_check(void);
void USB_bulk_data_handler(void);

#endif
