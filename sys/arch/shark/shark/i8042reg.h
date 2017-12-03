/*      $NetBSD: i8042reg.h,v 1.5.22.1 2017/12/03 11:36:43 jdolecek Exp $     */

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

#ifndef _I8042_H
#define _I8042_H

/* Get the standard physical keyboard definitions
*/
#include <shark/shark/kbdreg.h>

/* 8042 Status register location and bit definitions
*/
#define KBSTATP         0x64    /* kbd controller status port (I) */
#define KBSTATPO        0x04    /* kbd controller status port offset */
                                /* from base of 8042 registers       */
#define  KBS_DIB        0x01    /* kbd data in buffer */
#define  KBS_IBF        0x02    /* kbd input buffer low */
#define  KBS_WARM       0x04    /* kbd input buffer low */
#define  KBS_OCMD       0x08    /* kbd output buffer has command */
#define  KBS_NOSEC      0x10    /* kbd security lock not engaged */
#define  KBS_TERR       0x20    /* kbd transmission error. Note that */
                                /* some firmware uses this bit to    */
                                /* indicate data in the input buffer */
                                /* originated from the Auxiliary     */
                                /* device such as the mouse.         */
#define  KBS_RERR       0x40    /* kbd receive error */
#define  KBS_PERR       0x80    /* kbd parity error */

/* 8042 Command register location and command definitions
*/
#define KBCMDP          0x64    /* kbd controller port (O) */
#define KBCMDPO         0x04    /* kbd controller port offset from  */
                                /* base of 8042 registers           */
#define  KBC_RAMREAD    0x20    /* read from RAM */
#define  KBC_RAMWRITE   0x60    /* write to RAM */
#define  KBC_AUXDISABLE 0xa7    /* disable auxiliary port */
#define  KBC_AUXENABLE  0xa8    /* enable auxiliary port */
#define  KBC_AUXTEST    0xa9    /* test auxiliary port */
#define  KBC_KBDECHO    0xd2    /* echo to keyboard port */
#define  KBC_AUXECHO    0xd3    /* echo to auxiliary port */
#define  KBC_AUXWRITE   0xd4    /* write to auxiliary port */
#define  KBC_SELFTEST   0xaa    /* start self-test */
#define  KBC_KBDTEST    0xab    /* test keyboard port */
#define  KBC_KBDDISABLE 0xad    /* disable keyboard port */
#define  KBC_KBDENABLE  0xae    /* enable keyboard port */
#define  KBC_PULSE0     0xfe    /* pulse output bit 0 */
#define  KBC_PULSE1     0xfd    /* pulse output bit 1 */
#define  KBC_PULSE2     0xfb    /* pulse output bit 2 */
#define  KBC_PULSE3     0xf7    /* pulse output bit 3 */


/* 8042 Command responses.
*/
#define KBCR_AUXTEST_OK  0x00   /* Test on Aux device successful */
#define KBCR_KBDTEST_OK  0x00   /* Test on keyboard device successful */
#define KBCR_SELFTEST_OK 0x55   /* Controller self test success  */
 
/* Data input and output registers 
*/
#define KBDATAP         0x60    /* kbd data port (I) */
#define KBDATAPO        0x00    /* kbd data port offset(I) */
#define KBOUTP          0x60    /* kbd data port (O) */
#define KBOUTPO         0x00    /* kbd data port offset (O)*/

/* Modify Controller Command Byte (CCB) definitions
*/
#define K_RDCMDBYTE     0x20
#define K_LDCMDBYTE     0x60

#define KC8_TRANS       0x40    /* convert to old scan codes */
#define KC8_MDISABLE    0x20    /* disable mouse */
#define KC8_KDISABLE    0x10    /* disable keyboard */
#define KC8_IGNSEC      0x08    /* ignore security lock */
#define KC8_CPU         0x04    /* exit from protected mode reset */
#define KC8_MENABLE     0x02    /* enable mouse interrupt */
#define KC8_KENABLE     0x01    /* enable keyboard interrupt */
#define CMDBYTE         (KC8_TRANS|KC8_CPU|KC8_MENABLE|KC8_KENABLE)
#define NOAUX_CMDBYTE   (KC8_TRANS|KC8_MDISABLE|KC8_CPU|KC8_KENABLE)

/************************************************************************
** Macros for controlling the 8042 microprocessor 
*************************************************************************/

#define I8042_NPORTS     8
#define I8042_AUX_DATA   KBS_TERR|KBS_DIB /* Wait for input from mouse  */
#define I8042_KBD_DATA   KBS_DIB         /* Wait for input from keyboard*/
#define I8042_ANY_DATA   0xff            /* Wait for any input          */

#define I8042_CMD         0     /* Command is for keyboard controller   */
#define I8042_AUX_CMD     1     /* Command is for auxiliary port        */
#define I8042_KBD_CMD     2     /* Write to keyboard device data area   */
#define I8042_WRITE_CCB   3     /* Write Controller Command Byte        */


#define I8042_CHECK_RESPONSE  1 /* Check the command response           */
#define I8042_NO_RESPONSE     0 /* No response expected from command    */

#define I8042_DELAY  delay(2)   /* delay for waiting for register update*/
#define I8042_WAIT_THRESHOLD 500000 /* Maximum times to loop waiting for*/
                                    /* a change in status. Currently the*/
                                    /* value for this is determined by  */
                                    /* the time required for the        */
                                    /* keyboard to perform a reset.     */
#define I8042_RETRIES   3      /* Number of times to retry nak'd cmds   */


/*
** Forward routine declarations
*/
extern void i8042_flush(bus_space_tag_t,
                                   bus_space_handle_t);
extern int  i8042_cmd(bus_space_tag_t,
                                   bus_space_handle_t,
                                   u_char,
                                   u_char,
                                   u_char,
                                   u_char);
extern int  i8042_wait_output(bus_space_tag_t,
                                   bus_space_handle_t);
extern int  i8042_wait_input(bus_space_tag_t,
                                   bus_space_handle_t,
                                   u_char);

/* Macro to map bus space for the 8042 device.
** Returns 0 on success.
*/
#define I8042_MAP(iot, iobase, ioh) \
    (bus_space_map((iot), (iobase), I8042_NPORTS, 0, &(ioh))) 

/* Macro to unmap bus space for the 8042 device.
*/
#define I8042_UNMAP(iot, ioh) \
    (bus_space_unmap((iot), (ioh), I8042_NPORTS))

/* Macro to wait and retrieve data from the Auxiliary device.  
** NOTE: 
**   We always check the status before reading the data port because some 8042
**   firmware seems to update the status and data AFTER the interrupt has
**   been generated.  It is nasty because of the number of ISA access it 
**   requires but then the alternative is failed interrupts. Also very 
**   important is that there needs to be a delay after reading the data
**   since this causes the interrupt line to get cleared but because the 8042
**   is a slow processor we could return out of our interrupt handler and 
**   bounce straight back in again with no data to read.
*/
#define I8042_GETAUX_DATA(iot, ioh, status, value) \
{ \
   if (((status) = i8042_wait_input((iot), (ioh), I8042_AUX_DATA)) != 0) \
   { \
        (value) = bus_space_read_1(iot, ioh, KBOUTPO); \
        I8042_DELAY; \
   } \
}

/* Macro to wait and retrieve data from the Keyboard device.  
** NOTE: 
**   We always check the status before reading the data port because some 8042
**   firmware seems to update the status and data AFTER the interrupt has
**   been generated.  It is nasty because of the number of ISA access it 
**   requires but then the alternative is failed interrupts. Also very 
**   important is that there needs to be a delay after reading the data
**   since this causes the interrupt line to get cleared but because the 8042
**   is a slow processor we could return out of our interrupt handler and 
**   bounce straight back in again with no data to read.
*/
#define I8042_GETKBD_DATA(iot, ioh, status, value) \
{ \
   if (((status) = i8042_wait_input((iot), (ioh), I8042_KBD_DATA)) != 0) \
   { \
        (value) = bus_space_read_1(iot, ioh, KBOUTPO); \
	I8042_DELAY; \
   } \
}

/* Macro to test whether the controller is already initialised by checking
** the system bit in the status register.
*/
#define I8042_WARM(iot, ioh)   \
        ( bus_space_read_1((iot), (ioh), KBSTATPO ) & KBS_WARM )  

/* Macro to run self test on the 8042 controller and indicate success or
** failure.
*/
#define I8042_SELFTEST(iot, ioh)  \
   ( i8042_cmd((iot), (ioh), I8042_CMD, I8042_CHECK_RESPONSE, \
                   KBCR_SELFTEST_OK, KBC_SELFTEST) )

/* Macro to run keyboard diagnostic test and indicate success or failure
*/
#define I8042_KBDTEST(iot, ioh)  \
   ( i8042_cmd((iot), (ioh), I8042_CMD, I8042_CHECK_RESPONSE, \
                   KBCR_KBDTEST_OK, KBC_KBDTEST) )

/* Macro to run Auxiliary device diagnostic test and indicate success or
** failure.
*/
#define I8042_AUXTEST(iot, ioh)  \
   ( i8042_cmd((iot), (ioh), I8042_CMD, I8042_CHECK_RESPONSE, \
                   KBCR_AUXTEST_OK, KBC_AUXTEST) )

/* Macro to write the 8042 Controller Command byte
*/
#define I8042_WRITECCB(iot, ioh, value)  \
   ( i8042_cmd((iot), (ioh), I8042_WRITE_CCB, I8042_NO_RESPONSE, \
                   0, (value)) )

/* Macro to read the 8042 Controller Command Byte
*/
#define I8042_READCCB(iot, ioh, status, value)  \
{ \
    if ( ((status) = i8042_cmd((iot), (ioh), I8042_CMD, \
                                I8042_NO_RESPONSE, 0, K_RDCMDBYTE)) != 0 ) \
    { \
        if (((status) = i8042_wait_input((iot),(ioh),I8042_ANY_DATA)) != 0) \
        { \
             (value) = bus_space_read_1((iot), (ioh), KBDATAPO); \
        } \
    } \
}      

/* Macro to disable the Auxiliary devices clock line
*/
#define I8042_AUXDISABLE(iot, ioh)  \
   ( i8042_cmd((iot), (ioh), I8042_CMD, I8042_NO_RESPONSE, \
                   0, KBC_AUXDISABLE) )

/* Macro to enable the Auxiliary device clock line.
*/
#define I8042_AUXENABLE(iot, ioh)  \
   ( i8042_cmd((iot), (ioh), I8042_CMD, I8042_CHECK_RESPONSE, \
                   KBR_ACK, KBC_AUXENABLE) )

/* Macro to perform reset on Keyboard device hanging off the 8042.
*/
#define I8042_KBDRESET(iot, ioh, status)  \
{ \
   if ( ((status) = i8042_cmd((iot), (ioh), I8042_KBD_CMD, \
                                 I8042_CHECK_RESPONSE, \
                                 KBR_ACK, KBC_RESET)) != 0 ) \
   { \
        if (((status) = i8042_wait_input((iot),(ioh),I8042_ANY_DATA)) != 0) \
        { \
	     (status) = bus_space_read_1((iot), (ioh), KBDATAPO); \
	     if ((status) != KBR_RSTDONE) \
             { \
		   printf("I8042_KBDRESET: bad keyboard reset " \
			  "response of 0x%x\n", \
			  status); \
                   (status) = 0; \
             } \
        } \
   } \
} 

#endif








