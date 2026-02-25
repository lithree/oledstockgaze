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
    if (USBHS_EP3_Rx_Len > 0)
    {
        uint8_t message_type = USBHS_EP3_Rx_Buf[2];

        if (message_type == MSG_TYPE_TICKER_AND_PRICE)
        {
            char ticker_display_buf[TICKER_FIXED_LEN + 1];
            char price_display_buf[20];
            float received_price;

            if (USBHS_EP3_Rx_Len >= (HEADER_SIZE + COMBINED_PAYLOAD_LEN))
            {
                // Initialize ticker buffer with zeros to ensure null termination
                memset(ticker_display_buf, 0, sizeof(ticker_display_buf));
                // Extract ticker string
                memcpy(ticker_display_buf, USBHS_EP3_Rx_Buf + HEADER_SIZE, TICKER_FIXED_LEN);

                // Extract float price
                memcpy(&received_price, USBHS_EP3_Rx_Buf + HEADER_SIZE + TICKER_FIXED_LEN, sizeof(float));

                // Convert float to string manually (printf doesn't support %f on this system)
                int price_int = (int)received_price;
                int price_decimal = (int)((received_price - price_int) * 100);
                if (price_decimal < 0) price_decimal = -price_decimal;
                sprintf(price_display_buf, "$%d.%02d", price_int, price_decimal);

                // Calculate percentage change from opening price
                static float opening_price = 0;
                float percent_change = 0;
                char percent_buf[20] = {0};
                
                if (opening_price < 0.0001f) {
                    opening_price = received_price;  // First price is the opening
                }
                
                percent_change = ((received_price - opening_price) / opening_price) * 100.0f;
                
                int percent_int = (int)percent_change;
                int percent_decimal = (int)((percent_change - percent_int) * 100);
                if (percent_decimal < 0) percent_decimal = -percent_decimal;
                char sign = (percent_change >= 0) ? '+' : '-';
                sprintf(percent_buf, "%c%d.%02d%%", sign, percent_int < 0 ? -percent_int : percent_int, percent_decimal);

                // Clear previous text by showing a line of spaces
                OLED_ShowString(0, 0, "          ");
                OLED_ShowString(0, 1, "          ");

                // Update text display
                OLED_ShowString(0, 0, ticker_display_buf);
                OLED_ShowString(36, 0, price_display_buf);
                OLED_ShowChar(78, 0, sign);
                OLED_ShowString(80, 0, percent_buf);

                // Update the chart with the new price
                OLED_Chart_AddPoint(received_price);

                printf("Updated Ticker: %s, Price: %s, Change from Open: %s\r\n", ticker_display_buf, price_display_buf, percent_buf);
            }
        }
        
        // Reset received length and RE-ARM endpoint 3 to receive the next packet
        USBHS_EP3_Rx_Len = 0;
        USBHSD->UEP3_RX_CTRL = (USBHSD->UEP3_RX_CTRL & ~USBHS_UEP_R_RES_MASK) | USBHS_UEP_R_RES_ACK;
    }
}
