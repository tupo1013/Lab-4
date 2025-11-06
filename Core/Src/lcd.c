/*
 * lcd.c
 * Updated: Nov 5, 2025
 */

#include "main.h"     // HAL_GPIO_WritePin, HAL_Delay, __IO, pin aliases
#include "lcd.h"
#include "lcdfont.h"
#include <stdint.h>

unsigned char s[50];

_lcd_dev lcddev;

/* =================== Low-level =================== */
void LCD_WR_REG(uint16_t reg)
{
  LCD->LCD_REG = reg;
}

void LCD_WR_DATA(uint16_t data)
{
  LCD->LCD_RAM = data;
}

uint16_t LCD_RD_DATA(void)
{
  __IO uint16_t ram;
  ram = LCD->LCD_RAM;
  return ram;
}

/* =================== Address & Cursor =================== */
void lcd_AddressSet(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
  LCD_WR_REG(0x2A);
  LCD_WR_DATA(x1 >> 8);
  LCD_WR_DATA(x1 & 0xFF);
  LCD_WR_DATA(x2 >> 8);
  LCD_WR_DATA(x2 & 0xFF);

  LCD_WR_REG(0x2B);
  LCD_WR_DATA(y1 >> 8);
  LCD_WR_DATA(y1 & 0xFF);
  LCD_WR_DATA(y2 >> 8);
  LCD_WR_DATA(y2 & 0xFF);

  LCD_WR_REG(0x2C);
}

void lcd_SetCursor(uint16_t x, uint16_t y)
{
  LCD_WR_REG(0x2A);
  LCD_WR_DATA(x >> 8);
  LCD_WR_DATA(x & 0xFF);
  LCD_WR_REG(0x2B);
  LCD_WR_DATA(y >> 8);
  LCD_WR_DATA(y & 0xFF);
}

/* =================== Display on/off =================== */
void lcd_DisplayOn(void)
{
  LCD_WR_REG(0x29);
}

void lcd_DisplayOff(void)
{
  LCD_WR_REG(0x28);
}

/* =================== Pixel read =================== */
uint16_t lcd_ReadPoint(uint16_t x, uint16_t y)
{
  uint16_t r = 0, g = 0, b = 0;
  lcd_SetCursor(x, y);
  LCD_WR_REG(0x2E);
  r = LCD_RD_DATA();     // dummy
  r = LCD_RD_DATA();     // R + upper G
  b = LCD_RD_DATA();     // B + lower G
  g = r & 0xFF;
  g <<= 8;
  return (((r >> 11) << 11) | ((g >> 10) << 5) | (b >> 11));
}

/* =================== Clear/Fill/Primitives =================== */
void lcd_Clear(uint16_t color)
{
  uint16_t i, j;
  lcd_AddressSet(0, 0, lcddev.width - 1, lcddev.height - 1);
  for (i = 0; i < lcddev.width; i++) {
    for (j = 0; j < lcddev.height; j++) {
      LCD_WR_DATA(color);
    }
  }
}

void lcd_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color)
{
  uint16_t i, j;
  lcd_AddressSet(xsta, ysta, xend - 1, yend - 1);
  for (i = ysta; i < yend; i++) {
    for (j = xsta; j < xend; j++) {
      LCD_WR_DATA(color);
    }
  }
}

void lcd_DrawPoint(uint16_t x, uint16_t y, uint16_t color)
{
  lcd_AddressSet(x, y, x, y);
  LCD_WR_DATA(color);
}

void lcd_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
  uint16_t t;
  int xerr = 0, yerr = 0, delta_x, delta_y, distance;
  int incx, incy, uRow, uCol;

  delta_x = x2 - x1; delta_y = y2 - y1;
  uRow = x1; uCol = y1;
  if (delta_x > 0) incx = 1;
  else if (delta_x == 0) incx = 0;
  else { incx = -1; delta_x = -delta_x; }
  if (delta_y > 0) incy = 1;
  else if (delta_y == 0) incy = 0;
  else { incy = -1; delta_y = -delta_y; }
  distance = (delta_x > delta_y) ? delta_x : delta_y;

  for (t = 0; t <= distance; t++) {
    lcd_DrawPoint(uRow, uCol, color);
    xerr += delta_x;
    yerr += delta_y;
    if (xerr > distance) { xerr -= distance; uRow += incx; }
    if (yerr > distance) { yerr -= distance; uCol += incy; }
  }
}

void lcd_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
  lcd_DrawLine(x1, y1, x2, y1, color);
  lcd_DrawLine(x1, y1, x1, y2, color);
  lcd_DrawLine(x1, y2, x2, y2, color);
  lcd_DrawLine(x2, y1, x2, y2, color);
}

/* =================== Text & Numbers =================== */
void lcd_ShowChar(uint16_t x, uint16_t y, uint8_t num, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{
  uint8_t temp, sizex, t, m = 0;
  uint16_t i, TypefaceNum;
  uint16_t x0 = x;
  sizex = sizey / 2;
  TypefaceNum = (sizex / 8 + ((sizex % 8) ? 1 : 0)) * sizey;
  num = num - ' ';
  lcd_AddressSet(x, y, x + sizex - 1, y + sizey - 1);

  for (i = 0; i < TypefaceNum; i++) {
    if (sizey == 12) { /* bạn chưa có font 12 ở lcdfont.h */ }
    else if (sizey == 16) temp = ascii_1608[num][i];
    else if (sizey == 24) temp = ascii_2412[num][i];
    else if (sizey == 32) temp = ascii_3216[num][i];
    else return;

    for (t = 0; t < 8; t++) {
      if (!mode) {
        if (temp & (0x01 << t)) LCD_WR_DATA(fc);
        else LCD_WR_DATA(bc);
        m++;
        if (m % sizex == 0) { m = 0; break; }
      } else {
        if (temp & (0x01 << t)) lcd_DrawPoint(x, y, fc);
        x++;
        if ((x - x0) == sizex) { x = x0; y++; break; }
      }
    }
  }
}

uint32_t mypow(uint8_t m, uint8_t n)
{
  uint32_t result = 1;
  while (n--) result *= m;
  return result;
}

void lcd_ShowIntNum(uint16_t x, uint16_t y, uint16_t num, uint8_t len, uint16_t fc, uint16_t bc, uint8_t sizey)
{
  uint8_t t, temp;
  uint8_t enshow = 0;
  uint8_t sizex = sizey / 2;
  for (t = 0; t < len; t++) {
    temp = (num / mypow(10, len - t - 1)) % 10;
    if (enshow == 0 && t < (len - 1)) {
      if (temp == 0) {
        lcd_ShowChar(x + t * sizex, y, ' ', fc, bc, sizey, 0);
        continue;
      } else enshow = 1;
    }
    lcd_ShowChar(x + t * sizex, y, temp + 48, fc, bc, sizey, 0);
  }
}

void lcd_ShowFloatNum1(uint16_t x, uint16_t y, float num, uint8_t len, uint16_t fc, uint16_t bc, uint8_t sizey)
{
  uint8_t t, temp, sizex;
  uint16_t num1;
  sizex = sizey / 2;
  num1 = (uint16_t)(num * 100.0f);
  for (t = 0; t < len; t++) {
    temp = (num1 / mypow(10, len - t - 1)) % 10;
    if (t == (len - 2)) {
      lcd_ShowChar(x + (len - 2) * sizex, y, '.', fc, bc, sizey, 0);
      t++;
      len += 1;
    }
    lcd_ShowChar(x + t * sizex, y, temp + 48, fc, bc, sizey, 0);
  }
}

void lcd_ShowStr(uint16_t x, uint16_t y, uint8_t *str, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{
  uint16_t x0 = x;
  uint8_t bHz = 0;
  while (*str != 0) {
    if (!bHz) {
      if (x > (lcddev.width - sizey / 2) || y > (lcddev.height - sizey)) return;
      if (*str > 0x80) bHz = 1;
      else {
        if (*str == 0x0D) {
          y += sizey;
          x = x0;
          str++;
        } else {
          lcd_ShowChar(x, y, *str, fc, bc, sizey, mode);
          x += sizey / 2;
          str++;
        }
      }
    }
  }
}

void lcd_StrCenter(uint16_t x, uint16_t y, uint8_t *str, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{
  uint16_t len = strlen((const char *)str);
  uint16_t x1 = (lcddev.width - len * 8) / 2;
  lcd_ShowStr(x + x1, y, str, fc, bc, sizey, mode);
}

/* =================== Bitmap =================== */
void lcd_ShowPicture(uint16_t x, uint16_t y, uint16_t length, uint16_t width, const uint8_t pic[])
{
  uint8_t picH, picL;
  uint16_t i, j;
  uint32_t k = 0;
  lcd_AddressSet(x, y, x + length - 1, y + width - 1);
  for (i = 0; i < length; i++) {
    for (j = 0; j < width; j++) {
      picH = pic[k * 2];
      picL = pic[k * 2 + 1];
      LCD_WR_DATA(((uint16_t)picH << 8) | picL);
      k++;
    }
  }
}

/* =================== Orientation =================== */
void lcd_SetDir(uint8_t dir)
{
  if ((dir >> 4) % 4) {
    lcddev.width = 320;
    lcddev.height = 240;
  } else {
    lcddev.width = 240;
    lcddev.height = 320;
  }
}

/* =================== Init =================== */
void lcd_init(void)
{
  // Reset sequence
  HAL_GPIO_WritePin(FSMC_RES_GPIO_Port, FSMC_RES_Pin, GPIO_PIN_RESET);
  HAL_Delay(500);
  HAL_GPIO_WritePin(FSMC_RES_GPIO_Port, FSMC_RES_Pin, GPIO_PIN_SET);
  HAL_Delay(500);

  lcd_SetDir(L2R_U2D);

  LCD_WR_REG(0xD3);
  lcddev.id = LCD_RD_DATA(); // dummy
  lcddev.id = LCD_RD_DATA();
  lcddev.id = LCD_RD_DATA();
  lcddev.id <<= 8;
  lcddev.id |= LCD_RD_DATA();

  LCD_WR_REG(0xCF);
  LCD_WR_DATA(0x00);
  LCD_WR_DATA(0xC1);
  LCD_WR_DATA(0x30);

  LCD_WR_REG(0xED);
  LCD_WR_DATA(0x64);
  LCD_WR_DATA(0x03);
  LCD_WR_DATA(0x12);
  LCD_WR_DATA(0x81);

  LCD_WR_REG(0xE8);
  LCD_WR_DATA(0x85);
  LCD_WR_DATA(0x10);
  LCD_WR_DATA(0x7A);

  LCD_WR_REG(0xCB);
  LCD_WR_DATA(0x39);
  LCD_WR_DATA(0x2C);
  LCD_WR_DATA(0x00);
  LCD_WR_DATA(0x34);
  LCD_WR_DATA(0x02);

  LCD_WR_REG(0xF7);
  LCD_WR_DATA(0x20);

  LCD_WR_REG(0xEA);
  LCD_WR_DATA(0x00);
  LCD_WR_DATA(0x00);

  LCD_WR_REG(0xC0);  // Power control
  LCD_WR_DATA(0x1B); // VRH[5:0]
  LCD_WR_REG(0xC1);  // Power control
  LCD_WR_DATA(0x01); // SAP[2:0];BT[3:0]

  LCD_WR_REG(0xC5);  // VCM control
  LCD_WR_DATA(0x30);
  LCD_WR_DATA(0x30);

  LCD_WR_REG(0xC7);  // VCM control2
  LCD_WR_DATA(0xB7);

  LCD_WR_REG(0x36);  // Memory Access Control
  LCD_WR_DATA(0x08 | L2R_U2D);
  // LCD_WR_DATA(0x08 | DFT_SCAN_DIR);

  LCD_WR_REG(0x3A);
  LCD_WR_DATA(0x55);

  LCD_WR_REG(0xB1);
  LCD_WR_DATA(0x00);
  LCD_WR_DATA(0x1A);

  LCD_WR_REG(0xB6);  // Display Function Control
  LCD_WR_DATA(0x0A);
  LCD_WR_DATA(0xA2);

  LCD_WR_REG(0xF2);  // 3Gamma Function Disable
  LCD_WR_DATA(0x00);

  LCD_WR_REG(0x26);  // Gamma curve selected
  LCD_WR_DATA(0x01);

  LCD_WR_REG(0xE0);  // Set Gamma
  LCD_WR_DATA(0x0F); LCD_WR_DATA(0x2A); LCD_WR_DATA(0x28); LCD_WR_DATA(0x08);
  LCD_WR_DATA(0x0E); LCD_WR_DATA(0x08); LCD_WR_DATA(0x54); LCD_WR_DATA(0xA9);
  LCD_WR_DATA(0x43); LCD_WR_DATA(0x0A); LCD_WR_DATA(0x0F); LCD_WR_DATA(0x00);
  LCD_WR_DATA(0x00); LCD_WR_DATA(0x00); LCD_WR_DATA(0x00);

  LCD_WR_REG(0xE1);  // Set Gamma
  LCD_WR_DATA(0x00); LCD_WR_DATA(0x15); LCD_WR_DATA(0x17); LCD_WR_DATA(0x07);
  LCD_WR_DATA(0x11); LCD_WR_DATA(0x06); LCD_WR_DATA(0x2B); LCD_WR_DATA(0x56);
  LCD_WR_DATA(0x3C); LCD_WR_DATA(0x05); LCD_WR_DATA(0x10); LCD_WR_DATA(0x0F);
  LCD_WR_DATA(0x3F); LCD_WR_DATA(0x3F); LCD_WR_DATA(0x0F);

  LCD_WR_REG(0x2B);
  LCD_WR_DATA(0x00);
  LCD_WR_DATA(0x00);
  LCD_WR_DATA(0x01);
  LCD_WR_DATA(0x3F);

  LCD_WR_REG(0x2A);
  LCD_WR_DATA(0x00);
  LCD_WR_DATA(0x00);
  LCD_WR_DATA(0x00);
  LCD_WR_DATA(0xEF);

  LCD_WR_REG(0x11); // Exit Sleep
  HAL_Delay(120);
  LCD_WR_REG(0x29); // Display on

  HAL_GPIO_WritePin(FSMC_BLK_GPIO_Port, FSMC_BLK_Pin, GPIO_PIN_SET);
}

/* =================== Circles =================== */
static void _draw_circle_8(int xc, int yc, int x, int y, uint16_t c)
{
  lcd_DrawPoint(xc + x, yc + y, c);
  lcd_DrawPoint(xc - x, yc + y, c);
  lcd_DrawPoint(xc + x, yc - y, c);
  lcd_DrawPoint(xc - x, yc - y, c);
  lcd_DrawPoint(xc + y, yc + x, c);
  lcd_DrawPoint(xc - y, yc + x, c);
  lcd_DrawPoint(xc + y, yc - x, c);
  lcd_DrawPoint(xc - y, yc - x, c);
}

void lcd_DrawCircle(int xc, int yc, uint16_t c, int r, int fill)
{
  int x = 0, y = r, yi, d;
  d = 3 - 2 * r;

  if (fill) {
    while (x <= y) {
      for (yi = x; yi <= y; yi++) _draw_circle_8(xc, yc, x, yi, c);
      if (d < 0) d = d + 4 * x + 6;
      else { d = d + 4 * (x - y) + 10; y--; }
      x++;
    }
  } else {
    while (x <= y) {
      _draw_circle_8(xc, yc, x, y, c);
      if (d < 0) d = d + 4 * x + 6;
      else { d = d + 4 * (x - y) + 10; y--; }
      x++;
    }
  }
}

/* =================== Optional helpers =================== */
void lcd_ShowBackground(void)
{
  // Tuỳ bạn vẽ nền mặc định ở đây
  // vd: lcd_Clear(BLACK);
}

void DrawTestPage(uint8_t *str)
{
  lcd_Fill(0, 0, lcddev.width, 20, BLUE);
  lcd_Fill(0, lcddev.height - 20, lcddev.width, lcddev.height, BLUE);
  lcd_StrCenter(0, 2, str, WHITE, BLUE, 16, 1);
  lcd_StrCenter(0, lcddev.height - 18, (uint8_t*)"Test page", WHITE, BLUE, 16, 1);
  lcd_Fill(0, 20, lcddev.width, lcddev.height - 20, BLACK);
}

/* Nếu bạn muốn đổ toàn bộ buffer từ SRAM ngoài, có thể sửa ở đây */
void lcd_Display(void)
{
  uint16_t i, j;
  uint16_t send = 0; // placeholder
  lcd_AddressSet(0, 0, lcddev.width - 1, lcddev.height - 1);
  for (i = 0; i < lcddev.width; i++) {
    for (j = 0; j < lcddev.height; j++) {
      LCD_WR_DATA(send);
    }
  }
}
