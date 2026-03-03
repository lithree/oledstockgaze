#include "ch32v30x_usbhs_device.h"
#include "debug.h"
#include "string.h"
#include "lcd.h"
#include "flash.h"
#include "config.h"
#include "stdlib.h"
#include "usb_routine.h"

void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

/* Global Variable */
uint8_t LED_status;
uint8_t LED_flag;
volatile uint32_t systick_counter = 0;
volatile uint8_t pagechange_flag = 0;

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */

void GPIO_Toggle_INIT(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/*********************************************************************
 * @fn      SYSTICK_Init_Config
 *
 * @brief   SYSTICK_Init_Config.
 *
 * @return  none
 */
void SYSTICK_Init_Config(uint64_t ticks)
{
    SysTick->SR &= ~(1 << 0);//clear State flag
    SysTick->CMP = ticks;
    SysTick->CNT = 0;
    SysTick->CTLR = 0xF;

    NVIC_SetPriority(SysTicK_IRQn, 15);
    NVIC_EnableIRQ(SysTicK_IRQn);
}

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);
    printf("SystemClk:%d Hz\r\n", SystemCoreClock);
    printf("OLED Init...\r\n");
    /* USB20 device init */
    USBHS_RCC_Init();
    USBHS_Device_Init(ENABLE);
    NVIC_EnableIRQ(USBHS_IRQn);
    SYSTICK_Init_Config(SystemCoreClock-1);
    GPIO_Toggle_INIT();

    OLED_Init();
    OLED_Clear();
    OLED_ShowString(0, 0, "Waiting for USB...");
    OLED_Refresh();

    while (1)
    {
        USB_command_check();
        USB_bulk_data_handler();
    }
}

/*********************************************************************
 * @fn      SysTick_Handler
 *
 * @brief   SysTick_Handler.
 *
 * @return  none
 */
void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void SysTick_Handler(void)
{
    if(SysTick->SR == 1)
    {
        SysTick->SR = 0;//clear State flag
        pagechange_flag = 1;
        systick_counter++;
    }
}

