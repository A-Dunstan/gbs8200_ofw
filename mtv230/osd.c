#include "mtv230.h"

// default OSD settings: border and fade-in/fade-out
#define DEFAULT_OSD0 0x50
#define DEFAULT_OSD1 0

__xdata static uint8_t osd_state[2] = {DEFAULT_OSD0, DEFAULT_OSD1};

void osd_clear(void) {
  uint8_t osd_off = osd_state[0];
  // set the clr bits
  OSDCON[0] = osd_off | 0x06;  // WENclr | RAMclr
  // unset them
  OSDCON[0] = osd_off;
}

void osd_init(void) {
  OSDCON[0] = osd_state[0];
  OSDCON[1] = osd_state[1];
}

void osd_set_wide(uint8_t doubled) {
  if (doubled)
    osd_state[1] |= 0x10;
  else
    osd_state[1] &= ~0x10;
  OSDCON[1] = osd_state[1];
}

void osd_set_delays(uint8_t h, uint8_t v) {
  HORD = h;
  VERTD = v;
}

void osd_set_char_height(uint8_t height) {
  CH = height;
}

void osd_show_string(uint8_t x, uint8_t y, uint8_t attrib, const char* src) {
  uint8_t c;
  uint8_t len = 0;
  y &= 0x0F;
  x &= 0x1F;
  OSDRA = y;
  OSDCA = x;
  while (c = *src++) {
    OSDDT0 = c;
    ++len;
  }
  OSDRA = y|0x40;
  OSDCA = x;
  while (len--)
    OSDDT0 = attrib;
}

void osd_enable(uint8_t on) {
  uint8_t state = osd_state[0];
  if (on)
    state |= 0x80;
  else
    state &= ~0x80;
  osd_state[0] = OSDCON[0] = state;
}

void osd_show_hex2(uint8_t x, uint8_t y, uint8_t attrib, uint8_t value) {
  char dig[3];

  uint8_t h = value >> 4;
  uint8_t l = value & 0xF;

  if (h > 9) dig[0] = 'A' -10 + h;
  else dig[0] = '0' + h;
  if (l > 9) dig[1] = 'A' - 10 + l;
  else dig[1] = '0' + l;

  osd_show_string(x, y, attrib, dig);
}

void osd_show_hex4(uint8_t x, uint8_t y, uint8_t attrib, uint16_t value) {
  osd_show_hex2(x, y, attrib, value >> 8);
  osd_show_hex2(x+2, y, attrib, value & 0xFF);
}

void osd_show_char(uint8_t x, uint8_t y, uint8_t attrib, uint16_t c) {
  // set glyph address
  OSDRA = y;
  OSDCA = x;
  if (c >= 256)
    OSDDT1 = c & 0xFF;
  else
    OSDDT0 = c;
  // set attribute
  OSDRA = y|0x40;
  OSDCA = x;
  OSDDT0 = attrib;
}
