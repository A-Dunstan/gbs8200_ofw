
	; imports
	.globl _new_lang_menu
	.globl _EEPROM_read_byte
	.globl _EEPROM_write_byte
	.globl _config_I2C
	.globl _delay

	; exports
	.globl _enter_ispc_mode
	.globl _reset_TV5725

	.EQU MENU_STRINGS_BASE, 0x1E
	.EQU CURRENT_OPEN_MENU, 0x55
	.EQU CURSOR_LINE, 0x56
	.EQU BRIGHTNESS, 0x61
	.EQU CONTRAST, 0x62
	.EQU SATURATION, 0x63
	.EQU SHARPNESS, 0x64
	
	.EQU DOWN_BUTTON, 0xF39
	.EQU UP_BUTTON, 0xF3A
	.EQU SDA, P1.1
	.EQU SCL, P1.0

	.EQU init_timer_serial_pads, 0x7A91
	.EQU check_inputs, 0x81ED

	; page 8400 is landing page for all hooks (first "safe" erasable page)
	; 8400: RESET
	; 8403: unused
	; 840B: unused
	; 8413: IE1
	; 841B: TF1
	; 8423: RI_TI
	; 842B: check_inputs hook, called each loop by original firmware
	
	.area VECT (CODE,ABS)
	.org 0x7B5B
_enter_ispc_mode:
	.org 0x81CF
_reset_TV5725:

	.org 0x8400
	ljmp RESET
	
	.org 0x8403
	ajmp __interrupt_vect + 0x03
	
	.org 0x840B
	ajmp __interrupt_vect + 0x0B
	
	.org 0x8413
	ljmp IE1_check
	
	.org 0x841B
	dec SP
	dec SP
	ajmp __interrupt_vect + 0x1B
	
	.org 0x8423
	dec SP
	dec SP
	ajmp __interrupt_vect + 0x23
	
	.org 0x842B	
	ljmp input_hook
	
__interrupt_vect:

	;; reserved areas (patches live here)
	.org 0xAF56
	.ds 3
	.org 0xF000
	.ds 4
	.org 0xFA12
	.ds 8

	.area CSEG    (CODE)
RESET:
	;; reset the master I2C bus in case it glitched
	;; this is what causes the default firmware to occasionally reset to japanese/default settings!
	clr SCL
	inc DPTR
	clr SDA
	mov R0, #14
10$:
	setb SCL
	inc DPTR
	clr SCL
	djnz R0, 10$
	; send stop
	setb SCL
	inc DPTR
	setb SDA

	;; read byte 3 from EEPROM
	mov DPTR, #3
	lcall _EEPROM_read_byte
	mov A, DPL
	cjne A, #0x82, 1$
	
	;; check for up button
	mov DPTR, #UP_BUTTON
	movx A, @DPTR
	jnb ACC.0, 3$       ; up button, set/run original firmware
	ljmp __interrupt_vect
3$:
	mov DPTR, #3
	mov A, #0xFF
	push ACC
	lcall _EEPROM_write_byte
	dec SP
	sjmp 2$
	
1$:
	;; check for down button
	mov DPTR, #DOWN_BUTTON
	movx A, @DPTR
	jb ACC.0, 2$       ; no down button, continue to factory firmware
	; else set EEPROM byte 3 to 0x82 and run custom
	ajmp start_custom
	
2$:	
	; pop return address
	pop ACC
	pop ACC
	
	mov dpl, #250
	lcall _delay
	lcall init_timer_serial_pads
	clr EX0     ; disable INT0 because there's no handler in the original code
	
	lcall _config_I2C
	setb EA     ; enable interrupts
	ret
	
start_custom:
	mov DPTR, #3
	mov A, #0x82
	push ACC
	lcall _EEPROM_write_byte
	dec SP
	ljmp __interrupt_vect
	
input_hook:
	; check if strings_base is the original english table
	mov R0, #MENU_STRINGS_BASE
	cjne @R0,#0x31,1$
	inc R0
	cjne @R0,#0xC7,2$
	; load modified string table
	mov @R0, #<english_menus
	dec R0
	mov @R0, #>english_menus
	ret
	
1$:
	cjne @R0, #>english_menus, 2$
	inc R0
	cjne @R0, #<english_menus, 2$
	
	; language menu must be open
	mov R0, #CURRENT_OPEN_MENU
	cjne @R0, #6, 2$
	; current menu item must be 5
	inc R0
	cjne @R0, #5, 2$
	
	; call original input processor
	pop ACC
	pop ACC
	lcall check_inputs
	; check if menu/right button was pressed
	cjne R7, #-9, 2$
	; else switch to new firmware
	clr EA
	ajmp start_custom
2$:
	ret
	
IE1_check:
	; need to examine return address without trashing any registers
	push ACC
	mov A, R0
	mov R0, SP
	dec R0
	mov @R0, A              ; overwrite high byte of return address with original R0
	pop ACC
	dec R0
	xch A, @R0              ; swap low byte of return address with original A
	; return address will be 0103 or 0106
	jb ACC.0, 1$            ; jump if IE1 did not bring us here
	pop 0
	pop ACC
	ljmp __interrupt_vect + 0x13 ; otherwise jump to IE1
1$:
	; this was a call to 000E, load default display values to memory
	dec R0
	mov SP, R0              ; don't care about popping original values
	mov A, #50
	mov BRIGHTNESS, A
	mov CONTRAST, A
	mov SATURATION, A
	mov SHARPNESS, #5
	ret

english_menus:
	.dw 0x2FAB
	.dw 0x27CB
	.dw 0x29E7
	.dw 0x2B4F
	.dw 0x2C6F
	.dw 0x2DD7
	.dw _new_lang_menu  ; 0x2EF7
	.dw 0x26F3
