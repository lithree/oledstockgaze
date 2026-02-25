#include "lcd.h"

/* ==========================================================
 * 1. FONT DATA (6x8 Pixels)
 * ========================================================== */
const uint8_t F6x8[][6] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // sp
    {0x00, 0x00, 0x00, 0x2f, 0x00, 0x00}, // !
    {0x00, 0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x00, 0x14, 0x7f, 0x14, 0x7f, 0x14}, // #
    {0x00, 0x24, 0x2a, 0x7f, 0x2a, 0x12}, // $
    {0x00, 0x62, 0x64, 0x08, 0x13, 0x23}, // %
    {0x00, 0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x00, 0x1c, 0x22, 0x41, 0x00}, // (
    {0x00, 0x00, 0x41, 0x22, 0x1c, 0x00}, // )
    {0x00, 0x14, 0x08, 0x3E, 0x08, 0x14}, // *
    {0x00, 0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x00, 0x00, 0xA0, 0x60, 0x00}, // ,
    {0x00, 0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x00, 0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x00, 0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00, 0x00}, // 1
    {0x00, 0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x00, 0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x00, 0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x00, 0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x00, 0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x00, 0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x00, 0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x00, 0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x00, 0x08, 0x14, 0x22, 0x41, 0x00}, // <
    {0x00, 0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x00, 0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x00, 0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x00, 0x32, 0x49, 0x59, 0x51, 0x3E}, // @
    {0x00, 0x7C, 0x12, 0x11, 0x12, 0x7C}, // A
    {0x00, 0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x00, 0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x00, 0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x00, 0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x00, 0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x00, 0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x00, 0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x00, 0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x00, 0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x00, 0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x00, 0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x00, 0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x00, 0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x00, 0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x00, 0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x00, 0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x00, 0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x00, 0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x00, 0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x00, 0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x00, 0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x00, 0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x00, 0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x00, 0x61, 0x51, 0x49, 0x45, 0x43}, // Z
    {0x00, 0x00, 0x7F, 0x41, 0x41, 0x00}, // [
    {0x00, 0x02, 0x04, 0x08, 0x10, 0x20}, // \ backslash
    {0x00, 0x00, 0x41, 0x41, 0x7F, 0x00}, // ]
    {0x00, 0x04, 0x02, 0x01, 0x02, 0x04}, // ^
    {0x00, 0x40, 0x40, 0x40, 0x40, 0x40}, // _
    {0x00, 0x00, 0x01, 0x02, 0x04, 0x00}, // '
    {0x00, 0x20, 0x54, 0x54, 0x54, 0x78}, // a
    {0x00, 0x7F, 0x48, 0x44, 0x44, 0x38}, // b
    {0x00, 0x38, 0x44, 0x44, 0x44, 0x20}, // c
    {0x00, 0x38, 0x44, 0x44, 0x48, 0x7F}, // d
    {0x00, 0x38, 0x54, 0x54, 0x54, 0x18}, // e
    {0x00, 0x08, 0x7E, 0x09, 0x01, 0x02}, // f
    {0x00, 0x0C, 0x52, 0x52, 0x52, 0x3E}, // g
    {0x00, 0x7F, 0x08, 0x04, 0x04, 0x78}, // h
    {0x00, 0x00, 0x44, 0x7D, 0x40, 0x00}, // i
    {0x00, 0x20, 0x40, 0x44, 0x3D, 0x00}, // j
    {0x00, 0x7F, 0x10, 0x28, 0x44, 0x00}, // k
    {0x00, 0x00, 0x41, 0x7F, 0x40, 0x00}, // l
    {0x00, 0x7C, 0x04, 0x18, 0x04, 0x78}, // m
    {0x00, 0x7C, 0x08, 0x04, 0x04, 0x78}, // n
    {0x00, 0x38, 0x44, 0x44, 0x44, 0x38}, // o
    {0x00, 0xFC, 0x24, 0x24, 0x24, 0x18}, // p
    {0x00, 0x18, 0x24, 0x24, 0x18, 0xFC}, // q
    {0x00, 0x7C, 0x08, 0x04, 0x04, 0x08}, // r
    {0x00, 0x48, 0x54, 0x54, 0x54, 0x20}, // s
    {0x00, 0x04, 0x3F, 0x44, 0x40, 0x20}, // t
    {0x00, 0x3C, 0x40, 0x40, 0x20, 0x7C}, // u
    {0x00, 0x1C, 0x20, 0x40, 0x20, 0x1C}, // v
    {0x00, 0x3C, 0x40, 0x30, 0x40, 0x3C}, // w
    {0x00, 0x44, 0x28, 0x10, 0x28, 0x44}, // x
    {0x00, 0x0C, 0x50, 0x50, 0x50, 0x3C}, // y
    {0x00, 0x44, 0x64, 0x54, 0x4C, 0x44}, // z
    {0x00, 0x08, 0x36, 0x41, 0x00, 0x00}, // {
    {0x00, 0x00, 0x7F, 0x00, 0x00, 0x00}, // |
    {0x00, 0x00, 0x41, 0x36, 0x08, 0x00}, // }
    {0x00, 0x01, 0x02, 0x04, 0x02, 0x01}, // ~
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  // DEL
};

/* ==========================================================
 * 2. INTERNAL BUFFERS
 * ========================================================== */
// Frame Buffer (128x64 pixels)
static uint8_t OLED_GRAM[8][128];

// Circular Buffer (Ring Buffer) Logic
static uint8_t ChartData[128] = {0};      // Normalized display values
static float ChartRawData[128] = {0};     // Raw price values
static uint8_t ChartHead = 0;             // Points to the *next* write position
static uint8_t ChartCount = 0;            // Number of valid samples in buffer (<=128)
static float ChartMaxVal = 0;             // Track max value
static float ChartMinVal = 100000;        // Track min value
static const float ChartInitSpanPercent = 0.01f; // +/-1% initial vertical span around opening
static float ChartOpeningPrice = 0.0f;    // Opening price (not plotted)
static uint8_t ChartHasOpening = 0;       // Whether opening price has been set
static float ChartFirstDataValue = 0.0f;  // First actual data point (plotted)
static uint8_t ChartHasFirstData = 0;     // Whether first plotted data has been set

/* ==========================================================
 * 3. HARDWARE DRIVER (SPI2 & GPIO)
 * ========================================================== */

static void OLED_SendByte(uint8_t dat)
{
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET)
        ;
    SPI_I2S_SendData(SPI2, dat);
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_BSY) == SET)
        ;
}

static void OLED_WriteCmd(uint8_t cmd)
{
    OLED_CS_CLR;
    OLED_DC_CLR;
    OLED_SendByte(cmd);
    OLED_CS_SET;
}

static void OLED_WriteData(uint8_t dat)
{
    OLED_CS_CLR;
    OLED_DC_SET;
    OLED_SendByte(dat);
    OLED_CS_SET;
}

/* ==========================================================
 * 4. CORE FUNCTIONS
 * ========================================================== */

void OLED_Refresh(void)
{
    uint8_t i, n;
    for (i = 0; i < 8; i++)
    {
        OLED_WriteCmd(0xb0 + i); // Set Page Address
        OLED_WriteCmd(0x00);     // Low Col
        OLED_WriteCmd(0x10);     // High Col
        for (n = 0; n < 128; n++)
        {
            OLED_WriteData(OLED_GRAM[i][n]);
        }
    }
}

void OLED_Clear(void)
{
    uint8_t i, n;
    for (i = 0; i < 8; i++)
    {
        for (n = 0; n < 128; n++)
        {
            OLED_GRAM[i][n] = 0x00;
        }
    }
}

void OLED_On(void)
{
    uint8_t i, n;
    for (i = 0; i < 8; i++)
    {
        for (n = 0; n < 128; n++)
        {
            OLED_GRAM[i][n] = 0x00;
        }
    }
}

void OLED_DisplayOn(void)
{
    OLED_WriteCmd(0x8D); // Charge Pump
    OLED_WriteCmd(0x14); // Enable
    OLED_WriteCmd(0xAF); // Display ON
}

void OLED_DisplayOff(void)
{
    OLED_WriteCmd(0x8D); // Charge Pump
    OLED_WriteCmd(0x10); // Disable
    OLED_WriteCmd(0xAE); // Display OFF
}

/* ==========================================================
 * 5. GRAPHICS & RING BUFFER CHART
 * ========================================================== */

void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t t)
{
    uint8_t pos, bx, temp = 0;
    if (x > 127 || y > 63)
        return;

    // Y=0 maps to pos=7 (bottom page), Y=63 maps to pos=0 (top page)
    pos = 7 - y / 8;
    bx = y % 8;
    temp = 1 << (7 - bx);

    if (t)
        OLED_GRAM[pos][x] |= temp;
    else
        OLED_GRAM[pos][x] &= ~temp;
}

void OLED_Chart_AddPoint(float value)
{
    uint8_t i = 0, j = 0;
    uint8_t read_idx = 0;
    uint8_t norm_val = 0;

    // If opening not set yet, treat this value as opening price (do not plot)
    if (!ChartHasOpening)
    {
        ChartOpeningPrice = value;
        ChartHasOpening = 1;
        return; // don't store or plot opening
    }

    // If this is the first actual plotted data, initialize span around it
    if (!ChartHasFirstData)
    {
        ChartFirstDataValue = value;
        ChartHasFirstData = 1;
        float span = value * ChartInitSpanPercent;
        ChartMaxVal = value + span;
        ChartMinVal = value - span;
        // proceed to store this first plotted data below
    }

    // 1. Store raw price value at head
    ChartRawData[ChartHead] = value;

    // 2. Advance head and count
    ChartHead++;
    if (ChartHead >= 128)
        ChartHead = 0;
    if (ChartCount < 128)
        ChartCount++;

    // 3. Compute min/max across the current window (oldest -> newest)
    read_idx = (ChartHead + 128 - ChartCount) % 128; // oldest
    float window_min = 1e30f;
    float window_max = -1e30f;
    uint8_t scan_idx = read_idx;
    for (i = 0; i < ChartCount; i++)
    {
        float v = ChartRawData[scan_idx];
        if (v < window_min) window_min = v;
        if (v > window_max) window_max = v;
        scan_idx++;
        if (scan_idx >= 128) scan_idx = 0;
    }

    // store for external visibility
    ChartMinVal = window_min;
    ChartMaxVal = window_max;

    // 4. Renormalize ALL values in the window so smallest -> 0, largest -> 47
    scan_idx = read_idx;
    for (i = 0; i < ChartCount; i++)
    {
        if (window_max > window_min)
        {
            float f = (ChartRawData[scan_idx] - window_min) / (window_max - window_min);
            if (f < 0.0f) f = 0.0f;
            if (f > 1.0f) f = 1.0f;
            norm_val = (uint8_t)(f * 47.0f + 0.5f);
        }
        else
        {
            norm_val = 24; // center when no span
        }
        ChartData[scan_idx] = (norm_val > 47) ? 47 : norm_val;
        scan_idx++;
        if (scan_idx >= 128) scan_idx = 0;
    }

    // 5. Targeted Clear (Pages 2-7 only, keep text in Pages 0-1)
    for (i = 2; i < 8; i++)
    {
        for (j = 0; j < 128; j++)
        {
            OLED_GRAM[i][j] = 0x00;
        }
    }

    // 6. Render chart. If fewer than full width samples, right-align (new data appears at the right)
    uint8_t x_start = (ChartCount < 128) ? (128 - ChartCount) : 0;
    read_idx = (ChartHead + 128 - ChartCount) % 128; // oldest
    for (i = 0; i < ChartCount; i++)
    {
        uint8_t x = x_start + i;
        for (j = 0; j <= ChartData[read_idx]; j++)
        {
            OLED_DrawPoint(x, j, 1);
        }
        read_idx++;
        if (read_idx >= 128)
            read_idx = 0;
    }

    // 7. Update Screen
    OLED_Refresh();
}

/* ==========================================================
 * 6. TEXT FUNCTIONS
 * ========================================================== */

void OLED_ShowChar(uint8_t x, uint8_t y, char chr)
{
    uint8_t i;
    uint8_t width = 6;

    // Quick Rejection
    if (x >= 128 || y > 7)
        return;

    // Clipping
    if (x + 6 > 128)
    {
        width = 128 - x;
    }

    if (chr < 32 || chr > 127)
        chr = ' ';
    const uint8_t *pSrc = F6x8[(uint8_t)chr - 32];
    uint8_t *pDst = &OLED_GRAM[y][x];

    for (i = 0; i < width; i++)
    {
        *pDst++ = *pSrc++;
    }
}

void OLED_ShowString(uint8_t x, uint8_t y, char *str)
{
    while (*str != '\0')
    {
        OLED_ShowChar(x, y, *str);
        x += 6;
        if (x > 122)
        {
            x = 0;
            y++;
        }
        str++;
    }
}

/* ==========================================================
 * 7. INITIALIZATION
 * ========================================================== */

void OLED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    SPI_InitTypeDef SPI_InitStructure = {0};

    RCC_APB1PeriphClockCmd(OLED_RCC_SPI, ENABLE);
    RCC_APB2PeriphClockCmd(OLED_RCC_GPIO, ENABLE);

    // SPI Pins
    GPIO_InitStructure.GPIO_Pin = OLED_PIN_SCK | OLED_PIN_MOSI;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_PORT, &GPIO_InitStructure);

    // Control Pins
    GPIO_InitStructure.GPIO_Pin = OLED_PIN_CS | OLED_PIN_DC | OLED_PIN_RES;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_PORT, &GPIO_InitStructure);

    OLED_CS_SET;
    OLED_DC_SET;
    OLED_RST_SET;

    // SPI Config
    SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI2, &SPI_InitStructure);

    SPI_Cmd(SPI2, ENABLE);

    // Reset Sequence
    OLED_RST_CLR;
    Delay_Ms(100);
    OLED_RST_SET;
    Delay_Ms(50);

    // Init Commands
    OLED_WriteCmd(0xAE);
    OLED_WriteCmd(0x00);
    OLED_WriteCmd(0x10);
    OLED_WriteCmd(0x40);
    OLED_WriteCmd(0x81);
    OLED_WriteCmd(0xCF);
    OLED_WriteCmd(0xA1);
    OLED_WriteCmd(0xC8);
    OLED_WriteCmd(0xA6);
    OLED_WriteCmd(0xA8);
    OLED_WriteCmd(0x3F);
    OLED_WriteCmd(0xD3);
    OLED_WriteCmd(0x00);
    OLED_WriteCmd(0xD5);
    OLED_WriteCmd(0xF0);
    OLED_WriteCmd(0xD9);
    OLED_WriteCmd(0xF1);
    OLED_WriteCmd(0xDA);
    OLED_WriteCmd(0x12);
    OLED_WriteCmd(0xDB);
    OLED_WriteCmd(0x40);
    OLED_WriteCmd(0x8D);
    OLED_WriteCmd(0x14);
    OLED_WriteCmd(0xAF);

    OLED_Clear();
    OLED_Refresh();
}