/*
 * Authors (alphabetical order)
 * - Andre Bernet <bernet.andre@gmail.com>
 * - Andreas Weitl
 * - Bertrand Songis <bsongis@gmail.com>
 * - Bryan J. Rentoul (Gruvin) <gruvin@gmail.com>
 * - Cameron Weeks <th9xer@gmail.com>
 * - Erez Raviv
 * - Gabriel Birkus
 * - Jean-Pierre Parisy
 * - Karl Szmutny
 * - Michael Blandford
 * - Michal Hlavinka
 * - Pat Mackenzie
 * - Philip Moss
 * - Rob Thomson
 * - Romolo Manfredini <romolo.manfredini@gmail.com>
 * - Thomas Husterer
 *
 * opentx is based on code named
 * gruvin9x by Bryan J. Rentoul: http://code.google.com/p/gruvin9x/,
 * er9x by Erez Raviv: http://code.google.com/p/er9x/,
 * and the original (and ongoing) project by
 * Thomas Husterer, th9x: http://code.google.com/p/th9x/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "opentx.h"

void lcd_mask(uint8_t *p, uint8_t mask, LcdFlags att)
{
  // ASSERT_IN_DISPLAY(p);
  if ((p) >= DISPLAY_END) {
    return;
  }

  if (att&FILL_WHITE) {
    // TODO I could remove this, it's used for the top bar
    if (*p & 0x0F) mask &= 0xF0;
    if (*p & 0xF0) mask &= 0x0F;
  }

  if (att & FORCE) {
    *p |= mask;
  }
  else if (att & ERASE) {
    *p &= ~mask;
  }
  else {
    *p ^= mask;
  }
}

#define PIXEL_GREY_MASK(y, att) (((y) & 1) ? (0xF0 - (COLOUR_MASK(att) >> 12)) : (0x0F - (COLOUR_MASK(att) >> 16)))

void lcd_plot(coord_t x, coord_t y, LcdFlags att)
{
  if (x<0 || x>=LCD_W || y<0 || y>=LCD_H) return;
  uint8_t *p = &displayBuf[ y / 2 * LCD_W + x ];
  uint8_t mask = PIXEL_GREY_MASK(y, att);
  if (p<DISPLAY_END) {
    lcd_mask(p, mask, att);
  }
}

void lcd_hlineStip(coord_t x, coord_t y, coord_t w, uint8_t pat, LcdFlags att)
{
  if (y >= LCD_H) return;
  if (x+w > LCD_W) { 
    if (x >= LCD_W ) return;
    w = LCD_W - x; 
  }

  uint8_t *p  = &displayBuf[ y / 2 * LCD_W + x ];
  uint8_t mask = PIXEL_GREY_MASK(y, att);
  while (w--) {
    if (pat&1) {
      lcd_mask(p, mask, att);
      pat = (pat >> 1) | 0x80;
    }
    else {
      pat = pat >> 1;
    }
    p++;
  }
}

void lcd_vlineStip(coord_t x, scoord_t y, scoord_t h, uint8_t pat, LcdFlags att)
{
  if (x >= LCD_W) return;
  if (y >= LCD_H) return;
  if (h<0) { y+=h; h=-h; }
  if (y<0) { h+=y; y=0; if (h<=0) return; }
  if (y+h > LCD_H) { h = LCD_H - y; }

  if (pat==DOTTED && !(y%2)) {
    pat = ~pat;
  }

  while (h--) {
    if (pat & 1) {
      lcd_plot(x, y, att);
      pat = (pat >> 1) | 0x80;
    }
    else {
      pat = pat >> 1;
    }
    y++;
  }
}

void lcd_invert_line(int8_t line)
{
  uint8_t *p  = &displayBuf[line * 4 * LCD_W];
  for (coord_t x=0; x<LCD_W*4; x++) {
    ASSERT_IN_DISPLAY(p);
    *p++ ^= 0xff;
  }
}

#if !defined(BOOT)
void lcd_img(coord_t x, coord_t y, const pm_uchar * img, uint8_t idx, LcdFlags att)
{
  const pm_uchar *q = img;
  uint8_t w    = pgm_read_byte(q++);
  uint8_t hb   = (pgm_read_byte(q++)+7) / 8;
  bool    inv  = (att & INVERS) ? true : (att & BLINK ? BLINK_ON_PHASE : false);
  q += idx*w*hb;
  for (uint8_t yb = 0; yb < hb; yb++) {
    for (coord_t i=0; i<w; i++) {
      uint8_t b = pgm_read_byte(q++);
      uint8_t val = inv ? ~b : b;
      for (int k=0; k<8; k++) {
        if (val & (1<<k)) {
          lcd_plot(x+i, y+yb*8+k, 0);
        }
      }
    }
  }
}

void lcd_bmp(coord_t x, coord_t y, const uint8_t * img, coord_t offset, coord_t width)
{
  const uint8_t *q = img;
  uint8_t w = *q++;
  if (!width || width > w) {
    width = w;
  }
  if (x+width > LCD_W) {
    if (x >= LCD_W ) return;
    width = LCD_W-x;
  }
  uint8_t rows = (*q++ + 1) / 2;

  for (uint8_t row=0; row<rows; row++) {
    q = img + 2 + row*w + offset;
    uint8_t *p = &displayBuf[(row + (y/2)) * LCD_W + x];
    for (coord_t i=0; i<width; i++) {
      if ((p) >= DISPLAY_END) return;
      uint8_t b = *q++;
      if (y & 1) {
        *p = (*p & 0x0f) + ((b & 0x0f) << 4);
        *(p+LCD_W) = (*(p+LCD_W) & 0xf0) + ((b & 0xf0) >> 4);
      }
      else {
        *p = b;
      }
      p++;
    }
  }
}
#endif
