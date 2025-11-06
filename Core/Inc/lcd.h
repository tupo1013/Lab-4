/*
 * lcd.h
 * Updated: Nov 5, 2025
 */

#ifndef INC_LCD_H_
#define INC_LCD_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"     // kéo __IO, HAL types/macros

// =================== Scan direction ===================
#define DFT_SCAN_DIR  U2D_R2L

// chiều đọc ghi bộ nhớ hiển thị
#define L2R_U2D  0x00
#define L2R_D2U  0x80
#define R2L_U2D  0x40
#define R2L_D2U  0xC0

#define U2D_L2R  0x20
#define U2D_R2L  0x60
#define D2U_L2R  0xA0
#define D2U_R2L  0xE0

// =================== LCD device ===================
typedef struct {
  uint16_t width;
  uint16_t height;
  uint16_t id;
} _lcd_dev;

extern _lcd_dev lcddev;

// Memory-mapped LCD registers via FSMC/FMC
typedef struct {
  __IO uint16_t LCD_REG;
  __IO uint16_t LCD_RAM;
} LCD_TypeDef;

// Chú ý: địa chỉ này phụ thuộc cấu hình FSMC/FMC trên kit của bạn.
// Giữ nguyên như project gốc.
#define LCD_BASE        ((uint32_t)(0x60000000 | 0x000FFFFE))
#define LCD             ((LCD_TypeDef *) LCD_BASE)

// =================== Màu sắc 16-bit RGB565 ===================
#define WHITE          0xFFFF
#define BLACK          0x0000
#define BLUE           0x001F
#define BRED           0xF81F
#define GRED           0xFFE0
#define GBLUE          0x07FF
#define RED            0xF800
#define MAGENTA        0xF81F
#define GREEN          0x07E0
#define CYAN           0x7FFF
#define YELLOW         0xFFE0
#define BROWN          0xBC40
#define BRRED          0xFC07
#define GRAY           0x8430

#define DARKBLUE       0x01CF
#define LIGHTBLUE      0x7D7C
#define GRAYBLUE       0x5458

#define LIGHTGREEN     0x841F
#define LIGHTGRAY      0xEF5B
#define LGRAY          0xC618

#define LGRAYBLUE      0xA651
#define LBBLUE         0x2B12

// =================== Prototypes ===================
// Low-level
void     LCD_WR_REG(uint16_t reg);
void     LCD_WR_DATA(uint16_t data);
uint16_t LCD_RD_DATA(void);

// Cursor/Window
void lcd_SetCursor(uint16_t x, uint16_t y);
void lcd_AddressSet(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

// Power / basic ops
void lcd_DisplayOn(void);
void lcd_DisplayOff(void);
uint16_t lcd_ReadPoint(uint16_t x, uint16_t y);
void lcd_Clear(uint16_t color);

// Drawing
void lcd_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color);
void lcd_DrawPoint(uint16_t x, uint16_t y, uint16_t color);
void lcd_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcd_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcd_DrawCircle(int xc, int yc, uint16_t c, int r, int fill);

// Text / numbers
void     lcd_ShowChar(uint16_t x, uint16_t y, uint8_t num, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode);
uint32_t mypow(uint8_t m, uint8_t n);
void     lcd_ShowIntNum(uint16_t x, uint16_t y, uint16_t num, uint8_t len, uint16_t fc, uint16_t bc, uint8_t sizey);
void     lcd_ShowFloatNum1(uint16_t x, uint16_t y, float num, uint8_t len, uint16_t fc, uint16_t bc, uint8_t sizey);
void     lcd_ShowStr(uint16_t x, uint16_t y, uint8_t *str, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode);
void     lcd_StrCenter(uint16_t x, uint16_t y, uint8_t *str, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode);

// Bitmap
void lcd_ShowPicture(uint16_t x, uint16_t y, uint16_t length, uint16_t width, const uint8_t pic[]);

// Init / misc
void lcd_SetDir(uint8_t dir);
void lcd_init(void);
void lcd_ShowBackground(void);   // nếu bạn có implement riêng

#endif /* INC_LCD_H_ */
