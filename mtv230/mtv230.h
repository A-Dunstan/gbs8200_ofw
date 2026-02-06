#include <8051.h>
#include <compiler.h>
#include <stdint.h>

__xdata volatile uint8_t __at(0xF00) IICCTR;
__xdata volatile const uint8_t __at(0xF01) IICSTUS[2];
__xdata volatile uint8_t __at(0xF03) IIC_INTFLG;
__xdata volatile uint8_t __at(0xF04) IIC_INTEN;
__xdata volatile uint8_t __at(0xF05) MBUF;
__xdata volatile const uint8_t __at(0xF06) RCABUF;
__xdata volatile uint8_t __at(0xF06) TXABUF;
__xdata volatile uint8_t __at(0xF07) SLVAADR;
__xdata volatile const uint8_t __at(0xF08) RCBBUF;
__xdata volatile uint8_t __at(0xF08) TXBBUF;
__xdata volatile uint8_t __at(0xF09) SLVBADR;
__xdata volatile uint8_t __at(0xF0B) ISPSLV;
__xdata volatile uint8_t __at(0xF0C) ISPEN;
__xdata volatile uint8_t __at(0xF10) ADC;
__xdata volatile uint8_t __at(0xF18) WDT;
__xdata volatile uint8_t __at(0xF20) DA0;
__xdata volatile uint8_t __at(0xF21) DA1;
__xdata volatile uint8_t __at(0xF22) DA2;
__xdata volatile uint8_t __at(0xF23) DA3;
__xdata volatile uint8_t __at(0xF28) PORT6[3];
__xdata volatile uint8_t __at(0xF2B) PADMOD[4];
__xdata volatile uint8_t __at(0xF2F) OPTION;
__xdata volatile uint8_t __at(0xF30) PORT4[8];
__xdata volatile uint8_t __at(0xF38) PORT5[8];
__xdata volatile const uint8_t __at(0xF40) HVSTUS;
__xdata volatile const uint8_t __at(0xF41) HCNTH;
__xdata volatile const uint8_t __at(0xF42) HCNTL;
__xdata volatile const uint8_t __at(0xF43) VCNTH;
__xdata volatile const uint8_t __at(0xF44) VCNTL;
__xdata volatile uint8_t __at(0xF40) HVCTR0;
__xdata volatile uint8_t __at(0xF43) HVCTR3;
__xdata volatile uint8_t __at(0xF48) HV_INTFLG;
__xdata volatile uint8_t __at(0xF49) HV_INTEN;
__xdata volatile uint8_t __at(0xFA0) OSDRA;
__xdata volatile uint8_t __at(0xFA1) OSDCA;
__xdata volatile uint8_t __at(0xFA2) OSDDT0;
__xdata volatile uint8_t __at(0xFA3) OSDDT1;
__xdata volatile uint8_t __at(0xFC0) W1ROW;
__xdata volatile uint8_t __at(0xFC1) W1COL[2];
__xdata volatile uint8_t __at(0xFC3) W2ROW;
__xdata volatile uint8_t __at(0xFC4) W2COL[2];
__xdata volatile uint8_t __at(0xFC6) W3ROW;
__xdata volatile uint8_t __at(0xFC7) W3COL[2];
__xdata volatile uint8_t __at(0xFC9) W4ROW;
__xdata volatile uint8_t __at(0xFCA) W4COL[2];
__xdata volatile uint8_t __at(0xFCC) VERTD;
__xdata volatile uint8_t __at(0xFCD) HORD;
__xdata volatile uint8_t __at(0xFCE) CH;
__xdata volatile uint8_t __at(0xFD0) RSPACE;
__xdata volatile uint8_t __at(0xFD1) OSDCON[2];
__xdata volatile uint8_t __at(0xFD3) CHSC;
__xdata volatile uint8_t __at(0xFD4) FSSTP;
__xdata volatile uint8_t __at(0xFD5) WINSW;
__xdata volatile uint8_t __at(0xFD6) WINSH;
__xdata volatile uint8_t __at(0xFD7) WINSC[2];
__xdata volatile uint8_t __at(0xFD9) XDEL;

#define EEPROM_WP P1_2
// LED is ACTIVE LOW!
#define LED P1_4
#define BREAK_PIN PORT5[4]
#define SDA P1_1
#define SCL P1_0

void enter_ispc_mode(void);

void delay(uint8_t ms);
void delay_us(uint8_t us);

void config_I2C(void);
void i2c_send_stop(void) __reentrant;
void i2c_send_stop_EA(__bit oldEA) __reentrant;
__bit i2c_send_byte(uint8_t data) __reentrant;
__bit i2c_send_start(uint8_t i2c_address) __reentrant;
uint8_t i2c_receive_byte(__bit ack) __reentrant;
void i2c_read(uint8_t* dst, uint8_t length) __reentrant;
void i2c_write(const uint8_t* src, uint8_t length) __reentrant;
uint8_t i2c_request_byte(uint8_t i2c_address, __bit ack, __bit send_stop) __reentrant;
void i2c_transmit_byte(uint8_t i2c_address, uint8_t data, __bit send_stop) __reentrant;
void i2c_transmit_word(uint8_t i2c_address, uint8_t byte_u, uint8_t byte_l, __bit send_stop) __reentrant;
void i2c_handler(uint8_t mask) __reentrant;

void EEPROM_write(const uint8_t* src, uint16_t address, uint8_t length) __reentrant;
void EEPROM_read_from(uint8_t* dst, uint16_t address, uint8_t length) __reentrant;
void EEPROM_write_byte(uint16_t address, uint8_t data) __reentrant;
uint8_t EEPROM_read_byte(uint16_t address) __reentrant;

void init_TV5725(void);
void TV5725_write(const uint8_t* src, uint8_t address, uint8_t length) __reentrant;
void TV5725_read(uint8_t* dst, uint8_t address, uint8_t length) __reentrant;
void TV5725_write_reg(uint8_t bank, uint8_t reg, uint8_t data) __reentrant;
uint8_t TV5725_read_reg(uint8_t bank, uint8_t reg) __reentrant;

void osd_init(void);
void osd_set_wide(uint8_t doubled);
void osd_set_delays(uint8_t h, uint8_t v);
void osd_set_char_height(uint8_t height);
void osd_clear(void);
void osd_show_string(uint8_t x, uint8_t y, uint8_t attrib, const char*);
void osd_show_hex2(uint8_t x, uint8_t y, uint8_t attrib, uint8_t value);
void osd_show_hex4(uint8_t x, uint8_t y, uint8_t attrib, uint16_t value);
void osd_show_char(uint8_t x, uint8_t y, uint8_t attrib, uint16_t c);
void osd_enable(uint8_t on);
