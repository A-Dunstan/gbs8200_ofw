#include "mtv230.h"

#define EEPROM_I2C 0x50

void EEPROM_write(const uint8_t* src, uint16_t address, uint8_t length) __reentrant {
  __bit oldEA = EA;
  EA = 0;

  EEPROM_WP = 0;
  delay(3);

  i2c_transmit_word(EEPROM_I2C, address>>8, address & 0xFF, 0);
  i2c_write(src, length);
  i2c_send_stop();

  EEPROM_WP = 1;
  delay(10);

  EA = oldEA;
}

void EEPROM_read_from(uint8_t* dst, uint16_t address, uint8_t length) __reentrant {
  __bit oldEA = EA;
  EA = 0;

  // set address
  i2c_transmit_word(EEPROM_I2C, address>>8, address & 0xFF, 0);
  // restart, read
  *dst++ = i2c_request_byte(EEPROM_I2C, length!=1, 0);
  if (--length)
    i2c_read(dst, length);
  i2c_send_stop_EA(oldEA);
}

void EEPROM_write_byte(uint16_t address, uint8_t data) __reentrant {
  __bit oldEA = EA;
  EA = 0;

  EEPROM_WP = 0;
  delay(3);

  i2c_transmit_word(EEPROM_I2C, address>>8, address & 0xFF, 0);
  i2c_send_byte(data);
  i2c_send_stop();

  EEPROM_WP = 1;
  delay(10);

  EA = oldEA;
}

uint8_t EEPROM_read_byte(uint16_t address) __reentrant {
  __bit oldEA = EA;
  EA = 0;

  i2c_transmit_word(EEPROM_I2C, address>>8, address & 0xFF, 0);
  uint8_t data = i2c_request_byte(EEPROM_I2C, 0, 1);

  EA = oldEA;
  return data;
}
