/********************************** (C) COPYRIGHT *******************************
 * File Name          : gfx.h
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2025-12-05
 * Description        : Simple graphics library for e-paper display
 *******************************************************************************/

#ifndef GFX_H
#define GFX_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */
#include "CONFIG.h"

/*********************************************************************
 * CONSTANTS
 */

// Font sizes
typedef enum {
    FONT_SIZE_8 = 0,    // 8px height
    FONT_SIZE_12,       // 12px height
    FONT_SIZE_16,       // 16px height
    FONT_SIZE_24        // 24px height
} FontSize_t;

/*********************************************************************
 * TYPEDEFS
 */

// Simple font structure
typedef struct {
    uint8_t width;      // Character width in pixels
    uint8_t height;     // Character height in pixels
    const uint8_t *data; // Font bitmap data
} Font_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */

// Built-in fonts
extern const Font_t Font8x8;
extern const Font_t Font12x16;
extern const Font_t Font16x24;

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Initialize graphics buffer
 */
extern void GFX_Init(uint8_t *buffer, uint16_t width, uint16_t height);

/*
 * Clear buffer (fill with white)
 */
extern void GFX_Clear(void);

/*
 * Fill buffer with color
 */
extern void GFX_Fill(uint8_t color);

/*
 * Draw a pixel
 */
extern void GFX_DrawPixel(uint16_t x, uint16_t y, uint8_t color);

/*
 * Draw a line
 */
extern void GFX_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color);

/*
 * Draw a rectangle
 */
extern void GFX_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color);

/*
 * Draw a filled rectangle
 */
extern void GFX_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color);

/*
 * Draw a rounded rectangle
 */
extern void GFX_DrawRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint8_t color);

/*
 * Draw a filled rounded rectangle
 */
extern void GFX_FillRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint8_t color);

/*
 * Draw a character
 */
extern void GFX_DrawChar(uint16_t x, uint16_t y, char c, const Font_t *font, uint8_t color);

/*
 * Draw a string
 */
extern void GFX_DrawString(uint16_t x, uint16_t y, const char *str, const Font_t *font, uint8_t color);

/*
 * Draw a number
 */
extern void GFX_DrawNumber(uint16_t x, uint16_t y, int32_t num, const Font_t *font, uint8_t color);

/*
 * Get string width in pixels
 */
extern uint16_t GFX_GetStringWidth(const char *str, const Font_t *font);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* GFX_H */
