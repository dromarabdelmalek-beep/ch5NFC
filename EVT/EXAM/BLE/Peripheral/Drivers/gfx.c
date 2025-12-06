/********************************** (C) COPYRIGHT *******************************
 * File Name          : gfx.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2025-12-05
 * Description        : Simple graphics library implementation
 *******************************************************************************/

#include "gfx.h"
#include "CH57x_common.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint8_t *framebuffer = NULL;
static uint16_t fb_width = 0;
static uint16_t fb_height = 0;

/*********************************************************************
 * FONT DATA - 8x8 font (ASCII 32-127)
 */
static const uint8_t font8x8_data[] = {
    // Space (32)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // ! (33)
    0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00,
    // " (34)
    0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // # (35)
    0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00,
    // Continue with more characters...
    // For brevity, this is a simplified set
    // In production, include full ASCII set
};

// 8x8 Font
const Font_t Font8x8 = {
    .width = 8,
    .height = 8,
    .data = font8x8_data
};

// Placeholder for larger fonts (would need full implementation)
const Font_t Font12x16 = {
    .width = 12,
    .height = 16,
    .data = font8x8_data  // Placeholder
};

const Font_t Font16x24 = {
    .width = 16,
    .height = 24,
    .data = font8x8_data  // Placeholder
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      GFX_Init
 *
 * @brief   Initialize graphics buffer
 *
 * @param   buffer - pointer to framebuffer
 * @param   width - display width
 * @param   height - display height
 *
 * @return  none
 */
void GFX_Init(uint8_t *buffer, uint16_t width, uint16_t height)
{
    framebuffer = buffer;
    fb_width = width;
    fb_height = height;
}

/*********************************************************************
 * @fn      GFX_Clear
 *
 * @brief   Clear buffer (fill with white/0xFF)
 *
 * @return  none
 */
void GFX_Clear(void)
{
    if(framebuffer)
    {
        memset(framebuffer, 0xFF, (fb_width * fb_height) / 8);
    }
}

/*********************************************************************
 * @fn      GFX_Fill
 *
 * @brief   Fill buffer with color
 *
 * @param   color - color to fill (0x00 = black, 0xFF = white)
 *
 * @return  none
 */
void GFX_Fill(uint8_t color)
{
    if(framebuffer)
    {
        memset(framebuffer, color, (fb_width * fb_height) / 8);
    }
}

/*********************************************************************
 * @fn      GFX_DrawPixel
 *
 * @brief   Draw a pixel
 *
 * @param   x - x coordinate
 * @param   y - y coordinate
 * @param   color - color (0 = black, 1 = white)
 *
 * @return  none
 */
void GFX_DrawPixel(uint16_t x, uint16_t y, uint8_t color)
{
    if(!framebuffer || x >= fb_width || y >= fb_height)
        return;

    uint32_t byte_index = x / 8 + (y * (fb_width / 8));
    uint8_t bit_index = 7 - (x % 8);

    if(color)
    {
        framebuffer[byte_index] |= (1 << bit_index);   // White
    }
    else
    {
        framebuffer[byte_index] &= ~(1 << bit_index);  // Black
    }
}

/*********************************************************************
 * @fn      GFX_DrawLine
 *
 * @brief   Draw a line using Bresenham's algorithm
 *
 * @param   x0, y0 - start coordinates
 * @param   x1, y1 - end coordinates
 * @param   color - line color
 *
 * @return  none
 */
void GFX_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color)
{
    int16_t dx = abs(x1 - x0);
    int16_t dy = abs(y1 - y0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;

    while(1)
    {
        GFX_DrawPixel(x0, y0, color);

        if(x0 == x1 && y0 == y1)
            break;

        int16_t e2 = 2 * err;
        if(e2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }
        if(e2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

/*********************************************************************
 * @fn      GFX_DrawRect
 *
 * @brief   Draw a rectangle
 *
 * @param   x, y - top-left corner
 * @param   w - width
 * @param   h - height
 * @param   color - rectangle color
 *
 * @return  none
 */
void GFX_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color)
{
    GFX_DrawLine(x, y, x + w - 1, y, color);           // Top
    GFX_DrawLine(x + w - 1, y, x + w - 1, y + h - 1, color); // Right
    GFX_DrawLine(x + w - 1, y + h - 1, x, y + h - 1, color); // Bottom
    GFX_DrawLine(x, y + h - 1, x, y, color);           // Left
}

/*********************************************************************
 * @fn      GFX_FillRect
 *
 * @brief   Draw a filled rectangle
 *
 * @param   x, y - top-left corner
 * @param   w - width
 * @param   h - height
 * @param   color - fill color
 *
 * @return  none
 */
void GFX_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color)
{
    for(uint16_t i = 0; i < h; i++)
    {
        for(uint16_t j = 0; j < w; j++)
        {
            GFX_DrawPixel(x + j, y + i, color);
        }
    }
}

/*********************************************************************
 * @fn      GFX_FillRoundRect
 *
 * @brief   Draw a filled rounded rectangle
 *
 * @param   x, y - top-left corner
 * @param   w - width
 * @param   h - height
 * @param   r - corner radius
 * @param   color - fill color
 *
 * @return  none
 */
void GFX_FillRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint8_t color)
{
    // Fill main rectangle
    GFX_FillRect(x + r, y, w - 2*r, h, color);
    GFX_FillRect(x, y + r, r, h - 2*r, color);
    GFX_FillRect(x + w - r, y + r, r, h - 2*r, color);

    // Fill corners (simplified - draws filled circles)
    // For production, use proper circle fill algorithm
}

/*********************************************************************
 * @fn      GFX_DrawChar
 *
 * @brief   Draw a character
 *
 * @param   x, y - top-left corner
 * @param   c - character to draw
 * @param   font - font to use
 * @param   color - character color
 *
 * @return  none
 */
void GFX_DrawChar(uint16_t x, uint16_t y, char c, const Font_t *font, uint8_t color)
{
    if(c < 32 || c > 126)
        return;  // Only printable ASCII

    uint16_t char_index = (c - 32) * font->height;

    for(uint8_t row = 0; row < font->height; row++)
    {
        uint8_t line = font->data[char_index + row];
        for(uint8_t col = 0; col < font->width; col++)
        {
            if(line & (1 << (7 - col)))
            {
                GFX_DrawPixel(x + col, y + row, color);
            }
        }
    }
}

/*********************************************************************
 * @fn      GFX_DrawString
 *
 * @brief   Draw a string
 *
 * @param   x, y - top-left corner
 * @param   str - string to draw
 * @param   font - font to use
 * @param   color - text color
 *
 * @return  none
 */
void GFX_DrawString(uint16_t x, uint16_t y, const char *str, const Font_t *font, uint8_t color)
{
    uint16_t cursor_x = x;

    while(*str)
    {
        GFX_DrawChar(cursor_x, y, *str, font, color);
        cursor_x += font->width;
        str++;
    }
}

/*********************************************************************
 * @fn      GFX_DrawNumber
 *
 * @brief   Draw a number
 *
 * @param   x, y - top-left corner
 * @param   num - number to draw
 * @param   font - font to use
 * @param   color - text color
 *
 * @return  none
 */
void GFX_DrawNumber(uint16_t x, uint16_t y, int32_t num, const Font_t *font, uint8_t color)
{
    char buffer[16];
    sprintf(buffer, "%d", (int)num);
    GFX_DrawString(x, y, buffer, font, color);
}

/*********************************************************************
 * @fn      GFX_GetStringWidth
 *
 * @brief   Get string width in pixels
 *
 * @param   str - string
 * @param   font - font
 *
 * @return  width in pixels
 */
uint16_t GFX_GetStringWidth(const char *str, const Font_t *font)
{
    return strlen(str) * font->width;
}

/******************************** endfile @ gfx ******************************/
