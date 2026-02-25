#ifndef __LCD_H__
#define __LCD_H__

#include "debug.h"
#include "config.h"

/* OLED 物理分辨率 */
#define OLED_W            128
#define OLED_H            64

/* * 硬件连接定义 (SPI2 / GPIOB)
 * D0 (SCK)  -> PB13 (SPI2_SCK)
 * D1 (MOSI) -> PB15 (SPI2_MOSI)
 * RES       -> PB11
 * DC        -> PB10
 * CS        -> PB12
 */

#define OLED_RCC_GPIO     RCC_APB2Periph_GPIOB
#define OLED_RCC_SPI      RCC_APB1Periph_SPI2  // 注意：SPI2 是 APB1 设备
#define OLED_PORT         GPIOB

/* 硬件 SPI 引脚 */
#define OLED_PIN_SCK      GPIO_Pin_13
#define OLED_PIN_MOSI     GPIO_Pin_15

/* 软件控制引脚 */
#define OLED_PIN_DC       GPIO_Pin_10
#define OLED_PIN_RES      GPIO_Pin_11
#define OLED_PIN_CS       GPIO_Pin_12

/* 电平控制宏 */
#define OLED_CS_CLR       GPIO_ResetBits(OLED_PORT, OLED_PIN_CS)
#define OLED_CS_SET       GPIO_SetBits(OLED_PORT, OLED_PIN_CS)

#define OLED_DC_CLR       GPIO_ResetBits(OLED_PORT, OLED_PIN_DC) // 命令模式
#define OLED_DC_SET       GPIO_SetBits(OLED_PORT, OLED_PIN_DC)  // 数据模式

#define OLED_RST_CLR      GPIO_ResetBits(OLED_PORT, OLED_PIN_RES)
#define OLED_RST_SET      GPIO_SetBits(OLED_PORT, OLED_PIN_RES)

/* 基础功能函数 */
void OLED_Init(void);           // 初始化 OLED (包含 SPI2 初始化)
void OLED_Clear(void);          // 清屏
void OLED_On(void);
void OLED_DisplayOn(void);      // 开启显示
void OLED_DisplayOff(void);     // 关闭显示

/* 显示功能函数 */
// x: 0~127, y: 0~7 (行/Page)
void OLED_ShowChar(uint8_t x, uint8_t y, char chr);
void OLED_ShowString(uint8_t x, uint8_t y, char *str);
void OLED_Refresh(void);
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t t);
void OLED_Chart_AddPoint(float value);

#endif