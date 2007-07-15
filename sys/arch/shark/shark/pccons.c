/*      $NetBSD: pccons.c,v 1.30.2.2 2007/07/15 13:16:58 ad Exp $       */

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

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)pccons.c    5.11 (Berkeley) 5/21/91
 */

/*-
 * Copyright (c) 1993, 1994, 1995 Charles M. Hannum.  All rights reserved.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)pccons.c    5.11 (Berkeley) 5/21/91
 */


/*
**++
** 
**  FACILITY:
**
**    Driver for keyboard and display for PC-style console device.
**
**  ABSTRACT:
**
**    The driver has been cleaned up a little as part of the StrongARM 
**    porting work.  The main modifications have been to change the 
**    driver to use the bus_space_ macros, re-organise the sget and sput
**    mechanisms and use a more robust set of i8042 keybord controller
**    routines which are now external to this module and also used by the
**    opms mouse driver.
**
**  AUTHORS:
**
**    Unknown.  Modified by Patrick Crilly & John Court, 
**    Digital Equipment Corporation.
**
**  CREATION DATE:
**
**    Unknown
**
**--
*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pccons.c,v 1.30.2.2 2007/07/15 13:16:58 ad Exp $");

#include "opt_ddb.h"
#include "opt_xserver.h"

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/select.h>
#include <sys/tty.h>
#include <sys/fcntl.h>
#include <sys/uio.h>
#include <sys/callout.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/kauth.h>
#include <machine/kerndebug.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/irqhandler.h>
#include <machine/pio.h>

#include <machine/pccons.h>
#include <dev/ic/pcdisplay.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <shark/shark/i8042reg.h>
#include <shark/shark/ns87307reg.h>

#ifdef	OFW
#include <dev/ofw/openfirm.h>
#endif

#ifdef SHARK
#include <shark/shark/sequoia.h>
#include <sys/malloc.h>
#include <sys/reboot.h>
#include <machine/devmap.h>
#include <sys/msgbuf.h>
#include <machine/ofw.h>
extern int msgbufmapped;
#endif

/*
** Macro definitions
*/

/*
** If an address for the keyboard console
** hasn't been defined then supply a default
** one.
*/
#ifndef CONKBDADDR
#define CONKBDADDR  0x60
#endif

#define XFREE86_BUG_COMPAT

#ifndef BEEP_FREQ
#define BEEP_FREQ 1500
#endif
#ifndef BEEP_TIME
#define BEEP_TIME (hz/5)
#endif

#define PCBURST 128

/*
 * Non-US keyboards definition
 */
#if defined(FRENCH_KBD) || defined(GERMAN_KBD) || defined(NORWEGIAN_KBD) || defined(FINNISH_KBD)
# define NONUS_KBD
# define NUMERIC_SLASH_FIX
#endif

/* Macro to extract the minor device number from the device identifier 
*/
#define PCUNIT(x)       (minor(x))

#define COL             80
#define ROW             25
#define CHR             2

#define sysbeep(freq, time)     /* not yet implemented */

#define fillw(pattern, address, length)\
{\
    u_short p;\
    u_short *a;\
    int i;\
    for (p = (pattern), a = (address), i = 0; i < (length); a++, i++)\
        *a = p;\
}

/*
 * DANGER WIL ROBINSON -- the values of SCROLL, NUM, CAPS, and ALT are
 * important.
 */
#define SCROLL          0x0001  /* stop output */
#define NUM             0x0002  /* numeric shift  cursors vs. numeric */
#define CAPS            0x0004  /* caps shift -- swaps case of letter */
#define SHIFT           0x0008  /* keyboard shift */
#define CTL             0x0010  /* control shift  -- allows ctl function */
#define ASCII           0x0020  /* ascii code for this key */
#define ALT             0x0080  /* alternate shift -- alternate chars */
#define FUNC            0x0100  /* function key */
#define KP              0x0200  /* Keypad keys */
#define NONE            0x0400  /* no function */
#ifdef NONUS_KBD
#define ALTGR           0x0040  /* Alt graphic */
#endif
#define BREAKBIT        0x80    /* This bit used in XT scancodes to       */
                                /* indicate the release of key.           */ 

/*
** Softc structure for the pc device
*/
struct pc_softc 
{
    struct device      sc_dev;
#define SC_ASYNC_SCHEDULED   0x01 /* An update of keyboard and screen is    */
                                  /* already scheduled.                     */
#define SC_XMODE             0x02 /* We are operating with X above us.      */
#define SC_POLLING           0x04 /* We are polling instead of using        */
                                  /* keyboard interrupts.                   */
    u_char             sc_flags;  /* Values defined above.                  */
    struct tty         *sc_tty;
    struct
    {
	void               *sc_ih;
	u_char             sc_shift_state; /* State of shifting keys.       */
	u_char             sc_new_lock_state;
	u_char             sc_old_lock_state;
	u_char             sc_new_typematic_rate;
	u_char             sc_old_typematic_rate;
	bus_space_tag_t    sc_iot;           /* Kbd IO ports tag        */
	bus_space_handle_t sc_ioh;           /* Kbd IO ports handle     */
    } kbd;     
    struct  
    {
	int                cx, cy;           /* escape parameters       */
	int                row, col;         /* current cursor position */
	int                nrow, ncol, nchr; /* current screen geometry */
	u_char             state;            /* parser state            */
#define VSS_ESCAPE      1
#define VSS_EBRACE      2
#define VSS_EPARAM      3
	char               so;               /* in standout mode?        */
	char               color;            /* color or mono display    */
	char               at;               /* normal attributes        */
	char               so_at;            /* standout attributes      */
    } vs;
};

static callout_t async_update_ch;

/*
** Forward routine declarations
*/
int                    pcprobe             __P((struct device *, 
                                                struct cfdata *, 
                                                void *));
void                   pcattach            __P((struct device *,
                                                struct device *, 
                                                void *));
int                    pcintr              __P((void *));
char                   *sget               __P((struct pc_softc *));
void                   sput                __P((struct pc_softc *,
                                                const u_char *, 
                                                int,
                                                u_char));
void                   pcstart             __P((struct tty *));
int                    pcparam             __P((struct tty *, 
                                                struct termios *));
void                   set_cursor_shape    __P((struct pc_softc *));
#ifdef XFREE86_BUG_COMPAT
void                   get_cursor_shape    __P((struct pc_softc *));
#endif
void                   do_async_update     __P((void *));
void                   async_update        __P((struct pc_softc *,
                                                u_char));
void                   pccnprobe           __P((struct consdev *));
void                   pccninit            __P((struct consdev *));
void                   pccnputc            __P((dev_t, char));
int                    pccngetc            __P((dev_t));
void                   pccnpollc           __P((dev_t, int));

char *xinterpret(struct pc_softc *, u_char);

#ifdef SHARK
static void            force_vga_mode      __P((void));
int get_shark_screen_ihandle(void);
void shark_screen_cleanup(int);

/*
** Definitions needed by getDisplayInfo
*/
struct regspec
{
    u_int bustype;
    u_int addr;
    u_int size;
};

struct display_info
{
    int           init;
    paddr_t       paddr;
    int           linebytes;
    int           depth;
    int           height;
    int           width;
    char          *charSet;
    int           charSetLen;
    paddr_t       ioBase;
    int           ioLen;
};

static void            getDisplayInfo      __P((struct display_info *));

static struct display_info display;
#define displayInfo(property)  (display.init? display.property: -1)


#ifdef X_CGA_BUG
static void            cga_save_restore    __P((int));
/*
** Definitions needed by cga_save_restore
*/
#define CGA_SAVE 0
#define CGA_RESTORE 1
#define TEXT_LENGTH 16382
#define FONT_LENGTH 8192
#define AR_INDEX 0x3C0
#define AR_READ 0x3C1
#define GR_INDEX 0x3CE
#define GR_DATA 0x3CF
#define SR_INDEX 0x3C4
#define SR_DATA 0x3C5
#endif
#endif

/* 
** Global variables 
*/

/*
** Data structures required by config
*/
CFATTACH_DECL(pc, sizeof(struct pc_softc),
    pcprobe, pcattach, NULL, NULL);

extern struct cfdriver pc_cd;

dev_type_open(pcopen);
dev_type_close(pcclose);
dev_type_read(pcread);
dev_type_write(pcwrite);
dev_type_ioctl(pcioctl);
dev_type_tty(pctty);
dev_type_poll(pcpoll);
dev_type_mmap(pcmmap);

const struct cdevsw pc_cdevsw = {
	pcopen, pcclose, pcread, pcwrite, pcioctl,
	nostop, pctty, pcpoll, pcmmap, ttykqfilter, D_TTY
};

static unsigned int   addr_6845   = MONO_BASE;

/* Define globals used when acting as console at boot time. 
*/
static struct pc_softc bootSoftc;
static u_char          attachCompleted = false;
static u_char          actingConsole   = false;

/* Our default debug settings, when debug is compiled in via config option 
** KERNEL_DEBUG that is.
*/
int                    pcdebug         = KERN_DEBUG_ERROR | KERN_DEBUG_WARNING;

static u_short        *Crtat;          /* pointer to backing store        */
static u_short        *crtat;          /* pointer to current char         */
static u_short cursor_shape     = 0xffff; /* don't update until set by user */
static u_short old_cursor_shape = 0xffff;

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     kbd_init
**
**    This function initialises the keyboard controller and keyboard for use.
**    If the keyboard controller hasn't been reset then self-test the
**    controller (this gets the controller going) and self-test the keyboard.
**    Enable and reset the keyboard and then set the scan table in the
**    keyboard controller. 
**
**  FORMAL PARAMETERS:
**
**     iot    I/O tag for the mapped register space  
**     ioh    I/O handle for the mapped register space
**
**  IMPLICIT INPUTS:
**
**     None.
**
**  IMPLICIT OUTPUTS:
**
**     none.
**
**  FUNCTION VALUE:
**
**   1  - Keyboard and controller successfully initialised
**   0  - Failed to initialise keyboard or controller
*/
static int 
kbd_init(bus_space_tag_t     iot, 
         bus_space_handle_t  ioh)
{
    int status = 1;
    u_char value = 0;	/* XXX */
    
    /* Perfrom a controller reset if this is a cold reset happening 
    */
    if (!I8042_WARM(iot, ioh))
    {
        /*
        ** Perform an internal 8042 controller test. 
        */
        if (I8042_SELFTEST(iot, ioh))
        {
            /*
            ** Perform a keyboard interface test.  This causes the controller
            ** to test the keyboard clock and data lines.  
            */
            if (!I8042_KBDTEST(iot, ioh))
            {
                status = 0;
                KERN_DEBUG( pcdebug, KERN_DEBUG_ERROR, 
                           ("kbd_int: keyboard failed self test\n"));
            }
        }
        else    
        {
            status = 0;
            KERN_DEBUG(pcdebug, KERN_DEBUG_ERROR, 
                       ("kbd_init: 8042 controller failed self test.\n"));
        }
    } /* End if we need to do cold start self tests */
    /* Enable keyboard but not the Auxiliary device in the 8042 controller.
    */
    if (status && I8042_WRITECCB(iot, ioh, NOAUX_CMDBYTE))
    {
        /* Flush any garbage. 
        */
        i8042_flush(iot, ioh);
        /* Reset the keyboard. 
        */
        I8042_KBDRESET(iot, ioh, status);
        if (status)
        {
            /*
            ** Some keyboards seem to leave a second ack byte after the reset.
            ** This is kind of stupid, but we account for them anyway by just
            ** flushing the buffer.
            */
            i8042_flush(iot, ioh);
            /* Send the keyboard physical device an enable command. 
            */
            if (i8042_cmd(iot, ioh, I8042_KBD_CMD, 
                              I8042_CHECK_RESPONSE, KBR_ACK, KBC_ENABLE))
            {
                /*
                ** Some keyboard/8042 combinations do not seem to work if
                ** the keyboard is set to table 1; in fact, it would appear 
                ** that some keyboards just ignore the command altogether.  
                ** So by default, we use the AT scan codes and have the 
                ** 8042 translate them.  Unfortunately, this is known to not 
                ** work on some PS/2 machines.  We try desparately to deal
                ** with this by checking the (lack of a) translate bit 
                ** in the 8042 and attempting to set the keyboard to XT mode.
                ** If this all fails, well, tough luck.
                **
                ** XXX It would perhaps be a better choice to just use AT 
                ** scan codes and not bother with this.
                */
                I8042_READCCB(iot, ioh, status, value);
                if (status && (value & KC8_TRANS))
                {
                    /* The 8042 is translating for us; use AT codes. 
                    */
                    if (!i8042_cmd(iot, ioh, I8042_KBD_CMD, 
                                       I8042_CHECK_RESPONSE, KBR_ACK, 
                                       KBC_SETTABLE) 
                        || !i8042_cmd(iot, ioh, I8042_KBD_CMD, 
                                          I8042_CHECK_RESPONSE, KBR_ACK, 
                                          2)) 
                    {
                        status = 0;
                        KERN_DEBUG( pcdebug, KERN_DEBUG_ERROR,
                                   ("kbdinit: failed to scan table\n"));
                    }
                } 
                else 
                {
                    /* Stupid 8042; set keyboard to XT codes. 
                    */
                    if ( !i8042_cmd(iot, ioh, I8042_KBD_CMD,
                                        I8042_CHECK_RESPONSE, KBR_ACK, 
                                        KBC_SETTABLE) 
                        || !i8042_cmd(iot, ioh, I8042_KBD_CMD,
                                          I8042_CHECK_RESPONSE, KBR_ACK,
                                          1) ) 
                    {
                        status = 0;
                        KERN_DEBUG( pcdebug, KERN_DEBUG_ERROR,
                                   ("kbdinit: failed to scan table\n"));
                    }
                }
            } /* End if able to send keyboard physical device an enable */
            else
            {
                KERN_DEBUG( pcdebug, KERN_DEBUG_ERROR,
                           ("kbdinit: failed enable of physical keyboard\n"));
            }
        } /* End if reset of physical keyboard successful */
        else
        {
            KERN_DEBUG( pcdebug, KERN_DEBUG_ERROR,
                       ("kdbinit: keyboard failed reset. Status = 0x%x\n",
			status));
        }
    } /* End if able to write to CCB and cold self tests worked */
    else
    {
        KERN_DEBUG( pcdebug, KERN_DEBUG_ERROR, 
                   ("kdb_init: keyboard failed enable or keyboard self "
                    " tests failed. Status = 0x%x\n", status));
        status = 0;
    }

    return (status);
} /* End kbd_init() */


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     set_cursor_shape
**
**     The function name says it all.
**
**  FORMAL PARAMETERS:
**
**     None.
**
**  IMPLICIT INPUTS:
**
**     addr_6845    -  Base address of the video registers 
**
**  IMPLICIT OUTPUTS:
**
**     none.
**
**  FUNCTION VALUE:
**
**     none
** 
**
**--
*/
void
set_cursor_shape(struct pc_softc *sc)
{
    register int iobase = addr_6845;

    /* Write to the start of cursor register */ 
    outb(iobase, 10);
    outb(iobase+1, cursor_shape >> 8);

    /* Write to the end of cursor register */ 
    outb(iobase, 11);
    outb(iobase+1, cursor_shape);

    old_cursor_shape = cursor_shape;
} /* End set_cursor_shape() */


#ifdef XSERVER
#ifdef XFREE86_BUG_COMPAT


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     get_cursor_shape
**
**     The function name says it all.
**
**  FORMAL PARAMETERS:
**
**     None.
**
**  IMPLICIT INPUTS:
**
**     addr_6845    -  Base address of the video registers 
**
**  IMPLICIT OUTPUTS:
**
**     none.
**
**  FUNCTION VALUE:
**
**     none
**
**--
*/
void
get_cursor_shape(struct pc_softc *sc)
{
    register int iobase = addr_6845;
    
    /* Read from the start of cursor register */ 
    outb(iobase, 10);
    cursor_shape = inb(iobase+1) << 8;

    /* Read from the end of cursor register */ 
    outb(iobase, 11);
    cursor_shape |= inb(iobase+1);
    
    /*
     * real 6845's, as found on, MDA, Hercules or CGA cards, do
     * not support reading the cursor shape registers. the 6845
     * tri-states it's data bus. This is _normally_ read by the
     * CPU as either 0x00 or 0xff.. in which case we just use
     * a line cursor.
     */
    if (cursor_shape == 0x0000 || cursor_shape == 0xffff)
    {
        cursor_shape = 0x0b10;
    }
    else
    {
        cursor_shape &= 0x1f1f;
    }
} /* End get_cursor_shape() */
#endif /* XFREE86_BUG_COMPAT */
#endif /* XSERVER */



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     do_aysnc_update
**
**    This function is called to update "output" of the console device.
**    If either the leds or the typematic rate of the keyboard have 
**    been modified they are updated.  If the cursor has moved
**    its position on the screen it is updated.   
**
**  FORMAL PARAMETERS:
**
**     sc    -  Pointer to the softc structure.
**
**  IMPLICIT INPUTS:
**
**     none
**
**  IMPLICIT OUTPUTS:
**
**     none.
**
**  FUNCTION VALUE:
**
**   none.
** 
**--
*/
void
do_async_update(void *arg)
{
    struct pc_softc *sc = arg;
    u_char poll         = (sc->sc_flags & SC_POLLING) ? 
                          I8042_CHECK_RESPONSE : I8042_NO_RESPONSE;
    int pos;
    static int old_pos = -1;
    
    /* Turn off the async scheduled flag as we are it
    */
    sc->sc_flags &= ~SC_ASYNC_SCHEDULED;
    /* Update the leds on the keyboard if they have changed. 
    */
    if (sc->kbd.sc_new_lock_state != sc->kbd.sc_old_lock_state) 
    {
        sc->kbd.sc_old_lock_state = sc->kbd.sc_new_lock_state;
        if (!i8042_cmd(sc->kbd.sc_iot, sc->kbd.sc_ioh, I8042_KBD_CMD, poll,
                           KBR_ACK, KBC_MODEIND) ||
            !i8042_cmd(sc->kbd.sc_iot, sc->kbd.sc_ioh, I8042_KBD_CMD, poll,
                           KBR_ACK, sc->kbd.sc_new_lock_state) ) 
        {
            KERN_DEBUG( pcdebug, KERN_DEBUG_WARNING,
                       ("pc: timeout updating leds\n"));
            (void) i8042_cmd(sc->kbd.sc_iot, sc->kbd.sc_ioh, I8042_KBD_CMD, 
                                 poll, KBR_ACK, KBC_ENABLE);
        }
    }
    /* Update the keyboard typematic rate if it has changed.
    */
    if (sc->kbd.sc_new_typematic_rate != sc->kbd.sc_old_typematic_rate) 
    {
        sc->kbd.sc_old_typematic_rate = sc->kbd.sc_new_typematic_rate;
        if (!i8042_cmd(sc->kbd.sc_iot, sc->kbd.sc_ioh, I8042_KBD_CMD, poll,
                           KBR_ACK, KBC_TYPEMATIC) ||
            !i8042_cmd(sc->kbd.sc_iot, sc->kbd.sc_ioh, I8042_KBD_CMD, poll,
                           KBR_ACK, sc->kbd.sc_new_typematic_rate) ) 
        {
            KERN_DEBUG(pcdebug, KERN_DEBUG_WARNING,
                       ("pc: timeout updating typematic rate\n"));
            (void) i8042_cmd(sc->kbd.sc_iot, sc->kbd.sc_ioh, I8042_KBD_CMD, 
                                 poll, KBR_ACK, KBC_ENABLE);
        }
    }
    
    if ( (sc->sc_flags & SC_XMODE) == 0 )
    {
        /* Update position of cursor on screen 
        */
        pos = crtat - Crtat;
        if (pos != old_pos) 
        {
            register int iobase = addr_6845;
            
            outb(iobase, 14);
            outb(iobase+1, pos >> 8);
            outb(iobase, 15);
            outb(iobase+1, pos);
            old_pos = pos;
        }
        
        if (cursor_shape != old_cursor_shape)
        {
            set_cursor_shape(sc);
        }
    }

    return;
} /* End do_async_update() */



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     aysnc_update
**
**    This function is called to schedule an update of the "output" of
**    the console device.  If we've been called by the kernel or
**    operating in polling mode, dequeue any scheduled update and
**    peform the update by calling do_aysnc_update; otherwise
**    if no update is currently queued, schedule a timeout to 
**    perform one.
**
**  FORMAL PARAMETERS:
**
**     None.
**
**  IMPLICIT INPUTS:
**
**     none.
**
**  IMPLICIT OUTPUTS:
**
**     none.
**
**  FUNCTION VALUE:
**
**     none.
**--
*/
void
async_update(struct pc_softc *sc,
             u_char          nowait)
{
    
    if (nowait || (sc->sc_flags & SC_POLLING)) 
    {
        /* Untimeout any update already scheduled and force an update
        ** right now.
        */ 
        if (sc->sc_flags & SC_ASYNC_SCHEDULED)
        {
	    callout_stop(&async_update_ch);
        }
        do_async_update(sc);
    } 
    else if ((sc->sc_flags & SC_ASYNC_SCHEDULED) == 0)
    {
        /* Schedule an update 
        */
        sc->sc_flags |= SC_ASYNC_SCHEDULED;
	callout_reset(&async_update_ch, 1, do_async_update, sc);
    }

    return;
} /* End async_update() */



/*
**++
**  FUNCTIONAL DESCRIPTION:
**     
**     pcprobe
**
**     This is the probe routine for the console device.   If the
**     console device hasn't already been set up then the register
**     space for the keyboard controller is mapped and the
**     controller initialised.  The isa_atach_args is filled
**     in with the size of keyboard's register space.     
**
**  FORMAL PARAMETERS:
**
**     parent  - pointer to the parent device.
**     match   - not used
**     aux     - pointer to an isa_attach_args structure.
**     
**  IMPLICIT INPUTS:
**
**     actingConsole - indicates whether the pc device is being used as 
**                     the console device.
**
**  IMPLICIT OUTPUTS:
**
**     none.
**
**  FUNCTION VALUE:
**
**     0 - Probe failed to find the requested device.
**     1 - Probe successfully talked to the device. 
**
**  SIDE EFFECTS:
**
**     none.
**--
*/
int
pcprobe(struct device *parent, 
        struct cfdata *match, 
        void          *aux)
{
    struct isa_attach_args    *ia = aux;
    int                       probeOk = 1; /* assume it will succeed */
    int                       iobase;
    bus_space_tag_t           iot;
    bus_space_handle_t        ioh;

    if (ia->ia_nio < 1)
	return (0);
    if (ia->ia_nirq < 1)
	return (0);

    if (actingConsole == false)
    {
        iobase = ia->ia_io[0].ir_addr;
        iot    = ia->ia_iot;

        /* Map register space 
        */
        if ( I8042_MAP(iot, iobase, ioh) == 0 ) 
        {
            /*
            ** Initialise keyboard controller.  We don't fail the 
            ** probe even if the init failed.  This allows the
            ** console device to be used even if a keyboard
            ** isn't connected.
            */
            if (!kbd_init(iot, ioh))
            {
                KERN_DEBUG(pcdebug, KERN_DEBUG_ERROR,
                           ("pcprobe: failed to initialise keyboard\n"));  
		probeOk = 0;
            }
            I8042_UNMAP(iot, ioh); 
        } 
        else
        {
            probeOk = 0;
        }
    } /* end-if keyboard not initialised for console */
    /* 
    ** Fill in the isa structure with the number of 
    ** ports used and mapped memory size.
    */
    if (probeOk) {
	ia->ia_nio = 1;
	ia->ia_io[0].ir_size = I8042_NPORTS;

	ia->ia_nirq = 1;

	ia->ia_niomem = 0;
	ia->ia_ndrq = 0;
    }

    return (probeOk);
} /* End pcprobe() */



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      pcattach
**
**      This is the attach routine for the pc device.  It updates the
**      cursor position, sets up the interrupt routine for the keyboard
**      and the configures any child devices.
**
**  FORMAL PARAMETERS:
**
**      parent - pointer to my parents device structure.
**      self   - pointer to my softc with device structure at front.
**      aux    - pointer to the isa_attach_args structure.
**
**  IMPLICIT INPUTS:
**
**     actingConsole - indicates whether the pc device is being used as 
**                     the console device.
**
**  IMPLICIT OUTPUTS:
**
**      None
**
**  FUNCTION VALUE:
**
**      none.
**
**  SIDE EFFECTS:
**
**      none.
**--
*/
void
pcattach(struct device   *parent, 
         struct device   *self, 
         void            *aux)
{
    struct pc_softc            *sc = (void *)self;
    struct isa_attach_args     *ia = aux;
    int                        iobase;

    /* 
    ** If the keyboard isn't being used as a console device,
    ** map the register space.
    */
    if (actingConsole == false)
    {
        KERN_DEBUG( pcdebug, KERN_DEBUG_INFO,
                   ("\npcattach: mapping io space\n"));
        iobase                              = ia->ia_io[0].ir_addr;
        sc->sc_flags                        = 0x00;
        sc->kbd.sc_shift_state              = 0x00;
        sc->kbd.sc_new_lock_state           = 0x00;
        sc->kbd.sc_old_lock_state           = 0xff;
        sc->kbd.sc_new_typematic_rate       = 0xff;
        sc->kbd.sc_old_typematic_rate       = 0xff;
        sc->kbd.sc_iot                       = ia->ia_iot;
        if (I8042_MAP(sc->kbd.sc_iot,iobase,sc->kbd.sc_ioh) != 0)
        {
            panic("pcattach: kbd io mapping failed");
        }
	bootSoftc.vs.cx  = bootSoftc.vs.cy  = 0;
	bootSoftc.vs.row = bootSoftc.vs.col = 0;
	bootSoftc.vs.nrow                   = 0;
	bootSoftc.vs.ncol                   = 0;
	bootSoftc.vs.nchr                   = 0;
	bootSoftc.vs.state                  = 0;
	bootSoftc.vs.so                     = 0;
	bootSoftc.vs.color                  = 0;
	bootSoftc.vs.at                     = 0;
	bootSoftc.vs.so_at                  = 0;
#ifdef	SHARK
	/*
	 ** Get display information from ofw before leaving linear mode.
	 */
	getDisplayInfo(&display);

	/* Must force hardware into VGA mode before anything else
	** otherwise we may hang trying to do output.
	*/
	force_vga_mode();
#endif
    }
    else
    {
        /* We are the console so better fill in the softc with the values
        ** we have been using in the console up to this point.
        */
        sc->sc_flags                  = bootSoftc.sc_flags;
        sc->kbd.sc_shift_state        = bootSoftc.kbd.sc_shift_state;
        sc->kbd.sc_new_lock_state     = bootSoftc.kbd.sc_new_lock_state;
        sc->kbd.sc_old_lock_state     = bootSoftc.kbd.sc_old_lock_state;
        sc->kbd.sc_new_typematic_rate = bootSoftc.kbd.sc_new_typematic_rate;
        sc->kbd.sc_old_typematic_rate = bootSoftc.kbd.sc_old_typematic_rate;
        sc->kbd.sc_iot                = bootSoftc.kbd.sc_iot;
        sc->kbd.sc_ioh                = bootSoftc.kbd.sc_ioh;
	/* Copy across the video state as well.
	*/
	sc->vs.cx    = bootSoftc.vs.cx; 
	sc->vs.cy    = bootSoftc.vs.cy;
	sc->vs.row   = bootSoftc.vs.row;
        sc->vs.col   = bootSoftc.vs.col;
        sc->vs.nrow  = bootSoftc.vs.nrow;
        sc->vs.ncol  = bootSoftc.vs.ncol;
        sc->vs.nchr  = bootSoftc.vs.nchr;
	sc->vs.state = bootSoftc.vs.state;
	sc->vs.so    = bootSoftc.vs.so;
	sc->vs.color = bootSoftc.vs.color;
        sc->vs.at    = bootSoftc.vs.at;
        sc->vs.so_at = bootSoftc.vs.so_at;
    }

    /* At this point the softc is set up "enough" and want to swap console 
    ** functionality to use that standard interface.
    **
    ** THIS MUST BE DONE BEFORE ANY MORE OUTPUT IS DONE.
    */
    attachCompleted = true;

    /* Update screen 
    */
    printf(": %s\n", sc->vs.color ? "color" : "mono");

    do_async_update(sc);
    /* Set up keyboard controller interrupt 
    */
    sc->kbd.sc_ih = isa_intr_establish(ia->ia_ic, ia->ia_irq[0].ir_irq,
        IST_LEVEL, IPL_TTY, pcintr, sc);
    /* 
    ** Pass child devices our io handle so they can use
    ** the same io space as the keyboard.
    */
#if 0
    ia->ia_delaybah = sc->kbd.sc_ioh;
#else
    ia->ia_aux = (void *)sc->kbd.sc_ioh;
#endif
    /*
    ** Search for and configure any child devices.
    */
    while (config_found(self, ia, NULL) != NULL)        
        /* will break when no more children */ ;
} /* End pcattach() */



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     pcopen
**
**     This routine is to open the console device.  It first makes 
**     sure that the device unit specified is valid and not already 
**     in use.  A tty structure is allocated if one hasn't been
**     already.  It then sets up the tty flags and calls the line
**     disciplines open routine. 
**
**  FORMAL PARAMETERS:
**
**     dev  - Device identifier consisting of major and minor numbers.
**     flag - not used
**     mode - not used.
**     p    - not used.
**
**  IMPLICIT INPUTS:
**
**     none.
**
**  IMPLICIT OUTPUTS:
**
**     none.
**
**  FUNCTION VALUE:
**
**     Returns an errno value on error.  This is either the error 
**     error returned by the line discipline or one of the
**     following errnos.
**
**     ENXIO   - invalid device specified for open.
**     EBUSY   - The unit is already opened for exclusive use and the 
**               current requestor is NOT root.
**
**  SIDE EFFECTS:
**
**     none.
**--
*/
int
pcopen(dev_t       dev, 
       int         flag, 
       int         mode, 
       struct lwp *l)
{
    struct pc_softc *sc;
    int unit = PCUNIT(dev);
    struct tty *tp;
    
    KERN_DEBUG( pcdebug, KERN_DEBUG_INFO, ("pcopen by process %d\n",
                                           l->l_proc->p_pid));
    /*
    ** Sanity check the minor device number we have been instructed
    ** to open and set up our softc structure pointer. 
    */
    if (unit >= pc_cd.cd_ndevs)
    {
        return ENXIO;
    }
    sc = pc_cd.cd_devs[unit];
    if (sc == 0)
    {
        return ENXIO;
    }
    /*
    ** Check if we have a tty structure already and create one if not 
    */
    if (!sc->sc_tty) 
    {
        tp = sc->sc_tty = ttymalloc();
        tty_attach(tp);
    } 
    else
    {
        tp = sc->sc_tty;
    }
    /* 
    ** Initialise the tty structure with a H/W output routine,
    ** H/W parameter modification routine and device identifier.
    */
    tp->t_oproc = pcstart;
    tp->t_param = pcparam;
    tp->t_dev   = dev;

    if (kauth_authorize_device_tty(l->l_cred, KAUTH_DEVICE_TTY_OPEN, tp))
	return (EBUSY);
    
    if ((tp->t_state & TS_ISOPEN) == 0) 
    {
        /* 
        ** Initial open of the device so set up the tty with
        ** default flag settings
        */
        ttychars(tp);
        tp->t_iflag = TTYDEF_IFLAG;
        tp->t_oflag = TTYDEF_OFLAG;
        tp->t_cflag = TTYDEF_CFLAG;
        tp->t_lflag = TTYDEF_LFLAG;
        tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
        pcparam(tp, &tp->t_termios);
        ttsetwater(tp);
    } 
    tp->t_state |= TS_CARR_ON;
    /* 
    ** Invoke the line discipline open routine 
    */
    return ((*tp->t_linesw->l_open)(dev, tp));
} /* End pcopen() */



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     pcclose
**
**     This function is called on the last close for a device unit.
**     It calls the line discipline close to clean up any pending 
**     output and to flush queues.  It then close the tty device.
**
**  FORMAL PARAMETERS:
**
**     dev  - Device identifier consisting of major and minor numbers.
**     flag - Not used.
**     mode - Not used.
**     l    - Not used. 
**
**  IMPLICIT INPUTS:
**
**     none
**
**  IMPLICIT OUTPUTS:
**
**     none
**
**  FUNCTION VALUE:
**
**     0 - Always returns success.
**
**  SIDE EFFECTS:
**
**     none.
**--
*/
int
pcclose(dev_t       dev, 
        int         flag, 
        int         mode, 
        struct lwp *l)
{
    /* 
    ** Set up our pointers to the softc and tty structures.
    **
    ** Note : This all assumes that the system wont call us with 
    **        an invalid (ie. non-existent), device identifier.
    */
    struct pc_softc *sc = pc_cd.cd_devs[PCUNIT(dev)];
    struct tty      *tp = sc->sc_tty;
    /*
    ** Let the line discipline clean up any pending output and
    ** flush queues.
    */
    (*tp->t_linesw->l_close)(tp, flag);
    ttyclose(tp);

    return(0);
} /* End pcclose() */



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      Handles a read operation on the device.  We just
**      locate the tty device associated with the unit specified
**      and pass everything on to the line driver.
**
**  FORMAL PARAMETERS:
**      
**      dev  - Device identifier consisting of major and minor numbers.
**      uio  - Pointer to the user I/O information (ie. read buffer).
**      flag - Information on how the I/O should be done (eg. blocking
**             or non-blocking).
**
**  IMPLICIT INPUTS:
**
**      pc_cd - The console driver global anchor structure containing
**               pointers to all of the softc structures for each unit
**               (amoung other things).
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  FUNCTION VALUE:
**
**      Returns zero on success and an errno on failure.
**
**  SIDE EFFECTS:
**
**      none
**--
*/
int
pcread(dev_t       dev, 
       struct uio  *uio, 
       int         flag)
{
    struct pc_softc *sc = pc_cd.cd_devs[PCUNIT(dev)];
    struct tty      *tp = sc->sc_tty;
    
    return ((*tp->t_linesw->l_read)(tp, uio, flag));
} /* End pcread() */



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      Handles a write operation on the device.  We just
**      locate the tty device associated with the unit specified
**      and pass everything on to the line driver.
**
**  FORMAL PARAMETERS:
**      
**      dev  - Device identifier consisting of major and minor numbers.
**      uio  - Pointer to the user I/O information (ie. read buffer).
**      flag - Information on how the I/O should be done (eg. blocking
**             or non-blocking).
**
**  IMPLICIT INPUTS:
**
**      pc_cd - The console driver global anchor structure containing
**               pointers to all of the softc structures for each unit
**               (amoung other things).
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  FUNCTION VALUE:
**
**      Returns zero on success and an errno on failure.
**
**  SIDE EFFECTS:
**
**      none
**--
*/
int
pcwrite(dev_t      dev, 
        struct uio *uio, 
        int        flag)
{
    struct pc_softc *sc = pc_cd.cd_devs[PCUNIT(dev)];
    struct tty      *tp = sc->sc_tty;
    
    return ((*tp->t_linesw->l_write)(tp, uio, flag));
} /* End pcwrite() */



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      Handles a poll operation on the device.  We just
**      locate the tty device associated with the unit specified
**      and pass everything on to the line driver.
**
**  FORMAL PARAMETERS:
**      
**      dev    - Device identifier consisting of major and minor numbers.
**      events - Events to poll for
**      l      - The lwp performing the poll
**
**  IMPLICIT INPUTS:
**
**      pc_cd - The console driver global anchor structure containing
**               pointers to all of the softc structures for each unit
**               (amoung other things).
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  FUNCTION VALUE:
**
**      Returns zero on success and an errno on failure.
**
**  SIDE EFFECTS:
**
**      none
**--
*/
int
pcpoll(dev_t       dev, 
       int         events,
       struct lwp *l)
{
    struct pc_softc *sc = pc_cd.cd_devs[PCUNIT(dev)];
    struct tty      *tp = sc->sc_tty;
    
    return ((*tp->t_linesw->l_poll)(tp, events, l));
} /* End pcpoll() */



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      Return the tty structure for the given device unit.     
**
**  FORMAL PARAMETERS:
**      
**      dev  - Device identifier consisting of major and minor numbers.
**
**  IMPLICIT INPUTS:
**
**      pc_cd - The console driver global anchor structure containing
**               pointers to all of the softc structures for each unit
**               (amoung other things).
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  FUNCTION VALUE:
**
**      Returns zero on success and an errno on failure.
**
**  SIDE EFFECTS:
**
**      none
**--
*/
struct tty *
pctty(dev_t dev)
{
    struct pc_softc *sc = pc_cd.cd_devs[PCUNIT(dev)];
    struct tty      *tp = sc->sc_tty;
    
    return (tp);
} /* End pctty() */



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     pcintr
**
**     This is the interrupt handler routine for the keyboard controller.
**     If we're in polling mode then just return.  If not then call the 
**     sget routine to read the and convert the character in the keyboard
**     buffer.  The convereted character is then passed to the line 
**     discipline's read interrupt routine. 
**
**  FORMAL PARAMETERS:
**
**      arg - Pointer to our device/softc structure.
**
**  IMPLICIT INPUTS:
**
**      none.
**
**  IMPLICIT OUTPUTS:
**
**      none.
**
**  FUNCTION VALUE:
** 
**      0 - Interrupt not handled by the driver.
**      1 - Interrupt handled by the driver.
**
**  SIDE EFFECTS:
**
**      none.
**--
*/
int
pcintr(void *arg)
{
    struct pc_softc     *sc = arg;
    register struct tty *tp = sc->sc_tty;
    u_char              *cp;
    int                 handledInt;
    
    handledInt = 1;

    if (sc->sc_flags & SC_POLLING)
    {
        KERN_DEBUG( pcdebug, KERN_DEBUG_WARNING, 
                   ("pcintr: received interrupt in polling mode\n"));
    }
    else
    {
        /*
        ** Get the character string for the keyboard event.
        ** The scanned character string is passed to 
        ** the line discipline's received character routine.
        */
        cp = sget(sc);
        if (tp && (tp->t_state & TS_ISOPEN) && cp)
        {
            do
            {
                (*tp->t_linesw->l_rint)(*cp++, tp);
            }
            while (*cp);
        }
    } /* End else we aren't polling at the moment */
    
    return (handledInt);
} /* End pcintr() */



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     This routine is responsible for performing I/O controls on the 
**     console device. We first pass the command to our line discipline to 
**     see if it is one they are intended to handle.  If they "know nothing!!"
**     we pass it on to the generic ttioctl() routine.  If they also deny
**     responsibility, we then check if it's a ioctl we support.
**     The ioctls we support are: 
** 
**     CONSOLE_X_MODE_ON          - X is using the console device 
**     CONSOLE_X_MODE_OFF         - X is releasing the console device
**     CONSOLE_X_BELL             - ring the bell
**     CONSOLE_SET_TYPEMATIC_RATE - set keyboard typematic rate
**     
**  FORMAL PARAMETERS:
**
**     dev - Device identifier consisting of major and minor numbers.
**     cmd - The requested IOCTL command to be performed. This is a 
**           composite integer with the following structure but
**           refer to ttycom.h and ioccom.h for full details :
**
**           Bit Position      { 3322222222221111111111
**                             { 10987654321098765432109876543210 
**           Meaning           | DDDLLLLLLLLLLLLLGGGGGGGGCCCCCCCC
**
**           D - Command direction, in/out/both.
**           L - Command argument length.
**           G - Command group, 't' used for tty.
**           C - Actual command enumeration.
** 
**     data - user data
**     flag - Not used by us but passed to line discipline and ttioctl
**     l    - pointer to lwp structure of user.
**     
**  IMPLICIT INPUTS:
**
**     none.
**
**  IMPLICIT OUTPUTS:
**
**     none.
**
**  FUNCTION VALUE:
**
**     We will return any errors returned by either the line discipline 
**     routine or the ttioctl() routine.  In addition we may directly 
**     generate the following errors :
**
**     EINVAL  - Invalid value for typematic rate specified.
**     ENOTTY  - Unknown ioctl command attempted.    
**
**  SIDE EFFECTS:
**
**     The softc sc_hwflags may be altered which will effect the behaviour of 
**     several other routines when dealing with the hardware. 
**--
*/
int
pcioctl(dev_t       dev, 
        u_long      cmd, 
        void *    data, 
        int         flag, 
        struct lwp *l)
{
    struct pc_softc *sc = pc_cd.cd_devs[PCUNIT(dev)];
    struct tty      *tp = sc->sc_tty;
    int             error;
    
    /* Give the line discipline first crack at doing something with
    ** the requested operation.
    ** Error > 0 means that the operation requested was known by
    ** the line discipline but something went wrong. Error = 0 means the
    ** request was successfully handled by the line discipline so
    ** we don't need to to do anything. Error == EPASSTHROUGH means the line
    ** discipline doesn't handle this sort of operation. 
    */
    error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, l);
    if (error == EPASSTHROUGH)
    {
        /* Try the common tty ioctl routine to see if it recognises the
        ** request.  
        */
        error = ttioctl(tp, cmd, data, flag, l);
        if (error == EPASSTHROUGH)
        {
            /* Ok must be something specific to our device, 
            ** lets start by assuming we will succeed.
            */
            error = 0;
            switch (cmd) 
            {
#ifdef XSERVER
                case CONSOLE_X_MODE_ON:
#ifdef X_CGA_BUG
		    if ((sc->sc_flags & SC_XMODE) == 0 )
		    {
                        cga_save_restore(CGA_SAVE);
		    }
#endif
		    sc->sc_flags |= SC_XMODE;

		    /*
		     * throw away pending data so raw keycodes don't get
		     * mixed with translated characters.
		     */
		    ttyflush(tp, FREAD);
                break;
                
                case CONSOLE_X_MODE_OFF:
#ifdef X_CGA_BUG
		    if (sc->sc_flags & SC_XMODE)
		    {
			cga_save_restore(CGA_RESTORE);
		    }
#endif
		    sc->sc_flags &= ~SC_XMODE;
                    async_update(sc, false);

		    /*
		     * throw away pending data so raw keycodes don't get
		     * mixed with translated characters.
		     */
		    ttyflush(tp, FREAD);
                break;
                
                case CONSOLE_X_BELL:
                    /*
                    ** If set, data is a pointer to a length 2 array of
                    ** integers.  data[0] is the pitch in Hz and data[1]
                    ** is the duration in msec.
                    */
                    if (data)
                    {
                        sysbeep(((int*)data)[0], 
                                (((int*)data)[1] * hz) / 1000);
                    }
                    else
                    {
                        sysbeep(BEEP_FREQ, BEEP_TIME);
                    }
                break;

#ifdef SHARK
		case CONSOLE_X_TV_ON:
		    /* 
		    ** Switch on TV output, data indicates mode,
		    ** but we are currently hardwired to NTSC, so ignore it.
		    */  
		    consXTvOn();
		break;
		    
		case CONSOLE_X_TV_OFF:
		    /* 
		     ** Switch off TV output.
		     */  
		    consXTvOff();
		break;
		    
#endif /* SHARK */		
#endif /* XSERVER */
    
                case CONSOLE_SET_TYPEMATIC_RATE: 
                {
                    u_char      rate;
                
                    if (data)
                    {
                        rate = *((u_char *)data);
                        /*
                        ** Check that it isn't too big (which would cause 
                        ** it to be confused with a command).
                        */
                        if (rate & 0x80)
                        {
                            error = EINVAL;
                        }
                        else
                        {
                            /* Update rate in keyboard */
                            sc->kbd.sc_new_typematic_rate = rate;
                            async_update(sc, false);
                        }        
                    }
                    else
                    {
                        error = EINVAL;
                    }
                }
                break;
#ifdef SHARK
	        case CONSOLE_GET_LINEAR_INFO:
		    if(data)
		    {
			/* 
			 * Warning: Relies on OFW setting up the console in 
			 * linear mode 
			 */			
			paddr_t linearPhysAddr = displayInfo(paddr); 

			/* Size unknown - X Server will probe chip later. */
			((struct map_info *)data)->method = MAP_MMAP;
			((struct map_info *)data)->size = MAP_INFO_UNKNOWN;

			((struct map_info *)data)->u.map_info_mmap.map_offset 
			    = linearPhysAddr & L2_S_FRAME;
			((struct map_info *)data)->u.map_info_mmap.map_size 
			    = MAP_INFO_UNKNOWN;
			((struct map_info *)data)->u.map_info_mmap.internal_offset 
			    = linearPhysAddr & L2_S_OFFSET;
			((struct map_info *)data)->u.map_info_mmap.internal_size 
			    = MAP_INFO_UNKNOWN;
		    }
		    else
		    {
			error = EINVAL;
		    }
		break;  

		case CONSOLE_GET_IO_INFO:
		    if(data)
		    {
			paddr_t ioPhysAddr = displayInfo(ioBase);
			int ioLength = displayInfo(ioLen); 
			paddr_t pam_io_data;

			((struct map_info *)data)->method = MAP_MMAP;
			((struct map_info *)data)->size 
			    = ioLength / sizeof(bus_size_t);

			pam_io_data = vtophys(isa_io_data_vaddr());

			((struct map_info *)data)->u.map_info_mmap.map_offset 
			    = (pam_io_data + ioPhysAddr) & L2_S_FRAME;
			((struct map_info *)data)->u.map_info_mmap.internal_offset 
			    = (pam_io_data + ioPhysAddr) & L2_S_OFFSET;
			((struct map_info *)data)->u.map_info_mmap.map_size 
			    = (((struct map_info *)data)->u.map_info_mmap.internal_offset 
			       + ioLength + L2_S_SIZE - 1) & L2_S_FRAME;
			((struct map_info *)data)->u.map_info_mmap.internal_size 
			    = ioLength;
		    }
		    else
		    {
			error = EINVAL;
		    }
		break;
		    
	        case CONSOLE_GET_MEM_INFO:
		    if(data)
		    {
		        vaddr_t vam_mem_data = isa_mem_data_vaddr();

			/* 
			 * Address and size defined in shark.h and 
			 * isa_machdep.h 
			 */
			((struct map_info *)data)->method = MAP_MMAP;
			((struct map_info *)data)->size 
			    = VGA_BUF_LEN / sizeof(bus_size_t);

			((struct map_info *)data)->u.map_info_mmap.map_offset 
			    = (vtophys(vam_mem_data) + VGA_BUF) & L2_S_FRAME;
			((struct map_info *)data)->u.map_info_mmap.internal_offset 
			    = (vtophys(vam_mem_data) + VGA_BUF) & L2_S_OFFSET;
			((struct map_info *)data)->u.map_info_mmap.map_size 
			    = (((struct map_info *)data)->u.map_info_mmap.internal_offset 
				+ VGA_BUF_LEN + L2_S_SIZE - 1) & L2_S_FRAME;
			((struct map_info *)data)->u.map_info_mmap.internal_size 
			    = VGA_BUF_LEN;
		    }
		    else
		    {
			error = EINVAL;
		    }
		break;
	
#endif /* SHARK */
               
                default:
                    error = EPASSTHROUGH;
                break;
            } /* End switch on ioctl command */
        } /* End need to check if this is a device specific command */
    } /* End need to check if this is a common tty command */ 

    return (error);
} /* End pcioctl() */

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     pcstart
**
**     This routine is responsible for outputing any data in the tty output
**     queue.   If the tty is not already in the process of transmitting 
**     then we remove an amount of data from the output queue and copy it
**     to the screen via the sput routine.  If data still remains to be
**     transmitted we queue a timeout on the tty. If the amount of data 
**     sent puts the output queue below the low water mark we wake up
**     anyone sleeping.   
**
**  FORMAL PARAMETERS:
**
**     tp - pointer to the tty structure wanting to send data.
**
**  IMPLICIT INPUTS:
**
**     none
**
**  IMPLICIT OUTPUTS:
**
**     none.
**
**  FUNCTION VALUE:
**
**     none.
**
**  SIDE EFFECTS:
**
**     none
**     
**--
*/
void
pcstart(struct tty *tp)
{
    struct clist *cl;
    int s, len;
    u_char buf[PCBURST];
    struct pc_softc *sc = pc_cd.cd_devs[PCUNIT(tp->t_dev)];
    
    s = spltty();
    if (!(tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP)))
    {
        /*
        ** Copy data to be output from c-list to output buffer
        ** and send it to the screen via the sput routine.
        ** We need to do this outside spl since it could be fairly
        ** expensive.
        */
        tp->t_state |= TS_BUSY;
        
        splx(s);
        cl  = &tp->t_outq;
        len = q_to_b(cl, buf, PCBURST);
        sput(sc, buf, len, false);
        s = spltty();
        
        tp->t_state &= ~TS_BUSY;
        /*
        ** If there is still data to be output, queue a timeout
        ** on the tty.
        */
        if (cl->c_cc) 
        {
            tp->t_state |= TS_TIMEOUT;
	    callout_reset(&tp->t_rstrt_ch, 1, ttrstrt, tp);
        }
        /* 
        ** Check if we are under the low water mark and
        ** need to wake any sleepers
        */
        if (cl->c_cc <= tp->t_lowat) 
        {
            if (tp->t_state & TS_ASLEEP) 
            {
                tp->t_state &= ~TS_ASLEEP;
                wakeup(cl);
            }
            selwakeup(&tp->t_wsel);
        }
    } /* End if not busy, timeout, or stopped */ 
    
    /* Restore spl */
    splx(s);
    return;
} /* End pcstart() */

/*****************************************************************************/
/*                                                                           */
/*     The following are the routines that allow the pc device to operate    */
/*     as the console device.  They are prefixed pccn.                       */
/*                                                                           */
/*****************************************************************************/



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     pccnprobe
**
**     Probe for the generic console device.  
**
**  FORMAL PARAMETERS:
**
**     cp - pointer to a console device structure.
**
**  IMPLICIT INPUTS:
**
**     The isa_io_bs_tag is used to identify which I/O space needs to be 
**     mapped to talk to the keyboard device.  The CONKBDADDR option
**     is used to determine the base address of the keyboard controller.
**     This file supplies a default value, in case it is not defined 
**     in the config file 
**
**  IMPLICIT OUTPUTS:
**
**     none.
**
**  FUNCTION VALUE:
**
**     none - however cn_pri is effectively a return value as this determines
**            the priority of this device in being the console.
**
**  SIDE EFFECTS:
**
**     none.
**
**--
*/
void
pccnprobe(struct consdev *cp)
{
    int                   maj;
    int                   s;
    
    /* locate the major number 
    */
    maj = cdevsw_lookup_major(&pc_cdevsw);
    /* initialize required fields 
    */
    cp->cn_dev = makedev(maj, 0);
    cp->cn_pri = CN_DEAD;       /* assume can't configure keyboard */
    /* initialise the boot softc to use until we get attached with a
    ** real softc.
    */
    memset(&bootSoftc, 0, sizeof(bootSoftc));
    bootSoftc.kbd.sc_iot = &isa_io_bs_tag;
    
    s = splhigh();
    
    /* Setup base address of keyboard device on the SuperIO chip. Then
    ** map our register space within the bus.
    */
    if (i87307KbdConfig(bootSoftc.kbd.sc_iot, CONKBDADDR, IRQ_KEYBOARD))
    {
        if (I8042_MAP(bootSoftc.kbd.sc_iot,CONKBDADDR,bootSoftc.kbd.sc_ioh) 
	    == 0) 
        {
	    /*  
	    ** Initialise the keyboard.  Look for another device if
            ** keyboard wont initialise but leave us as a backup display
	    ** unit.
	    */
	    if (kbd_init(bootSoftc.kbd.sc_iot, bootSoftc.kbd.sc_ioh))
	    {
		cp->cn_pri = CN_INTERNAL;
	    }
            I8042_UNMAP(bootSoftc.kbd.sc_iot, bootSoftc.kbd.sc_ioh);
        }
    }
    splx(s);
    
    return;
} /* End pccnprobe() */



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     pccninit
**
**     This is the intialisation entry point called through the constab
**     array.  All we do is intialise the keyboard controller.  We ignore
**     any failure, this allows the generic console device to be used
**     even if no keyboard is plugged in.
**
**  FORMAL PARAMETERS:
** 
**     cp - pointer to a console device structure.
**      
**  IMPLICIT INPUTS:
**
**     none
**
**  IMPLICIT OUTPUTS:
**
**     actingConsole is set to one to indicate that the console device has
**     been initialised.  
**
**  FUNCTION VALUE:
**
**     none.
**
**  SIDE EFFECTS:
**
**     none.
**
**--
*/
void
pccninit(struct consdev *cp)
{
    int                s = splhigh();

    callout_init(&async_update_ch, 0);

    actingConsole = true;
    if (I8042_MAP(bootSoftc.kbd.sc_iot, CONKBDADDR, bootSoftc.kbd.sc_ioh))
    {
        panic("pccninit: kbd mapping failed");
    }
    bootSoftc.sc_flags                  = 0x00;
    bootSoftc.kbd.sc_shift_state        = 0x00;
    bootSoftc.kbd.sc_new_lock_state     = 0x00;
    bootSoftc.kbd.sc_old_lock_state     = 0xff;
    bootSoftc.kbd.sc_new_typematic_rate = 0xff;
    bootSoftc.kbd.sc_old_typematic_rate = 0xff;
    bootSoftc.vs.cx  = bootSoftc.vs.cy  = 0;
    bootSoftc.vs.row = bootSoftc.vs.col = 0;
    bootSoftc.vs.nrow                   = 0;
    bootSoftc.vs.ncol                   = 0;
    bootSoftc.vs.nchr                   = 0;
    bootSoftc.vs.state                  = 0;
    bootSoftc.vs.so                     = 0;
    bootSoftc.vs.color                  = 0;
    bootSoftc.vs.at                     = 0;
    bootSoftc.vs.so_at                  = 0;

#ifdef	SHARK
    /*
     ** Get display information from ofw before leaving linear mode.
     */
    getDisplayInfo(&display);
    
    /* Must force hardware into VGA mode before anything else
     ** otherwise we may hang trying to do console output.
     */
    force_vga_mode();

    pccnputc(0, '\n');
#endif
    
    splx(s);

    return;
} /* End pccninit() */


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     pccnputc
**
**     This is a simple putc routine for the generic console device.  
**     It simply calls sput to write the character to the screen.
**
**  FORMAL PARAMETERS:
**
**     dev  - our device identification
**     c    - the character to be transmited.
**
**  IMPLICIT INPUTS:
**
**     none.
**
**  IMPLICIT OUTPUTS:
**
**     none.
**
**  FUNCTION VALUE:
**
**     none.
**
**  SIDE EFFECTS:
**
**     none
**
**--
*/
void
pccnputc(dev_t  dev, 
         char   c)
{
    struct pc_softc  *currentSC;
    
    /* Ok so we are a little bit complicated because I insist on using a 
    ** decent approach to softc's.  Here we check whether the attach has
    ** been performed on the console device and if so get the REAL softc.
    ** otherwise we use the bootSoftc cause we are the console in boot
    ** sequence.
    */
    if (attachCompleted == true)
    {
        currentSC = pc_cd.cd_devs[PCUNIT(dev)];
    }
    else
    {
        currentSC = &bootSoftc;
    }
    /* write the character to the screen 
    */
    if (c == '\n')
    {
        sput(currentSC, "\r\n", 2, true);
    }
    else
    {
        sput(currentSC, &c, 1, true);
    }

    return;
} /* End pccnputc() */


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     pccngetc
**
**     This routine reads a character from the keyboard output buffer.
**     It waits until there is a charcater available and then calls
**     the sget routine to convert the character, before returning it.
**      
**  FORMAL PARAMETERS:
**
**     dev - The identifier for our device. 
**
**  IMPLICIT INPUTS:
**
**     none.
**
**  IMPLICIT OUTPUTS:
**
**     none.
**
**  FUNCTION VALUE:
**
**     Returns the converted character read from the keyboard.
**
**  SIDE EFFECTS:
**    
**     none
**
**--
*/
int
pccngetc(dev_t dev)
{
    register char    *cp = NULL;
    struct pc_softc  *currentSC;
    
    /* Ok so we are a little bit complicated because I insist on using a 
    ** decent approach to softc's.  Here we check whether the attach has
    ** been performed on the console device and if so get the REAL softc.
    ** otherwise we use the bootSoftc cause we are the console in boot
    ** sequence.
    */
    if (attachCompleted == true)
    {
        currentSC = pc_cd.cd_devs[PCUNIT(dev)];
    }
    else
    {
        currentSC = &bootSoftc;
    }

    if ( (currentSC->sc_flags & SC_XMODE) == 0 )
    {
        do 
        {
            /* wait for a keyboard byte from the 8042 controller.
            */
            for (;;)
            {
                if ( !i8042_wait_input(currentSC->kbd.sc_iot, 
                          currentSC->kbd.sc_ioh, I8042_KBD_DATA) )
                {
                    KERN_DEBUG( pcdebug, KERN_DEBUG_WARNING, 
                         ("pccnget: in deep dodo got bad status\n")); 
                }
                break;
            } /* End do forever until we get a character */
            /* Read and convert character in keyboard buffer 
            */
            cp = sget(currentSC);
        } while (!cp);
    
        if (*cp == '\r')
        {
            return '\n';
        }
    } /* End if NOT in Xmode */

    return *cp;
} /* End pccngetc() */



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     pcncpollc
**
**     This routine is used to activate/deactivate the polling interface
**     to the generic console device.  If polling is being turned off
**     and the console device is in use, the interrupt routine is called 
**     to pass up any character that may still be in the keyboard output
**     buffer.
**
**  FORMAL PARAMETERS:
**
**     dev  - our device identification
**     on   - 1 - set device to polling
**            0 - turn off polling on the device
**
**  IMPLICIT INPUTS:
**
**     none
**
**  IMPLICIT OUTPUTS:
**
**     none.
**
**  FUNCTION VALUE:
**
**     none.
**
**  SIDE EFFECTS:
**
**     none
**--
*/
void
pccnpollc(dev_t dev, 
          int   on)
{
    struct pc_softc  *currentSC;
    
    /* Ok so we are a little bit complicated because I insist on using a 
    ** decent approach to softc's.  Here we check whether the attach has
    ** been performed on the console device and if so get the REAL softc.
    ** otherwise we use the bootSoftc cause we are the console in boot
    ** sequence.
    */
    if (attachCompleted == true)
    {
        currentSC = pc_cd.cd_devs[PCUNIT(dev)];
    }
    else
    {
        currentSC = &bootSoftc;
    }

    currentSC->sc_flags = on ? currentSC->sc_flags | SC_POLLING :
                               currentSC->sc_flags & ~SC_POLLING;
    if (!on) 
    {
        int s;
        
        /* Comming out of polling mode.  Make sure we try and process any
        ** character that may be in the buffer at this time that we have 
        ** already ignored the interrupt for.
        */
        s = spltty();
        pcintr(currentSC);
        splx(s);
    } /* end-if not polling */

    return;
} /* End pccnpollc() */ 



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     pcparam
**
**     This routine is called by the tty to modify hardware settings on 
**     the device as specified by the termios structured passed to it
**     (.e.g. line speed).  As there is nothing we can set in the hardware, 
**      we just update the fields in the tty structure and return success.  
**
**  FORMAL PARAMETERS:
**
**     tp - Pointer to a tty structure for our device.
**     t  - Pointer to the termios structure containing new configuration
**          parameters. 
**
**  IMPLICIT INPUTS:
**
**     none
**
**  IMPLICIT OUTPUTS:
**
**     none
**
**  FUNCTION VALUE:
**
**     0 - Always succeeds.
**  
**  SIDE EFFECTS:
**
**     none.     
**--
*/
int
pcparam(struct tty     *tp, 
        struct termios *t)
{
    
    tp->t_ispeed = t->c_ispeed;
    tp->t_ospeed = t->c_ospeed;
    tp->t_cflag  = t->c_cflag;

    return 0;
} /* End pcparam() */



/*
** wrtchar
**
** Write a character to the screen.
** Load pointer with current cursor address.
** Write character and the character attributes
** to to cursor address. Update cursor address 
** and column location.
*/
#define wrtchar(sc, c, at) \
do { \
    char *__cp = (char *)crtat; \
\
    *__cp++    = (c); \
    *__cp      = (at); \
    crtat++; sc->vs.col++; \
} while (0)

/* translate ANSI color codes to standard pc ones */
static char fgansitopc[] = {
    FG_BLACK, FG_RED, FG_GREEN, FG_BROWN, FG_BLUE,
    FG_MAGENTA, FG_CYAN, FG_LIGHTGREY
};

static char bgansitopc[] = {
    BG_BLACK, BG_RED, BG_GREEN, BG_BROWN, BG_BLUE,
    BG_MAGENTA, BG_CYAN, BG_LIGHTGREY
};



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     sput
**  
**     This routine is used to write to the screen using
**     pc3 terminal emulation.   
**
**  FORMAL PARAMETERS:
**
**     None.
**
**  IMPLICIT INPUTS:
**
**     None.
**
**  IMPLICIT OUTPUTS:
**
**     none.
**
**  FUNCTION VALUE:
**
**     none
**
**--
*/
void
sput(struct pc_softc   *sc,
     const u_char      *cp, 
     int               n,
     u_char            nowait)
{
    u_char c, scroll = 0;
    
    if (sc->sc_flags & SC_XMODE)
        return;

    /* Initialise the display if not done already */
    if (crtat == 0) 
    {
        u_short volatile *cp2;
#ifdef DOESNT_ALWAYS_DO_THE_RIGHT_THING
        u_short was;
#endif
        unsigned cursorat;
	vaddr_t vam_mem_data = isa_mem_data_vaddr();
        
        /* 
        ** Check to see if we have color support
        ** by attempting to write and read from
        ** the colour buffer.  If successful then
        ** we operate in color mode otherwise
        ** mono.
        */
        cp2 = (void *)((u_long)(CGA_BUF) + vam_mem_data);
#ifdef DOESNT_ALWAYS_DO_THE_RIGHT_THING
        was = *cp2;              /* save whatever is at CGA_BUF */
        *cp2 = (u_short) 0xA55A;
        if (*cp2 != 0xA55A) 
        {
            cp2 = (void *)((u_long)(MONO_BUF) + vam_mem_data);
            addr_6845 = MONO_BASE;
            sc->vs.color = 0;
        } 
        else 
        {
            *cp2 = was;          /* restore previous contents of CGA_BUF */
            addr_6845 = CGA_BASE;
            sc->vs.color = 1;
        }
#else
	addr_6845 = CGA_BASE;
	sc->vs.color = 1;
#endif
        
        /* Extract cursor location */
        outb(addr_6845, 14);
        cursorat = inb(addr_6845+1) << 8;
        outb(addr_6845, 15);
        cursorat |= inb(addr_6845+1);
        
#ifdef FAT_CURSOR
        cursor_shape = 0x0012;
#endif
        /* Save cursor locations */
        Crtat = __UNVOLATILE(cp2);
        crtat = __UNVOLATILE(cp2 + cursorat);
        
        /* Set up screen size and colours */
        sc->vs.ncol = COL;
        sc->vs.nrow = ROW;
        sc->vs.nchr = COL * ROW;
        sc->vs.at = FG_LIGHTGREY | BG_BLACK;        
        if (sc->vs.color == 0)
        {
            sc->vs.so_at = FG_BLACK | BG_LIGHTGREY;
        }
        else
        {
            sc->vs.so_at = FG_YELLOW | BG_BLACK;
        }
        fillw((sc->vs.at << 8) | ' ', crtat, sc->vs.nchr - cursorat);
    } /* end-if screen unitialised */ 
    
    /* Process supplied character string */ 
    while (n--) 
    {
        if (!(c = *cp++))
            continue;
        
        switch (c) 
        {
        case 0x1B:
            if (sc->vs.state >= VSS_ESCAPE) 
            {
                wrtchar(sc, c, sc->vs.so_at); 
                sc->vs.state = 0;
                goto maybe_scroll;
            }
            else
            {
                sc->vs.state = VSS_ESCAPE;
            }
            break;
            
        case '\t': 
        {
            int inccol = 8 - (sc->vs.col & 7);
            crtat += inccol;
            sc->vs.col += inccol;
        }
        maybe_scroll:
            if (sc->vs.col >= COL) 
            {
                sc->vs.col -= COL;
                scroll = 1;
            }
            break;
            
        case '\010':
            if (crtat <= Crtat)
                break;
            --crtat;
            if (--sc->vs.col < 0)
                sc->vs.col += COL;  /* non-destructive backspace */
            break;
            
        case '\r':
            crtat -= sc->vs.col;
            sc->vs.col = 0;
            break;
            
        case '\n':
            crtat += sc->vs.ncol;
            scroll = 1;
            break;
            
        default:
            switch (sc->vs.state) 
            {
            case 0:
                if (c == '\a')
                {
                    sysbeep(BEEP_FREQ, BEEP_TIME);
                }
                else 
                {
                    /*
                     * If we're outputting multiple printed
                     * characters, just blast them to the
                     * screen until we reach the end of the
                     * buffer or a control character.  This
                     * saves time by short-circuiting the
                     * switch.
                     * If we reach the end of the line, we
                     * break to do a scroll check.
                     */
                    for (;;) 
                    {
                        if (sc->vs.so)
                        {
                            wrtchar(sc, c, sc->vs.so_at);
                        }
                        else
                        {
                            wrtchar(sc, c, sc->vs.at);
                        }
                        if (sc->vs.col >= sc->vs.ncol) 
                        {
                            sc->vs.col = 0;
                            scroll = 1;
                            break;
                        }
                        if (!n || (c = *cp) < ' ')
                            break;
                        n--, cp++;
                    }
                }
                break;

            case VSS_ESCAPE:
                if (c == '[') 
                {       /* Start ESC [ sequence */
                    sc->vs.cx = sc->vs.cy = 0;
                    sc->vs.state = VSS_EBRACE;
                } 
                else if (c == 'c')  /* Clear screen & home */
                { 
                    fillw((sc->vs.at << 8) | ' ',
                          Crtat, sc->vs.nchr);
                    crtat = Crtat;
                    sc->vs.col = 0;
                    sc->vs.state = 0;
                } else /* Invalid, clear state */
                { 
                    wrtchar(sc, c, sc->vs.so_at); 
                    sc->vs.state = 0;
                    goto maybe_scroll;
                }
                break;
                
            default: /* VSS_EBRACE or VSS_EPARAM */
                switch (c) 
                {
                    int pos;
                case 'm':
                    if (!sc->vs.cx)
                        sc->vs.so = 0;
                    else
                        sc->vs.so = 1;
                    sc->vs.state = 0;
                    break;
                case 'A': { /* back cx rows */
                    int cx = sc->vs.cx;
                    if (cx <= 0)
                        cx = 1;
                    else
                        cx %= sc->vs.nrow;
                    pos = crtat - Crtat;
                    pos -= sc->vs.ncol * cx;
                    if (pos < 0)
                        pos += sc->vs.nchr;
                    crtat = Crtat + pos;
                    sc->vs.state = 0;
                    break;
                }
                case 'B': { /* down cx rows */
                    int cx = sc->vs.cx;
                    if (cx <= 0)
                        cx = 1;
                    else
                        cx %= sc->vs.nrow;
                    pos = crtat - Crtat;
                    pos += sc->vs.ncol * cx;
                    if (pos >= sc->vs.nchr) 
                        pos -= sc->vs.nchr;
                    crtat = Crtat + pos;
                    sc->vs.state = 0;
                    break;
                }
                case 'C': { /* right cursor */
                    int cx = sc->vs.cx,
                    col = sc->vs.col;
                    if (cx <= 0)
                        cx = 1;
                    else
                        cx %= sc->vs.ncol;
                    pos = crtat - Crtat;
                    pos += cx;
                    col += cx;
                    if (col >= sc->vs.ncol) {
                        pos -= sc->vs.ncol;
                        col -= sc->vs.ncol;
                    }
                    sc->vs.col = col;
                    crtat = Crtat + pos;
                    sc->vs.state = 0;
                    break;
                }
                case 'D': { /* left cursor */
                    int cx = sc->vs.cx,
                    col = sc->vs.col;
                    if (cx <= 0)
                        cx = 1;
                    else
                        cx %= sc->vs.ncol;
                    pos = crtat - Crtat;
                    pos -= cx;
                    col -= cx;
                    if (col < 0) {
                        pos += sc->vs.ncol;
                        col += sc->vs.ncol;
                    }
                    sc->vs.col = col;
                    crtat = Crtat + pos;
                    sc->vs.state = 0;
                    break;
                }
                case 'J': /* Clear ... */
                    switch (sc->vs.cx) {
                    case 0:
                        /* ... to end of display */
                        fillw((sc->vs.at << 8) | ' ', 
                              crtat,
                              Crtat + sc->vs.nchr - crtat);
                        break;
                    case 1:
                        /* ... to next location */
                        fillw((sc->vs.at << 8) | ' ',
                              Crtat, crtat - Crtat + 1);
                        break;
                    case 2:
                        /* ... whole display */
                        fillw((sc->vs.at << 8) | ' ',
                              Crtat, sc->vs.nchr);
                        break;
                    }
                    sc->vs.state = 0;
                    break;
                case 'K': /* Clear line ... */
                    switch (sc->vs.cx) {
                    case 0:
                        /* ... current to EOL */
                        fillw((sc->vs.at << 8) | ' ',
                              crtat, sc->vs.ncol - sc->vs.col);
                        break;
                    case 1:
                        /* ... beginning to next */
                        fillw((sc->vs.at << 8) | ' ',
                              crtat - sc->vs.col, sc->vs.col + 1);
                        break;
                    case 2:
                        /* ... entire line */
                        fillw((sc->vs.at << 8) | ' ',
                              crtat - sc->vs.col, sc->vs.ncol);
                        break;
                    }
                    sc->vs.state = 0;
                    break;
                case 'f': /* in system V consoles */
                case 'H': { /* Cursor move */
                    int cx = sc->vs.cx,
                    cy = sc->vs.cy;
                    if (!cx || !cy) {
                        crtat = Crtat;
                        sc->vs.col = 0;
                    } else {
                        if (cx > sc->vs.nrow)
                            cx = sc->vs.nrow;
                        if (cy > sc->vs.ncol)
                            cy = sc->vs.ncol;
                        crtat = Crtat +
                            (cx - 1) * sc->vs.ncol + cy - 1;
                        sc->vs.col = cy - 1;
                    }
                    sc->vs.state = 0;
                    break;
                }
                case 'M': { /* delete cx rows */
                    u_short *crtAt = crtat - sc->vs.col;
                    int cx = sc->vs.cx,
                    row = (crtAt - Crtat) / sc->vs.ncol,
                    nrow = sc->vs.nrow - row;
                    if (cx <= 0)
                        cx = 1;
                    else if (cx > nrow)
                        cx = nrow;
                    if (cx < nrow)
                        bcopy(crtAt + sc->vs.ncol * cx,
                              crtAt, sc->vs.ncol * (nrow -
                                                cx) * CHR);
                    fillw((sc->vs.at << 8) | ' ',
                          crtAt + sc->vs.ncol * (nrow - cx),
                          sc->vs.ncol * cx);
                    sc->vs.state = 0;
                    break;
                }
                case 'S': { /* scroll up cx lines */
                    int cx = sc->vs.cx;
                    if (cx <= 0)
                        cx = 1;
                    else if (cx > sc->vs.nrow)
                        cx = sc->vs.nrow;
                    if (cx < sc->vs.nrow)
                        bcopy(Crtat + sc->vs.ncol * cx,
                              Crtat, sc->vs.ncol * (sc->vs.nrow -
                                                cx) * CHR);
                    fillw((sc->vs.at << 8) | ' ',
                          Crtat + sc->vs.ncol * (sc->vs.nrow - cx),
                          sc->vs.ncol * cx);
#if 0
                    crtat -= sc->vs.ncol * cx; /* XXX */
#endif
                    sc->vs.state = 0;
                    break;
                }
                case 'L': { /* insert cx rows */
                    u_short *crtAt = crtat - sc->vs.col;
                    int cx = sc->vs.cx,
                    row = (crtAt - Crtat) / sc->vs.ncol,
                    nrow = sc->vs.nrow - row;
                    if (cx <= 0)
                        cx = 1;
                    else if (cx > nrow)
                        cx = nrow;
                    if (cx < nrow)
                        bcopy(crtAt,
                              crtAt + sc->vs.ncol * cx,
                              sc->vs.ncol * (nrow - cx) *
                              CHR);
                    fillw((sc->vs.at << 8) | ' ', 
                          crtAt, sc->vs.ncol * cx);
                    sc->vs.state = 0;
                    break;
                }
                case 'T': { /* scroll down cx lines */
                    int cx = sc->vs.cx;
                    if (cx <= 0)
                        cx = 1;
                    else if (cx > sc->vs.nrow)
                        cx = sc->vs.nrow;
                    if (cx < sc->vs.nrow)
                        bcopy(Crtat,
                              Crtat + sc->vs.ncol * cx,
                              sc->vs.ncol * (sc->vs.nrow - cx) *
                              CHR);
                    fillw((sc->vs.at << 8) | ' ', 
                          Crtat, sc->vs.ncol * cx);
#if 0
                    crtat += sc->vs.ncol * cx; /* XXX */
#endif
                    sc->vs.state = 0;
                    break;
                }
                case ';': /* Switch params in cursor def */
                    sc->vs.state = VSS_EPARAM;
                    break;
                case 'r':
                    sc->vs.so_at = (sc->vs.cx & FG_MASK) |
                        ((sc->vs.cy << 4) & BG_MASK);
                    sc->vs.state = 0;
                    break;
                case 'x': /* set attributes */
                    switch (sc->vs.cx) {
                    case 0:
                        sc->vs.at = FG_LIGHTGREY | BG_BLACK;
                        break;
                    case 1:
                        /* ansi background */
                        if (!sc->vs.color)
                            break;
                        sc->vs.at &= FG_MASK;
                        sc->vs.at |= bgansitopc[sc->vs.cy & 7];
                        break;
                    case 2:
                        /* ansi foreground */
                        if (!sc->vs.color)
                            break;
                        sc->vs.at &= BG_MASK;
                        sc->vs.at |= fgansitopc[sc->vs.cy & 7];
                        break;
                    case 3:
                        /* pc text attribute */
                        if (sc->vs.state >= VSS_EPARAM)
                            sc->vs.at = sc->vs.cy;
                        break;
                    }
                    sc->vs.state = 0;
                    break;
                    
                default: /* Only numbers valid here */
                    if ((c >= '0') && (c <= '9')) {
                        if (sc->vs.state >= VSS_EPARAM) {
                            sc->vs.cy *= 10;
                            sc->vs.cy += c - '0';
                        } else {
                            sc->vs.cx *= 10;
                            sc->vs.cx += c - '0';
                        }
                    } else
                        sc->vs.state = 0;
                    break;
                }
                break;
            }
        }
        if (scroll) {
            scroll = 0;
            /* scroll check */
            if (crtat >= Crtat + sc->vs.nchr) {
                if (nowait == false) {
                    int s = spltty();
                    if (sc->kbd.sc_new_lock_state & SCROLL)
                        tsleep(&(sc->kbd.sc_new_lock_state),
                               PUSER, "pcputc", 0);
                    splx(s);
                }
                bcopy(Crtat + sc->vs.ncol, Crtat,
                      (sc->vs.nchr - sc->vs.ncol) * CHR);
                fillw((sc->vs.at << 8) | ' ',
                      Crtat + sc->vs.nchr - sc->vs.ncol,
                      sc->vs.ncol);
                crtat -= sc->vs.ncol;
            }
        }

   }
    async_update(sc, nowait);
}

#define CODE_SIZE       4               /* Use a max of 4 for now... */
#ifndef NONUS_KBD
typedef struct 
{
    u_short     type;
    char        unshift[CODE_SIZE];
    char        shift[CODE_SIZE];
    char        ctl[CODE_SIZE];
} Scan_def;

static Scan_def scan_codes[] = {
{ NONE, "",             "",             "" },           /* 0 unused */
{ ASCII,"\033",         "\033",         "\033" },       /* 1 ESCape */
{ ASCII,"1",            "!",            "!" },          /* 2 1 */
{ ASCII,"2",            "@",            "\000" },       /* 3 2 */
{ ASCII,"3",            "#",            "#" },          /* 4 3 */
{ ASCII,"4",            "$",            "$" },          /* 5 4 */
{ ASCII,"5",            "%",            "%" },          /* 6 5 */
{ ASCII,"6",            "^",            "\036" },       /* 7 6 */
{ ASCII,"7",            "&",            "&" },          /* 8 7 */
{ ASCII,"8",            "*",            "\010" },       /* 9 8 */
{ ASCII,"9",            "(",            "(" },          /* 10 9 */
{ ASCII,"0",            ")",            ")" },          /* 11 0 */
{ ASCII,"-",            "_",            "\037" },       /* 12 - */
{ ASCII,"=",            "+",            "+" },          /* 13 = */
{ ASCII,"\177",         "\177",         "\010" },       /* 14 backspace */
{ ASCII,"\t",           "\177\t",       "\t" },         /* 15 tab */
{ ASCII,"q",            "Q",            "\021" },       /* 16 q */
{ ASCII,"w",            "W",            "\027" },       /* 17 w */
{ ASCII,"e",            "E",            "\005" },       /* 18 e */
{ ASCII,"r",            "R",            "\022" },       /* 19 r */
{ ASCII,"t",            "T",            "\024" },       /* 20 t */
{ ASCII,"y",            "Y",            "\031" },       /* 21 y */
{ ASCII,"u",            "U",            "\025" },       /* 22 u */
{ ASCII,"i",            "I",            "\011" },       /* 23 i */
{ ASCII,"o",            "O",            "\017" },       /* 24 o */
{ ASCII,"p",            "P",            "\020" },       /* 25 p */
{ ASCII,"[",            "{",            "\033" },       /* 26 [ */
{ ASCII,"]",            "}",            "\035" },       /* 27 ] */
{ ASCII,"\r",           "\r",           "\n" },         /* 28 return */
#if defined (CAPS_IS_CONTROL)
{ CAPS,  "",             "",             "" },           /* 29 caps */
#else
{ CTL,  "",             "",             "" },           /* 29 control */
#endif
{ ASCII,"a",            "A",            "\001" },       /* 30 a */
{ ASCII,"s",            "S",            "\023" },       /* 31 s */
{ ASCII,"d",            "D",            "\004" },       /* 32 d */
{ ASCII,"f",            "F",            "\006" },       /* 33 f */
{ ASCII,"g",            "G",            "\007" },       /* 34 g */
{ ASCII,"h",            "H",            "\010" },       /* 35 h */
{ ASCII,"j",            "J",            "\n" },         /* 36 j */
{ ASCII,"k",            "K",            "\013" },       /* 37 k */
{ ASCII,"l",            "L",            "\014" },       /* 38 l */
{ ASCII,";",            ":",            ";" },          /* 39 ; */
{ ASCII,"'",            "\"",           "'" },          /* 40 ' */
{ ASCII,"`",            "~",            "`" },          /* 41 ` */
{ SHIFT,"",             "",             "" },           /* 42 shift */
{ ASCII,"\\",           "|",            "\034" },       /* 43 \ */
{ ASCII,"z",            "Z",            "\032" },       /* 44 z */
{ ASCII,"x",            "X",            "\030" },       /* 45 x */
{ ASCII,"c",            "C",            "\003" },       /* 46 c */
{ ASCII,"v",            "V",            "\026" },       /* 47 v */
{ ASCII,"b",            "B",            "\002" },       /* 48 b */
{ ASCII,"n",            "N",            "\016" },       /* 49 n */
{ ASCII,"m",            "M",            "\r" },         /* 50 m */
{ ASCII,",",            "<",            "<" },          /* 51 , */
{ ASCII,".",            ">",            ">" },          /* 52 . */
{ ASCII,"/",            "?",            "\037" },       /* 53 / */
{ SHIFT,"",             "",             "" },           /* 54 shift */
{ KP,   "*",            "*",            "*" },          /* 55 kp * */
{ ALT,  "",             "",             "" },           /* 56 alt */
{ ASCII," ",            " ",            "\000" },       /* 57 space */
#if defined (CAPS_IS_CONTROL)
{ CTL, "",             "",             "" },           /* 58 ctl */
#else
{ CAPS, "",             "",             "" },           /* 58 caps */
#endif
{ FUNC, "\033[M",       "\033[Y",       "\033[k" },     /* 59 f1 */
{ FUNC, "\033[N",       "\033[Z",       "\033[l" },     /* 60 f2 */
{ FUNC, "\033[O",       "\033[a",       "\033[m" },     /* 61 f3 */
{ FUNC, "\033[P",       "\033[b",       "\033[n" },     /* 62 f4 */
{ FUNC, "\033[Q",       "\033[c",       "\033[o" },     /* 63 f5 */
{ FUNC, "\033[R",       "\033[d",       "\033[p" },     /* 64 f6 */
{ FUNC, "\033[S",       "\033[e",       "\033[q" },     /* 65 f7 */
{ FUNC, "\033[T",       "\033[f",       "\033[r" },     /* 66 f8 */
{ FUNC, "\033[U",       "\033[g",       "\033[s" },     /* 67 f9 */
{ FUNC, "\033[V",       "\033[h",       "\033[t" },     /* 68 f10 */
{ NUM,  "",             "",             "" },           /* 69 num lock */
{ SCROLL,"",            "",             "" },           /* 70 scroll lock */
{ KP,   "7",            "\033[H",       "7" },          /* 71 kp 7 */
{ KP,   "8",            "\033[A",       "8" },          /* 72 kp 8 */
{ KP,   "9",            "\033[I",       "9" },          /* 73 kp 9 */
{ KP,   "-",            "-",            "-" },          /* 74 kp - */
{ KP,   "4",            "\033[D",       "4" },          /* 75 kp 4 */
{ KP,   "5",            "\033[E",       "5" },          /* 76 kp 5 */
{ KP,   "6",            "\033[C",       "6" },          /* 77 kp 6 */
{ KP,   "+",            "+",            "+" },          /* 78 kp + */
{ KP,   "1",            "\033[F",       "1" },          /* 79 kp 1 */
{ KP,   "2",            "\033[B",       "2" },          /* 80 kp 2 */
{ KP,   "3",            "\033[G",       "3" },          /* 81 kp 3 */
{ KP,   "0",            "\033[L",       "0" },          /* 82 kp 0 */
{ KP,   ".",            "\177",         "." },          /* 83 kp . */
{ NONE, "",             "",             "" },           /* 84 0 */
{ NONE, "100",          "",             "" },           /* 85 0 */
{ NONE, "101",          "",             "" },           /* 86 0 */
{ FUNC, "\033[W",       "\033[i",       "\033[u" },     /* 87 f11 */
{ FUNC, "\033[X",       "\033[j",       "\033[v" },     /* 88 f12 */
{ NONE, "102",          "",             "" },           /* 89 0 */
{ NONE, "103",          "",             "" },           /* 90 0 */
{ NONE, "",             "",             "" },           /* 91 0 */
{ NONE, "",             "",             "" },           /* 92 0 */
{ NONE, "",             "",             "" },           /* 93 0 */
{ NONE, "",             "",             "" },           /* 94 0 */
{ NONE, "",             "",             "" },           /* 95 0 */
{ NONE, "",             "",             "" },           /* 96 0 */
{ NONE, "",             "",             "" },           /* 97 0 */
{ NONE, "",             "",             "" },           /* 98 0 */
{ NONE, "",             "",             "" },           /* 99 0 */
{ NONE, "",             "",             "" },           /* 100 */
{ NONE, "",             "",             "" },           /* 101 */
{ NONE, "",             "",             "" },           /* 102 */
{ NONE, "",             "",             "" },           /* 103 */
{ NONE, "",             "",             "" },           /* 104 */
{ NONE, "",             "",             "" },           /* 105 */
{ NONE, "",             "",             "" },           /* 106 */
{ NONE, "",             "",             "" },           /* 107 */
{ NONE, "",             "",             "" },           /* 108 */
{ NONE, "",             "",             "" },           /* 109 */
{ NONE, "",             "",             "" },           /* 110 */
{ NONE, "",             "",             "" },           /* 111 */
{ NONE, "",             "",             "" },           /* 112 */
{ NONE, "",             "",             "" },           /* 113 */
{ NONE, "",             "",             "" },           /* 114 */
{ NONE, "",             "",             "" },           /* 115 */
{ NONE, "",             "",             "" },           /* 116 */
{ NONE, "",             "",             "" },           /* 117 */
{ NONE, "",             "",             "" },           /* 118 */
{ NONE, "",             "",             "" },           /* 119 */
{ NONE, "",             "",             "" },           /* 120 */
{ NONE, "",             "",             "" },           /* 121 */
{ NONE, "",             "",             "" },           /* 122 */
{ NONE, "",             "",             "" },           /* 123 */
{ NONE, "",             "",             "" },           /* 124 */
{ NONE, "",             "",             "" },           /* 125 */
{ NONE, "",             "",             "" },           /* 126 */
{ NONE, "",             "",             "" },           /* 127 */
};

#else /* NONUS_KBD */
        
typedef struct {
        u_short type;
        char unshift[CODE_SIZE];
        char shift[CODE_SIZE];
        char ctl[CODE_SIZE];
        char altgr[CODE_SIZE];
} Scan_def;

#ifdef GERMAN_KBD
        
static Scan_def scan_codes[] = {
{ NONE, "",     "",     "",     "" },   /* 0 unused */
{ ASCII,        "\033", "\033", "\033", "\033"}, /* 1 ESCape */
{ ASCII,        "1",    "!",    "!",    "" },   /* 2 1 */
{ ASCII,        "2",    "\"",   "\"",   "\xb2" }, /* 3 2 */
{ ASCII,        "3",    "\xa7", "\xa7", "\xb3" }, /* 4 3 */
{ ASCII,        "4",    "$",    "$",    "" },   /* 5 4 */
{ ASCII,        "5",    "%",    "%",    "" },   /* 6 5 */
{ ASCII,        "6",    "&",    "&",    "" },   /* 7 6 */
{ ASCII,        "7",    "/",    "/",    "{" },  /* 8 7 */
{ ASCII,        "8",    "(",    "(",    "[" },  /* 9 8 */
{ ASCII,        "9",    ")",    ")",    "]" },  /* 10 9 */
{ ASCII,        "0",    "=",    "=",    "}" },  /* 11 0 */
{ ASCII,        "\xdf","?",     "?",    "\\" }, /* 12 - */
{ ASCII,        "'",    "`",    "`",    "" },   /* 13 = */
{ ASCII,        "\177", "\177", "\010", "\177" }, /* 14 backspace */
{ ASCII,        "\t",   "\177\t", "\t", "\t" }, /* 15 tab */
{ ASCII,        "q",    "Q",    "\021", "@" },  /* 16 q */
{ ASCII,        "w",    "W",    "\027", "w" },  /* 17 w */
{ ASCII,        "e",    "E",    "\005", "e" },  /* 18 e */
{ ASCII,        "r",    "R",    "\022", "r" },  /* 19 r */
{ ASCII,        "t",    "T",    "\024", "t" },  /* 20 t */
{ ASCII,        "z",    "Z",    "\032", "z" },  /* 21 y */
{ ASCII,        "u",    "U",    "\025", "u" },  /* 22 u */
{ ASCII,        "i",    "I",    "\011", "i" },  /* 23 i */
{ ASCII,        "o",    "O",    "\017", "o" },  /* 24 o */
{ ASCII,        "p",    "P",    "\020", "p" },  /* 25 p */
{ ASCII,        "\xfc", "\xdc", "\xfc", "\xdc" }, /* 26 [ */
{ ASCII,        "+",    "*",    "+",    "~" },  /* 27 ] */
{ ASCII,        "\r",   "\r",   "\n",   "\r" }, /* 28 return */
{ CTL,  "",     "",     "",     "" },   /* 29 control */
{ ASCII,        "a",    "A",    "\001", "a" },  /* 30 a */
{ ASCII,        "s",    "S",    "\023", "s" },  /* 31 s */
{ ASCII,        "d",    "D",    "\004", "d" },  /* 32 d */
{ ASCII,        "f",    "F",    "\006", "f" },  /* 33 f */
{ ASCII,        "g",    "G",    "\007", "g" },  /* 34 g */
{ ASCII,        "h",    "H",    "\010", "h" },  /* 35 h */
{ ASCII,        "j",    "J",    "\n",   "j" },  /* 36 j */
{ ASCII,        "k",    "K",    "\013", "k" },  /* 37 k */
{ ASCII,        "l",    "L",    "\014", "l" },  /* 38 l */
{ ASCII,        "\xf6", "\xd6", "\xf6", "\xd6" }, /* 39 ; */
{ ASCII,        "\xe4", "\xc4", "\xe4", "\xc4" }, /* 40 ' */
{ ASCII,        "\136", "\370", "\136", "\370" }, /* 41 ` */
{ SHIFT,        "",     "",     "",     "" },   /* 42 shift */
{ ASCII,        "#",    "'",    "#",    "'" },  /* 43 \ */
{ ASCII,        "y",    "Y",    "\x19", "y" },  /* 44 z */
{ ASCII,        "x",    "X",    "\030", "x" },  /* 45 x */
{ ASCII,        "c",    "C",    "\003", "c" },  /* 46 c */
{ ASCII,        "v",    "V",    "\026", "v" },  /* 47 v */
{ ASCII,        "b",    "B",    "\002", "b" },  /* 48 b */
{ ASCII,        "n",    "N",    "\016", "n" },  /* 49 n */
{ ASCII,        "m",    "M",    "\r",   "m" },  /* 50 m */
{ ASCII,        ",",    ";",    ",",    ";" },  /* 51 , */
{ ASCII,        ".",    ":",    ".",    ":" },  /* 52 . */
{ ASCII,        "-",    "_",    "-",    "_" },  /* 53 / */
{ SHIFT,        "",     "",     "",     "" },   /* 54 shift */
{ KP,   "*",    "*",    "*",    "*" },  /* 55 kp * */
{ ALT,  "",     "",     "",     "" },   /* 56 alt */
{ ASCII,        " ",    " ",    "\000", " " },  /* 57 space */
{ CAPS, "",     "",     "",     "" },   /* 58 caps */
{ FUNC, "\033[M",       "\033[Y",       "\033[k",       "" }, /* 59 f1 */
{ FUNC, "\033[N",       "\033[Z",       "\033[l",       "" }, /* 60 f2 */
{ FUNC, "\033[O",       "\033[a",       "\033[m",       "" }, /* 61 f3 */
{ FUNC, "\033[P",       "\033[b",       "\033[n",       "" }, /* 62 f4 */
{ FUNC, "\033[Q",       "\033[c",       "\033[o",       "" }, /* 63 f5 */
{ FUNC, "\033[R",       "\033[d",       "\033[p",       "" }, /* 64 f6 */
{ FUNC, "\033[S",       "\033[e",       "\033[q",       "" }, /* 65 f7 */
{ FUNC, "\033[T",       "\033[f",       "\033[r",       "" }, /* 66 f8 */
{ FUNC, "\033[U",       "\033[g",       "\033[s",       "" }, /* 67 f9 */
{ FUNC, "\033[V",       "\033[h",       "\033[t",       "" }, /* 68 f10*/
{ NUM,  "",             "",             "",             "" }, /* 69 numlock */
{ SCROLL,       "",             "",             "",     "" }, /*70 scroll lock */
{ KP,   "7",            "\033[H",       "7",    "" },   /* 71 kp 7 */
{ KP,   "8",            "\033[A",       "8",    "" },   /* 72 kp 8 */
{ KP,   "9",            "\033[I",       "9",    "" },   /* 73 kp 9 */
{ KP,   "-",            "-",            "-",    "" },   /* 74 kp - */
{ KP,   "4",            "\033[D",       "4",    "" },   /* 75 kp 4 */
{ KP,   "5",            "\033[E",       "5",    "" },   /* 76 kp 5 */
{ KP,   "6",            "\033[C",       "6",    "" },   /* 77 kp 6 */
{ KP,   "+",            "+",            "+",    "" },   /* 78 kp + */
{ KP,   "1",            "\033[F",       "1",    "" },   /* 79 kp 1 */
{ KP,   "2",            "\033[B",       "2",    "" },   /* 80 kp 2 */
{ KP,   "3",            "\033[G",       "3",    "" },   /* 81 kp 3 */
{ KP,   "0",            "\033[L",       "0",    "" },   /* 82 kp 0 */
{ KP,   ",",            "\177",         ",",    "" },   /* 83 kp . */
{ NONE, "",             "",             "",     "" },   /* 84 0 */
{ NONE, "100",          "",             "",     "" },   /* 85 0 */
{ ASCII,        "<",            ">",            "<",    "|" },  /* 86 <> */
{ FUNC, "\033[W",       "\033[i",       "\033[u","" },  /* 87 f11 */
{ FUNC, "\033[X",       "\033[j",       "\033[v","" },  /* 88 f12 */
{ NONE, "102",          "",             "",     "" },   /* 89 0 */
{ NONE, "103",          "",             "",     "" },   /* 90 0 */
{ NONE, "",             "",             "",     "" },   /* 91 0 */
{ NONE, "",             "",             "",     "" },   /* 92 0 */
{ NONE, "",             "",             "",     "" },   /* 93 0 */
{ NONE, "",             "",             "",     "" },   /* 94 0 */
{ NONE, "",             "",             "",     "" },   /* 95 0 */
{ NONE, "",             "",             "",     "" },   /* 96 0 */
{ NONE, "",             "",             "",     "" },   /* 97 0 */
{ NONE, "",             "",             "",     "" },   /* 98 0 */
{ NONE, "",             "",             "",     "" },   /* 99 0 */
{ NONE, "",             "",             "",     "" },   /* 100 */
{ NONE, "",             "",             "",     "" },   /* 101 */
{ NONE, "",             "",             "",     "" },   /* 102 */
{ NONE, "",             "",             "",     "" },   /* 103 */
{ NONE, "",             "",             "",     "" },   /* 104 */
{ NONE, "",             "",             "",     "" },   /* 105 */
{ NONE, "",             "",             "",     "" },   /* 106 */
{ NONE, "",             "",             "",     "" },   /* 107 */
{ NONE, "",             "",             "",     "" },   /* 108 */
{ NONE, "",             "",             "",     "" },   /* 109 */
{ NONE, "",             "",             "",     "" },   /* 110 */
{ NONE, "",             "",             "",     "" },   /* 111 */
{ NONE, "",             "",             "",     "" },   /* 112 */
{ NONE, "",             "",             "",     "" },   /* 113 */
{ NONE, "",             "",             "",     "" },   /* 114 */
{ NONE, "",             "",             "",     "" },   /* 115 */
{ NONE, "",             "",             "",     "" },   /* 116 */
{ NONE, "",             "",             "",     "" },   /* 117 */
{ NONE, "",             "",             "",     "" },   /* 118 */
{ NONE, "",             "",             "",     "" },   /* 119 */
{ NONE, "",             "",             "",     "" },   /* 120 */
{ NONE, "",             "",             "",     "" },   /* 121 */
{ NONE, "",             "",             "",     "" },   /* 122 */
{ NONE, "",             "",             "",     "" },   /* 123 */
{ NONE, "",             "",             "",     "" },   /* 124 */
{ NONE, "",             "",             "",     "" },   /* 125 */
{ NONE, "",             "",             "",     "" },   /* 126 */
{ NONE, "",             "",             "",     "" }    /* 127 */
};
#endif /* GERMAN_KBD */

#ifdef NORWEGIAN_KBD
static Scan_def scan_codes[] = {
    { NONE, "", "", "", "" },   /* 0 unused */
    { ASCII,    "\033", "\033", "\033", "\033" }, /* 1 ESCape */
    { ASCII,    "1",    "!",    "", "\241" },   /* 2 1 */
    { ASCII,    "2",    "\"",   "\000", "@" },  /* 3 2 */
    { ASCII,    "3",    "#",    "", "\243" },   /* 4 3 */
    { ASCII,    "4",    "$",    "", "$" },  /* 5 4 */
    { ASCII,    "5",    "%",    "\034", "\\" }, /* 6 5 */
    { ASCII,    "6",    "&",    "\034", "|" },  /* 7 6 */
    { ASCII,    "7",    "/",    "\033", "{" },  /* 8 7 */
    { ASCII,    "8",    "(",    "\033", "[" },  /* 9 8 */
    { ASCII,    "9",    ")",    "\035", "]" },  /* 10 9 */
    { ASCII,    "0",    "=",    "\035", "}" },  /* 11 0 */
    { ASCII,    "+",    "?",    "\037", "\277" },   /* 12 - */
    { ASCII,    "\\",   "`",    "\034", "'" },  /* 13 = */
    { ASCII,    "\177", "\177", "\010", "\177" },   /* 14 backspace */
    { ASCII,    "\t",   "\177\t", "\t", "\t" }, /* 15 tab */
    { ASCII,    "q",    "Q",    "\021", "q" },  /* 16 q */
    { ASCII,    "w",    "W",    "\027", "w" },  /* 17 w */
    { ASCII,    "e",    "E",    "\005", "\353" },   /* 18 e */
    { ASCII,    "r",    "R",    "\022", "r" },  /* 19 r */
    { ASCII,    "t",    "T",    "\024", "t" },  /* 20 t */
    { ASCII,    "y",    "Y",    "\031", "y" },  /* 21 y */
    { ASCII,    "u",    "U",    "\025", "\374" },   /* 22 u */
    { ASCII,    "i",    "I",    "\011", "i" },  /* 23 i */
    { ASCII,    "o",    "O",    "\017", "\366" },   /* 24 o */
    { ASCII,    "p",    "P",    "\020", "p" },  /* 25 p */
    { ASCII,    "\345", "\305", "\334", "\374" },   /* 26 [ */
    { ASCII,    "~",    "^",    "\036", "" },   /* 27 ] */
    { ASCII,    "\r",   "\r",   "\n",   "\r" }, /* 28 return */
    { CTL,  "", "", "", "" },   /* 29 control */
    { ASCII,    "a",    "A",    "\001", "\344" },   /* 30 a */
    { ASCII,    "s",    "S",    "\023", "\337" },   /* 31 s */
    { ASCII,    "d",    "D",    "\004", "d" },  /* 32 d */
    { ASCII,    "f",    "F",    "\006", "f" },  /* 33 f */
    { ASCII,    "g",    "G",    "\007", "g" },  /* 34 g */
    { ASCII,    "h",    "H",    "\010", "h" },  /* 35 h */
    { ASCII,    "j",    "J",    "\n",   "j" },  /* 36 j */
    { ASCII,    "k",    "K",    "\013", "k" },  /* 37 k */
    { ASCII,    "l",    "L",    "\014", "l" },  /* 38 l */
    { ASCII,    "\370", "\330", "\326", "\366" },   /* 39 ; */
    { ASCII,    "\346", "\306", "\304", "\344" },   /* 40 ' */
    { ASCII,    "|",    "@",    "\034", "\247" },   /* 41 ` */
    { SHIFT,    "", "", "", "" },   /* 42 shift */
    { ASCII,    "'",    "*",    "'",    "'" },  /* 43 \ */
    { ASCII,    "z",    "Z",    "\032", "z" },  /* 44 z */
    { ASCII,    "x",    "X",    "\030", "x" },  /* 45 x */
    { ASCII,    "c",    "C",    "\003", "c" },  /* 46 c */
    { ASCII,    "v",    "V",    "\026", "v" },  /* 47 v */
    { ASCII,    "b",    "B",    "\002", "b" },  /* 48 b */
    { ASCII,    "n",    "N",    "\016", "n" },  /* 49 n */
    { ASCII,    "m",    "M",    "\015", "m" },  /* 50 m */
    { ASCII,    ",",    ";",    ",",    "," },  /* 51 , */
    { ASCII,    ".",    ":",    ".",    "." },  /* 52 . */
    { ASCII,    "-",    "_",    "\037", "-" },  /* 53 / */
    { SHIFT,    "", "", "", "" },   /* 54 shift */
    { KP,   "*",    "*",    "*",    "*" },  /* 55 kp * */
    { ALT,  "", "", "", "" },   /* 56 alt */
    { ASCII,    " ",    " ",    "\000", " " },  /* 57 space */
    { CAPS, "", "", "", "" },   /* 58 caps */
    { FUNC, "\033[M",   "\033[Y",   "\033[k",   "" }, /* 59 f1 */
    { FUNC, "\033[N",   "\033[Z",   "\033[l",   "" }, /* 60 f2 */
    { FUNC, "\033[O",   "\033[a",   "\033[m",   "" }, /* 61 f3 */
    { FUNC, "\033[P",   "\033[b",   "\033[n",   "" }, /* 62 f4 */
    { FUNC, "\033[Q",   "\033[c",   "\033[o",   "" }, /* 63 f5 */
    { FUNC, "\033[R",   "\033[d",   "\033[p",   "" }, /* 64 f6 */
    { FUNC, "\033[S",   "\033[e",   "\033[q",   "" }, /* 65 f7 */
    { FUNC, "\033[T",   "\033[f",   "\033[r",   "" }, /* 66 f8 */
    { FUNC, "\033[U",   "\033[g",   "\033[s",   "" }, /* 67 f9 */
    { FUNC, "\033[V",   "\033[h",   "\033[t",   "" }, /* 68 f10 */
    { NUM,  "",     "",     "",     "" }, /* 69 num lock */
    { SCROLL,   "",     "",     "",     "" }, /* 70 scroll lock */
    { KP,   "7",        "\033[H",   "7",    "" },   /* 71 kp 7 */
    { KP,   "8",        "\033[A",   "8",    "" },   /* 72 kp 8 */
    { KP,   "9",        "\033[I",   "9",    "" },   /* 73 kp 9 */
    { KP,   "-",        "-",        "-",    "" },   /* 74 kp - */
    { KP,   "4",        "\033[D",   "4",    "" },   /* 75 kp 4 */
    { KP,   "5",        "\033[E",   "5",    "" },   /* 76 kp 5 */
    { KP,   "6",        "\033[C",   "6",    "" },   /* 77 kp 6 */
    { KP,   "+",        "+",        "+",    "" },   /* 78 kp + */
    { KP,   "1",        "\033[F",   "1",    "" },   /* 79 kp 1 */
    { KP,   "2",        "\033[B",   "2",    "" },   /* 80 kp 2 */
    { KP,   "3",        "\033[G",   "3",    "" },   /* 81 kp 3 */
    { KP,   "0",        "\033[L",   "0",    "" },   /* 82 kp 0 */
    { KP,   ".",        "\177",     ".",    "" },   /* 83 kp . */
    { NONE, "",     "",     "", "" },   /* 84 0 */
    { NONE, "100",      "",     "", "" },   /* 85 0 */
    { ASCII,    "<",        ">",        "\273", "\253" },   /* 86 < > */
    { FUNC, "\033[W",   "\033[i",   "\033[u","" },  /* 87 f11 */
    { FUNC, "\033[X",   "\033[j",   "\033[v","" },  /* 88 f12 */
    { NONE, "102",      "",     "", "" },   /* 89 0 */
    { NONE, "103",      "",     "", "" },   /* 90 0 */
    { NONE, "",     "",     "", "" },   /* 91 0 */
    { NONE, "",     "",     "", "" },   /* 92 0 */
    { NONE, "",     "",     "", "" },   /* 93 0 */
    { NONE, "",     "",     "", "" },   /* 94 0 */
    { NONE, "",     "",     "", "" },   /* 95 0 */
    { NONE, "",     "",     "", "" },   /* 96 0 */
    { NONE, "",     "",     "", "" },   /* 97 0 */
    { NONE, "",     "",     "", "" },   /* 98 0 */
    { NONE, "",     "",     "", "" },   /* 99 0 */
    { NONE, "",     "",     "", "" },   /* 100 */
    { NONE, "",     "",     "", "" },   /* 101 */
    { NONE, "",     "",     "", "" },   /* 102 */
    { NONE, "",     "",     "", "" },   /* 103 */
    { NONE, "",     "",     "", "" },   /* 104 */
    { NONE, "",     "",     "", "" },   /* 105 */
    { NONE, "",     "",     "", "" },   /* 106 */
    { NONE, "",     "",     "", "" },   /* 107 */
    { NONE, "",     "",     "", "" },   /* 108 */
    { NONE, "",     "",     "", "" },   /* 109 */
    { NONE, "",     "",     "", "" },   /* 110 */
    { NONE, "",     "",     "", "" },   /* 111 */
    { NONE, "",     "",     "", "" },   /* 112 */
    { NONE, "",     "",     "", "" },   /* 113 */
    { NONE, "",     "",     "", "" },   /* 114 */
    { NONE, "",     "",     "", "" },   /* 115 */
    { NONE, "",     "",     "", "" },   /* 116 */
    { NONE, "",     "",     "", "" },   /* 117 */
    { NONE, "",     "",     "", "" },   /* 118 */
    { NONE, "",     "",     "", "" },   /* 119 */
    { NONE, "",     "",     "", "" },   /* 120 */
    { NONE, "",     "",     "", "" },   /* 121 */
    { NONE, "",     "",     "", "" },   /* 122 */
    { NONE, "",     "",     "", "" },   /* 123 */
    { NONE, "",     "",     "", "" },   /* 124 */
    { NONE, "",     "",     "", "" },   /* 125 */
    { NONE, "",     "",     "", "" },   /* 126 */
    { NONE, "",     "",     "", "" }    /* 127 */
};
#endif /* NORWEGIAN_KBD */

#ifdef FINNISH_KBD
static Scan_def scan_codes[] = {
    { NONE, "", "", "", "" },   /* 0 unused */
    { ASCII,    "\033", "\033", "\033", "\033" },   /* 1 ESCape */
    { ASCII,    "1",    "!",    "", "\241" },   /* 2 1 */
    { ASCII,    "2",    "\"",   "\000", "@" },  /* 3 2 */
    { ASCII,    "3",    "#",    "", "\243" },   /* 4 3 */
    { ASCII,    "4",    "$",    "", "$" },  /* 5 4 */
    { ASCII,    "5",    "%",    "\034", "%" },  /* 6 5 */
    { ASCII,    "6",    "&",    "\034", "&" },  /* 7 6 */
    { ASCII,    "7",    "/",    "\033", "{" },  /* 8 7 */
    { ASCII,    "8",    "(",    "\033", "[" },  /* 9 8 */
    { ASCII,    "9",    ")",    "\035", "]" },  /* 10 9 */
    { ASCII,    "0",    "=",    "\035", "}" },  /* 11 0 */
    { ASCII,    "+",    "?",    "\037", "\\" }, /* 12 - */
    { ASCII,    "'",    "`",    "\034", "'" },  /* 13 = */
    { ASCII,    "\177", "\177", "\010", "\177" },   /* 14 backspace */
    { ASCII,    "\t",   "\177\t", "\t", "\t" }, /* 15 tab */
    { ASCII,    "q",    "Q",    "\021", "q" },  /* 16 q */
    { ASCII,    "w",    "W",    "\027", "w" },  /* 17 w */
    { ASCII,    "e",    "E",    "\005", "\353" },   /* 18 e */
    { ASCII,    "r",    "R",    "\022", "r" },  /* 19 r */
    { ASCII,    "t",    "T",    "\024", "t" },  /* 20 t */
    { ASCII,    "y",    "Y",    "\031", "y" },  /* 21 y */
    { ASCII,    "u",    "U",    "\025", "\374" },   /* 22 u */
    { ASCII,    "i",    "I",    "\011", "i" },  /* 23 i */
    { ASCII,    "o",    "O",    "\017", "\366" },   /* 24 o */
    { ASCII,    "p",    "P",    "\020", "p" },  /* 25 p */
    { ASCII,    "\345", "\305", "\035", "}" },  /* 26 [ */
    { ASCII,    "~",    "^",    "\036", "~" },  /* 27 ] */
    { ASCII,    "\r",   "\r",   "\n",   "\r" }, /* 28 return */
    { CTL,  "", "", "", "" },   /* 29 control */
    { ASCII,    "a",    "A",    "\001", "\344" },   /* 30 a */
    { ASCII,    "s",    "S",    "\023", "\337" },   /* 31 s */
    { ASCII,    "d",    "D",    "\004", "d" },  /* 32 d */
    { ASCII,    "f",    "F",    "\006", "f" },  /* 33 f */
    { ASCII,    "g",    "G",    "\007", "g" },  /* 34 g */
    { ASCII,    "h",    "H",    "\010", "h" },  /* 35 h */
    { ASCII,    "j",    "J",    "\n",   "j" },  /* 36 j */
    { ASCII,    "k",    "K",    "\013", "k" },  /* 37 k */
    { ASCII,    "l",    "L",    "\014", "l" },  /* 38 l */
    { ASCII,    "\366", "\326", "\034", "|" },  /* 39 ; */
    { ASCII,    "\344", "\304", "\033", "{" },  /* 40 ' */
    { ASCII,    "\247", "\275", "\000", "@" },  /* 41 ` */
    { SHIFT,    "", "", "", "" },   /* 42 shift */
    { ASCII,    "'",    "*",    "'",    "'" },  /* 43 \ */
    { ASCII,    "z",    "Z",    "\032", "z" },  /* 44 z */
    { ASCII,    "x",    "X",    "\030", "x" },  /* 45 x */
    { ASCII,    "c",    "C",    "\003", "c" },  /* 46 c */
    { ASCII,    "v",    "V",    "\026", "v" },  /* 47 v */
    { ASCII,    "b",    "B",    "\002", "b" },  /* 48 b */
    { ASCII,    "n",    "N",    "\016", "n" },  /* 49 n */
    { ASCII,    "m",    "M",    "\015", "m" },  /* 50 m */
    { ASCII,    ",",    ";",    ",",    "," },  /* 51 , */
    { ASCII,    ".",    ":",    ".",    "." },  /* 52 . */
    { ASCII,    "-",    "_",    "\037", "-" },  /* 53 / */
    { SHIFT,    "", "", "", "" },   /* 54 shift */
    { KP,   "*",    "*",    "*",    "*" },  /* 55 kp * */
    { ALT,  "", "", "", "" },   /* 56 alt */
    { ASCII,    " ",    " ",    "\000", " " },  /* 57 space */
    { CAPS, "", "", "", "" },   /* 58 caps */
    { FUNC, "\033[M",   "\033[Y",   "\033[k",   "" },   /* 59 f1 */
    { FUNC, "\033[N",   "\033[Z",   "\033[l",   "" },   /* 60 f2 */
    { FUNC, "\033[O",   "\033[a",   "\033[m",   "" },   /* 61 f3 */
    { FUNC, "\033[P",   "\033[b",   "\033[n",   "" },   /* 62 f4 */
    { FUNC, "\033[Q",   "\033[c",   "\033[o",   "" },   /* 63 f5 */
    { FUNC, "\033[R",   "\033[d",   "\033[p",   "" },   /* 64 f6 */
    { FUNC, "\033[S",   "\033[e",   "\033[q",   "" },   /* 65 f7 */
    { FUNC, "\033[T",   "\033[f",   "\033[r",   "" },   /* 66 f8 */
    { FUNC, "\033[U",   "\033[g",   "\033[s",   "" },   /* 67 f9 */
    { FUNC, "\033[V",   "\033[h",   "\033[t",   "" },   /* 68 f10 */
    { NUM,  "",     "",     "",     "" },   /* 69 num lock */
    { SCROLL,   "",     "",     "",     "" },   /* 70 scroll lock */
    { KP,   "7",        "\033[H",   "7",    "" },   /* 71 kp 7 */
    { KP,   "8",        "\033[A",   "8",    "" },   /* 72 kp 8 */
    { KP,   "9",        "\033[I",   "9",    "" },   /* 73 kp 9 */
    { KP,   "-",        "-",        "-",    "" },   /* 74 kp - */
    { KP,   "4",        "\033[D",   "4",    "" },   /* 75 kp 4 */
    { KP,   "5",        "\033[E",   "5",    "" },   /* 76 kp 5 */
    { KP,   "6",        "\033[C",   "6",    "" },   /* 77 kp 6 */
    { KP,   "+",        "+",        "+",    "" },   /* 78 kp + */
    { KP,   "1",        "\033[F",   "1",    "" },   /* 79 kp 1 */
    { KP,   "2",        "\033[B",   "2",    "" },   /* 80 kp 2 */
    { KP,   "3",        "\033[G",   "3",    "" },   /* 81 kp 3 */
    { KP,   "0",        "\033[L",   "0",    "" },   /* 82 kp 0 */
    { KP,   ".",        "\177",     ".",    "" },   /* 83 kp . */
    { NONE, "",     "",     "", "" },   /* 84 0 */
    { NONE, "100",      "",     "", "" },   /* 85 0 */
    { ASCII,    "<",        ">",        "<",    "|" },  /* 86 < > */
    { FUNC, "\033[W",   "\033[i",   "\033[u","" },  /* 87 f11 */
    { FUNC, "\033[X",   "\033[j",   "\033[v","" },  /* 88 f12 */
    { NONE, "102",      "",     "", "" },   /* 89 0 */
    { NONE, "103",      "",     "", "" },   /* 90 0 */
    { NONE, "",     "",     "", "" },   /* 91 0 */
    { NONE, "",     "",     "", "" },   /* 92 0 */
    { NONE, "",     "",     "", "" },   /* 93 0 */
    { NONE, "",     "",     "", "" },   /* 94 0 */
    { NONE, "",     "",     "", "" },   /* 95 0 */
    { NONE, "",     "",     "", "" },   /* 96 0 */
    { NONE, "",     "",     "", "" },   /* 97 0 */
    { NONE, "",     "",     "", "" },   /* 98 0 */
    { NONE, "",     "",     "", "" },   /* 99 0 */
    { NONE, "",     "",     "", "" },   /* 100 */
    { NONE, "",     "",     "", "" },   /* 101 */
    { NONE, "",     "",     "", "" },   /* 102 */
    { NONE, "",     "",     "", "" },   /* 103 */
    { NONE, "",     "",     "", "" },   /* 104 */
    { NONE, "",     "",     "", "" },   /* 105 */
    { NONE, "",     "",     "", "" },   /* 106 */
    { NONE, "",     "",     "", "" },   /* 107 */
    { NONE, "",     "",     "", "" },   /* 108 */
    { NONE, "",     "",     "", "" },   /* 109 */
    { NONE, "",     "",     "", "" },   /* 110 */
    { NONE, "",     "",     "", "" },   /* 111 */
    { NONE, "",     "",     "", "" },   /* 112 */
    { NONE, "",     "",     "", "" },   /* 113 */
    { NONE, "",     "",     "", "" },   /* 114 */
    { NONE, "",     "",     "", "" },   /* 115 */
    { NONE, "",     "",     "", "" },   /* 116 */
    { NONE, "",     "",     "", "" },   /* 117 */
    { NONE, "",     "",     "", "" },   /* 118 */
    { NONE, "",     "",     "", "" },   /* 119 */
    { NONE, "",     "",     "", "" },   /* 120 */
    { NONE, "",     "",     "", "" },   /* 121 */
    { NONE, "",     "",     "", "" },   /* 122 */
    { NONE, "",     "",     "", "" },   /* 123 */
    { NONE, "",     "",     "", "" },   /* 124 */
    { NONE, "",     "",     "", "" },   /* 125 */
    { NONE, "",     "",     "", "" },   /* 126 */
    { NONE, "",     "",     "", "" },   /* 127 */
};
#endif /* FINNISH_KBD */

#ifdef FRENCH_KBD
    
static Scan_def scan_codes[] = {
    { NONE,     "", "", "", "" },   /* 0 unused */
    { ASCII,    "\033", "\033", "\033", "\033" }, /* 1 ESCape */
    { ASCII,    "&",    "1",    "&",    "" },   /* 2 1 */
    { ASCII,    "\351", "2",    "\211", "~" },  /* 3 2 */
    { ASCII,    "\"",   "3",    "\"",   "#" },  /* 4 3 */
    { ASCII,    "'",    "4",    "'",    "{" },  /* 5 4 */
    { ASCII,    "(",    "5",    "(",    "[" },  /* 6 5 */
    { ASCII,    "-",    "6",    "-",    "|" },  /* 7 6 */
    { ASCII,    "\350", "7",    "\210", "`" },  /* 8 7 */
    { ASCII,    "_",    "8",    "\037", "\\" }, /* 9 8 */
    { ASCII,    "\347", "9",    "\207", "^" },  /* 10 9 */
    { ASCII,    "\340", "0",    "\340", "@" },  /* 11 0 */
    { ASCII,    ")",    "\260", ")",    "]" },  /* 12 - */
    { ASCII,    "=",    "+",    "+",    "}" },  /* 13 = */
    { ASCII,    "\177", "\177", "\010", "\177" }, /* 14 backspace */
    { ASCII,    "\t",   "\177\t", "\t", "\t" }, /* 15 tab */
    { ASCII,    "a",    "A",    "\001", "a" },  /* 16 q */
    { ASCII,    "z",    "Z",    "\032", "z" },  /* 17 w */
    { ASCII,    "e",    "E",    "\005", "e" },  /* 18 e */
    { ASCII,    "r",    "R",    "\022", "r" },  /* 19 r */
    { ASCII,    "t",    "T",    "\024", "t" },  /* 20 t */
    { ASCII,    "y",    "Y",    "\031", "y" },  /* 21 y */
    { ASCII,    "u",    "U",    "\025", "u" },  /* 22 u */
    { ASCII,    "i",    "I",    "\011", "i" },  /* 23 i */
    { ASCII,    "o",    "O",    "\017", "o" },  /* 24 o */
    { ASCII,    "p",    "P",    "\020", "p" },  /* 25 p */
    { NONE,     "", "", "", "" },   /* 26 [ */
    { ASCII,    "$",    "\243", "$",    "$" },  /* 27 ] */
    { ASCII,    "\r",   "\r",   "\n",   "\r" }, /* 28 return */
    { CTL,      "", "", "", "" },   /* 29 control */
    { ASCII,    "q",    "Q",    "\021", "q" },  /* 30 a */
    { ASCII,    "s",    "S",    "\023", "s" },  /* 31 s */
    { ASCII,    "d",    "D",    "\004", "d" },  /* 32 d */
    { ASCII,    "f",    "F",    "\006", "f" },  /* 33 f */
    { ASCII,    "g",    "G",    "\007", "g" },  /* 34 g */
    { ASCII,    "h",    "H",    "\010", "h" },  /* 35 h */
    { ASCII,    "j",    "J",    "\n",   "j" },  /* 36 j */
    { ASCII,    "k",    "K",    "\013", "k" },  /* 37 k */
    { ASCII,    "l",    "L",    "\014", "l" },  /* 38 l */
    { ASCII,    "m",    "M",    "\r",   "m" },  /* 39 ; */
    { ASCII,    "\371", "%",    "\231", "\371" }, /* 40 ' */
    { ASCII,    "\262", "", "\262", "\262" }, /* 41 ` */
    { SHIFT,    "", "", "", "" },   /* 42 shift */
    { ASCII,    "*",    "\265", "*",    "*" },  /* 43 \ */
    { ASCII,    "w",    "W",    "\027", "w" },  /* 44 z */
    { ASCII,    "x",    "X",    "\030", "x" },  /* 45 x */
    { ASCII,    "c",    "C",    "\003", "c" },  /* 46 c */
    { ASCII,    "v",    "V",    "\026", "v" },  /* 47 v */
    { ASCII,    "b",    "B",    "\002", "b" },  /* 48 b */
    { ASCII,    "n",    "N",    "\016", "n" },  /* 49 n */
    { ASCII,    ",",    "?",    ",",    "," },  /* 50 m */
    { ASCII,    ";",    ".",    ";",    ";" },  /* 51 , */
    { ASCII,    ":",    "/",    "\037", ":" },  /* 52 . */
    { ASCII,    "!",    "\266", "!",    "!" },  /* 53 / */
    { SHIFT,    "", "", "", "" },   /* 54 shift */
    { KP,       "*",    "*",    "*",    "*" },  /* 55 kp * */
    { ALT,      "", "", "", "" },   /* 56 alt */
    { ASCII,    " ",    " ",    "\000", " " },  /* 57 space */
    { CAPS, "", "", "", "" },   /* 58 caps */
    { FUNC, "\033[M",   "\033[Y",   "\033[k",   "" }, /* 59 f1 */
    { FUNC, "\033[N",   "\033[Z",   "\033[l",   "" }, /* 60 f2 */
    { FUNC, "\033[O",   "\033[a",   "\033[m",   "" }, /* 61 f3 */
    { FUNC, "\033[P",   "\033[b",   "\033[n",   "" }, /* 62 f4 */
    { FUNC, "\033[Q",   "\033[c",   "\033[o",   "" }, /* 63 f5 */
    { FUNC, "\033[R",   "\033[d",   "\033[p",   "" }, /* 64 f6 */
    { FUNC, "\033[S",   "\033[e",   "\033[q",   "" }, /* 65 f7 */
    { FUNC, "\033[T",   "\033[f",   "\033[r",   "" }, /* 66 f8 */
    { FUNC, "\033[U",   "\033[g",   "\033[s",   "" }, /* 67 f9 */
    { FUNC, "\033[V",   "\033[h",   "\033[t",   "" }, /* 68 f10 */
    { NUM,  "",     "",     "",     "" }, /* 69 num lock */
    { SCROLL,   "",     "",     "",     "" }, /* 70 scroll lock */
    { KP,   "7",        "\033[H",   "7",    "" },   /* 71 kp 7 */
    { KP,   "8",        "\033[A",   "8",    "" },   /* 72 kp 8 */
    { KP,   "9",        "\033[I",   "9",    "" },   /* 73 kp 9 */
    { KP,   "-",        "-",        "-",    "" },   /* 74 kp - */
    { KP,   "4",        "\033[D",   "4",    "" },   /* 75 kp 4 */
    { KP,   "5",        "\033[E",   "5",    "" },   /* 76 kp 5 */
    { KP,   "6",        "\033[C",   "6",    "" },   /* 77 kp 6 */
    { KP,   "+",        "+",        "+",    "" },   /* 78 kp + */
    { KP,   "1",        "\033[F",   "1",    "" },   /* 79 kp 1 */
    { KP,   "2",        "\033[B",   "2",    "" },   /* 80 kp 2 */
    { KP,   "3",        "\033[G",   "3",    "" },   /* 81 kp 3 */
    { KP,   "0",        "\033[L",   "0",    "" },   /* 82 kp 0 */
    { KP,   ".",        "\177",     ".",    "" },   /* 83 kp . */
    { NONE, "",     "",     "", "" },   /* 84 0 */
    { NONE, "100",      "",     "", "" },   /* 85 0 */
    { ASCII,    "<",        ">",        "<",    "<" },  /* 86 < > */
    { FUNC, "\033[W",   "\033[i",   "\033[u","" },  /* 87 f11 */
    { FUNC, "\033[X",   "\033[j",   "\033[v","" },  /* 88 f12 */
    { NONE, "102",      "",     "", "" },   /* 89 0 */
    { NONE, "103",      "",     "", "" },   /* 90 0 */
    { NONE, "",     "",     "", "" },   /* 91 0 */
    { NONE, "",     "",     "", "" },   /* 92 0 */
    { NONE, "",     "",     "", "" },   /* 93 0 */
    { NONE, "",     "",     "", "" },   /* 94 0 */
    { NONE, "",     "",     "", "" },   /* 95 0 */
    { NONE, "",     "",     "", "" },   /* 96 0 */
    { NONE, "",     "",     "", "" },   /* 97 0 */
    { NONE, "",     "",     "", "" },   /* 98 0 */
    { NONE, "",     "",     "", "" },   /* 99 0 */
    { NONE, "",     "",     "", "" },   /* 100 */
    { NONE, "",     "",     "", "" },   /* 101 */
    { NONE, "",     "",     "", "" },   /* 102 */
    { NONE, "",     "",     "", "" },   /* 103 */
    { NONE, "",     "",     "", "" },   /* 104 */
    { NONE, "",     "",     "", "" },   /* 105 */
    { NONE, "",     "",     "", "" },   /* 106 */
    { NONE, "",     "",     "", "" },   /* 107 */
    { NONE, "",     "",     "", "" },   /* 108 */
    { NONE, "",     "",     "", "" },   /* 109 */
    { NONE, "",     "",     "", "" },   /* 110 */
    { NONE, "",     "",     "", "" },   /* 111 */
    { NONE, "",     "",     "", "" },   /* 112 */
    { NONE, "",     "",     "", "" },   /* 113 */
    { NONE, "",     "",     "", "" },   /* 114 */
    { NONE, "",     "",     "", "" },   /* 115 */
    { NONE, "",     "",     "", "" },   /* 116 */
    { NONE, "",     "",     "", "" },   /* 117 */
    { NONE, "",     "",     "", "" },   /* 118 */
    { NONE, "",     "",     "", "" },   /* 119 */
    { NONE, "",     "",     "", "" },   /* 120 */
    { NONE, "",     "",     "", "" },   /* 121 */
    { NONE, "",     "",     "", "" },   /* 122 */
    { NONE, "",     "",     "", "" },   /* 123 */
    { NONE, "",     "",     "", "" },   /* 124 */
    { NONE, "",     "",     "", "" },   /* 125 */
    { NONE, "",     "",     "", "" },   /* 126 */
    { NONE, "",     "",     "", "" }    /* 127 */
};

#endif /* FRENCH_KBD */


/*
 * XXXX Add tables for other keyboards here
 */
        
#endif /* NONUS_KBD */

/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**    xinterpret
**
**    This routine interprets a read scan code according to what
**    Xserver requires. 
**
**  FORMAL PARAMETERS:
**
**    sc   - Input : The console softc structure.
**    dt   - Input : The scan code read from the keyboard.
**
**  IMPLICIT INPUTS:
**
**
**  IMPLICIT OUTPUTS:
**
**     none.
**
**  FUNCTION VALUE:
**
**     A pointer to the converted characters read from the keyboard   
**     NULL if no characters read
**
**  SIDE EFFECTS:
**
**     A sleeper waiting on kbd.sc_new_lock_state may be woken up if the
**     SCROLL key has been depressed.
**--
*/
char * 
xinterpret(struct pc_softc  *sc,
           u_char           dt)
{
    u_char            dt_make;
    static u_char     capchar[2];
    
#if defined(DDB) && defined(XSERVER_DDB)
    /* F12 enters the debugger while in X mode */
    if (dt == 88)
    {
        Debugger();
    }
#endif
    capchar[0] = dt;
    capchar[1] = 0;
    /*
    ** Check for locking keys.
    **
    ** XXX Setting the LEDs this way is a bit bogus.  What if the
    ** keyboard has been remapped in X?
    */
    dt_make = dt & ~BREAKBIT;
    switch (scan_codes[dt_make].type) 
    {
        case NUM:
        case CAPS:
        case SCROLL:
            if (dt & BREAKBIT) 
            {
                /* This is the break code, reset kbd.sc_shift_state and
                ** exit.
                */
                sc->kbd.sc_shift_state &= ~(scan_codes[dt_make].type);
            }
            else if ( (sc->kbd.sc_shift_state & scan_codes[dt_make].type) == 0 )
            {
                /* This is NOT a repeat of an already recorded 
                ** kbd.sc_shift_state, so record it now and update.
                */
                sc->kbd.sc_shift_state     |= scan_codes[dt_make].type;
                sc->kbd.sc_new_lock_state  ^= scan_codes[dt_make].type;
                if ( (scan_codes[dt_make].type == SCROLL) &&
                    ((sc->kbd.sc_new_lock_state & SCROLL) == 0) )
                {
                    /* For scroll we also have to wake up sleepers.
                    */
                    wakeup(&(sc->kbd.sc_new_lock_state));
                }
                async_update(sc, false);
            }
        break;
    } /* End switch on scan code type */

    return capchar;
} /* End xinterpret */



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     sget
**
**    This routine reads a character from the keyboard and 
**    converts it using the above scan code tables.     
**
**  FORMAL PARAMETERS:
**
**     iot    I/O tag for the mapped register space  
**     ioh    I/O handle for the mapped register space
**
**  IMPLICIT INPUTS:
**
**     None.
**
**  IMPLICIT OUTPUTS:
**
**     ack   - received ack from keyboard
**     nak   - received nak from keyboard
**
**  FUNCTION VALUE:
**
**     A pointer to the converted characters read from the keyboard   
**     NULL if no characters read
**
**  SIDE EFFECTS:
**
**--
*/
char *
sget(struct pc_softc *sc)
{
    u_char             dt = 0;	/* XXX */
    u_char             dt_make;

    u_char             status;
    static u_char      extended = 0;
    static u_char      capchar[2];
    bus_space_tag_t    iot = sc->kbd.sc_iot;
    bus_space_handle_t ioh = sc->kbd.sc_ioh;
    char               *returnValue;
    
    returnValue = NULL;

    /* Grab the next byte of keyboard data.  There is no point in looping
    ** here since the controller only puts one byte at a time in the 
    ** output buffer and we are LONG gone before it has time to put the
    ** next byte in after our initial read (at least on a StrongARM we are).
    */
    I8042_GETKBD_DATA(iot, ioh, status, dt);
    
    if (status)
    {    
        if (sc->sc_flags & SC_XMODE) 
        {
            returnValue = xinterpret(sc, dt);
        }       
        else if (dt == KBR_EXTENDED)
        {
            extended = true;
        }
        else
        {
#ifdef DDB
            /*
            ** Check for cntl-alt-esc.
            */
            if ((dt == 1) && 
		(sc->kbd.sc_shift_state & (CTL | ALT)) == (CTL | ALT)) 
            {
                Debugger();
                dt |= 0x80;     /* discard esc (ddb discarded ctl-alt) */
            }
#endif
#ifdef	SHARK
	    if ((dt == 127) && 
		(sc->kbd.sc_shift_state & (CTL | ALT)) == (CTL | ALT)) 
	    {
		cpu_reboot(0, NULL);
	    }
#endif

            /*
            ** Record make sequence regardless of what dt is since the make 
            ** code is the key to the scan table.
            */
#ifdef NUMERIC_SLASH_FIX
/* fix numeric / on non US keyboard */
            if (extended && dt == 53)
            {
                capchar[0] = '/';
                extended = 0;
                return (capchar);
            }
#endif
            dt_make = dt & ~BREAKBIT;
            switch (scan_codes[dt_make].type) 
            {
                /*
                ** locking keys
                */
                case NUM:
                case CAPS:
                case SCROLL:
                    if (dt & BREAKBIT) 
                    {
                        /* This is the break code, reset kbd.sc_shift_state and
                        ** exit.
                        */
                        sc->kbd.sc_shift_state &= ~(scan_codes[dt_make].type);
                    }
                    else if ((sc->kbd.sc_shift_state &
			      scan_codes[dt_make].type) == 0)
                    {
                        /* This isn't a repeat of an already registered
                        ** shift function key. So record it now.
                        */
                        sc->kbd.sc_shift_state    |= scan_codes[dt_make].type;
                        sc->kbd.sc_new_lock_state ^= scan_codes[dt_make].type;
                        if ( (scan_codes[dt_make].type == SCROLL) &&
                            ((sc->kbd.sc_new_lock_state & SCROLL) == 0) )
                        {
                            /* For scroll we also have to wake up sleepers.
                            */
                            wakeup(&(sc->kbd.sc_new_lock_state));
                        }
                        /* Update external view of what happened.
                        */
                        async_update(sc, false);
                    }
                break;
                /*
                ** non-locking keys. Just record in our shift_state.
                */
#ifdef NONUS_KBD  
                case ALT:
                    if (dt & BREAKBIT) 
                    {
                        /* This is the break code, reset shift_state and
                        ** exit.
                        */
                        u_short type = extended ? ALTGR : ALT;
                        sc->kbd.sc_shift_state &= ~type;
                    }
                    else
                    {
                        u_short type = extended ? ALTGR : ALT;
                        sc->kbd.sc_shift_state |= type;
                    }
                break;
#else
                case ALT:
#endif
                case SHIFT:
                case CTL:
                    if (dt & BREAKBIT) 
                    {
                        /* This is the break code, reset shift_state and
                        ** exit.
                        */
                        sc->kbd.sc_shift_state &= ~(scan_codes[dt_make].type);
                    }
                    else
                    {
                        sc->kbd.sc_shift_state |= scan_codes[dt_make].type;
                    }
                break;
                case ASCII:
                    if ((dt & BREAKBIT) == 0)
                    {
#ifdef NONUS_KBD
                        if (sc->kbd.sc_shift_state & ALTGR)
                        {
                            capchar[0] = scan_codes[dt_make].altgr[0];
                            if (sc->kbd.sc_shift_state & CTL)
                            {
                                capchar[0] &= 0x1f;
                            }
                        } else
#endif
                        /* control has highest priority 
                        */
                        if (sc->kbd.sc_shift_state & CTL)
                        {
                            capchar[0] = scan_codes[dt_make].ctl[0];
                        }
                        else if (sc->kbd.sc_shift_state & SHIFT)
                        {
                            capchar[0] = scan_codes[dt_make].shift[0];
                        }
                        else
                        {
                            capchar[0] = scan_codes[dt_make].unshift[0];
                        }
                        if ((sc->kbd.sc_new_lock_state & CAPS) && 
                            capchar[0] >= 'a' && capchar[0] <= 'z') 
                        {
                            capchar[0] -= ('a' - 'A');
                        }
                        capchar[0] |= (sc->kbd.sc_shift_state & ALT);
                        extended = 0;
                        returnValue = capchar;
                    }
                break;
                case NONE:
                break;
                case FUNC: 
                    if ((dt & BREAKBIT) == 0)
                    {
                        if (sc->kbd.sc_shift_state & SHIFT)
                        {
                            returnValue = scan_codes[dt_make].shift;
                        }
                        else if (sc->kbd.sc_shift_state & CTL)
                        {
                            returnValue = scan_codes[dt_make].ctl;
                        }
                        else
                        {
                            returnValue = scan_codes[dt_make].unshift;
                        }
                        extended = 0;
                    }
                break;
                case KP: 
                    if ((dt & BREAKBIT) == 0)
                    {
                        if (sc->kbd.sc_shift_state & (SHIFT | CTL) ||
                            (sc->kbd.sc_new_lock_state & NUM) == 0 || extended)
                        {
                            returnValue = scan_codes[dt_make].shift;
                        }
                        else
                        {
                            returnValue = scan_codes[dt_make].unshift;
                        }
                        extended = 0;
                    }
                break;
            } /* End switch on scan code type */
            extended = 0;
        } /* End else not in Xmode and not an extended scan code indication */
    } /* End if keyboard data found */
    
    return (returnValue);
} /* End sget() */


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     pcmmap
**
**    This function returns the page where a specified offset into the 
**    video memory occurs. 
**
**  FORMAL PARAMETERS:
**
**     dev      - device identifier consisting of major and minor numbers.
**     offset   - offset into video memory
*      nprot    - unused
**
**  IMPLICIT INPUTS:
**
**     0xa0000 - Base address of screeen buffer 
**
**  IMPLICIT OUTPUTS:
**
**     none.
**
**  FUNCTION VALUE:
**
**       The page that the offset into video memory is located at
**   -1  Couldn't map offset
**
**--
*/
paddr_t
pcmmap(dev_t   dev, 
       off_t   offset, 
       int     nprot)
{
#ifdef SHARK
    paddr_t pam_io_data;
    vaddr_t vam_mem_data;

    if (offset < 0)
	return (-1);

    if(offset >> 24 == displayInfo(paddr) >> 24)
    {
	/* Display memory - allow any address since we
	** don't know the size
	*/
	return arm_btop(offset);
    }

    pam_io_data  = vtophys(isa_io_data_vaddr());
    vam_mem_data = isa_mem_data_vaddr();

    if(offset >> 24 == pam_io_data >> 24)
    {
	/* IO space - Should not allow addresses outside of the 
	   VGA registers */
	if(offset >= ((pam_io_data + displayInfo(ioBase)) & L2_S_FRAME) &&
	   offset <= pam_io_data + displayInfo(ioBase)
	   + displayInfo(ioLen))
	{
	    return arm_btop(offset);
	}
	
    }
    else if(offset >> 24 == vtophys(vam_mem_data) >> 24)
    {
	/* Memory - Allow 0xA0000 to 0xBFFFF */
	if(offset >= ((vtophys(vam_mem_data) + VGA_BUF) & L2_S_FRAME) &&
	   offset <= vtophys(vam_mem_data) + VGA_BUF
	   + VGA_BUF_LEN)
	{
	    return arm_btop(offset);
	}
    }
    return -1;
    
#else /* not SHARK */
    if (offset > 0x20000)
    {
	return -1;
    }
    return arm_btop(0xa0000 + offset);
#endif
} /* End pcmmap() */


#ifdef	SHARK
static int _shark_screen_ihandle = 0;
static int _shark_screen_is_stdout = 0;

int
get_shark_screen_ihandle()
{
	int chosen_phandle;
	int stdout_ihandle, stdout_phandle;
	char buf[128];

	if (_shark_screen_ihandle != 0)
		goto out;

	/*
	 * Find out whether the firmware's chosen stdout is
	 * a display.  If so, use the existing ihandle so the firmware
	 * doesn't become Unhappy.  If not, just open it.
	 */
	if ((chosen_phandle = OF_finddevice("/chosen")) == -1 ||
	    OF_getprop(chosen_phandle, "stdout", &stdout_ihandle, 
		       sizeof(stdout_ihandle)) != sizeof(stdout_ihandle)) {
		goto notstdout;
	}
	stdout_ihandle = of_decode_int((unsigned char *)&stdout_ihandle);
	if ((stdout_phandle = OF_instance_to_package(stdout_ihandle)) == -1 ||
	    OF_getprop(stdout_phandle, "device_type", buf, sizeof(buf)) <= 0) {
		goto notstdout;
	}
	if (strcmp(buf, "display") == 0) {
		/* The display is the standard output device. */
		_shark_screen_ihandle = stdout_ihandle;
		_shark_screen_is_stdout = 1;
		goto out;
	}

notstdout:
	if ((_shark_screen_ihandle = OF_open("screen")) == 0)
		_shark_screen_ihandle = -1;

out:
	return (_shark_screen_ihandle);
}

#define OFW_PUTC(cp)						\
    do {							\
	if (*(cp) != '\n')					\
		(void)OF_write(_shark_screen_ihandle, (cp), 1);	\
	else							\
		(void)OF_write(_shark_screen_ihandle, "\r\n", 2);\
    } while (0)

void
shark_screen_cleanup(redisplay)
	int redisplay;
{
	char *cp;

	if (_shark_screen_ihandle != -1 && _shark_screen_ihandle != 0) {
		/*
		 * reset the screen.  If we had to open the screen device,
		 * close it, too.
		 *
		 * If we didn't have to open the screen, that means that
		 * it's the console's stdout.  If we're told to (e.g.
		 * when halting), write the contents of the message buffer
		 * to it, so all debugging info isn't lost.
		 */
		(void)OF_call_method("install", _shark_screen_ihandle, 0, 0);
		(void)OF_write(_shark_screen_ihandle, "\f", 1);
		if (!_shark_screen_is_stdout) {
			OF_close(_shark_screen_ihandle);
		} else if (redisplay && msgbufmapped &&
		    msgbufp->msg_magic == MSG_MAGIC) {
			for (cp = &msgbufp->msg_bufc[msgbufp->msg_bufx];
			    cp < &msgbufp->msg_bufc[msgbufp->msg_bufx];
			    cp++) {
				OFW_PUTC(cp);
			}
			for (cp = &msgbufp->msg_bufc[0];
			    cp < &msgbufp->msg_bufc[msgbufp->msg_bufx];
			    cp++) {
				OFW_PUTC(cp);
			}
		}
	}
}
#undef OFW_PUTC

/* Throw the CT65550 into VGA mode. */
static void
force_vga_mode(void)
{
    int ihandle;

    if ((ihandle = get_shark_screen_ihandle()) == -1) {
	printf("pccons: Unable to open PC screen device");
    } else {
	if (OF_call_method("text-mode3", ihandle, 0, 0) != 0) {
	    printf("pccons: Text-mode3 method invocation on PC "
		   "screen device failed\n");
	}
    }
    return;
} /* End force_vga_mode() */

/* Get information from OFW about the display device */
static void
getDisplayInfo(struct display_info *displayInfP)
{
    static char CharSet_store[64];		/* XXX */
    static struct regspec DisplayRegs_store[8];	/* XXX */

    struct regspec *displayRegs;
    int regSize;
    u_int nReg, r;
    u_int ioStart, ioEnd, memStart, memEnd, *startp, *endp;
    int ihandle, phandle;
    unsigned long tempval;
    
    if ((ihandle = get_shark_screen_ihandle()) != -1
       && (phandle = OF_instance_to_package(ihandle)) != -1)
    {
	displayInfP->init = true;
	
	/* Linear frame buffer virtual and physical address */
	if (OF_getprop(phandle, "address", &tempval, 
		       sizeof(vaddr_t)) > 0)
	{
	    vaddr_t vaddr;

	    vaddr = (vaddr_t)of_decode_int((unsigned char *)&tempval);
	    /* translation must be done by OFW, because we have not yet
	       called ofw_configmem and switched over to netBSD memory
	       manager */
	    displayInfP->paddr = ofw_gettranslation(vaddr);
	}
	else
	{
	    displayInfP->paddr = (paddr_t)(-1);
	}

	/* Bytes per scan line */
	if (OF_getprop(phandle, "linebytes", &tempval, sizeof(int)) > 0)
	{
	    displayInfP->linebytes = of_decode_int((unsigned char *)&tempval);
	}
	else
	{
	    displayInfP->linebytes = -1;
	}

	/* Colour depth - bits per pixel */
	if (OF_getprop(phandle, "depth", &tempval, sizeof(int)) > 0)
	{
	    displayInfP->depth = of_decode_int((unsigned char *)&tempval);
	}
	else
	{
	    displayInfP->depth = -1;
	}

	/* Display height - pixels */
	if (OF_getprop(phandle, "height", &tempval, sizeof(int)) > 0)
	{
	    displayInfP->height = of_decode_int((unsigned char *)&tempval);
	}
	else
	{
	    displayInfP->height = -1;
	}

	/* Display width - pixels */
	if (OF_getprop(phandle, "width", &tempval, sizeof(int)) > 0)
	{
	    displayInfP->width = of_decode_int((unsigned char *)&tempval);
	}
	else
	{
	    displayInfP->width = -1;
	}

	/* Name of character Set */
	if ((displayInfP->charSetLen = OF_getproplen(phandle, "character-set")) > 0)
	{
	    displayInfP->charSet = CharSet_store;
	    displayInfP->charSetLen = OF_getprop(phandle, "character-set", 
					     displayInfP->charSet, displayInfP->charSetLen);
	}
	if (displayInfP->charSetLen <= 0)
	{
	    displayInfP->charSet = (char *)-1;
	}
	
	/* Register values.
	 * XXX Treated as a contiguous range for memory mapping
	 * XXX this code is still horribly broken, but it's better
	 *     now -- cgd (Nov. 14, 1997)
	 */
	if ((regSize = OF_getproplen(phandle, "reg")) > 0)
	{
	    displayRegs = DisplayRegs_store;
	    if ((regSize = OF_getprop(phandle, "reg", displayRegs, regSize)) > 0)
	    {
		nReg = regSize / sizeof(struct regspec);
		ioStart = memStart = 0xffffffff;
		ioEnd = memEnd = 0;

		for (r = 0; r < nReg; r++) {
			displayRegs[r].bustype =
			    of_decode_int((void *)&(displayRegs[r]).bustype);
			displayRegs[r].addr =
			    of_decode_int((void *)&(displayRegs[r]).addr);
			displayRegs[r].size =
			    of_decode_int((void *)&(displayRegs[r]).size);

			if (displayRegs[r].bustype & 1)	{ /* XXX I/O space */
				startp = &ioStart;
				endp = &ioEnd;
			} else {
				startp = &memStart;
				endp = &memEnd;
			}
			if (displayRegs[r].addr < *startp)
				*startp = displayRegs[r].addr;
			if (displayRegs[r].addr + displayRegs[r].size > *endp)
				*endp = displayRegs[r].addr +
				    displayRegs[r].size;
		}
		if (memStart != 0xffffffff) {
			displayInfP->paddr = memStart;
		} else {
#if 0
			/* XXX leave this alone until 'address' hack dies */
			displayInfP->paddr = (paddr_t)-1;
#endif
		}
		if (ioStart != 0xffffffff) {
			displayInfP->ioBase = ioStart;
			displayInfP->ioLen = ioEnd - ioStart;
		} else {
			displayInfP->ioBase = (paddr_t)-1;
			displayInfP->ioLen = -1;
		}
	    }
	}
	if (regSize <= 0)
	{
	    displayInfP->ioBase = (paddr_t)-1;
	    displayInfP->ioLen = -1;
	}
    }
    else
    {
	displayInfP->paddr  = (paddr_t)-1;
	displayInfP->linebytes = -1;
	displayInfP->depth = -1;
	displayInfP->height = -1;
	displayInfP->width = -1;
	displayInfP->charSet = (char *)-1;
	displayInfP->charSetLen = 0;
	displayInfP->ioBase = (paddr_t)-1;
	displayInfP->ioLen = 0;
    }
    return;
}
    
#ifdef X_CGA_BUG
static void
cga_save_restore(int mode)
{
    int i;
    char tmp;
    static char *textInfo, *fontInfo;
    static char GraphicsReg[9];
    static char SequencerReg[5];
    static char AttributeReg10;
    
    
    switch (mode)
    {
	case CGA_SAVE:
	    /*
	     * Copy text from screen.
	     */
	    textInfo = (char *)malloc(16384, M_DEVBUF, M_NOWAIT);
	    bcopy(Crtat, textInfo, TEXT_LENGTH);			
	    
	    /*
	     ** Save the registers before we change them
	     */
	    /* reset flip-flop */
	    tmp = inb(CGA_BASE + 0x0A);
	    /* Read Mode Control register */
	    outb(AR_INDEX, 0x30);
	    AttributeReg10 = inb(AR_READ);
	    fontInfo = (char *)malloc(8192, M_DEVBUF, M_NOWAIT);
	    for (i = 0; i < 9; i++)
	    {
		outb(GR_INDEX, i);
		GraphicsReg[i] = inb(GR_DATA);
	    }
	    for (i = 0; i < 5; i++)
	    {
		outb(SR_INDEX, i);
		SequencerReg[i] = inb(SR_DATA);
	    }
	    
	    /*
	     ** Blank the screen
	     */
	    outb(SR_INDEX, 1);
	    outb(SR_DATA, (inb(SR_DATA) | 0x20));
	    
	    /*
	     ** Set up frame buffer to point to plane 2 where the
	     ** font information is stored.
	     */
	    /* reset flip-flop */
	    tmp = inb(CGA_BASE + 0x0A); 
	    /* graphics mode */
	    outb(AR_INDEX,0x30); outb(AR_INDEX, 0x01); 
	    /* write to plane 2 */
	    outb(SR_INDEX, 0x02); outb(SR_DATA, 0x04);    
	    /* enable plane graphics */
	    outb(SR_INDEX, 0x04); outb(SR_DATA, 0x06);    
	    /* read plane 2 */
	    outb(GR_INDEX, 0x04); outb(GR_DATA, 0x02);    
	    /* write mode 0, read mode 0 */
	    outb(GR_INDEX, 0x05); outb(GR_DATA, 0x00);
	    /* set graphics mode - Warning: CGA specific */
	    outb(GR_INDEX, 0x06); outb(GR_DATA, 0x0d);
	    
	    /*
	     * Copy font information
	     */
	    bcopy(Crtat, fontInfo, FONT_LENGTH);			
	    /*
             * Restore registers in case the X Server wants to save
	     * the text too.
	     */
	    tmp = inb(0x3D0 + 0x0A); /* reset flip-flop */
	    outb(AR_INDEX,0x30); outb(AR_INDEX, AttributeReg10); 
	    for (i = 0; i < 9; i ++)
	    {
		outb(GR_INDEX, i); 
		outb(GR_DATA, GraphicsReg[i]);
	    }
	    for (i = 0; i < 5; i++)
	    {
		outb(SR_INDEX, i);
		outb(SR_DATA, SequencerReg[i]);
	    }
	    break;
		
        case CGA_RESTORE:
	    /*
	     ** Set up frame buffer to point to plane 2 where the
	     ** font information is stored.
	     */
	    /* reset flip-flop */
	    tmp = inb(CGA_BASE + 0x0A); 
	    /* graphics mode */
	    outb(AR_INDEX,0x30); outb(AR_INDEX, 0x01); 
	    /* write to plane 2 */
	    outb(SR_INDEX, 0x02); outb(SR_DATA, 0x04);    
	    /* enable plane graphics */
	    outb(SR_INDEX, 0x04); outb(SR_DATA, 0x06);    
	    /* read plane 2 */
	    outb(GR_INDEX, 0x04); outb(GR_DATA, 0x02);    
	    /* write mode 0, read mode 0 */
	    outb(GR_INDEX, 0x05); outb(GR_DATA, 0x00);
	    /* set graphics mode - Warning: CGA specific */
	    outb(GR_INDEX, 0x06); outb(GR_DATA, 0x0d);
	    
	    /*
	     ** Restore font information 
	     */
	    bcopy(fontInfo, Crtat, FONT_LENGTH);
	    
	    /*
	     ** Put registers back the way they were for text.
	     */
	    tmp = inb(0x3D0 + 0x0A); /* reset flip-flop */
	    outb(AR_INDEX,0x30); outb(AR_INDEX, AttributeReg10); 
	    for (i = 0; i < 9; i ++)
	    {
		outb(GR_INDEX, i); 
		outb(GR_DATA, GraphicsReg[i]);
	    }
	    for (i = 0; i < 5; i++)
	    {
		outb(SR_INDEX, i);
		outb(SR_DATA, SequencerReg[i]);
	    }

	    /*
	     ** Restore text information
	     */
	    bcopy(textInfo, Crtat, TEXT_LENGTH);
	   
	    break;
	
	default:
	    panic("unknown save/restore mode");
	    break;    
	}
}
#endif
#endif
