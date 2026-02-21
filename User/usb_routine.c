#include "ch32v30x_usbhs_device.h"
#include "debug.h"
#include "usb_routine.h"
#include "string.h"

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
        // Echo the data back on Endpoint 4
        memcpy(USBHS_EP4_Tx_Buf, USBHS_EP3_Rx_Buf, USBHS_EP3_Rx_Len);
        USBHSD->UEP4_TX_LEN = USBHS_EP3_Rx_Len;
        USBHSD->UEP4_TX_CTRL = (USBHSD->UEP4_TX_CTRL & ~USBHS_UEP_T_RES_MASK) | USBHS_UEP_T_RES_ACK;

        // Clear the received length flag
        USBHS_EP3_Rx_Len = 0;

        // Re-arm Endpoint 3 to receive the next packet
        USBHSD->UEP3_RX_CTRL = (USBHSD->UEP3_RX_CTRL & ~USBHS_UEP_R_RES_MASK) | USBHS_UEP_R_RES_ACK;
    }
}
