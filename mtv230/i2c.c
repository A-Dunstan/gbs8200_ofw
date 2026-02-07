#include "mtv230.h"

// I2C debugging variables, have to be in xdata that original firmware doesn't use
__xdata static uint8_t __at(0x900) debug_p[2];
__xdata static uint8_t __at(0x902) debug_flag;

void config_I2C(void) {
  // configure I2C slaveA on HSCL/HSDA pins, addresses 0x40-0x43

  debug_flag = 0;

  // enable 5-bit address, enable clock-stretching, set master to ISCL/ISDA
  OPTION = 0xA8; // PWMF | slvAbs1 | ENSCL

  // disable slaveB
  SLVBADR = 0;

  // connect P3.0+P3.1 to HSCL/HSDA
  PADMOD[0] = 0x83; // HIICE | FclkE | P62E

  // clear any current I2C interrupts
  IIC_INTFLG = 0;
  // enable slaveA TX/RX interrupts
  IIC_INTEN = 0x18;  // TXAI | RCAI

  // clear stale RCABUF and prime TXABUF
  TXABUF = RCABUF;

  // slaveA address range is 0x40-0x43
  SLVAADR = 0x80 | 0x40;

  // IE1 high priority
  PX1 = 1;
  // IE1 level-triggered
  IT1 = 0;
  // unmask IE1
  EX1 = 1;
}

void i2c_send_stop(void) __reentrant {
  // SDA rises while SCL is high
  SDA = 0;
  NOP();NOP();
  SCL = 1;
  NOP();NOP();
  for (uint8_t i=255; SCL==0 && i != 0; i--);
  SDA = 1;
  NOP();NOP();
}

void i2c_send_stop_EA(__bit oldEA) __reentrant {
  i2c_send_stop();
  EA = oldEA;
}

__bit i2c_send_byte(uint8_t data) __reentrant {
  uint8_t i, j;
  // send 8 bits
  for (j=8; j != 0; j--) {
    SDA = data & 0x80;
    NOP();
    SCL = 1;
    NOP();NOP();NOP();NOP();
    for (i=255; SCL==0 && i != 0; i--);
    SCL = 0;
    data <<= 1;
  }
  SDA = 1; // pin = input
  NOP();NOP();NOP();NOP();
  // receive ack/nak
  SCL = 1;
  NOP();NOP();NOP();NOP();
  for (i=255; SCL==0 && i != 0; i--);
  __bit ack = !SDA;
  SCL = 0;
  return ack;
}

__bit i2c_send_start(uint8_t i2c_address) __reentrant {
  // SDA falls while SCL is high
  SDA = 1;
  NOP();NOP();
  SCL = 1;
  for (uint8_t i=255; SCL==0 && i != 0; i--);
  NOP();NOP();
  SDA = 0;
  NOP();NOP();
  // leave SCL ready for next bit
  SCL = 0;
  return i2c_send_byte(i2c_address);
}

uint8_t i2c_receive_byte(__bit ack) __reentrant {
  uint8_t data = 0;
  uint8_t i, j;

  SDA = 1; // pin = input
  NOP();NOP();
  // receive 8 bits
  for (i=8; i != 0; i--) {
    SCL = 1;
    NOP();NOP();
    for (j=255; SCL==0 && j != 0; j--);
    data = (data<<1) | SDA;
    SCL = 0;
    NOP();NOP();
  }
  // send ack/nak
  SDA = !ack;
  NOP();NOP();
  SCL = 1;
  for (j=255; SCL==0 && j != 0; j--);

  NOP();NOP();
  SCL = 0;
  NOP();NOP();
  SDA = 1;

  return data;
}

void i2c_read(uint8_t* dst, uint8_t length) __reentrant {
  do {
    *dst++ = i2c_receive_byte(length>1);
  } while (--length);
}

void i2c_write(const uint8_t* src, uint8_t length) __reentrant {
  do {
    if (i2c_send_byte(*src++) == 0) break;
  } while (--length);
}

uint8_t i2c_request_byte(uint8_t i2c_address, __bit ack, __bit send_stop) __reentrant {
  i2c_send_start((i2c_address<<1)|1);
  uint8_t data = i2c_receive_byte(ack);
  if (send_stop)
    i2c_send_stop();

  return data;
}

void i2c_transmit_byte(uint8_t i2c_address, uint8_t data, __bit send_stop) __reentrant {
  i2c_send_start(i2c_address<<1);
  i2c_send_byte(data);
  if (send_stop)
    i2c_send_stop();
}

void i2c_transmit_word(uint8_t i2c_address, uint8_t byte_u, uint8_t byte_l, __bit send_stop) __reentrant {
  i2c_send_start(i2c_address<<1);
  i2c_send_byte(byte_u);
  i2c_send_byte(byte_l);
  if (send_stop)
    i2c_send_stop();
}

void i2c_handler(uint8_t mask) __reentrant {
  uint8_t status = IICSTUS[0];
  uint8_t port = status & 3; // slvAlsb1 | slvAlsb0

  if (mask & 0x10) { // TXAI
    uint8_t outbyte;

    if (port == 3) { // TV5725 - need to preserve current bank register
      uint8_t bank;
      TV5725_read(&bank, 0xF0, 1);
      // read request register
      outbyte = TV5725_read_reg(debug_p[0], debug_p[1]);
      // restore original bank register
      TV5725_write(&bank, 0xF0, 1);
    } else {
      uint16_t ptr = (debug_p[0] << 8) | debug_p[1];
      if (port == 2) { // EEPROM
        outbyte = EEPROM_read_byte(ptr);
      } else if (port == 1) { // ROM (code)
        outbyte = *(__code uint8_t* const)(ptr);
      } else { // RAM - could be internal or XRAM
        if (debug_p[0])
          outbyte = *(__xdata uint8_t* const)(ptr); // XRAM
        else
          outbyte = *(__data uint8_t* const)(debug_p[1]); // IRAM
      }
    }
    TXABUF = outbyte;
    ++debug_p[1];
  }

  if (mask & 0x08) { // RCAI
    uint8_t inbyte = RCABUF;
    if (status & 0x40) { // wAdrA is set, this is the high address byte
      debug_p[0] = inbyte;
      debug_flag = 1;
      return;
    } else if (debug_flag == 1) { // this is the second address byte
      debug_p[1] = inbyte;
      debug_flag = 0;
    } else { // else we need to write a byte
      if (port == 3) { // TV5725
        uint8_t bank;
        TV5725_read(&bank, 0xF0, 1);
        TV5725_write_reg(debug_p[0], debug_p[1], inbyte);
        TV5725_write(&bank, 0xF0, 1);
      } else {
        uint16_t ptr = (debug_p[0] << 8) | debug_p[1];
        if (port == 2) { // EEPROM
          EEPROM_write_byte(ptr, inbyte);
        } else { // assume RAM - can't write to ROM
          if (debug_p[0])
            *(__xdata uint8_t*)(ptr) = inbyte; // XRAM
          else
            *(__data uint8_t*)(debug_p[1]) = inbyte; // IRAM
        }
      }
      ++debug_p[1];
    }
    // prime tx buffer
    TXABUF = 0;
  }
}
