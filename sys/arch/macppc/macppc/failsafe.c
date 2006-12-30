/*
 * Copyright (C) 2006 Kyma Systems.
 * All rights reserved.
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
 *	This product includes software developed by Kyma Systems.
 * 4. The name of Kyma Systems may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY KYMA SYSTEMS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL KYMA BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Early serial console driver for the G5, uses the 970's Real Mode 
 * Cache Inhibited Mode (RMCI) to do cache inhibited I/O. This console
 * is used until the real console (either ofcons or zsc) is initialized
 * after the pmap layer is up
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <dev/cons.h>
#include <sys/tty.h>
#include <powerpc/cpu.h>

#if defined (PMAC_G5)
extern void mfhid4(volatile uint64_t * ret_hid4);
extern void mthid4(volatile uint64_t * hid4);

/* Local functions */
static unsigned char RMCI_readb(volatile unsigned char *addr);
static void RMCI_writeb(unsigned char data, volatile unsigned char *addr);

#define SCC_TXRDY       4
#define SCC_RXRDY       1
#define HID4_RMCI		0x0000010000000000ULL

static volatile unsigned char *sccc = (volatile unsigned char *) 0x80013020ul;
static volatile unsigned char *sccd = (volatile unsigned char *) 0x80013030ul;

static unsigned char
RMCI_readb(volatile unsigned char *addr)
{
    uint32_t omsr, rmsr;
    uint64_t ohid, rhid;
    uint8_t c;

    /* Switch to RMCI  */
    mfhid4(&ohid);
    rhid = ohid | HID4_RMCI;
    mthid4(&rhid);

    /* Switch to real data accesses */
    asm volatile ("mfmsr %0":"=r" (omsr));
    rmsr = omsr & ~(PSL_DR);
    asm volatile ("sync; mtmsr %0; sync; isync"::"r" (rmsr));

    /* Read byte */
    c = *addr;
    __asm__ __volatile__("sync");

    /* Restore MSR */
    asm volatile ("sync; mtmsr %0; sync; isync"::"r" (omsr));

    /* Restore HID4 */
    mthid4(&ohid);

    return c;
}

static void
RMCI_writeb(unsigned char data, volatile unsigned char *addr)
{
    uint32_t omsr, rmsr;
    uint64_t ohid, rhid;

    /* Switch to RMCI */
    mfhid4(&ohid);
    rhid = ohid | HID4_RMCI;
    mthid4(&rhid);

    /* Switch to real data accesses */
    asm volatile ("mfmsr %0":"=r" (omsr));
    rmsr = omsr & ~(PSL_DR);
    asm volatile ("sync; mtmsr %0; sync; isync"::"r" (rmsr));

    /* Write data */
    *addr = data;
    __asm__ __volatile__("sync");

    /* Restore MSR */
    asm volatile ("sync; mtmsr %0; sync; isync"::"r" (omsr));

    /* Restore HID4 */
    mthid4(&ohid);
    return;
}

static void
failsafe_putc(dev_t dev, int c)
{
    char ch = c;

    while ((RMCI_readb(sccc) & SCC_TXRDY) == 0);
    RMCI_writeb(ch, sccd);
    return;
}

static int
failsafe_getc(dev_t dev)
{
    char ch = '\0';
    while ((RMCI_readb(sccc) & SCC_RXRDY) == 0);

    ch = RMCI_readb(sccd);
    return ch;
}


static void
failsafe_probe(struct consdev *cd)
{
    cd->cn_pri = CN_REMOTE;
}

static void
failsafe_init(struct consdev *cd)
{
}

static void
failsafe_pollc(dev_t dev, int on)
{
}


struct consdev failsafe_cons = {
    failsafe_probe,
    failsafe_init,
    failsafe_getc,
    failsafe_putc,
    failsafe_pollc,
    NULL,
};

#endif /* PMAC_G5 */
