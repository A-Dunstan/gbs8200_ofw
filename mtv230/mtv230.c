#include "mtv230.h"

volatile uint32_t TICK_MS;

typedef enum {
  LEFT,
  DOWN,
  UP,
  RIGHT,
  START,
  TICK,
} menu_action;

typedef void (*menu_func)(menu_action);

void menu_none(menu_action);
void main_menu(menu_action);
void eeprom_display(menu_action);
void tv5725_display(menu_action);

static __xdata menu_func menu;

// must return 0!
unsigned char _sdcc_external_startup(void) {
  IE = 0;
  IP = 0;
  PSW = 0;
  TCON = 0;
  TR0 = 0;
  TR1 = 0;
  TMOD = 0;
  PCON = 0;
  SCON = 0;
  P1 = 0xFF;
  P3 = 0xFF;

  PADMOD[0] = 0x83;  // HIICE | FclkE | P62E
  PADMOD[1] = 0;
  PADMOD[2] = 0xFF;  // P47oe | P46oe | P45oe | P44oe | P43oe | P42oe | P41oe | P40oe
  PADMOD[3] = 0xE0;  // P57oe | P56oe | P55oe
  OPTION  = 0xA8;  // PWMF | Slvabs1 | ENSCL

  // start watchdog, 2 seconds
  WDT = 0xC0;   // WEN | WCLR

  TICK_MS = 0;
  TMOD = 0x10;  // timer 1 = 16-bit timer
  TL1 = -20000 & 0xFF;
  TH1 = (-20000 >> 8) & 0xFF;
  ET1 = 1;
  TR1 = 1;
  PT1 = 1;

  config_I2C();

  EA = 1;

  menu = main_menu;

  return 0;
}

uint32_t read_tick(void) __naked {
  __asm
    mov R0, #_TICK_MS+1
    mov A, @R0     ; byte 1
  1$:
    inc R0
    mov DPH, A
    mov B, @R0     ; byte 2
    inc R0
    mov 7, @R0     ; byte 3
    mov R0, #_TICK_MS
    mov DPL, @R0   ; byte 0
    inc R0
    mov A, @R0     ; byte 1 (again)
    ; if not equal there was a rollover, try again
    cjne A, DPH, 1$
    mov A, R7
    ret
  __endasm;
}

void main() {
  static __xdata uint32_t last_ms;
  __xdata uint8_t last_buttons = 0;
  __xdata uint8_t held = 0;

  last_ms = read_tick();
  osd_init();
  init_TV5725();

  menu(START);

  while (1) {
    uint8_t i;

    if (!(BREAK_PIN & 1)) enter_ispc_mode();
    // feed watchdog
    WDT = 0xC0;   // WEN | WCLR

    uint8_t buttons = last_buttons;
    for (i=0; i < 4; i++) {
      if (!(PORT5[i] & 1)) {
        if (!(buttons & (1<<i))) {
          buttons |= 1<<i;
          menu(LEFT+i);
        }
      } else {
        buttons &= ~(1<<i);
        held &= ~(1<<i);
      }
    }
    last_buttons = buttons;

    if (read_tick() - last_ms > 500) {

      buttons = held & last_buttons;
      for (i=0; i < 4; i++) {
        if (buttons & (1<<i))
          menu(LEFT+i);
      }
      held = last_buttons;

      menu(TICK);

      last_ms += 500;
      LED = !LED;
    }
  }
}

void IE0_isr(void) __interrupt(IE0_VECTOR) {}
void TF0_isr(void) __interrupt(TF0_VECTOR) {}
void RI_TI_isr(void) __interrupt(SI0_VECTOR) {
  RI = 0;
  TI = 0;
}

void TF1_isr(void) __interrupt(TF1_VECTOR)
{
  // need to know the exact cycles between stopping/start the timer: use assembly
  __asm
    mov A, #<(3-20000)
    clr TR1            ; pause timer1
    add A, TL1         ; 1 cycle
    mov TL1, A         ; 1 cycle
    mov A, #>(3-20000) ; 1 cycle
    addc A, TH1        ; 1 cycle
    mov TH1, A         ; 1 cycle
    setb TR1           ; run timer1 (1 cycle)
  __endasm;

  TICK_MS += 10;
}

__code const char new_lang_menu[][36] = {
  "    Language Menu",
  " 1. English",
  " 2. \x7B \x7C",
  " .. Return",
  " ",
  " SWITCH FIRMWARE",
  ""
};

void delay(uint8_t ms) __naked {
  ms;
  __asm
  1$:
    nop
    nop
    nop
    nop
    nop
    mov A, #249
  2$:
    nop
    nop
    nop
    nop
    nop
    dec A
    jnz 2$
    djnz DPL, 1$
    ret
  __endasm;
}

void delay_us(uint8_t us) __naked {
  us;
  __asm
  1$:
    djnz DPL, 1$
    ret
  __endasm;
}

void IE1_isr(void) __interrupt(IE1_VECTOR) {
  // possible IE1 causes: I2C
  uint8_t i2c_ints = IIC_INTFLG & IIC_INTEN;
  if (i2c_ints) i2c_handler(i2c_ints);
}

void eeprom_display(menu_action action) {
  __xdata static uint16_t address = 0;

  if (action == LEFT) {
    menu = main_menu;
    main_menu(START);
    return;
  }
  else if (action == RIGHT) address += 0x70;
  else if (action == UP) address -= 8;
  else if (action == DOWN) address += 8;
  else if (action == START) {
    osd_clear();
    osd_show_string(9, 0, 3, "EEPROM DUMP");
    for (uint8_t y=1; y < 15; y++)
      osd_show_char(4, y, 6, ':');
  }
  else return;

  for (uint8_t y=1; y < 15; y++) {
    __xdata uint8_t buf[8];

    address &= 0x0FFF;
    osd_show_hex4(0, y, 6, address);
    EEPROM_read_from(buf, address, 8);
    address += 8;
    for (uint8_t x=0; x < 8; x++) {
      osd_show_hex2(6+x*3, y, 7, buf[x]);
    }
  }
  address -= 0x70;
  osd_enable(1);
}

typedef struct {
  uint8_t bank;
  uint8_t reg_begin;
  uint8_t rows;
  __code const char* name;
} tv5725_chapter;

static const tv5725_chapter chapters[] = {
  { 0, 0, 6, "Status"},
  { 1, 0, 6, "Input Formatter"},
  { 2, 0, 8, "Deinterlace"},
  { 1, 0x30, 6, "HD Bypass"},
  { 0, 0x40, 4, "Miscellaneous"},
  { 4, 0, 4, "Memory"},
  { 4, 0x20, 5, "Capture & Playback"},
  { 4, 0x40, 4, "Read & Write Fifo"},
  { 3, 0, 14, "Video Processor"},
  { 0, 0x90, 2, "OSD"},
  { 1, 0x60, 6, "Mode Detect"},
  { 5, 0, 4, "ADC"},
  { 5, 0x20, 10, "Sync Proc"}
};

void tv5725_display(menu_action action) {
  static __code tv5725_chapter* __xdata page = &chapters[0];

  if (action == UP) {
    if (page == &chapters[0])
      page = &chapters[12];
    else
      --page;
    osd_clear();
  } else if (action == DOWN) {
    if (page == &chapters[12])
      page = &chapters[0];
    else
      ++page;
    osd_clear();
  } else if (action == LEFT) {
    menu = main_menu;
    main_menu(START);
    return;
  } else if (action == RIGHT) {
    // move a highlight??
    return;
  } else if (action == START) {
    osd_clear();
  }

  // set bank
  TV5725_write(&page->bank, 0xF0, 1);
  uint8_t reg = page->reg_begin;
  osd_show_string(2, 0, 3, page->name);

  for (uint8_t y = 1; y <= page->rows; y++) {
    __xdata uint8_t buf[8];

    TV5725_read(buf, reg, 8);
    osd_show_hex2(2, y, 6, reg);
    osd_show_char(4, y, 6, ':');
    for (uint8_t x = 0; x < 8; x++) {
      osd_show_hex2(6+x*3, y, 7, buf[x]);
    }
    reg += 8;
  }
  osd_enable(1);
}

void main_menu(menu_action action) {
  static __xdata uint8_t timeout;
  static __xdata uint8_t active = 1;
  const uint8_t max_active = 3;

  switch (action) {
    case TICK: // reduce timeout
      if (--timeout != 0)
        return;
      // fallthrough to close after timeout
    case LEFT: // close
      menu = menu_none;
      menu_none(START);
      return;
    case DOWN:
      if (active == max_active)
        active = 1;
      else
        ++active;
      break;
    case UP:
      if (active == 1)
        active = max_active;
      else
        --active;
      break;
    case RIGHT: // open new menu
      if (active == 1) {
        menu = eeprom_display;
      } else if (active == 2) {
        menu = tv5725_display;
      } else { // active == 3
        EEPROM_write_byte(3, 0xFF);
        // watchdog will reboot
        while(1);
      }
      menu(START);
      return;
    case START: // begin
      osd_clear();
      osd_show_string(4, 0, 3, "MAIN MENU");
      break;
  }

  timeout = 20;
  osd_show_string(2, 2, (active==1 ? 6:7), "1. EEPROM dump");
  osd_show_string(2, 3, (active==2 ? 6:7), "2. TV5725 dump");
  osd_show_string(2, 4, (active==3 ? 6:7), "3. Revert to original");
  osd_enable(1);
}

void menu_none(menu_action action) {
  if (action == START) {
    osd_enable(0);
  } if (action == RIGHT) {
    menu = main_menu;
    main_menu(START);
  }
}

