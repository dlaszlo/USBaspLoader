/* Name: main.c
 * Project: USBaspLoader
 * Author: Christian Starkjohann
 * Author: Stephan Baerwolf
 * Creation Date: 2007-12-08
 * Modification Date: 2013-03-31
 * Tabsize: 4
 * Copyright: (c) 2007 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt)
 */

#include "spminterface.h"  /* must be included as first! */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <util/delay.h>


#if 0
/*
 * 29.09.2012 /  30.09.2012
 * 
 * Since cpufunc.h is not needed in this context and
 * since it is not available in all toolchains, this include
 * becomes deactivated by github issue-report.
 * (In case of trouble it remains in sourcecode for reactivation.)
 * 
 * The autor would like to thank Lena-M for reporting this
 * issue (https://github.com/baerwolf/USBaspLoader/issues/1).
 */
#include <avr/cpufunc.h>
#endif

#include <avr/boot.h>

#include <string.h>



#include "bootloaderconfig.h"
#include "usbdrv/usbdrv.c"

#if I2C_LCD
#include "lcd.h"
#endif

#ifndef BOOTLOADER_ADDRESS
  #error need to know the bootloaders flash address!
#endif
#define BOOTLOADER_PAGEADDR	(BOOTLOADER_ADDRESS - (BOOTLOADER_ADDRESS % SPM_PAGESIZE))

/* ------------------------------------------------------------------------ */

/* Request constants used by USBasp */
#define USBASP_FUNC_CONNECT         1
#define USBASP_FUNC_DISCONNECT      2
#define USBASP_FUNC_TRANSMIT        3
#define USBASP_FUNC_READFLASH       4
#define USBASP_FUNC_ENABLEPROG      5
#define USBASP_FUNC_WRITEFLASH      6
#define USBASP_FUNC_READEEPROM      7
#define USBASP_FUNC_WRITEEEPROM     8
#define USBASP_FUNC_SETLONGADDRESS  9

// additional USBasp Commands
#define USBASP_FUNC_SETISPSCK	     10
#define USBASP_FUNC_TPI_CONNECT      11
#define USBASP_FUNC_TPI_DISCONNECT   12
#define USBASP_FUNC_TPI_RAWREAD      13
#define USBASP_FUNC_TPI_RAWWRITE     14
#define USBASP_FUNC_TPI_READBLOCK    15
#define USBASP_FUNC_TPI_WRITEBLOCK   16
#define USBASP_FUNC_GETCAPABILITIES 127
/* ------------------------------------------------------------------------ */

#ifndef ulong
#   define ulong    unsigned long
#endif
#ifndef uint
#   define uint     unsigned int
#endif


/* allow compatibility with avrusbboot's bootloaderconfig.h: */
#ifdef BOOTLOADER_INIT
#   define bootLoaderInit()         BOOTLOADER_INIT
#   define bootLoaderExit()
#endif
#ifdef BOOTLOADER_CONDITION
#   define bootLoaderCondition()    BOOTLOADER_CONDITION
#endif

/* device compatibility: */
#ifndef GICR    /* ATMega*8 don't have GICR, use MCUCR instead */
#   define GICR     MCUCR
#endif

/* ------------------------------------------------------------------------ */

#if (FLASHEND) > 0xffff /* we need long addressing */
#   define CURRENT_ADDRESS  currentAddress.l
#   define addr_t           ulong
#else
#   define CURRENT_ADDRESS  currentAddress.w[0]
#   define addr_t           uint
#endif

typedef union longConverter{
    addr_t  l;
    uint    w[sizeof(addr_t)/2];
    uchar   b[sizeof(addr_t)];
}longConverter_t;


#if EXIT_AFTER_UPLOAD
static volatile unsigned char   requestExit = 0;
  #if BOOTLOADER_CAN_EXIT
static volatile unsigned char	stayInLoader = 0xfe;
  #endif
#endif


static longConverter_t  	currentAddress; /* in bytes */
static uchar            	bytesRemaining;
static uchar            	isLastPage;
#if HAVE_EEPROM_PAGED_ACCESS
static uchar            	currentRequest;
#else
static const uchar      	currentRequest = 0;
#endif

static const uchar  signatureBytes[4] = {
#ifdef SIGNATURE_BYTES
    SIGNATURE_BYTES
#elif defined (__AVR_ATmega8535__)
    0x1e, 0x93, 0x08, 0
#elif defined (__AVR_ATmega8__) || defined (__AVR_ATmega8A__) || defined (__AVR_ATmega8HVA__)
    0x1e, 0x93, 0x07, 0
#elif defined (__AVR_ATmega16__)
    0x1e, 0x94, 0x03, 0
#elif defined (__AVR_ATmega32__)
    0x1e, 0x95, 0x02, 0
#elif defined (__AVR_ATmega48__) || defined (__AVR_ATmega48A__) || defined (__AVR_ATmega48P__)
    #error ATmega48 does not support bootloaders!
    0x1e, 0x92, 0x05, 0
#elif defined (__AVR_ATmega48PA__)
    #error ATmega48 does not support bootloaders!
    0x1e, 0x92, 0x0A, 0
#elif defined (__AVR_ATmega88__) || defined (__AVR_ATmega88A__) || defined (__AVR_ATmega88P__)
    0x1e, 0x93, 0x0a, 0
#elif defined (__AVR_ATmega88PA__)
    0x1e, 0x93, 0x0F, 0
#elif defined (__AVR_ATmega164A__)
    0x1e, 0x94, 0x0f, 0
#elif defined (__AVR_ATmega164P__) || defined (__AVR_ATmega164PA__)
    0x1e, 0x94, 0x0a, 0
#elif defined (__AVR_ATmega168__) || defined (__AVR_ATmega168A__) || defined (__AVR_ATmega168P__)
    0x1e, 0x94, 0x06, 0
#elif defined (__AVR_ATmega168PA__)
    0x1e, 0x94, 0x0B, 0
#elif defined (__AVR_ATmega324A__)
    0x1e, 0x95, 0x15, 0
#elif defined (__AVR_ATmega324P__)
    0x1e, 0x95, 0x08, 0
#elif defined (__AVR_ATmega324PA__)
    0x1e, 0x95, 0x11, 0
#elif defined (__AVR_ATmega328__)
    0x1e, 0x95, 0x14, 0
#elif defined (__AVR_ATmega328P__)
    0x1e, 0x95, 0x0f, 0
#elif defined (__AVR_ATmega640__)
    0x1e, 0x96, 0x08, 0
#elif defined (__AVR_ATmega644__) || defined (__AVR_ATmega644A__)
    0x1e, 0x96, 0x09, 0
#elif defined (__AVR_ATmega644P__) || defined (__AVR_ATmega644PA__)
    0x1e, 0x96, 0x0a, 0
#elif defined (__AVR_ATmega128__)
    0x1e, 0x97, 0x02, 0
#elif defined (__AVR_ATmega1280__)
    0x1e, 0x97, 0x03, 0
#elif defined (__AVR_ATmega1281__)
    0x1e, 0x97, 0x04, 0
#elif defined (__AVR_ATmega1284__)
    0x1e, 0x97, 0x06, 0
#elif defined (__AVR_ATmega1284P__)
    0x1e, 0x97, 0x05, 0
#elif defined (__AVR_ATmega2560__)
    0x1e, 0x98, 0x01, 0
#elif defined (__AVR_ATmega2561__)
    0x1e, 0x98, 0x02, 0
#else
#   if (defined(SIGNATURE_0) && defined(SIGNATURE_1) && defined(SIGNATURE_2))
#     warning "Device signature is not known - using AVR Libc suggestion..."
    SIGNATURE_0, SIGNATURE_1, SIGNATURE_2, 0
#   else
#     error "Device signature is not known, please edit main.c!"
#   endif
#endif
};

/* ------------------------------------------------------------------------ */

#if (USE_BOOTUP_CLEARRAM)
/*
* Under normal circumstances, RESET will not clear contents of RAM.
* As always, if you want it done - do it yourself...
*/
void __attribute__ ((section(".init3"),naked,used,no_instrument_function)) __func_clearram(void);
void __func_clearram(void) {
  extern size_t __bss_end;
  asm volatile (
    "__clearram:\n\t"
    "ldi r29, %[ramendhi]\n\t"
    "ldi r28, %[ramendlo]\n\t"
    "__clearramloop%=:\n\t"
    "st -Y , __zero_reg__\n\t"
    "cp r28, %A[bssend]\n\t"
    "cpc r29, %B[bssend]\n\t"
    "brne __clearramloop%=\n\t"
    :
    : [ramendhi] "M" (((RAMEND+1)>>8) & 0xff),
      [ramendlo] "M" (((RAMEND+1)>>0) & 0xff),
      [bssend] "r" (&__bss_end)
    : "memory"
      );
}
#endif

static void (*nullVector)(void) __attribute__((__noreturn__));

static void __attribute__((__noreturn__)) leaveBootloader(void);
static void leaveBootloader(void) {
    DBG1(0x01, 0, 0);
    cli();
    usbDeviceDisconnect();
    bootLoaderExit();
    USB_INTR_ENABLE = 0;
    USB_INTR_CFG = 0;       /* also reset config bits */
    GICR = (1 << IVCE);     /* enable change of interrupt vectors */
    GICR = (0 << IVSEL);    /* move interrupts to application flash section */

/*
 * There seems to be another funny compiler Bug.
 * When gcc is using "eicall" opcode it forgets to modify EIND.
 * On devices with large flash memory there are some target address bits
 * missing. In this case some zero bits...
 */
#if (defined(EIND) && ((FLASHEND)>131071))
  EIND=0;
#endif
/* We must go through a global function pointer variable instead of writing
 *  ((void (*)(void))0)();
 * because the compiler optimizes a constant 0 to "rcall 0" which is not
 * handled correctly by the assembler.
 */
    nullVector();
}

/* ------------------------------------------------------------------------ */


uchar usbFunctionSetup_USBASP_FUNC_TRANSMIT(usbRequest_t *rq) {
  uchar rval = 0;
  usbWord_t address;
  address.bytes[1] = rq->wValue.bytes[1];
  address.bytes[0] = rq->wIndex.bytes[0];

  if(rq->wValue.bytes[0] == 0x30){        /* read signature */
    rval = rq->wIndex.bytes[0] & 3;
    rval = signatureBytes[rval];
#if HAVE_READ_LOCK_FUSE
#if defined (__AVR_ATmega8535__) || 					\
    defined (__AVR_ATmega8__) || defined (__AVR_ATmega8A__) || 		\
    defined (__AVR_ATmega16__) || defined (__AVR_ATmega32__)
  }else if(rq->wValue.bytes[0] == 0x58 && rq->wValue.bytes[1] == 0x00){  /* read lock bits */
      rval = boot_lock_fuse_bits_get(GET_LOCK_BITS);
  }else if(rq->wValue.bytes[0] == 0x50 && rq->wValue.bytes[1] == 0x00){  /* read lfuse bits */
      rval = boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS);
  }else if(rq->wValue.bytes[0] == 0x58 && rq->wValue.bytes[1] == 0x08){  /* read hfuse bits */
      rval = boot_lock_fuse_bits_get(GET_HIGH_FUSE_BITS);

#elif defined (__AVR_ATmega48__)   || defined (__AVR_ATmega48A__)   || defined (__AVR_ATmega48P__)   || defined (__AVR_ATmega48PA__)  ||  \
defined (__AVR_ATmega88__)   || defined (__AVR_ATmega88A__)   || defined (__AVR_ATmega88P__)   || defined (__AVR_ATmega88PA__)  ||  \
defined (__AVR_ATmega164A__) || defined (__AVR_ATmega164P__)  || 								      \
defined (__AVR_ATmega168__)  || defined (__AVR_ATmega168A__)  || defined (__AVR_ATmega168P__)  || defined (__AVR_ATmega168PA__) ||  \
defined (__AVR_ATmega324A__) || defined (__AVR_ATmega324P__)  ||								      \
defined (__AVR_ATmega328__)  || defined (__AVR_ATmega328P__)  ||								      \
defined (__AVR_ATmega640__)  ||													\
defined (__AVR_ATmega644__)  || defined (__AVR_ATmega644A__)  || defined (__AVR_ATmega644P__) || defined (__AVR_ATmega644PA__)  ||  \
defined (__AVR_ATmega128__)  ||													      \
defined (__AVR_ATmega1280__) ||													\
defined (__AVR_ATmega1281__) ||													\
defined (__AVR_ATmega1284__) || defined (__AVR_ATmega1284P__)  ||													\
defined (__AVR_ATmega2560__) ||													\
defined (__AVR_ATmega2561__)
  }else if(rq->wValue.bytes[0] == 0x58 && rq->wValue.bytes[1] == 0x00){  /* read lock bits */
      rval = boot_lock_fuse_bits_get(GET_LOCK_BITS);
  }else if(rq->wValue.bytes[0] == 0x50 && rq->wValue.bytes[1] == 0x00){  /* read lfuse bits */
      rval = boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS);
  }else if(rq->wValue.bytes[0] == 0x58 && rq->wValue.bytes[1] == 0x08){  /* read hfuse bits */
      rval = boot_lock_fuse_bits_get(GET_HIGH_FUSE_BITS);
  }else if(rq->wValue.bytes[0] == 0x50 && rq->wValue.bytes[1] == 0x08){  /* read efuse bits */
      rval = boot_lock_fuse_bits_get(GET_EXTENDED_FUSE_BITS );
#else
  #warning "HAVE_READ_LOCK_FUSE is activated but MCU unknown -> will not support this feature"
#endif
#endif
#if HAVE_FLASH_BYTE_READACCESS
  }else if(rq->wValue.bytes[0] == 0x20){  /* read FLASH low  byte */
#if ((FLASHEND) > 65535)
      rval = pgm_read_byte_far((((addr_t)address.word)<<1)+0);
#else
      rval = pgm_read_byte((((addr_t)address.word)<<1)+0);
#endif
  }else if(rq->wValue.bytes[0] == 0x28){  /* read FLASH high byte */
#if ((FLASHEND) > 65535)
      rval = pgm_read_byte_far((((addr_t)address.word)<<1)+1);
#else
      rval = pgm_read_byte((((addr_t)address.word)<<1)+1);
#endif
#endif
#if HAVE_EEPROM_BYTE_ACCESS
  }else if(rq->wValue.bytes[0] == 0xa0){  /* read EEPROM byte */
      rval = eeprom_read_byte((void *)address.word);
  }else if(rq->wValue.bytes[0] == 0xc0){  /* write EEPROM byte */
      eeprom_write_byte((void *)address.word, rq->wIndex.bytes[1]);
#endif
#if HAVE_CHIP_ERASE
  }else if(rq->wValue.bytes[0] == 0xac && rq->wValue.bytes[1] == 0x80){  /* chip erase */
      addr_t addr;
#if HAVE_BLB11_SOFTW_LOCKBIT
      for(addr = 0; addr < (addr_t)(BOOTLOADER_PAGEADDR) ; addr += SPM_PAGESIZE) {
#else
      for(addr = 0; addr <= (addr_t)(FLASHEND) ; addr += SPM_PAGESIZE) {
#endif
	  /* wait and erase page */
	  DBG1(0x33, 0, 0);
#   ifndef NO_FLASH_WRITE
	  boot_spm_busy_wait();
	  cli();
	  boot_page_erase(addr);
	  sei();
#   endif
      }
#endif
  }else{
      /* ignore all others, return default value == 0 */
  }
        
  return rval;
}


usbMsgLen_t usbFunctionSetup(uchar data[8])
{
    usbRequest_t    *rq = (void *)data;
    usbMsgLen_t     len = 0;
    static uchar    replyBuffer[4];

    usbMsgPtr = (usbMsgPtr_t)replyBuffer;

#if I2C_LCD
    if(rq->bRequest == USBASP_FUNC_CONNECT){
	LCD_setCursor(0, 1);
	LCD_writeStr("upload...");
    }
#endif

    if(rq->bRequest == USBASP_FUNC_TRANSMIT){   /* emulate parts of ISP protocol */
        replyBuffer[3] = usbFunctionSetup_USBASP_FUNC_TRANSMIT(rq);
        len = (usbMsgLen_t)4;
    }else if((rq->bRequest == USBASP_FUNC_ENABLEPROG) || (rq->bRequest == USBASP_FUNC_SETISPSCK)){
        /* replyBuffer[0] = 0; is never touched and thus always 0 which means success */
        len = (usbMsgLen_t)1;
    }else if(rq->bRequest >= USBASP_FUNC_READFLASH && rq->bRequest <= USBASP_FUNC_SETLONGADDRESS){
        currentAddress.w[0] = rq->wValue.word;
        if(rq->bRequest == USBASP_FUNC_SETLONGADDRESS){
#if (FLASHEND) > 0xffff
            currentAddress.w[1] = rq->wIndex.word;
#endif
        }else{
            bytesRemaining = rq->wLength.bytes[0];
            /* if(rq->bRequest == USBASP_FUNC_WRITEFLASH) only evaluated during writeFlash anyway */
            isLastPage = rq->wIndex.bytes[1] & 0x02;
#if HAVE_EEPROM_PAGED_ACCESS
            currentRequest = rq->bRequest;
#endif
            len = USB_NO_MSG; /* hand over to usbFunctionRead() / usbFunctionWrite() */
        }

    }else if(rq->bRequest == USBASP_FUNC_DISCONNECT){
#if BOOTLOADER_CAN_EXIT
      stayInLoader &= (0xfe);
  #if EXIT_AFTER_UPLOAD
      requestExit = 1;
    #if I2C_LCD
      LCD_setCursor(0, 1);
      LCD_writeStr("reset... ");
    #endif
  #else
    #if I2C_LCD
      LCD_setCursor(0, 1);
      LCD_writeStr("         ");
    #endif
  #endif
#endif
    }else{
        /* ignore: others, but could be USBASP_FUNC_CONNECT */
#if BOOTLOADER_CAN_EXIT
	stayInLoader |= (0x01);
#endif
    }
    return len;
}

uchar usbFunctionWrite(uchar *data, uchar len)
{
uchar   i,isLast;

    DBG1(0x31, (void *)&currentAddress.l, 4);
    if(len > bytesRemaining)
        len = bytesRemaining;
    bytesRemaining -= len;
    isLast = bytesRemaining == 0;
    for(i = 0; i < len;) {
      if(currentRequest >= USBASP_FUNC_READEEPROM){
	eeprom_write_byte((void *)(currentAddress.w[0]++), *data++);
	i++;
      } else {
#if HAVE_BLB11_SOFTW_LOCKBIT
	if (CURRENT_ADDRESS >= (addr_t)(BOOTLOADER_PAGEADDR)) {
	  return 1;
	}
#endif
	i += 2;
	DBG1(0x32, 0, 0);
	cli();
	boot_page_fill(CURRENT_ADDRESS, *(short *)data);
	sei();
	CURRENT_ADDRESS += 2;
	data += 2;
	/* write page when we cross page boundary or we have the last partial page */
	if((currentAddress.w[0] & (SPM_PAGESIZE - 1)) == 0 || (isLast && i >= len && isLastPage)){
#if (!HAVE_CHIP_ERASE) || (HAVE_ONDEMAND_PAGEERASE)
	    DBG1(0x33, 0, 0);
#   ifndef NO_FLASH_WRITE
	    cli();
	    boot_page_erase(CURRENT_ADDRESS - 2);   /* erase page */
	    sei();
	    boot_spm_busy_wait();                   /* wait until page is erased */
#   endif
#endif
	    DBG1(0x34, 0, 0);
#ifndef NO_FLASH_WRITE
	    cli();
	    boot_page_write(CURRENT_ADDRESS - 2);
	    sei();
	    boot_spm_busy_wait();
	    cli();
	    boot_rww_enable();
	    sei();
#endif
	}
        }
        DBG1(0x35, (void *)&currentAddress.l, 4);
    }
    return isLast;
}

uchar usbFunctionRead(uchar *data, uchar len)
{
uchar   i;

    if(len > bytesRemaining)
        len = bytesRemaining;
    bytesRemaining -= len;
    for(i = 0; i < len; i++){
        if(currentRequest >= USBASP_FUNC_READEEPROM){
            *data = eeprom_read_byte((void *)currentAddress.w[0]);
        }else{
#if ((FLASHEND) > 65535)
            *data = pgm_read_byte_far(CURRENT_ADDRESS);
#else
            *data = pgm_read_byte(CURRENT_ADDRESS);
#endif
        }
        data++;
        CURRENT_ADDRESS++;
    }
    return len;
}

/* ------------------------------------------------------------------------ */

static void initForUsbConnectivity(void)
{
#if HAVE_UNPRECISEWAIT
    /* (0.25s*F_CPU)/(4 cycles per loop) ~ (65536*waitloopcnt)
     * F_CPU/(16*65536) ~ waitloopcnt
     * F_CPU / 1048576 ~ waitloopcnt
     */
    uint8_t waitloopcnt = 1 + (F_CPU/1048576);
#endif
    usbInit();
    /* enforce USB re-enumerate: */
    usbDeviceDisconnect();  /* do this while interrupts are disabled */
#if HAVE_UNPRECISEWAIT
    asm volatile (
      /*we really don't care what value Z has...
       * ...if we loop 65536/F_CPU more or less...
       * ...unimportant - just save some opcodes
       */
"initForUsbConnectivity_sleeploop:			\n\t"
      "sbiw	r30,	1				\n\t"
      "sbci	%0,	0				\n\t"
      "brne	initForUsbConnectivity_sleeploop	\n\t"
      : "+d" (waitloopcnt)
      :
      : "r30","r31"
    );
#else
    _delay_ms(260);         /* fake USB disconnect for > 250 ms */
#endif
    usbDeviceConnect();
    sei();
}

int __attribute__((__noreturn__)) main(void)
{
    /* initialize  */
    bootLoaderInit();
    wdt_reset();

    if(!(MCUCSR & (1 << EXTRF))){   /* If this was not an external reset, ignore */
        leaveBootloader();
    }

    odDebugInit();
    DBG1(0x00, 0, 0);
#ifndef NO_FLASH_WRITE
    GICR = (1 << IVCE);  /* enable change of interrupt vectors */
    GICR = (1 << IVSEL); /* move interrupts to boot flash section */
#endif
    if(bootLoaderCondition()){
#if NEED_WATCHDOG
#	if (defined(MCUSR) && defined(WDRF))
	/* 
	 * Fix issue 6: (special thanks to coldtobi)
	 * 
	 * The WDRF bit in the MCUSR needs to be cleared first,
	 * otherwise it is not possible to disable the watchdog
	 */
	MCUSR &= ~(_BV(WDRF));
#	endif
	wdt_disable();    /* main app may have enabled watchdog */
#endif
	MCUCSR = 0;       /* clear all reset flags for next time */
#if I2C_LCD
	LCD_init();
	LCD_setCursor(0, 0);
	LCD_writeStr("Bootloader");
#endif
        initForUsbConnectivity();
        do{
            usbPoll();
#if BOOTLOADER_CAN_EXIT
	if (stayInLoader >= 0x10) {
	  if (!bootLoaderCondition()) {
	    stayInLoader-=0x10;
	  } 
	} else {
	  if (bootLoaderCondition()) {
	    if (stayInLoader > 1) stayInLoader-=2;
	  }
	}
#endif
#if BOOTLOADER_CAN_EXIT
  #if EXIT_AFTER_UPLOAD
          if (requestExit) {
            _delay_ms(500);
            break;
          }
  #endif
        }while (stayInLoader);	/* main event loop, if BOOTLOADER_CAN_EXIT*/
#else
        }while (1);  		/* main event loop */
#endif
    }
    leaveBootloader();
}

/* ------------------------------------------------------------------------ */
