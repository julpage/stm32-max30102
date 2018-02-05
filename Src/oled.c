#include "stm32f1xx_hal.h"
#include "oled.h"
#include "oledfont.h"

#define SIZE 16
#define XLevelL 0x02
#define XLevelH 0x10
#define Max_Column 128
#define Max_Row 64
#define Brightness 0xFF
#define X_WIDTH 128
#define Y_WIDTH 64

#define OLED_NUM_A 0x1
#define OLED_NUM_B 0x2
#define OLED_NUM_C 0x4
#define OLED_NUM_D 0x8
#define OLED_NUM_E 0x10
#define OLED_NUM_F 0x20
#define OLED_NUM_G 0x40

extern I2C_HandleTypeDef hi2c1;

uint8_t point_x = 0;
uint8_t point_y = 0;
uint8_t oled_cache[8][128];

uint8_t whatToDo = 0;
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    static uint8_t y = 0;
    static uint8_t dat = 0;
    if (whatToDo == 4)
    {
        OLED_WR_Byte(0X8D, OLED_CMD); //SET DCDC命令
        OLED_WR_Byte(0X10, OLED_CMD); //DCDC OFF
        OLED_WR_Byte(0XAE, OLED_CMD); //DISPLAY OFF
    }
    else if (whatToDo == 3)
    {
        // OLED_WR_Byte((x & 0x0f), OLED_CMD); //0+列之地低位
        dat = 0x00;
        HAL_I2C_Mem_Write_IT(&hi2c1, OLED_ADDR, 0x00, I2C_MEMADD_SIZE_8BIT, &dat, 1);
        whatToDo = 0;
    }
    else if (whatToDo == 2)
    {
        // OLED_WR_Byte(((x & 0xf0) >> 4) | 0x10, OLED_CMD); //8+列地址高位
        dat = 0x10;
        HAL_I2C_Mem_Write_IT(&hi2c1, OLED_ADDR, 0x00, I2C_MEMADD_SIZE_8BIT, &dat, 1);
        whatToDo = 3;
    }
    else if (whatToDo == 1)
    {
        // OLED_WR_Byte(0xb0 + y, OLED_CMD); //b+页地址
        dat = 0xb0 + y;
        HAL_I2C_Mem_Write_IT(&hi2c1, OLED_ADDR, 0x00, I2C_MEMADD_SIZE_8BIT, &dat, 1);
        whatToDo = 2;
    }
    else if (whatToDo == 0)
    {
        HAL_I2C_Mem_Write_IT(&hi2c1, OLED_ADDR, 0x40, I2C_MEMADD_SIZE_8BIT, (uint8_t *)(&oled_cache[y]), 128);
        y += 1;
        if (y > 7)
        {
            y = 0;
        }
        whatToDo = 1;
    }
}

void OLED_WR_Byte(uint8_t dat, uint8_t cmd)
{
    if (cmd == OLED_CMD)
        HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR, 0x00, I2C_MEMADD_SIZE_8BIT, &dat, 1, 50);
    else
    {
        // HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR, 0x40, I2C_MEMADD_SIZE_8BIT, &dat, 1, 5);
        // // HAL_I2C_Mem_Write_IT(&hi2c1, OLED_ADDR, 0x40, I2C_MEMADD_SIZE_8BIT, &dat, 1);
        oled_cache[point_y][point_x] = dat;
        point_x += 1;
    }
}

////开启OLED显示
//void OLED_Display_On(void)
//{
//    OLED_WR_Byte(0X8D, OLED_CMD); //SET DCDC命令
//    OLED_WR_Byte(0X14, OLED_CMD); //DCDC ON
//    OLED_WR_Byte(0XAF, OLED_CMD); //DISPLAY ON
//}
//关闭OLED显示
void OLED_Display_Off(void)
{
    whatToDo = 4;
}
//坐标设置
void OLED_Set_Pos(uint8_t x, uint8_t y)
{
    point_x = x;
    point_y = y;
}

//清屏函数
void OLED_Clear(void)
{
    uint8_t i, n;
    for (i = 0; i < 8; i++)
    {
        OLED_Set_Pos(0, i);
        for (n = 0; n < 128; n++)
            OLED_WR_Byte(0x00, OLED_DATA);
    }
}
//在指定位置显示一个字符串,包括部分字符
//x:0~127
//y:0~7
void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *str)
{
    uint8_t c = 0, i = 0;
    while (*str != '\0')
    {
        c = *str - ' '; //得到偏移后的值
        if (x > Max_Column - 1)
        {
            x = 0;
            y++;
        }
        OLED_Set_Pos(x, y);
        for (i = 0; i < 6; i++)
            OLED_WR_Byte(F6x8[c][i], OLED_DATA);
        x += 6;
        str++;
    }
}

//初始化SSD1306
void OLED_Init(void)
{
    OLED_WR_Byte(0xAE, OLED_CMD); //--display off
    OLED_WR_Byte(0x00, OLED_CMD); //---列地址低4位
    OLED_WR_Byte(0x10, OLED_CMD); //---列地址高4位
    OLED_WR_Byte(0x40, OLED_CMD); //--set start line address
    OLED_WR_Byte(0xB0, OLED_CMD); //--页地址
    OLED_WR_Byte(0x81, OLED_CMD); // contract control
    OLED_WR_Byte(0xf0, OLED_CMD); //--128
    OLED_WR_Byte(0xA1, OLED_CMD); //set segment remap
    OLED_WR_Byte(0xA6, OLED_CMD); //--a6正显，a7反显
    OLED_WR_Byte(0xA8, OLED_CMD); //--set multiplex ratio(1 to 64)
    OLED_WR_Byte(0x3F, OLED_CMD); //--1/32 duty
    OLED_WR_Byte(0xC8, OLED_CMD); //c0或c8垂直翻转
    OLED_WR_Byte(0xD3, OLED_CMD); //-set display offset
    OLED_WR_Byte(0x00, OLED_CMD); //

    OLED_WR_Byte(0xD5, OLED_CMD); //set osc division
    OLED_WR_Byte(0x80, OLED_CMD); //

    //     OLED_WR_Byte(0xD8, OLED_CMD); //set area color mode off
    //     OLED_WR_Byte(0x05, OLED_CMD); //

    OLED_WR_Byte(0xD9, OLED_CMD); //Set Pre-Charge Period
    OLED_WR_Byte(0xF1, OLED_CMD); //

    OLED_WR_Byte(0xDA, OLED_CMD); //set com pin configuartion
    OLED_WR_Byte(0x12, OLED_CMD); //

    OLED_WR_Byte(0xDB, OLED_CMD); //set Vcomh
    OLED_WR_Byte(0x30, OLED_CMD); //

    OLED_WR_Byte(0x8D, OLED_CMD); //set charge pump enable
    OLED_WR_Byte(0x14, OLED_CMD); //

    OLED_WR_Byte(0xAF, OLED_CMD); //--屏幕开关，af开，ae关

    OLED_Clear();

    uint8_t i;
    // 画心形
    OLED_showHeart(1);
    //画bpm
    const uint8_t bpm[] = {
        0x3E, 0x28, 0x38, 0x00, 0x3E, 0x0A, 0x0E, 0x00, 0x3E, 0x02, 0x1C, 0x02, 0x3E};
    OLED_Set_Pos(58, 4);
    for (i = 0; i < 13; i++)
    {
        OLED_WR_Byte(bpm[i], OLED_DATA);
    }
    //画%
    const uint8_t percent[] = {
        0x63, 0x33, 0x18, 0x0C, 0x66, 0x63};
    OLED_Set_Pos(121, 4);
    for (i = 0; i < 6; i++)
    {
        OLED_WR_Byte(percent[i], OLED_DATA);
    }
    //画spo2
    const uint8_t spo2[] = {
        0x38, 0x44, 0x44, 0x44, 0x88, 0x00, 0xFC, 0x44, 0x44, 0x44, 0x38,
        0xE1, 0x12, 0x12, 0x12, 0xE1, 0x00, 0x23, 0x10, 0x10, 0x90, 0x60,
        0x07, 0x08, 0x08, 0x08, 0x07, 0x00, 0x0C, 0x0A, 0x09, 0x08, 0x08};
    OLED_Set_Pos(73, 2);
    for (i = 0; i < 11; i++)
    {
        OLED_WR_Byte(spo2[i], OLED_DATA);
    }
    OLED_Set_Pos(73, 3);
    for (i = 0; i < 11; i++)
    {
        OLED_WR_Byte(spo2[11 + i], OLED_DATA);
    }
    OLED_Set_Pos(73, 4);
    for (i = 0; i < 11; i++)
    {
        OLED_WR_Byte(spo2[22 + i], OLED_DATA);
    }
    OLED_ShowString(0, 0, "Call_Now_Yeah Studio");
    OLED_ShowString(0, 1, "Heart");
    OLED_ShowString(33, 1, "Rate");
    OLED_ShowString(60, 1, "&");
    OLED_ShowString(69, 1, "Oximetry");

    HAL_I2C_MemTxCpltCallback(&hi2c1);
}

const uint8_t heart[] = {
    0xF8, 0xFC, 0xFE, 0xFF, 0xFF, 0xFF, 0xFE, 0xFC,
    0xF8, 0xF0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF,
    0xFF, 0xFF, 0xFE, 0xFC, 0xF8,
    0x01, 0x07, 0x1F, 0x3F, 0x7F, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x7F, 0x3F, 0x1F, 0x07, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03,
    0x07, 0x0F, 0x0F, 0x0F, 0x07, 0x03, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00};
void OLED_showHeart(uint8_t showOrNot)
{
    uint8_t i;
    // 画心形
    OLED_Set_Pos(1, 2);
    if (showOrNot)
    {
        for (i = 0; i < 21; i++)
        {
            OLED_WR_Byte(heart[i], OLED_DATA);
        }
        OLED_Set_Pos(1, 3);
        for (i = 0; i < 21; i++)
        {
            OLED_WR_Byte(heart[21 + i], OLED_DATA);
        }
        OLED_Set_Pos(1, 4);
        for (i = 0; i < 21; i++)
        {
            OLED_WR_Byte(heart[42 + i], OLED_DATA);
        }
    }
    else
    {
        for (i = 0; i < 21; i++)
        {
            OLED_WR_Byte(0, OLED_DATA);
        }
        OLED_Set_Pos(1, 3);
        for (i = 0; i < 21; i++)
        {
            OLED_WR_Byte(0, OLED_DATA);
        }
        OLED_Set_Pos(1, 4);
        for (i = 0; i < 21; i++)
        {
            OLED_WR_Byte(0, OLED_DATA);
        }
    }
}

//显示数字，心率888，spo2 888
const uint8_t oled_nums[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x00};
const uint8_t oled_nums_pos[] = {24, 36, 47, 86, 98, 109};
void OLED_showNum(uint8_t which, uint8_t num)
{
    uint8_t dat = oled_nums[num];
    uint8_t start = oled_nums_pos[which];
    uint8_t i, temp;
    OLED_Set_Pos(start, 2);
    for (i = 0; i < 10; i++)
    {
        temp = 0;
        if (i <= 1)
        {
            if (dat & OLED_NUM_F)
                temp |= 0xf8;
        }
        else if (i >= 2 && i <= 7)
        {
            if (dat & OLED_NUM_A)
                temp |= 0x06;
        }
        else
        {
            if (dat & OLED_NUM_B)
                temp |= 0xf8;
        }
        OLED_WR_Byte(temp, OLED_DATA);
    }
    OLED_Set_Pos(start, 3);
    for (i = 0; i < 10; i++)
    {
        temp = 0;
        if (i <= 1)
        {
            if (dat & OLED_NUM_F)
                temp |= 0x03;
            if (dat & OLED_NUM_E)
                temp |= 0xf0;
        }
        else if (i >= 2 && i <= 7)
        {
            if (dat & OLED_NUM_G)
                temp |= 0x0c;
        }
        else
        {
            if (dat & OLED_NUM_B)
                temp |= 0x03;
            if (dat & OLED_NUM_C)
                temp |= 0xf0;
        }
        OLED_WR_Byte(temp, OLED_DATA);
    }
    OLED_Set_Pos(start, 4);
    for (i = 0; i < 10; i++)
    {
        temp = 0;
        if (i <= 1)
        {
            if (dat & OLED_NUM_E)
                temp |= 0x07;
        }
        else if (i >= 2 && i <= 7)
        {
            if (dat & OLED_NUM_D)
                temp |= 0x18;
        }
        else
        {
            if (dat & OLED_NUM_C)
                temp |= 0x07;
        }
        OLED_WR_Byte(temp, OLED_DATA);
    }
}

//画图,128*24
uint8_t pos_x_this = 0;
char pos_y_old = 0;
char max(char a, char b) { return a > b ? a : b; }
char min(char a, char b) { return a < b ? a : b; }
void OLED_drawChart(float value)
{
    uint8_t dat[] = {0xff, 0xff, 0xff};
    char pos_y = (char)(value * 0.12) + 10;
    if (pos_y > 23)
        pos_y = 23;
    if (pos_y <= 0)
        pos_y = 0;
    char i, ii, blank;
    char y_max = max(pos_y, pos_y_old);
    char y_min = min(pos_y, pos_y_old);

    if (y_max == y_min)
        y_max = y_min + 1;
    for (i = 0; i < 3; i++)
    {
        if ((y_min - i * 8) >= 0)
            dat[i] &= (uint8_t)(dat[i] >> (y_min - i * 8));
        if (((i + 1) * 8 - y_max) >= 0)
            dat[i] &= (uint8_t)(dat[i] << ((i + 1) * 8 - y_max));
    }
    for (i = 0; i < 3; i++)
    {
        OLED_Set_Pos(pos_x_this, 7 - i);
        OLED_WR_Byte(dat[i], OLED_DATA);
        blank = 127 - pos_x_this;
        if (blank > 3)
            blank = 3;
        for (ii = 0; ii < blank; ii++)
            OLED_WR_Byte(0, OLED_DATA);
    }
    pos_y_old = pos_y;
    pos_x_this++;
    if (pos_x_this > 127)
        pos_x_this = 0;
}