/*      $NetBSD: opms.c,v 1.27.2.1 2016/07/09 20:24:56 skrll Exp $        */

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
 * Copyright (c) 1994 Charles M. Hannum.
 * Copyright (c) 1992, 1993 Erik Forsberg.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL I BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/*
**++
** 
**  FACILITY:
**
**    Mouse driver.
**
**  ABSTRACT:
**
**    The mose driver has been cleand up for use with the PC87307VUL
**    Super I/O chip.  The main modification has been to change the 
**    driver to use the bus_space_ macros.  This allows the mouse
**    to be configured to any base address.  It relies on the keyboard
**    passing its io handle in the isa_attach_args structure.
**
**    NOTE : The mouse is an auxiliary device off the keyboard and as such
**           shares the same device registers.  This shouldn't be an issue
**           since each logical device generates its own unique IRQ.  But
**           it is worth noting that reseting or mucking with one can affect
**           the other.
**
**  AUTHORS:
**
**    Unknown.  Modified by Patrick Crilly, and John Court 
**              Digital Equipment Corporation.
**
**  CREATION DATE:
**
**    Unknown
**
**--
*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: opms.c,v 1.27.2.1 2016/07/09 20:24:56 skrll Exp $");

#include "opms.h"
#if NOPMS > 1
#error Only one PS/2 style mouse may be configured into your system.
#endif

#include "locators.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/poll.h>
#include <sys/conf.h>
#include <machine/kerndebug.h>

#include <dev/isa/isavar.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/pio.h>
#include <machine/mouse.h>

#include <shark/shark/i8042reg.h>

/*
** Macro definitions
*/

/* mouse commands */
#define PMS_SET_SCALE11 0xe6    /* set scaling 1:1                      */
#define PMS_SET_SCALE21 0xe7    /* set scaling 2:1                      */
#define PMS_SET_RES     0xe8    /* set resolution                       */
#define PMS_GET_SCALE   0xe9    /* get scaling factor                   */
#define PMS_SET_STREAM  0xea    /* set streaming mode                   */
#define PMS_SET_SAMPLE  0xf3    /* set sampling rate                    */
#define PMS_MOUSE_ENABLE  0xf4  /* mouse on                             */
#define PMS_MOUSE_DISABLE 0xf5  /* mouse off                            */
#define PMS_RESET         0xff  /* reset                                */

#define PMS_CHUNK       128     /* chunk size for read                  */
#define PMS_BSIZE       1020    /* buffer size                          */

/* Masks for the first byte of a packet */
#define PS2LBUTMASK   0x01      /* left mouse button set                */
#define PS2RBUTMASK   0x02      /* right mouse button set               */
#define PS2MBUTMASK   0x04      /* middle mouse button set              */
#define PS2OVERFLOW   0xc0      /* Movement overflow in X or Y direction*/
#define PS2BUTTONBIT  0x08      /* This bit ALWAYS set in button byte   */

/* Mouse states */
#define PMS_INIT        0x00    /* initial state                        */
#define PMS_OPEN        0x01    /* device is open                       */
#define PMS_ASLP        0x02    /* waiting for mouse data               */

/* Byte that we're reading in the mouse protocol */
#define PMS_RD_BYTE1   0x00     /* reading first byte                   */
#define PMS_RD_BYTE2   0x01     /* reading second byte                  */
#define PMS_RD_BYTE3   0x02     /* reading third byte                   */

/* Macro to extract the minor device nuymber from the device identifier */
#define PMSUNIT(dev)    (minor(dev))

/* Softc structure for the mouse */
struct opms_softc 
{               
    void               *sc_ih;
    struct clist       sc_q;
    struct selinfo     sc_rsel;
    u_char             sc_state;        /* mouse driver state */
    u_char             sc_status;       /* mouse button status */
    u_char             sc_protocol_byte;/* mouse byte that we're waiting on */ 
    int                sc_x;
    int                sc_y;            /* accumulated motion in the Y axis */
    bus_space_tag_t    sc_iot;
    bus_space_handle_t sc_ioh;
};

/*
** Forward routine declarations
*/
int           opmsprobe(device_t, cfdata_t, void *);
void          opmsattach(device_t, device_t, void *);
int           opmsintr(void *);

/* 
** Global variables 
*/

/* Autoconfiguration data structures */
CFATTACH_DECL_NEW(opms, sizeof(struct opms_softc),
    opmsprobe, opmsattach, NULL, NULL);

extern struct cfdriver opms_cd;

dev_type_open(opmsopen);
dev_type_close(opmsclose);
dev_type_read(opmsread);
dev_type_ioctl(opmsioctl);
dev_type_poll(opmspoll);
dev_type_kqfilter(opmskqfilter);

const struct cdevsw opms_cdevsw = {
	.d_open = opmsopen,
	.d_close = opmsclose,
	.d_read = opmsread,
	.d_write = nowrite,
	.d_ioctl = opmsioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = opmspoll,
	.d_mmap = nommap,
	.d_kqfilter = opmskqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

/* variable to control which debugs printed if kernel compiled with 
** option KERNEL_DEBUG. 
*/
int opmsdebug = KERN_DEBUG_WARNING | KERN_DEBUG_ERROR;


/*
**++
**  FUNCTIONAL DESCRIPTION:
**     
**     opmsprobe
**
**     This is the probe routine for the mouse. It checks to see that 
**     we are attaching to the pc or vt device.  It then resets mouse
**     and disables interrupts on it.  Interrupts on the mouse are 
**     enabled when the mouse device is opened.
**
**  FORMAL PARAMETERS:
**
**     parent  - pointer to the parent device.
**     match   - not used
**     aux     - pointer to an isa_attach_args structure.
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
**     0 - Probe failed to find the requested device.
**     1 - Probe successfully talked to the device. 
**
**  SIDE EFFECTS:
**
**     none.
**--
*/
int
opmsprobe(device_t parent, cfdata_t match, void *aux)
{
    struct cfdata             *cf     = match;
    int                       probeOk = 0;    /* assume failure */
    struct isa_attach_args    *ia = aux;

    KERN_DEBUG(opmsdebug, KERN_DEBUG_INFO, ("opmsprobe: entered\n"));
    /*
    ** We only attach to the keyboard controller via
    ** the console drivers. (We really wish we could be the
    ** child of a real keyboard controller driver.)
    */
    if (parent != NULL &&
        (device_is_a(parent, "pc") ||
         device_is_a(parent, "vt")))
    {
        /* 
        ** The mouse shares registers with the parent, so
        ** we expect that the parent has mapped the io space.
        ** Check an IRQ has been specified in the configuration
        */
        if (cf->cf_loc[SPCKBDCF_IRQ] != -1)
        {
            /* Clear out any garbage left in there at this point in time
            */
            i8042_flush(ia->ia_iot, (bus_space_handle_t)ia->ia_aux);
            /* reset the mouse and check the command is received.
            */
            if (!i8042_cmd(ia->ia_iot, (bus_space_handle_t)ia->ia_aux, I8042_AUX_CMD, 
                               I8042_CHECK_RESPONSE, KBR_ACK, PMS_RESET))
            {
                KERN_DEBUG(opmsdebug, KERN_DEBUG_ERROR, 
                           ("opms_reset: reset failed\n"));
            }
            else
            {
                /*
                ** Test the mouse port. A value of 0 in the data
                ** register indicates success. 
                */
                if ( I8042_AUXTEST(ia->ia_iot, (bus_space_handle_t)ia->ia_aux) )
                {
                    probeOk = 1;
                }
                else
                {
                    KERN_DEBUG(opmsdebug, KERN_DEBUG_ERROR,
                               ("opmsprobe: aux test failed\n"));
                }
                /* 
                ** Disable the mouse.  It is enabled when 
                ** the opms device is opened.
                */
                (void) I8042_WRITECCB(ia->ia_iot, (bus_space_handle_t)ia->ia_aux, 
                                          NOAUX_CMDBYTE);
            }
        } /* end-if irq set */
    } /* end-if supported console device */
    return (probeOk);
    
} /* End opmsprobe */


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      opmsattach
**
**      Initialise mouse softc structure and set up our interrupt routine.
**
**      NOTE:  Do not use the irq in the isa_attach_args structure to
**             set up the interrupt, as this is the parent's irq.
**
**  FORMAL PARAMETERS:
**
**      parent - pointer to my parents device structure.
**      self   - pointer to my softc with device structure at front.
**      aux    - pointer to the isa_attach_args structure.
**
**  IMPLICIT INPUTS:
**
**      None
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
opmsattach(device_t parent, device_t self, void *aux)
{
    struct opms_softc         *sc = device_private(self);
    int                       irq = device_cfdata(self)->cf_loc[SPCKBDCF_IRQ];
    struct isa_attach_args    *ia = aux;

    printf(" irq %d\n", irq);
    /* 
    ** Initialise the mouse softc structure.
    ** Other initialization was done by opmsprobe. 
    */
    sc->sc_iot    = ia->ia_iot;
    sc->sc_ioh    = (bus_space_handle_t)ia->ia_aux;
    sc->sc_state  = PMS_INIT;

    selinit(&sc->sc_rsel);
    sc->sc_ih     = isa_intr_establish(ia->ia_ic, irq, IST_LEVEL, 
                                       IPL_TTY, opmsintr, sc);
    KERN_DEBUG(opmsdebug, KERN_DEBUG_INFO,
               ("opmsattach: IOT %p: IOH 0x%lx\n", sc->sc_iot, sc->sc_ioh));

    return;
} /* End opmsattach */



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     opmsopen
**
**     This routine is to open the mouse device.
**     It first makes sure that the device unit specified is valid 
**     and not already in use.  It then enables the mouse device
**     and interrupts on it. 
**
**  FORMAL PARAMETERS:
**
**     dev  - Device identifier consisting of major and minor numbers.
**     flag - not used
**     mode - not used.
**     l    - not used.
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
**     0       - Success
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
opmsopen(dev_t dev, int flag, int mode, struct lwp *l)
{
    struct opms_softc    *sc;
    
    sc = device_lookup_private(&opms_cd, PMSUNIT(dev));
    if (!sc)
        return ENXIO;

    /* Check to see if the mouse has already been opened. 
    */
    if (sc->sc_state & PMS_OPEN)
        return EBUSY;

    /* Initialise the mouse softc structure 
    */
    if (clalloc(&sc->sc_q, PMS_BSIZE, 0) == -1)
    {
        return ENOMEM;
    } 
    sc->sc_state |= PMS_OPEN;
    sc->sc_status = 0;
    sc->sc_x      = 0;
    sc->sc_y      = 0;
    /* Enable the device both in the mouse and in the keyboard after making
    ** sure there isn't any garbage in the input buffer.
    */
    i8042_flush(sc->sc_iot, sc->sc_ioh);
    sc->sc_protocol_byte = PMS_RD_BYTE1;
    (void) i8042_cmd(sc->sc_iot, sc->sc_ioh, I8042_AUX_CMD, 
                         I8042_NO_RESPONSE, 0, PMS_MOUSE_ENABLE);
    (void) I8042_AUXENABLE(sc->sc_iot, sc->sc_ioh);
    /* Enable interrupts on the axilliary device.
    */
    (void) I8042_WRITECCB(sc->sc_iot, sc->sc_ioh, CMDBYTE);
    return 0;
} /* End opmsopen */


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     opmsclose
**
**     This function is called on the last close for a device unit.
**     It flushes the queue of mouse events and disables the mouse.
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
**     None
**
**  IMPLICIT OUTPUTS:
**
**     None
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
opmsclose(dev_t dev, int flag, int mode, struct lwp *l)
{
    struct opms_softc *sc = device_lookup_private(&opms_cd, PMSUNIT(dev));

    /* Disable the mouse device and interrupts on it. Note that if we don't
    ** flush the device first it seems to generate LOTs of interrupts after
    ** disable and hangs the system.
    */
    i8042_flush(sc->sc_iot, sc->sc_ioh);
    (void) i8042_cmd(sc->sc_iot, sc->sc_ioh, I8042_AUX_CMD, 
                         I8042_NO_RESPONSE, 0, PMS_MOUSE_DISABLE);
    i8042_flush(sc->sc_iot, sc->sc_ioh);
    (void) I8042_WRITECCB(sc->sc_iot, sc->sc_ioh, NOAUX_CMDBYTE);
    (void) I8042_AUXDISABLE(sc->sc_iot, sc->sc_ioh);
    sc->sc_state &= ~PMS_OPEN;
    /* Free the event queue 
    */
    clfree(&sc->sc_q);

    return 0;
} /* End opmsclose */


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**      opmsread
**
**      This function handles a read on the mouse device.  If there isn't 
**      data immdediatley available sleep until there is or return an error
**      depending upon the flag parameter.  If data is available pass as
**      much of it to the user as possible. 
**
**  FORMAL PARAMETERS:
**      
**      dev  - Device identifier consisting of major and minor numbers.
**      uio  - Pointer to the user I/O information (ie. read buffer).
**      flag - Information on how the I/O should be done (eg. blocking
**
**  IMPLICIT INPUTS:
**
**      none
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
opmsread(dev_t dev, struct uio *uio, int flag)
{
    struct opms_softc *sc = device_lookup_private(&opms_cd, PMSUNIT(dev));
    int s;
    int error = 0;
    size_t length;
    u_char buffer[PMS_CHUNK];
    
    /* 
    ** Check if there is data to be read. If there isn't 
    ** either sleep until there is or return an error,
    ** depending on whether we're in blocking mode or not.
    */
    s = spltty();
    while (sc->sc_q.c_cc == 0) 
    {
        if (flag & IO_NDELAY) 
        {
            error = EWOULDBLOCK;
            break;
        }
        else
        {
            sc->sc_state |= PMS_ASLP;
            error = tsleep((void *)sc, PZERO | PCATCH, "opmsread", 0);
            if (error) 
            {
                sc->sc_state &= ~PMS_ASLP;
                break;
            }
        }
    } /* end-if no data in event queue */
    splx(s);
    /* Transfer as many chunks as possible. 
    */
    if (!error )
    {
        while (sc->sc_q.c_cc > 0 && uio->uio_resid > 0) 
        {
            length = min(sc->sc_q.c_cc, uio->uio_resid);
            if (length > sizeof(buffer))
            {
                length = sizeof(buffer);
            }
            /* Remove a small chunk from the input queue. 
            */
            (void) q_to_b(&sc->sc_q, buffer, length);
            /* Copy the data to the user process. 
            */
            if ((error = uiomove(buffer, length, uio)) != 0)
            {
                break;
            }
        } /* while there is data in the event queue */
    } /* end-if not error */

    return (error);
}


/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     opmsioctl
**
**     This routine is responsible for handling ioctls on the
**     mouse device.  Only one ioctl is supported:
**     MOUSEIOCREAD  - which retruns the current mouse location
**                     and flushes the mouse event queue.
**
**  FORMAL PARAMETERS:
**      
**     dev  - device identifier consisting of major and minor numbers.
**     cmd  - requested ioctl 
**     addr - address of user buffer for mouse info
**     flag - unused
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
**     EINVAL  - not a supported iotcl
**
**  SIDE EFFECTS:
**
**     none
**--
*/
int
opmsioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
{
    struct opms_softc     *sc = device_lookup_private(&opms_cd,PMSUNIT(dev));
    struct mouseinfo     info;
    int                  oldIpl;
    int                  error;
    
    switch (cmd) 
    {
#ifdef  SHARK
        case MOUSEIOC_READ:
#else
        case MOUSEIOCREAD:
#endif
            oldIpl = spltty();
            info.status = sc->sc_status;
            if (sc->sc_x || sc->sc_y)
            {
                info.status |= MOVEMENT;
            }
            /* Read x co-ordinate 
            */
            if (sc->sc_x > 127)
            {
                info.xmotion = 127;
            }
            else if (sc->sc_x < -127)
            {
                /* Bounding at -127 avoids a bug in XFree86. 
                */
                info.xmotion = -127;
            }
            else
            {
                info.xmotion = sc->sc_x;
            }
            /* Read y co-ordinate 
            */
            if (sc->sc_y > 127)
            {
                info.ymotion = 127;
            }
            else if (sc->sc_y < -127)
            {
                info.ymotion = -127;
            }
            else
            {
                info.ymotion = sc->sc_y;
            }
            /* Reset historical information. 
            */
            sc->sc_x = 0;
            sc->sc_y = 0;
            sc->sc_status &= ~BUTCHNGMASK;
            ndflush(&sc->sc_q, sc->sc_q.c_cc);  /* empty event queue */
        
            splx(oldIpl);
            error = copyout(&info, addr, sizeof(struct mouseinfo));
        break;
        
        default:
            error = EINVAL;
        break;
    } /* end-switch cmd */
    
    return error;
} /* End opmsioctl */



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     opmsintr
**
**     This is the interrupt handler routine for the mouse.  The mouse 
**     protocol consists of 3 bytes.  The first byte indicates which
**     buttons are pressed, the second byte the change to the x-coordinate
**     and the third byte the change to the y-coordinate.  The interrupt
**     routine reads a byte from the mouse, using the mouse state to 
**     keep track of which byte in the protocol we're up to.  After reading 
**     the third byte, the event is added to the event queue and any 
**     read waiting is woken.    
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
opmsintr(void *arg)
{
    struct opms_softc     *sc   = arg;
    static u_char        buttons;
    u_char               changed;
    u_char               status;
    u_char               buffer[5];
    int                  handledInt;
    bus_space_tag_t      iot;
    bus_space_handle_t   ioh;
    static signed char   dx;
    static signed char   dy;
    u_char               value = 0;	/* XXX */
    
    iot = sc->sc_iot;
    ioh = sc->sc_ioh;
    handledInt = 0;
    
    /* 
    ** If the mouse isn't open then we shouldn't get an 
    ** interrupt.
    */
    if (sc->sc_state & PMS_OPEN) 
    {
        /* We wait for status to be updated. For some reason the status is 
        ** updated after the interrupt is generated. I would summise this 
        ** is because the H/W waits for an ack on the IRQ to determine
        ** whether to put Keyboard or Aux port data into the buffer and
        ** status information, but that is only a guess.
        */
        I8042_GETAUX_DATA(iot, ioh, status, value);
        
        if ( status )
        {
            handledInt = 1;
            switch (sc->sc_protocol_byte) 
            {
                /* First byte of protocol consists of the following mouse bits
                ** Bit 7  6  5  4   3  2  1  0
                **     YO XO YS XS  1  0  R  L
                ** YO = Overflow occurred in the Y direction.
                ** XO = Overflow occurred in the X direction.
                ** YS = Sign of movement data on Y axis (1 = negative)
                ** XS = Sign of movement data on X axis (1 = negative)
                ** R  = Right button state (1 = pressed down)
                ** L  = Left button state (1 = pressed down)
                */
                case PMS_RD_BYTE1:
                    buttons = value;
                
                    if (buttons & PS2BUTTONBIT)
                    {
                        sc->sc_protocol_byte = PMS_RD_BYTE2;
                    }
                    else
                    {
                        KERN_DEBUG( opmsdebug, KERN_DEBUG_WARNING, 
                                   ("opmsintr: bad button byte received 0x%x\n",
                                    buttons));
                    }
                break;
                /* Second byte of protocol is the amount of movement in the 
                ** X axis
                */
                case PMS_RD_BYTE2:
                    dx = value;
                    /* Bounding at -127 avoids a bug in XFree86. */
                    dx = (dx == -128) ? -127 : dx;
                    sc->sc_protocol_byte = PMS_RD_BYTE3;
                break;
                /* Third byte if protocol is the amount of movement in the
                ** Y axis.
                */
                case PMS_RD_BYTE3:
                    dy = value;
                    dy = (dy == -128) ? -127 : dy;
                    sc->sc_protocol_byte = PMS_RD_BYTE1;
                    /* 
                    ** Update changes to buttons. 
                    **
                    ** We need to swap the ordering of which button
                    ** is set.  The ps2 mouse records set buttons with
                    ** the following ordering (from LSB) 
                    ** left-right-middle whereas the generic mouse 
                    ** (defined in ../include/mouse.h) records
                    ** it as (from LSB) right-middle-left.
                    ** The next three bits in the buttons field
                    ** record whether there has been a change in
                    ** state of the button.
                    */
                    buttons       = ((buttons & PS2LBUTMASK) << 2) |
                        ((buttons & (PS2RBUTMASK | PS2MBUTMASK)) >> 1);
                    changed       = ((buttons ^ sc->sc_status) & BUTSTATMASK) 
                        << 3;
                    sc->sc_status = buttons | 
                        (sc->sc_status & ~BUTSTATMASK) | changed;
                    /* 
                    ** If the postion of mouse or state of buttons has
                    ** changed update status info, queue an event
                    ** and wakeup any read that is waiting.
                    */
                    if (dx || dy || changed) 
                    {
                        /* Update accumulated movements. 
                        */
                        sc->sc_x += dx;
                        sc->sc_y += dy;
                        /* Add this event to the queue. 
                        */
                        buffer[0] = 0x80 | (buttons ^ BUTSTATMASK);
                        buffer[1] = dx;
                        buffer[2] = dy;
                        buffer[3] = 0;
                        buffer[4] = 0;
                        (void) b_to_q(buffer, sizeof buffer, &sc->sc_q);
                        /* If there is a read blocked, wake it up 
                        */
                        if (sc->sc_state & PMS_ASLP) 
                        {
                            sc->sc_state &= ~PMS_ASLP;
                            wakeup((void *)sc);
                        }
                        /* Wakeup any selects waiting */
                        selnotify(&sc->sc_rsel, 0, 0);
                    }
                break;
                default :
                    KERN_DEBUG( opmsdebug, KERN_DEBUG_WARNING, 
                               ("opmsintr: Invalid byte protocol state 0x%x\n",
                                sc->sc_protocol_byte));
                    sc->sc_protocol_byte = PMS_RD_BYTE1;
                break;
            }
        } /* End do until data not for mouse or no data at all */
    } /* End if in an open state */

    return (handledInt);
} /* End opmsintr */



/*
**++
**  FUNCTIONAL DESCRIPTION:
**
**     opmspoll
**
**     This routine is used to poll the device for the presence of a set 
**     of events (e.g. is there data to be read).  If any of the polled 
**     for events are present on the device, then these events are returned;
**     otherwise the process is marked as waiting for the set of events
**
**  FORMAL PARAMETERS:
**
**      dev    - device identifier consisting of major and minor numbers.
**      events - the set of events to check for.
**      l      - pointer to lwp structure of user.
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
**      Set of events that were polled for and are currently available.  
**
**  SIDE EFFECTS:
**
**      none.
**--
*/
int
opmspoll(dev_t dev, int events, struct lwp *l)
{
    struct opms_softc     *sc     = device_lookup_private(&opms_cd, PMSUNIT(dev));
    int                  revents = 0;
    int                  oldIpl;

    oldIpl = spltty();
    
    if (events & (POLLIN | POLLRDNORM))
    {
        if (sc->sc_q.c_cc > 0)
        {
            revents |= events & (POLLIN | POLLRDNORM);
        }
        else
        {
            /* Record that the process has done a select 
            */
            selrecord(l, &sc->sc_rsel);
        }
    }
    splx(oldIpl);
    return (revents);
} /* End opmspoll */

static void
filt_opmsrdetach(struct knote *kn)
{
	struct opms_softc *sc = kn->kn_hook;
	int s;

	s = spltty();
	SLIST_REMOVE(&sc->sc_rsel.sel_klist, kn, knote, kn_selnext);
	splx(s);
}

static int
filt_opmsread(struct knote *kn, long hint)
{
	struct opms_softc *sc = kn->kn_hook;

	kn->kn_data = sc->sc_q.c_cc;
	return (kn->kn_data > 0);
}

static const struct filterops opmsread_filtops =
	{ 1, NULL, filt_opmsrdetach, filt_opmsread };

int
opmskqfilter(dev_t dev, struct knote *kn)
{
	struct opms_softc *sc = device_lookup_private(&opms_cd, PMSUNIT(dev));
	struct klist *klist;
	int s;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &sc->sc_rsel.sel_klist;
		kn->kn_fop = &opmsread_filtops;
		break;

	default:
		return (1);
	}

	kn->kn_hook = sc;

	s = spltty();
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	splx(s);

	return (0);
}
