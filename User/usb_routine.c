#include "ch32v30x_usbhs_device.h"
#include "debug.h"
#include "usb_routine.h"
#include "string.h"
#include "stdio.h" 
#include "lcd.h" 

uint8_t ret = 0;

uint8_t  Dat_Up_Buf[1024];
uint8_t  Bulk_Out_Buf[1024];
volatile uint16_t Bulk_Out_Len = 0;

void USB_command_check()
{
    if (LED_flag)
    {
        printf("Command Received: %d\r\n", LED_status);

        if (LED_status)
        {
            GPIO_WriteBit(GPIOA, GPIO_Pin_15, Bit_SET);
        }
        else
        {
            GPIO_WriteBit(GPIOA, GPIO_Pin_15, Bit_RESET);
        }

        LED_flag = 0;
    }
    /* Determine if enumeration is complete, perform data transfer if completed */
    if (USBHS_DevEnumStatus)
    {
        /* Data Transfer */
        if (RingBuffer_Comm.RemainPack)
        {
            ret = USBHS_Endp_DataUp(DEF_UEP1, &Data_Buffer[(RingBuffer_Comm.DealPtr) * DEF_USBD_HS_PACK_SIZE], RingBuffer_Comm.PackLen[RingBuffer_Comm.DealPtr], DEF_UEP_DMA_LOAD);
            if (ret == 0)
            {
                NVIC_DisableIRQ(USBHS_IRQn);
                RingBuffer_Comm.RemainPack--;
                RingBuffer_Comm.DealPtr++;
                if (RingBuffer_Comm.DealPtr == DEF_Ring_Buffer_Max_Blks)
                {
                    RingBuffer_Comm.DealPtr = 0;
                }
                NVIC_EnableIRQ(USBHS_IRQn);
            }
        }

        /* Monitor whether the remaining space is available for further downloads */
        if (RingBuffer_Comm.RemainPack < (DEF_Ring_Buffer_Max_Blks - DEF_RING_BUFFER_RESTART))
        {
            if (RingBuffer_Comm.StopFlag)
            {
                RingBuffer_Comm.StopFlag = 0;
                USBHSD->UEP1_RX_CTRL = (USBHSD->UEP1_RX_CTRL & ~USBHS_UEP_R_RES_MASK) | USBHS_UEP_R_RES_ACK;
            }
        }
    }
}

void USB_bulk_data_handler(void)
{
    // Check if a USB packet has been received on Endpoint 3
    if (USBHS_EP3_Rx_Len > 0)
    {
        uint8_t message_type = USBHS_EP3_Rx_Buffer[2]; // Get the message type from the header

        if (message_type == MSG_TYPE_TICKER_AND_PRICE)
        {
            char ticker_display_buf[TICKER_FIXED_LEN + 1];
            char price_display_buf[20]; 
            float received_price;

            if (USBHS_EP3_Rx_Len >= (HEADER_SIZE + COMBINED_PAYLOAD_LEN))
            {
                // Extract ticker string
                memcpy(ticker_display_buf, USBHS_EP3_Rx_Buffer + HEADER_SIZE, TICKER_FIXED_LEN);
                ticker_display_buf[TICKER_FIXED_LEN] = '\0'; 

                // Extract float price
                memcpy(&received_price, USBHS_EP3_Rx_Buffer + HEADER_SIZE + TICKER_FIXED_LEN, sizeof(float));

                // Format price for display
                sprintf(price_display_buf, "$%.2f", received_price);

                // Use existing LCD logic:
                // 1. Clear text area (Pages 0 and 1) - actually OLED_ShowString overwrites
                // 2. Display ticker on Page 0
                OLED_ShowString(0, 0, "Ticker: ");
                OLED_ShowString(48, 0, ticker_display_buf);
                // 3. Display price on Page 1
                OLED_ShowString(0, 1, "Price : ");
                OLED_ShowString(48, 1, price_display_buf);

                // 4. Update the chart with the new price
                // Since OLED_Chart_AddPoint(value) clamps to 47, it's safe.
                // We might want to scale it if received_price is larger.
                OLED_Chart_AddPoint((uint8_t)received_price);

                printf("Updated Ticker: %s, Price: %s\r\n", ticker_display_buf, price_display_buf);
            }
        }
        
        USBHS_EP3_Rx_Len = 0;
        USBHSD->UEP3_RX_CTRL = (USBHSD->UEP3_RX_CTRL & ~USBHS_UEP_R_RES_MASK) | USBHS_UEP_R_RES_ACK;
    }
}
