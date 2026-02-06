#include <Wire.h>
#include "mtv230/gbs.h"

#define MTV_IIC Wire

#define BREAK_PIN A3

#define MTV_COMMAND 0x3F
#define MTV_DATA    0x3E

#define MTV_DEBUG0  0x40      // read/write to RAM
#define MTV_DEBUG1  0x41      // read from ROM
#define MTV_DEBUG2  0x42      // read/write to EEPROM
#define MTV_DEBUG3  0x43      // read/write to TV5725

#define COMMAND_NONE           0x00
#define COMMAND_PROGRAM        0xA0
#define COMMAND_PAGE_ERASE     0x30
#define COMMAND_FLASH_ERASE    0x68
#define COMMAND_CLEAR_CRC      0xD0
#define COMMAND_RESET          0x48
#define COMMAND_OSD            0x01
#define COMMAND_CODE           0

class mtv_crc {
private:
  uint16_t crc;
  static const uint16_t tab[256];
public:
  mtv_crc(void) {
    MTV_IIC.beginTransmission(MTV_COMMAND);
    MTV_IIC.write(COMMAND_CLEAR_CRC);
    MTV_IIC.endTransmission();
    crc = 0xFFFF;
  }

  void update(uint8_t d) {
    crc = (crc << 8) | ((crc >> 8) ^ d);
    crc ^= pgm_read_word(&tab[crc & 0xFF]);
  }
  bool verify(uint8_t _cmd, uint8_t _page, uint8_t _address) const {
    if (MTV_IIC.requestFrom(MTV_COMMAND, 5) == 5) {
      uint8_t cmd = MTV_IIC.read() & 0xF8;
      uint8_t page = MTV_IIC.read();
      uint8_t address = MTV_IIC.read();
      uint16_t tcrc = (MTV_IIC.read() << 8) | MTV_IIC.read();
      Serial.print(F("CRC Verify: cmd "));
      Serial.print(cmd, HEX);
      Serial.print(F(" page "));
      Serial.print(page, HEX);
      Serial.print(F(" address "));
      Serial.print(address, HEX);
      Serial.print(F(" crc "));
      Serial.print(tcrc, HEX);
      Serial.print(F(" ("));
      Serial.print(crc, HEX);
      Serial.println(F(")"));
      if (cmd != _cmd) {
        Serial.println(F("CRC fail: cmd mismatch"));
      } else if (page != _page) {
        Serial.println(F("CRC fail: page mismatch"));
      } else if (address != _address) {
        Serial.println(F("CRC fail: address mismatch"));
      } else if (tcrc != crc) {
        Serial.println(F("CRC fail: crc mismatch"));
      }
      else return true;
    }
    else Serial.println(F("CRC failed to verify, transmission failure"));

    return false;
  }
};

const uint16_t mtv_crc::tab[256] PROGMEM = {
0x0000, 0x8004, 0x800d, 0x0009, 0x801f, 0x001b, 0x0012, 0x8016,
0x803b, 0x003f, 0x0036, 0x8032, 0x0024, 0x8020, 0x8029, 0x002d,
0x8073, 0x0077, 0x007e, 0x807a, 0x006c, 0x8068, 0x8061, 0x0065,
0x0048, 0x804c, 0x8045, 0x0041, 0x8057, 0x0053, 0x005a, 0x805e,
0x80e3, 0x00e7, 0x00ee, 0x80ea, 0x00fc, 0x80f8, 0x80f1, 0x00f5,
0x00d8, 0x80dc, 0x80d5, 0x00d1, 0x80c7, 0x00c3, 0x00ca, 0x80ce,
0x0090, 0x8094, 0x809d, 0x0099, 0x808f, 0x008b, 0x0082, 0x8086,
0x80ab, 0x00af, 0x00a6, 0x80a2, 0x00b4, 0x80b0, 0x80b9, 0x00bd,
0x81c3, 0x01c7, 0x01ce, 0x81ca, 0x01dc, 0x81d8, 0x81d1, 0x01d5,
0x01f8, 0x81fc, 0x81f5, 0x01f1, 0x81e7, 0x01e3, 0x01ea, 0x81ee,
0x01b0, 0x81b4, 0x81bd, 0x01b9, 0x81af, 0x01ab, 0x01a2, 0x81a6,
0x818b, 0x018f, 0x0186, 0x8182, 0x0194, 0x8190, 0x8199, 0x019d,
0x0120, 0x8124, 0x812d, 0x0129, 0x813f, 0x013b, 0x0132, 0x8136,
0x811b, 0x011f, 0x0116, 0x8112, 0x0104, 0x8100, 0x8109, 0x010d,
0x8153, 0x0157, 0x015e, 0x815a, 0x014c, 0x8148, 0x8141, 0x0145,
0x0168, 0x816c, 0x8165, 0x0161, 0x8177, 0x0173, 0x017a, 0x817e,
0x8383, 0x0387, 0x038e, 0x838a, 0x039c, 0x8398, 0x8391, 0x0395,
0x03b8, 0x83bc, 0x83b5, 0x03b1, 0x83a7, 0x03a3, 0x03aa, 0x83ae,
0x03f0, 0x83f4, 0x83fd, 0x03f9, 0x83ef, 0x03eb, 0x03e2, 0x83e6,
0x83cb, 0x03cf, 0x03c6, 0x83c2, 0x03d4, 0x83d0, 0x83d9, 0x03dd,
0x0360, 0x8364, 0x836d, 0x0369, 0x837f, 0x037b, 0x0372, 0x8376,
0x835b, 0x035f, 0x0356, 0x8352, 0x0344, 0x8340, 0x8349, 0x034d,
0x8313, 0x0317, 0x031e, 0x831a, 0x030c, 0x8308, 0x8301, 0x0305,
0x0328, 0x832c, 0x8325, 0x0321, 0x8337, 0x0333, 0x033a, 0x833e,
0x0240, 0x8244, 0x824d, 0x0249, 0x825f, 0x025b, 0x0252, 0x8256,
0x827b, 0x027f, 0x0276, 0x8272, 0x0264, 0x8260, 0x8269, 0x026d,
0x8233, 0x0237, 0x023e, 0x823a, 0x022c, 0x8228, 0x8221, 0x0225,
0x0208, 0x820c, 0x8205, 0x0201, 0x8217, 0x0213, 0x021a, 0x821e,
0x82a3, 0x02a7, 0x02ae, 0x82aa, 0x02bc, 0x82b8, 0x82b1, 0x02b5,
0x0298, 0x829c, 0x8295, 0x0291, 0x8287, 0x0283, 0x028a, 0x828e,
0x02d0, 0x82d4, 0x82dd, 0x02d9, 0x82cf, 0x02cb, 0x02c2, 0x82c6,
0x82eb, 0x02ef, 0x02e6, 0x82e2, 0x02f4, 0x82f0, 0x82f9, 0x02fd,
};

int read_osd(uint16_t address, uint8_t len, uint16_t *dst) {
  MTV_IIC.beginTransmission(MTV_COMMAND);
  MTV_IIC.write(COMMAND_OSD);
  MTV_IIC.write(address >> 7);
  int r = MTV_IIC.endTransmission();
  if (r != 0) {
    Serial.print(F("read_osd: command failed "));
    Serial.println(r);
    return r;
  }

  r = MTV_IIC.requestFrom(MTV_DATA, len*2, address<<1, 1, 1);
  if (r < 0) {
    Serial.print(F("read_osd: requestFrom failed "));
    Serial.println(r);
    return r;
  }
  for (int i=r; i > 1; i -= 2) {
    *dst = MTV_IIC.read() << 8;
    *dst++ |= MTV_IIC.read() & 0xFF;
    ++address;
    --len;
  }
  r >>= 1;
  if (r && len) {
    int n = read_osd(address, len, dst);
    if (n < 0) return n;
    r += n;
  }
  return r;
}

int write_osd(uint16_t address, uint8_t len, const uint16_t *src) {
  mtv_crc CRC;
  MTV_IIC.beginTransmission(MTV_COMMAND);
  MTV_IIC.write(COMMAND_PROGRAM|COMMAND_OSD);
  MTV_IIC.write(address >> 7);
  int r = MTV_IIC.endTransmission();
  if (r != 0) {
    Serial.print(F("write_osd: command failed "));
    Serial.println(r);
    return r;
  }

  const uint16_t *end = src + len;
  while (src < end) {
    MTV_IIC.beginTransmission(MTV_DATA);
    MTV_IIC.write(address++ << 1);
    uint16_t w = pgm_read_word(src);
    src++;
    MTV_IIC.write(w >> 8);
    CRC.update(w >> 8);
    MTV_IIC.write(w & 0xff);
    CRC.update(w & 0xff);
    r = MTV_IIC.endTransmission();
    delayMicroseconds(60);
    if (r != 0) {
      Serial.print(F("Failed to write OSD "));
      Serial.println(r);
      break;
    }
  }

  if (!CRC.verify(COMMAND_PROGRAM|COMMAND_OSD, address>>7, address<<1)) {
    return 0;
  }

  return len;
}

int read_code(uint16_t address, uint8_t len, uint8_t *dst) {
  MTV_IIC.beginTransmission(MTV_COMMAND);
  MTV_IIC.write(COMMAND_CODE);
  MTV_IIC.write(address >> 8);
  int r = MTV_IIC.endTransmission();
  if (r != 0) {
    Serial.print(F("read_code: command failed "));
    Serial.println(r);
    return -1;
  }

  r = MTV_IIC.requestFrom(MTV_DATA, len, address, 1, 1);
  if (r < 0) return r;
  for (int i=r; i > 0; i--) {
    *dst++ = MTV_IIC.read();
  }
  return r;
}

int write_code(uint16_t address, uint8_t len, const uint8_t* src) {
  const uint8_t page = (uint8_t)(address >> 8);
  mtv_crc CRC;

  MTV_IIC.beginTransmission(MTV_COMMAND);
  MTV_IIC.write(COMMAND_PROGRAM);
  MTV_IIC.write(page);
  int r = MTV_IIC.endTransmission();
  if (r != 0) {
    Serial.print(F("write_code: command failed "));
    Serial.println(r);
    return -1;
  }

  // Wire library doesn't provide a way to delay between bytes, so each one is a new transaction
  auto end = src + (len ? len : 256);
  while (src < end) {
    MTV_IIC.beginTransmission(MTV_DATA);
    MTV_IIC.write(address++ & 0xFF);
    char c = pgm_read_byte(src++);
    MTV_IIC.write(c);
    CRC.update(c);
    r = MTV_IIC.endTransmission();
    delayMicroseconds(60);
    if (r != 0) {
      Serial.print(F("Failed to write data "));
      Serial.println(r);
      break;
    }
  }

  if (!CRC.verify(COMMAND_PROGRAM, page, address & 0xFF)) {
    return 0;
  }

  return (len ? len : 256);
}

int write_big_code(uint16_t address, uint16_t len, const uint8_t* src) {
  int ret = 0;
  const uint8_t* end = src + len;

  // separate into 512-byte pages
  uint16_t end_address = (address + 512) & ~511;
  uint16_t max_len = end_address - address;
  if (len > max_len) len = max_len;

  // does page need erasing?
  for (uint16_t i=0; i < len;i+=16) {
    uint8_t buf[16];
    if (read_code(address+i, 16, buf) == 16) {
      uint16_t l = len - i;
      if (l > 16) l = 16;
      if (memcmp_P(buf, src+i, l) == 0) {
        ret += l;
        continue;
      }
    }
    // either changed content was found or reading failed; erase the page
    erase_code_page(address);
    ret = 0;
    break;
  }

  if (ret == (int)len) {
    src += len;
    address += len;
    len = 0;
  }

  while (len) {
    uint16_t write_len = len;
    if (write_len > 256) write_len = 256;
    int wrote = write_code(address, (uint8_t)(write_len & 0xFF), src);
    if (wrote <= 0) return wrote;
    src += wrote;
    address += wrote;
    len -= wrote;
    ret += wrote;
  }

  if (src != end) {
    int next = write_big_code(address, end-src, src);
    if (next <= 0) return next;
    ret += next;
  }
  return ret;
}

// code page size is 512 bytes: not 256!
static int erase_code_page(uint16_t address) {
  MTV_IIC.beginTransmission(MTV_COMMAND);
  MTV_IIC.write(COMMAND_PAGE_ERASE);
  MTV_IIC.write(address >> 8);
  int r = MTV_IIC.endTransmission();
  if (r != 0) {
    Serial.print(F("erase_code_page: command failed "));
    Serial.println(r);
    return -1;
  }

  // need to write one byte to trigger it
  MTV_IIC.beginTransmission(MTV_DATA);
  MTV_IIC.write(0);
  MTV_IIC.write(0);
  r = MTV_IIC.endTransmission();
  delay(12);

  if (r != 0) {
    Serial.print(F("Failed to erase page "));
    Serial.println(r);
    return -2;
  }

  Serial.print(F("Code page "));
  Serial.print(address & ~511, HEX);
  Serial.println(F(" was erased"));
  return 0;
}

int erase_osd_page(uint16_t address) {
  MTV_IIC.beginTransmission(MTV_COMMAND);
  MTV_IIC.write(COMMAND_PAGE_ERASE|COMMAND_OSD);
  MTV_IIC.write(address >> 7);
  int r = MTV_IIC.endTransmission();
  if (r != 0) {
    Serial.print(F("erase_osd_page: command failed "));
    Serial.println(r);
    return -1;
  }

  MTV_IIC.beginTransmission(MTV_DATA);
  MTV_IIC.write(0);
  MTV_IIC.write(0);
  r = MTV_IIC.endTransmission();
  delay(12);

  if (r != 0) {
    Serial.print(F("Failed to erase page "));
    Serial.println(r);
    return -2;
  }

  return 0;
}

int mtv_reset(void) {
  MTV_IIC.beginTransmission(MTV_COMMAND);
  MTV_IIC.write(COMMAND_RESET);
  return MTV_IIC.endTransmission();
}

static int probe_addr(TwoWire& bus, uint8_t addr) {
  bus.beginTransmission(addr);
  return bus.endTransmission();
}

bool non_destructive_patch(uint16_t address, int len, const uint8_t* patch) {
  uint8_t original_code[len];
  if (read_code(address, len, original_code) != len) {
    Serial.print(F("Failed to read code @"));
    Serial.println(address, HEX);
    return false;
  }

  for (int i=0; i < len; i++) {
    uint8_t p = pgm_read_byte(patch+i);
    if (~original_code[i] & p) {
      // patch requires adding bits - this won't work
      Serial.print(F("Patch failure: Addr "));
      Serial.print(address+i, HEX);
      Serial.print(F(" original "));
      Serial.print(original_code[i], HEX);
      Serial.print(F(" vs patch "));
      Serial.println(p, HEX);
      return false;
    }
  }
  
  return write_code(address, len, patch) == len;
}

void do_patch(int main_only) {
  if (write_big_code(0x8400, sizeof(code8400), code8400) != sizeof(code8400)) {
    Serial.println(F("Failed to patch 0x8400"));
    return;
  }

  if (main_only) {
    Serial.println(F("Main patch done"));
    return;
  }

  // these are safe patches, they overwrite empty pages and can be rewritten at any time

  static const uint8_t codeFA12[] PROGMEM = {
    0x12, 0x84, 0x00,  // lcall 8400
    0x7F, 0xFA,        // mov R7, #250
    0x02, 0x7A, 0x8E   // ljmp 7A8E
  };
  if (write_big_code(0xFA12, sizeof(codeFA12), codeFA12) != sizeof(codeFA12)) {
    Serial.println(F("Failed to patch FA12"));
    return;
  }

  static const uint8_t codeAF56[] PROGMEM = {
    0x02, 0x84, 0x13 // ljmp 8413
  };
  if (write_big_code(0xAF56, sizeof(codeAF56), codeAF56) != sizeof(codeAF56)) {
    Serial.println(F("Failed to patch 0xAF56"));
    return;
  }

  // these patches work by REMOVING EXISTING BITS ONLY, since adding bits requires erasing a full page

  // reset hook, overwrite "r7, #250" with a jump to FA12 (see follow-up patch above)
  static const uint8_t code7A8C[] PROGMEM = {
    0x02      // ljmp (FA12)
  };
  if (!non_destructive_patch(0x7A8C, sizeof(code7A8C), code7A8C)) {
    Serial.println(F("Failed to patch 7A8C"));
    return;
  }

  static const uint8_t code8255[] PROGMEM = {
    0x91, 0x1B,      // acall 841B
    0xC0, 0xE0,      // push ACC
    0x02, 0x82, 0x15 // ljmp 8215
  };
  if (!non_destructive_patch(0x8255, sizeof(code8255), code8255)) {
    Serial.println(F("Failed to patch 0x8255"));
    return;
  }
  // this overwrites "push ACC" with a jump to 8255 (see previous). This is in the original TF1 handler which the original firmware doesn't use...
  static const uint8_t code8213[] PROGMEM = {0x80, 0x40};
  if (!non_destructive_patch(0x8213, sizeof(code8213), code8213)) {
    Serial.println(F("Failed to patch 0x8213"));
    return;
  }

  static const uint8_t code8295[] PROGMEM = {
    0x91, 0x2B,         // acall 842B
    0x21, 0xED,         // ajmp 81ED (check_inputs)
  };
  if (!non_destructive_patch(0x8295, sizeof(code8295), code8295)) {
    Serial.println(F("Failed to patch 8295"));
    return;
  }
  // 81ED is the original "check_buttons" function. Hook it to do some menu manipulation stuff.
  static const uint8_t code755d[] PROGMEM = {
    0x00, 0xE1  // redirects a call from 81ED to 00E1, which then jumps to 8295 (see previous patch)
  };
  if (!non_destructive_patch(0x755D, sizeof(code755d), code755d)) {
    Serial.println(F("Failed to patch 755d"));
    return;
  }

  // INT1 - kinda mangled because there's a function sitting on the IE1 vector...
  static const uint8_t code000e[] PROGMEM = {
    0x00, 0x00,       // nops
    0x02, 0x71, 0x20, // ljmp 7120 ; calls AF56
    0x02, 0x71        // ljmp 7163 ; INT1 lands here, also calls AF56
  };
  if (!non_destructive_patch(0x000e, sizeof(code000e), code000e)) {
    Serial.println(F("Failed to patch 0x000E"));
    return;
  }

  Serial.println(F("All patches done."));
}

void mtv_break(void) {
  pinMode(BREAK_PIN, OUTPUT);
  digitalWrite(BREAK_PIN, LOW);
  delay(500);
  pinMode(BREAK_PIN, INPUT);

  delay(10);

  if (probe_addr(MTV_IIC, MTV_COMMAND)) {
    Serial.println(F("MTV230M IIC command failed to ack"));
    return;
  }
  if (probe_addr(MTV_IIC, MTV_DATA)) {
    Serial.println(F("MTV230M IIC data failed to ack"));
    return;
  }

  Serial.println(F("MTV230M IIC is ready"));
}

void print_2hex(int i) {
  char upper = (i >> 4) & 0xF;
  char lower = i & 0xF;
  if (upper >= 10) upper += 'A' - 10;
  else upper += '0';
  if (lower >= 10) lower += 'A' - 10;
  else lower += '0';
  Serial.print(upper);
  Serial.print(lower);
}

static void dump_bank(uint8_t address, int bank, int reg, int count) {
  for (int i=0; i < count; i++) {
    Serial.print('S');
    Serial.print(bank);
    Serial.print('_');
    print_2hex(reg+i*16);
    Serial.print(F(": "));
    MTV_IIC.requestFrom(address, (uint8_t)16);
    for (int j=0; j < 16; j++) {
      print_2hex(MTV_IIC.read());
      Serial.print(' ');
    }
    Serial.println();
  }
  Serial.println(F("-------00-01-02-03-04-05-06-07-08-09-0A-0B-0C-0D-0E-0F"));
}

void dump_eeprom_banks(void) {
  MTV_IIC.beginTransmission(MTV_DEBUG2);
  MTV_IIC.write(1);
  MTV_IIC.write(0);
  MTV_IIC.endTransmission();
  MTV_IIC.requestFrom(MTV_DEBUG2, 1);

  Serial.println(F("Dumping banks from EEPROM:"));
  dump_bank(MTV_DEBUG2, 0, 64, 2);
  dump_bank(MTV_DEBUG2, 1, 0, 3);
  dump_bank(MTV_DEBUG2, 2, 0, 4);
  dump_bank(MTV_DEBUG2, 3, 0, 7);
  MTV_IIC.beginTransmission(MTV_DEBUG2);
  MTV_IIC.write(2);
  MTV_IIC.write(0);
  MTV_IIC.endTransmission();
  MTV_IIC.requestFrom(MTV_DEBUG2, 1);
  dump_bank(MTV_DEBUG2, 4, 0, 6);
  dump_bank(MTV_DEBUG2, 5, 0, 7);
}

static const int reg_banks[6][2] = {
  {64, 2},
  {0, 3},
  {0, 4},
  {0, 7},
  {0, 6},
  {0, 7}
};

void dump_TV5725_banks(void) {
  Serial.println(F("Dumping banks from TV5725:"));

  for (int i = 0; i < 6; i++) {
    MTV_IIC.beginTransmission(MTV_DEBUG3);
    MTV_IIC.write(i);
    MTV_IIC.write(reg_banks[i][0]);
    MTV_IIC.endTransmission();
    MTV_IIC.requestFrom(MTV_DEBUG3, 1);
    dump_bank(MTV_DEBUG3, i, reg_banks[i][0], reg_banks[i][1]);
  }
}

void save_regs(void) {
  uint8_t buf[16];
  uint16_t eeprom_addr = 256;
  for (int i=0; i < 6; i++) {
    for (int j=0; j < reg_banks[i][1]; j++) {
      MTV_IIC.beginTransmission(MTV_DEBUG3);
      MTV_IIC.write(i);
      MTV_IIC.write(reg_banks[i][0]+j*16);
      MTV_IIC.endTransmission();
      MTV_IIC.requestFrom(MTV_DEBUG3, 1);
      MTV_IIC.requestFrom(MTV_DEBUG3, 16);
      MTV_IIC.readBytes(buf, 16);
      MTV_IIC.beginTransmission(MTV_DEBUG2);
      MTV_IIC.write(eeprom_addr>>8);
      MTV_IIC.write(eeprom_addr & 0xFF);
      MTV_IIC.write(buf, 16);
      MTV_IIC.endTransmission();
      eeprom_addr += 16;
    }
  }
  Serial.println(F("Registers saved to EEPROM"));
}

void probe_debug(void) {
  for (int i=0; i < 4; i++) {
    Serial.print(F("MTV230 debug"));
    Serial.print(i);
    if (probe_addr(MTV_IIC, MTV_DEBUG0+i)) {
      Serial.println(F(" I2C not found"));
    } else {
      Serial.println(F(" I2C is ready"));
    }
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  MTV_IIC.begin();
  probe_debug();

  Serial.println(F("MTV230 patcher ready"));
}

int parseHex(Stream& S) {
  int hex = 0;
  bool first = true;

  while (1) {
    int c = S.peek();
    if (c == -1) {
      delay(10);
      c = S.peek();
    }
    switch (c) {
      case '0':case '1':case '2':case '3':case '4':case '5':
      case '6':case '7':case '8':case '9':
        hex <<= 4;
        hex |= c-'0';
        first = false;
        break;
      case 'a':case 'b':case 'c':
      case 'd':case 'e':case 'f':
        hex <<= 4;
        hex |= c-'a'+10;
        first = false;
        break;
      case 'A':case 'B':case 'C':
      case 'D':case 'E':case 'F':
        hex <<= 4;
        hex |= c-'A'+10;
        first = false;
        break;
      case ' ':
      case '\n':
      case '\r':
      case '\t':
        if (first) break;
        // fallthrough
      default:
        return hex;
    }
    S.read();
  }

  return hex;
}

void loop() {
  int command = Serial.read();
  switch (command) {
    case 'e':
      {
        int addr = parseHex(Serial);
        if (MTV_IIC.requestFrom(MTV_DEBUG2, 2, addr, 2, true) != 2) {
          Serial.print(F("Failed to read EEPROM byte at address "));
          Serial.println(addr, HEX);
        } else {
          MTV_IIC.read();
          Serial.print(F("EEPROM byte "));
          Serial.print(addr, HEX);
          Serial.print(F(": "));
          Serial.println(MTV_IIC.read(), HEX);
        }
      }
      break;
    case 'E':
      {
        int addr = parseHex(Serial);
        int data = parseHex(Serial);
        MTV_IIC.beginTransmission(MTV_DEBUG2);
        MTV_IIC.write(addr >> 8);
        MTV_IIC.write(addr);
        MTV_IIC.write(data);
        if (MTV_IIC.endTransmission() != 0) {
          Serial.print(F("Failed to write EEPROM byte "));
          Serial.println(addr, HEX);
        } else {
          Serial.print(F("EEPROM byte "));
          Serial.print(addr, HEX);
          Serial.println(F(" was written."));
        }
      }
      break;
    case 'r':
      {
        int page = parseHex(Serial);
        int reg = parseHex(Serial);
        if (MTV_IIC.requestFrom(MTV_DEBUG3, 2, (page<<8)|reg, 2, true) != 2) {
          Serial.print(F("Failed to read TV5725 S"));
          Serial.print(page, HEX);
          Serial.print('_');
          Serial.println(reg, HEX);
        } else {
          MTV_IIC.read();
          Serial.print(F("TV5725 reg S"));
          Serial.print(page, HEX);
          Serial.print('_');
          Serial.print(reg, HEX);
          Serial.print(F(": "));
          Serial.println(MTV_IIC.read(), HEX);
        }
      }
      break;
    case 'w':
      {
        int page = Serial.parseInt();
        int reg = parseHex(Serial);
        int data = parseHex(Serial);
        MTV_IIC.beginTransmission(MTV_DEBUG3);
        MTV_IIC.write(page);
        MTV_IIC.write(reg);
        MTV_IIC.write(data);
        if (MTV_IIC.endTransmission() != 0) {
          Serial.print(F("Failed to write TV5725 S"));
          Serial.print(page, HEX);
          Serial.print('_');
          Serial.println(reg, HEX);
        } else {
          Serial.print(F("TV5725 S"));
          Serial.print(page, HEX);
          Serial.print('_');
          Serial.print(reg, HEX);
          Serial.print(F(" is now "));
          Serial.println(data, HEX);
        }
      }
      break;
    case 'f':
      {
        uint16_t buf[18];
        int glyph = Serial.parseInt();
        if (read_osd(glyph*32, 18, buf) != 18) {
          Serial.print(F("Failed to fetch data for glyph "));
          Serial.println(glyph);
        } else {
          Serial.print(F("Glyph "));
          Serial.print(glyph);
          Serial.println(':');
          for (int i=0; i < 18; i++) {
            uint16_t p = buf[i] << 4;
            for (int j=0; j < 12; j++) {
              Serial.print(p & 0x8000 ? '#' : ' ');
              p <<= 1;
            }
            Serial.println();
          }
        }
      }
      break;
    case 'c':
      {
        int addr = parseHex(Serial);
        if (MTV_IIC.requestFrom(MTV_DEBUG1, 2, addr, 2, true) != 2) {
          Serial.print(F("Failed to read code at address "));
          Serial.println(addr, HEX);
        } else {
          Serial.print(F("CODE byte "));
          Serial.print(addr & 0xFFFF, HEX);
          Serial.print(F(": "));
          MTV_IIC.read();
          Serial.println(MTV_IIC.read(), HEX);
        }
      }
      break;
    case 'b':
      Serial.println(F("Interrupting MTV230M..."));
      mtv_break();
      break;
    case 'R':
      Serial.println(F("Resetting MTV230M..."));
      mtv_break();
      mtv_reset();
      break;
    case 'p':
      mtv_break();
      do_patch(1);
      mtv_reset();
      break;
    case '$':
      mtv_break();
      do_patch(0);
      break;
    case 'm':
      {
        int addr = parseHex(Serial);
        if (MTV_IIC.requestFrom(MTV_DEBUG0, 2, addr, 2, true) != 2) {
          Serial.print(F("Failed to read memory at address "));
          Serial.println(addr, HEX);
        } else {
          Serial.print(F("MEM byte "));
          Serial.print(addr, HEX);
          Serial.print(F(": "));
          MTV_IIC.read();
          Serial.println(MTV_IIC.read(), HEX);
        }
      }
      break;
    case 'M':
      {
        int addr = parseHex(Serial);
        int data = parseHex(Serial);
        MTV_IIC.beginTransmission(MTV_DEBUG0);
        MTV_IIC.write(addr >> 8);
        MTV_IIC.write(addr & 0xFF);
        MTV_IIC.write(data);
        if (MTV_IIC.endTransmission() == 0) {
          Serial.print(F("MEM byte "));
          Serial.print(addr, HEX);
          Serial.print(F(" set to "));
          Serial.println(data, HEX);
        } else {
          Serial.println(F("Failed to set memory byte"));
        }
      }
      break;
    case 'd':
      dump_eeprom_banks();
      break;
    case 'D':
      dump_TV5725_banks();
      break;
    case 'S':
      save_regs();
      break;
    default:
      delay(1);
  }
}
