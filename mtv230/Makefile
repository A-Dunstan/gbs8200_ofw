
ASLINKFLAGS = -Wl-bHOME=0x8400 --xram-loc 0x800

%.rel : %.c mtv230.h
	sdcc -c $<
	
%.rel : %.a51
	sdas8051 -l -o $<
	
all : gbs.h

gbs.h : mtv230.ihx
	srec_cat -Disable_Sequence_Warnings $< -intel -o gbs.h -C-Array code8400 -PREfix static -POSTfix PROGMEM
	
mtv230.ihx : mtv230.rel stub.rel i2c.rel eeprom.rel tv5725.rel osd.rel
	sdcc $(ASLINKFLAGS) $^
	
clean :
	@rm -f gbs.h *.ihx *.lk *.lst *.map, *.mem *.rel *.rst *.sym *.map *.asm
