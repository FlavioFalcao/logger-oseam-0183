/*-------------------------------------------------------------------------/
/  Stand-alone MMC boot loader  R0.01
/--------------------------------------------------------------------------/
/
/  Copyright (C) 2010, ChaN, all right reserved.
/
/ * This software is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/--------------------------------------------------------------------------/
/ Dec 6, 2010  R0.01  First release
/--------------------------------------------------------------------------/
/ This is a stand-alone MMC/SD boot loader for megaAVRs. It requires a 4KB
/ boot section for code, four GPIO pins for MMC/SD as shown in sch.jpg and
/ nothing else. To port the boot loader into your project, follow the
/ instruction sdescribed below.
/
/ 1. Setup the hardware. Attach a memory card socket to the any GPIO port
/    where you like. Select boot size at least 4KB for the boot loader with
/    BOOTSZ fuses and enable boot loader with BOOTRST fuse.
/
/ 2. Setup the software. Change the four port definitions in the asmfunc.S.
/    Change MCU_TARGET, BOOT_ADR and MCU_FREQ in the Makefile. The BOOT_ADR
/    is a BYTE address of boot section in the flash. Build the boot loader
/    and write it to the device with a programmer.
/
/ 3. Build the application program and output it in binary form instead of
/    hex format. Rename the file "app.bin" and put it into the memory card.
/
/ 4. Insert the card and turn the target power on. When the boot loader found
/    the application file, the file is written into the flash memory prior to
/    start the application program. On-board LED lights (if exist) during
/    the flash programming operation.
/
/-------------------------------------------------------------------------*/


#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <string.h>
#include "pff.h"


void flash_erase (DWORD);				/* Erase a flash page (asmfunc.S) */
void flash_write (DWORD, const BYTE*);	/* Program a flash page (asmfunc.S) */

void init_leds();
void led_power_on();
void led_power_off();
void led_power_toggle();
void led_write_on();
void led_write_off();

static inline int mem_cmpP(const void* dst, const void* src, int cnt);

FATFS Fatfs;				// Petit-FatFs work area 
BYTE Buff[SPM_PAGESIZE];	// Page data buffer 
char filename[13] ="OSMFWxxx.BIN\0"; 			// filename
char filename2[13] ="OSMFIRMW.BIN\0"; 			// filename

static uint8_t pagecmp(uint16_t addr, uint8_t *data)
{
	uint16_t i;

	for (i = 0; i < SPM_PAGESIZE; i++) {
		if (pgm_read_byte(addr++) != *data++)
			return 1;
	}

	return 0;
}

void doProgram() {
	DWORD fa;	/* Flash address */
	WORD br;	/* Bytes read */

	led_write_on();
	for (fa = 0; fa < BOOT_ADR; fa += SPM_PAGESIZE) {	/* Update all application pages */
		memset(Buff, 0xFF, SPM_PAGESIZE);		/* Clear buffer */
		pf_read(Buff, SPM_PAGESIZE, &br);		/* Load a page data */
							
		if (br) {					/* Bytes Read > 0? */	
			uint8_t i;
			for (i = br; i < SPM_PAGESIZE; i++)     /* Pad the remaining last page with 0xFF so that comparison goes OK */
				Buff[i] = 0xFF;
	//		if (pagecmp(fa, Buff)) {		/* Only flash if page is changed */
				flash_erase(fa);		/* Erase a page */
				flash_write(fa, Buff);		/* Write it if the data is available */				
	//		}
		}
	}
	led_write_off();
}

void checkProgram() {	
	led_power_on();
	pf_mount(&Fatfs);	/* Initialize file system */
	
    WORD flashver = eeprom_read_word((const uint16_t *)E2END - 1);
	if (flashver > 999) {
		flashver = 9;
	}
	BYTE y, tmp;
	WORD x;
	BYTE found = 0;
	
	for (x = flashver+10; x > flashver; x--) {
		y = x / 100;
		filename[5] = y + 0x30;
		tmp = x % 100;

		y = tmp / 10;
		filename[6] = y + 0x30;
		tmp = x % 10;

		filename[7] = tmp + 0x30;

		if (pf_open(filename) == FR_OK) { /* File opens normally */		
			found = 1;
			doProgram();
		}
		led_power_toggle();
	}

	if (found == 0) {
		if (pf_open(filename2) == FR_OK) { /* File opens normally */		
			doProgram();
		}
	}

	if (pgm_read_word(0) != 0xFFFF)		/* Start application if exist */
		((void(*)(void))0)();
}

int main (void)
{
	init_leds();
	
	while (1) {
		checkProgram();
    
		uint8_t i = 0;
	
		for (i = 0; i <= 10; i++) {
			led_power_toggle();
			_delay_ms(100);
		}
	}
}
