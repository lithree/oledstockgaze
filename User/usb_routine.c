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
uint8_t current_display_mode = DISPLAY_MODE_STOCK_PLOT;  // Default to stock plot mode
static uint8_t mode_switch_in_progress = 0;  // Flag to ignore data while mode switching

// Watchlist tracking
static uint8_t watchlist_row = 1;  // Track current row for watchlist display
static char watchlist_tickers[7][TICKER_FIXED_LEN + 1];  // Store tickers in display
static float watchlist_opening_prices[7];  // Store opening prices for each ticker
static uint8_t watchlist_count = 0;  // Number of tickers currently displayed

void USB_command_check()
{
    /* Mode Switch Command via Control Transfer (EP0) */
    if (USBHS_Mode_Switch_Flag)
    {
        uint8_t mode = USBHS_Mode_Switch_Value;
        
        if (mode == DISPLAY_MODE_STOCK_PLOT || mode == DISPLAY_MODE_WATCHLIST)
        {
            /* Set flag to prevent data processing during mode switch */
            mode_switch_in_progress = 1;
            
            /* Completely clear the screen */
            OLED_Clear();
            OLED_Refresh();
            
            /* Update mode */
            current_display_mode = mode;
            
            if (mode == DISPLAY_MODE_STOCK_PLOT)
            {
                printf("Switched to Mode 1: Stock Plot Display\r\n");
            }
            else if (mode == DISPLAY_MODE_WATCHLIST)
            {
                printf("Switched to Mode 2: Watchlist Display\r\n");
                /* Reset watchlist display state */
                watchlist_count = 0;
                watchlist_row = 1;
                memset(watchlist_tickers, 0, sizeof(watchlist_tickers));
                memset(watchlist_opening_prices, 0, sizeof(watchlist_opening_prices));
                OLED_Refresh();
            }
            
            /* Mode switch complete, ready to process data */
            mode_switch_in_progress = 0;
        }
        
        /* Clear the mode switch flag after processing */
        USBHS_Mode_Switch_Flag = 0;
        return;
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
    // Ignore all data processing during mode switch
    if (mode_switch_in_progress) {
        // Re-arm endpoint if there's data but don't process it
        if (USBHS_EP3_Rx_Len > 0) {
            USBHS_EP3_Rx_Len = 0;
            USBHSD->UEP3_RX_CTRL = (USBHSD->UEP3_RX_CTRL & ~USBHS_UEP_R_RES_MASK) | USBHS_UEP_R_RES_ACK;
        }
        return;
    }

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

                // Convert float to string manually
                int price_int = (int)received_price;
                int price_decimal = (int)((received_price - price_int) * 100);
                if (price_decimal < 0) price_decimal = -price_decimal;
                sprintf(price_display_buf, "$%d.%02d", price_int, price_decimal);

                // Mode 1: Stock Plot Display
                if (current_display_mode == DISPLAY_MODE_STOCK_PLOT)
                {
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
                // Mode 2: Watchlist Display
                else if (current_display_mode == DISPLAY_MODE_WATCHLIST)
                {
                    // Check if this ticker is already displayed
                    int existing_row = -1;
                    int ticker_index = -1;
                    for (int i = 0; i < watchlist_count; i++) {
                        if (strcmp(watchlist_tickers[i], ticker_display_buf) == 0) {
                            existing_row = i + 1;  // Rows start at 1
                            ticker_index = i;
                            break;
                        }
                    }
                    
                    // If it's a new ticker, add it
                    if (existing_row == -1 && watchlist_count < 6) {
                        ticker_index = watchlist_count;
                        watchlist_count++;
                        watchlist_row = watchlist_count;
                        strncpy(watchlist_tickers[ticker_index], ticker_display_buf, TICKER_FIXED_LEN);
                        watchlist_opening_prices[ticker_index] = received_price;  // First price is opening price
                        existing_row = watchlist_row;
                    }
                    
                    // Wrap around after 6 items
                    if (watchlist_count >= 6 && existing_row == -1) {
                        OLED_Clear();
                        watchlist_count = 0;
                        watchlist_row = 1;
                        memset(watchlist_tickers, 0, sizeof(watchlist_tickers));
                        memset(watchlist_opening_prices, 0, sizeof(watchlist_opening_prices));
                        OLED_ShowString(0, 0, "Watchlist:");
                        OLED_Refresh();
                    }
                    
                    // Display or update the item with percentage change
                    if (existing_row > 0 && ticker_index >= 0) {
                        // Calculate percentage change
                        float opening_price = watchlist_opening_prices[ticker_index];
                        float percent_change = 0;
                        char percent_buf[10] = {0};
                        
                        if (opening_price > 0.0001f) {
                            percent_change = ((received_price - opening_price) / opening_price) * 100.0f;
                        }
                        
                        int percent_int = (int)percent_change;
                        int percent_decimal = (int)((percent_change - percent_int) * 100);
                        if (percent_decimal < 0) percent_decimal = -percent_decimal;
                        char sign = (percent_change >= 0) ? '+' : '-';
                        
                        // Format: "TICK $XX.XX +X.XX%"
                        char watchlist_line[30];
                        sprintf(watchlist_line, "%s %s %c%d.%02d%%", 
                                ticker_display_buf, price_display_buf, sign,
                                percent_int < 0 ? -percent_int : percent_int, percent_decimal);
                        
                        OLED_ShowString(0, existing_row, watchlist_line);
                        OLED_Refresh();
                    }
                    
                    printf("Watchlist Updated: %s %s (Row: %d)\r\n", ticker_display_buf, price_display_buf, existing_row);
                }
            }
        }
        
        // Reset received length and RE-ARM endpoint 3 to receive the next packet
        USBHS_EP3_Rx_Len = 0;
        USBHSD->UEP3_RX_CTRL = (USBHSD->UEP3_RX_CTRL & ~USBHS_UEP_R_RES_MASK) | USBHS_UEP_R_RES_ACK;
    }
}
