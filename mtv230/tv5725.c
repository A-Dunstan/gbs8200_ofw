#include "mtv230.h"

void reset_TV5725(void);
const uint8_t __at(0x71A) init_1024x768_ypbpr[];
const uint8_t __at(0x145C) init_1024x768_rgbs[];

#define TV5725_I2C 0x17

#define INPUT_YPBPR 0
#define INPUT_RGBS  1
#define INPUT_RGBGV 2

void TV5725_write(const uint8_t* src, uint8_t address, uint8_t length) __reentrant {
  __bit oldEA = EA;
  EA = 0;

  i2c_transmit_byte(TV5725_I2C, address, 0);
  i2c_write(src, length);
  i2c_send_stop_EA(oldEA);
}

void TV5725_read(uint8_t* dst, uint8_t address, uint8_t length) __reentrant {
  __bit oldEA = EA;
  EA = 0;

  i2c_transmit_byte(TV5725_I2C, address, 0);
  // restart
  *dst++ = i2c_request_byte(TV5725_I2C, length!=1, 0);
  if (--length)
    i2c_read(dst, length);
  i2c_send_stop_EA(oldEA);
}

void TV5725_write_reg(uint8_t bank, uint8_t reg, uint8_t data) __reentrant {
  __bit oldEA = EA;
  EA = 0;

  i2c_transmit_word(TV5725_I2C, 0xF0, bank, 0);
  i2c_transmit_word(TV5725_I2C, reg, data, 1);

  EA = oldEA;
}

uint8_t TV5725_read_reg(uint8_t bank, uint8_t reg) __reentrant {
  __bit oldEA = EA;
  EA = 0;

  i2c_transmit_word(TV5725_I2C, 0xF0, bank, 0);
  i2c_transmit_byte(TV5725_I2C, reg, 0);
  uint8_t data = i2c_request_byte(TV5725_I2C, 0, 1);

  EA = oldEA;
  return data;
}

void TV5725_mask_reg(uint8_t bank, uint8_t reg, uint8_t set, uint8_t clear) {
  __bit oldEA = EA;
  EA = 0;

  // set bank
  i2c_transmit_word(TV5725_I2C, 0xF0, bank, 0);
  // set register address
  i2c_transmit_byte(TV5725_I2C, reg, 0);
  // read original value
  uint8_t data = i2c_request_byte(TV5725_I2C, 0, 0);
  // add bits
  data |= set;
  // remove bite
  data &= ~clear;
  // send new value
  i2c_transmit_word(TV5725_I2C, reg, data, 1);

  EA = oldEA;
}

void program_TV5725_from_ROM(__code const uint8_t* src) {
  __bit oldEA;
  uint8_t reg, val;

  reg = src[0];
  val = src[1];
  if (reg==0xFF && val==0xFF) return;

  oldEA = EA;
  EA = 0;

  i2c_transmit_word(TV5725_I2C, reg, val, 0);
  ++reg;

  while (1) {
    uint8_t next_reg = src[0];
    val = src[1];

    if (next_reg==0xFF && val==0xFF) break;

    if (next_reg == reg) {
      i2c_send_byte(val);
      ++reg;
    } else {
      i2c_transmit_word(TV5725_I2C, next_reg, val, 0);
      reg = next_reg+1;
    }
    src += 2;
  }
  i2c_send_stop_EA(oldEA);
}

static void latch_plls(void) {
  TV5725_mask_reg(5, 0x11, 0, 0x80);
  TV5725_mask_reg(5, 0x11, 0x80, 0);
  TV5725_mask_reg(5, 0x18, 0, 0x80);
  TV5725_mask_reg(5, 0x18, 0x80, 0);
  TV5725_mask_reg(5, 0x19, 0, 0x80);
  TV5725_mask_reg(5, 0x19, 0x80, 0);
}

static void set_YUV_input(void) {
  TV5725_write_reg(1, 0, 0x62);
  TV5725_write_reg(5, 3, 0xFB);
}

void set_RGB_input(void) {
  TV5725_write_reg(1, 0, 0x60);
  TV5725_write_reg(5, 3, 0xF1);
}

void set_input(uint8_t input) {
  switch (input) {
    case INPUT_YPBPR:
      set_YUV_input();
      TV5725_mask_reg(5, 2, 0, 0x40);
      break;
    case INPUT_RGBS:
      set_RGB_input();
      TV5725_mask_reg(5, 2, 0x40, 0);
      break;
  }
  latch_plls();
  // turn off DACs
  TV5725_mask_reg(0, 0x44, 0, 1);
}

static const uint8_t output_720x480x60[] = {
  0xF0, 3, // bank 3
  0x01, 0x59, // S3_01 VDS_HSYNC_RST = x359 (858-1)
  0x02, 0xC3, // S3_02
  0x03, 0x20, // S3_03 VDS_VSYNC_RST = x20C (525-1)
  0x0A, 0x3E, // S3_0A VDS_HS_ST = x3E (62) hsync width, sync is active low and "start" is the rising edge
  0x0B, 0x00,
  0x0C, 0x00, // VDS_HS_SP = 0, active low falling edge occurs at time=0
  0x0D, 0x06, // S3_0D VDS_VS_ST = x06 (6) vsync length, again this is the rising edge
  0x0E, 0x00,
  0x0F, 0x00, // VDS_VS_SP = 0
  0xF0, 0, // bank 0
  0x40, 0x4C, // S0_40 mclk=144MHz
  0x41, 0x05, // S0_41 pix_clk = 27MHz
  0x44, 0x01, // S0_44 disable DACs
  0x46, 0x40, // S0_46 everything in reset except VDS
  0x4F, 0x30, // S0_4F enable HSYNC/VSYNC output pins
  0xFF, 0xFF
};

void init_TV5725(void) {
  reset_TV5725();

//  program_TV5725_from_ROM(init_1024x768_ypbpr);
//  set_input(INPUT_YPBPR);
  program_TV5725_from_ROM(output_720x480x60);

  osd_set_wide(0);
  osd_set_char_height(11);
  osd_set_delays(36, 12);
  osd_clear();
}
