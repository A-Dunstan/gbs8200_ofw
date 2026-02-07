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
  if (reg==0xFF) return;
  val = src[1];

  oldEA = EA;
  EA = 0;

  i2c_transmit_word(TV5725_I2C, reg, val, 0);
  ++reg;

  while (1) {
    uint8_t next_reg = src[0];

    if (next_reg==0xFF) break;
    val = src[1];

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
  0x4F, 0x30, // S0_4F enable HSYNC/VSYNC and pixclk output pins
  0xFF
};

static const uint8_t output_640x480x60[] = {
  0xF0, 3, // bank 3
  0x01, 0x1F, // S3_01 VDS_HSYNC_RST = x31F (800-1)
  0x02, 0xC3, // S3_02
  0x03, 0x20, // S3_03 VDS_VSYNC_RST = x20C (525-1)
  0x0A, 0x60, // S3_0A VDS_HS_ST = x60 (96) hsync width, sync is active low and "start" is the rising edge
  0x0B, 0x00,
  0x0C, 0x00, // VDS_HS_SP = 0, active low falling edge occurs at time=0
  0x0D, 0x02, // S3_0D VDS_VS_ST = x02 (2) vsync length, again this is the rising edge
  0x0E, 0x00,
  0x0F, 0x00, // VDS_VS_SP = 0
  0xF0, 5, // bank 5
  0x00, 0x04, // ADC_CLK: ADC_CLK_PLLAD
  0x11, 0x10, // PLLAD_CONTROL_00: PLLAD_PDZ
  // PLL: 27Mhz * M(400) / N(429) = 25.175MHz
  0x12, 0x8F,
  0x13, 0x01,
  0x14, 0xAC,
  0x15, 0x01,
  0x16, 0xA0,
  0x17, 0x04,
  0x11, 0x90,
  0xF0, 0, // bank 0
  0x40, 0x4C, // S0_40 mclk=144MHz
  0x41, 0x35, // S0_41 pix_clk = ADCLK
  0x44, 0x01, // S0_44 disable DACs
  0x46, 0x40, // S0_46 everything in reset except VDS
  0x4F, 0x30, // S0_4F enable HSYNC/VSYNC and pixclk output pins
  0xFF
};

static const uint8_t output_1920x1080x60[] = {
  0xF0, 3, // bank 3
  0x01, 0x97, // S3_01 VDS_HSYNC_RST = x897 (2200-1)
  0x02, 0x48, // S3_02
  0x03, 0x46, // S3_03 VDS_VSYNC_RST = x20C (1125-1)
  0x0A, 0x00, // S3_0A VDS_HS_ST = 0
  0x0B, 0x40,
  0x0C, 0x04, // VDS_HS_SP = 0x2C (44)
  0x0D, 0x00, // S3_0D VDS_VS_ST = 0
  0x0E, 0x50,
  0x0F, 0x00, // VDS_VS_SP = 5
  0xF0, 5, // bank 5
  0x00, 0x04, // ADC_CLK: ADC_CLK_PLLAD
  0x11, 0x30, // PLLAD_CONTROL_00: PLLAD_FS, PLLAD_PDZ
  // PLL: 27Mhz * M(11) / N(2) = 148.5MHz
  0x12, 0x0A,
  0x13, 0x00,
  0x14, 0x01,
  0x15, 0x00,
  0x16, 0x00,
  0x17, 0x04,
  0x11, 0x90,
  0xF0, 0, // bank 0
  0x40, 0x4C, // S0_40 mclk=144MHz
  0x41, 0x35, // S0_41 pix_clk = ADCLK
  0x44, 0x01, // S0_44 disable DACs
  0x46, 0x40, // S0_46 everything in reset except VDS
  0x4F, 0x30, // S0_4F enable HSYNC/VSYNC and pixclk output pins
  0xFF
};

static const uint8_t output_1440x1080x60[] = {
  0xF0, 3, // bank 3
  0x01, 0x7F, // S3_01 VDS_HSYNC_RST = x77F (1920-1)
  0x02, 0x67, // S3_02
  0x03, 0x46, // S3_03 VDS_VSYNC_RST = x464 (1125-1)
  0x0A, 0x98, // S3_0A VDS_HS_ST = x98 (152)
  0x0B, 0x00,
  0x0C, 0x00,
  0x0D, 0x00, // VDS_VS_ST = 0
  0x0E, 0x50,
  0x0F, 0x00, // VDS_VS_SP = 5
  0xF0, 0, // bank 0
  0x40, 0x4C,
  0x41, 0x95, // pixclk = 129.6MHz
  0x44, 0x01,
  0x46, 0x40,
  0x4F, 0x30,
  0xFF
};

static const uint8_t output_1600x1200x60[] = {
  0xF0, 3, // bank 3
  0x01, 0x6F, // S3_01 VDS_HSYNC_RST = x86F (2160-1)
  0x02, 0x18, // S3_02
  0x03, 0x4E, // S3_03 VDS_VSYNC_RST = x4E1 (1250-1)
  0x0A, 0x00, // S3_0A VDS_HS_ST = 0
  0x0B, 0x00,
  0x0C, 0x0C, // VDS_HS_SP = xC0 (192)
  0x0D, 0x00, // VDS_VS_ST = 0
  0x0E, 0x30,
  0x0F, 0x00, // VDS_VS_SP = 3
  0xF0, 0, // bank 0
  0x40, 0x4C,
  0x41, 0xA5, // pixclk = 162MHz
  0x44, 0x01,
  0x46, 0x40,
  0x4F, 0x30,
  0xFF
};

void init_TV5725(void) {
  reset_TV5725();

//  program_TV5725_from_ROM(init_1024x768_ypbpr);
//  set_input(INPUT_YPBPR);
  program_TV5725_from_ROM(output_1600x1200x60);

  osd_set_wide(1);
  osd_set_char_height(127);
  osd_set_delays(55, 12);
  osd_clear();
}
