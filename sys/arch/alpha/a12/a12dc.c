/* $NetBSD: a12dc.c,v 1.6.4.1 2002/05/16 16:03:35 gehenna Exp $ */

/* [Notice revision 2.2]
 * Copyright (c) 1997, 1998 Avalon Computer Systems, Inc.
 * All rights reserved.
 *
 * Author: Ross Harvey
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright and
 *    author notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Avalon Computer Systems, Inc. nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. This copyright will be assigned to The NetBSD Foundation on
 *    1/1/2000 unless these terms (including possibly the assignment
 *    date) are updated in writing by Avalon prior to the latest specified
 *    assignment date.
 *
 * THIS SOFTWARE IS PROVIDED BY AVALON COMPUTER SYSTEMS, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL AVALON OR THE CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * The A12 uses what DEC calls a "detached console", i.e., some of the console
 * implementation is on a dedicated processor with its own RAM.
 * 
 * The A12 Detached Console interface uses two 16 bit registers (per CPU), one
 * going from the CPU to the a12ctrl processor and one going back the other
 * way. The first is polled, the second produces a GInt.
 * 
 * In the very early days we loaded program images through this interface.
 * 
 * Consequently, it developed an overly complicated (but sort of fast)
 * inverting sync/ack that isn't needed at all for its present application as
 * a text console device.
 * 
 * One possible solution: most of the channels are undefined, so a console
 * channel using a stateless ack could be defined, with corresponding changes
 * to the backplane 68360 code.
 * 
 * This file is complicated somewhat by its use in three different kernels:
 * NetBSD, the A12 CPU-resident console, and the a12ctrl backplane processor.
 * (The protocol is symmetrical.)
 */

#include "opt_avalon_a12.h"		/* Config options headers */
#include "opt_kgdb.h"

#ifndef BSIDE
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: a12dc.c,v 1.6.4.1 2002/05/16 16:03:35 gehenna Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <sys/tty.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/uio.h>
#include <sys/device.h>
#include <sys/conf.h>

#include <dev/cons.h>

#include <machine/cpuconf.h>
#include <machine/autoconf.h>
#include <machine/rpb.h>

#include <alpha/pci/a12creg.h>
#include <alpha/pci/a12cvar.h>
#include <alpha/pci/pci_a12.h>

#include "a12dcreg.h"

#define	A12DC()		/* Generate ctags(1) key */

#define	MAX_MODULES 1

int	a12dcmatch __P((struct device *, struct cfdata *, void *));
void	a12dcattach __P((struct device *, struct device *, void *));

struct a12dc_softc {
	struct  device sc_dev;
} a12dc_softc;

struct cfattach a12dc_ca = {
	sizeof(struct a12dc_softc), a12dcmatch, a12dcattach,
};

extern	struct cfdriver a12dc_cd;

dev_type_open(a12dcopen);
dev_type_close(a12dcclose);
dev_type_read(a12dcread);
dev_type_write(a12dcwrite);
dev_type_ioctl(a12dcioctl);
dev_type_stop(a12dcstop);
dev_type_tty(a12dctty);
dev_type_poll(a12dcpoll);

const struct cdevsw a12dc_cdevsw = {
	a12dcopen, a12dcclose, a12dcread, a12dcwrite, a12dcioctl,
	a12dcstop, a12dctty, a12dcpoll, nommap, D_TTY
};

int	a12dcfound;		/* There Can Be Only One. */

struct	a12dc_config { int im_not_used; } a12dc_configuration;

static	struct tty *a12dc_tty[1];

void a12dcstart __P((struct tty *));
void a12dctimeout __P((void *));
int a12dcparam __P((struct tty *, struct termios *));
void a12dc_init(struct a12dc_config *, int);
static void a12cdrputc __P((int));
int a12dccngetc(dev_t);
void a12dccnputc(dev_t, int);
void a12dccnpollc(dev_t, int);
/* static int get_bc_char(int mn, int chan); */
/* static int get_bc_any(int,int *,int *); */
int a12dcintr __P((void *));
static void a12_worry(int worry_number);
static void A12InitBackDriver(int);

int
a12dcmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pcibus_attach_args *pba = aux;

	return	cputype == ST_AVALON_A12
	    &&	strcmp(pba->pba_busname, a12dc_cd.cd_name) == 0
	    &&	!a12dcfound;
}

void
a12dcattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct tty *tp;
	struct a12dc_config *ccp;

	/* note that we've attached the chipset; can't have 2 A12Cs. */
	a12dcfound = 1;

	printf(": driver %s\n", "$Revision: 1.6.4.1 $");

	tp = a12dc_tty[0] = ttymalloc();
	tp->t_oproc = a12dcstart;
	tp->t_param = a12dcparam;
	tty_attach(tp);

	ccp = &a12dc_configuration;
	a12dc_init(ccp, 1);
}

void
a12dc_init(ccp, mallocsafe)
	struct a12dc_config *ccp;
	int mallocsafe;
{
}

/*
 * XXX definitions specific to only one of the kernels will be moved into
 *     kernel-local include files. (One of these days.)
 */
#define	spla12dc() spltty()
#define	a12yield()
#define	CDRADDR(m)	((mmreg_t *)(REGADDR(A12_CDR))) /* ignore m */
#define	modenormal()
#define	mb()		alpha_mb()
#define wmb()		alpha_wmb()

#else

#include "product.def"
#include "ghs.h"
#include "a12ctrl.h"

#define	wmb()
#define	mb()
#define swpipl(a) (a)
#define splx(a) (a)
#define spla12dc()
#define hrhpanic(s,v) panic((s))
#define	CDRADDR(m)	((mmreg_t *)(A12_BACKPLANE+(m)))

#endif

static int msgetput __P((register mstate_type *, int, int));
static void checkinit __P((void));

static void
A12InitBackDriver(int m)
{
	/*
	 * XXX memset() would be good here, but there is a (temporary) reason
	 * for all this right now
	 */
	a12_mstate[m].xcdr	    = CDRADDR(m);
	a12_mstate[m].txrdy_out = A12C_TXRDY;
	a12_mstate[m].txrdy_in  = A12C_TXRDY;
	a12_mstate[m].txack_out = A12C_TXACK;
	a12_mstate[m].txack_in  = A12C_TXACK;
	a12_mstate[m].txsv	    = tx_idle;
	a12_mstate[m].rxsv	    = rx_idle;
	a12_mstate[m].reset_scanner= 1;
	a12_mstate[m].reset_loader = 1;
	a12_mstate[m].up = 1;
	a12_mstate[m].lastr = 0;		/* last char. received */
	a12_mstate[m].lastrc = 0;		/* last received channel */
	a12_mstate[m].rx_busy_wait = 0;	/* last received busy ticks */
	a12_mstate[m].rx_busy_cnt = 0;	/* last received start clock */
	a12_mstate[m].lastt = 0;		/* last char. trasmitted */
	a12_mstate[m].lasttc = 0;		/* last transmit channel */
	a12_mstate[m].tx_busy_wait = 0;	/* last transmitted busy ticks */
	a12_mstate[m].tx_busy_cnt = 0;	/* last transmitted start clock */
	a12_mstate[m].tbytes = 0;
	a12_mstate[m].tblkbytes = 0;
	a12_mstate[m].tblks = 0;
	a12_mstate[m].blwobe = 0;
	a12_mstate[m].blwbe = 0;
	a12_mstate[m].max_blkretry = 0;
	a12_mstate[m].max_blktime = 0;
	a12_mstate[m].max_blktimesz = 0;
	a12_mstate[m].avg_blksz = 0;
	a12_mstate[m].avg_blktime = 0;
	a12_mstate[m].retry_time = 0;
}

static void
checkinit()
{
static int did_init;

	if (!did_init) {
		A12InitBackDriver(0);
		did_init = 1;
	}
}

int
a12dcopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	int unit = minor(dev);
	struct tty *tp;
	int s;
 
	if (unit >= 1)
		return ENXIO;

#ifdef KGDB
	if (flags & COM_HW_KGDB)
		return EBUSY;
#endif

	if (!a12dc_tty[unit]) {
		tp = a12dc_tty[unit] = ttymalloc();
		tty_attach(tp);
	} else
		tp = a12dc_tty[unit];

	if ((tp->t_state & TS_ISOPEN) &&
	    (tp->t_state & TS_XCLUDE) &&
	    p->p_ucred->cr_uid != 0)
		return EBUSY;

	s = spltty();

	tp->t_oproc = a12dcstart;
	tp->t_param = a12dcparam;
	tp->t_dev = dev;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		tp->t_state |= TS_CARR_ON;
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = TTYDEF_CFLAG|CLOCAL;
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = 9600;
		ttsetwater(tp);
		/* XXX XXX XXX
		a12_intr_register_icw(a12dcintr);
		*/
	} else if (tp->t_state&TS_XCLUDE && p->p_ucred->cr_uid != 0) {
		splx(s);
		return EBUSY;
	}

	splx(s);

	return (*tp->t_linesw->l_open)(dev, tp);
}
 
int
a12dcclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	int unit = minor(dev);
	struct tty *tp = a12dc_tty[unit];

	(*tp->t_linesw->l_close)(tp, flag);
	ttyclose(tp);
	return 0;
}
 
int
a12dcread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct tty *tp = a12dc_tty[minor(dev)];

	return ((*tp->t_linesw->l_read)(tp, uio, flag));
}
 
int
a12dcwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct tty *tp = a12dc_tty[minor(dev)];
 
	return ((*tp->t_linesw->l_write)(tp, uio, flag));
}

int
a12dcpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct tty *tp = a12dc_tty[minor(dev)];
 
	return ((*tp->t_linesw->l_poll)(tp, events, p));
}
 
int
a12dcioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	int unit = minor(dev);
	struct tty *tp = a12dc_tty[unit];
	int error;

	error = (*tp->t_linesw->l_ioctl)(tp, cmd, data, flag, p);
	if (error != EPASSTHROUGH)
		return error;
	return ttioctl(tp, cmd, data, flag, p);
}

int
a12dcparam(tp, t)
	struct tty *tp;
	struct termios *t;
{
	return 0;
}

void
a12dcstart(tp)
	struct tty *tp;
{
	int s;

	s = spltty();
	if (tp->t_state & (TS_TTSTOP | TS_BUSY))
		goto out;
	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)&tp->t_outq);
		}
		selwakeup(&tp->t_wsel);
	}
	tp->t_state |= TS_BUSY;
	while (tp->t_outq.c_cc != 0)
		a12dccnputc(tp->t_dev, getc(&tp->t_outq));
	tp->t_state &= ~TS_BUSY;
out:
	splx(s);
}

/*
 * Stop output on a line.
 */
void
a12dcstop(tp, flag)
	struct tty *tp;
{
	int s;

	s = spltty();
	if (tp->t_state & TS_BUSY)
		if ((tp->t_state & TS_TTSTOP) == 0)
			tp->t_state |= TS_FLUSH;
	splx(s);
}

int
a12dcintr(v)
	void *v;
{
#if 1
	DIE();
#else
	struct tty *tp = v;
	u_char c;

	while (a12dccnlookc(tp->t_dev, &c)) {
		if (tp->t_state & TS_ISOPEN)
			(*tp->t_linesw->l_rint)(c, tp);
	}
#endif
}

struct tty *
a12dctty(dev)
	dev_t dev;
{

	if (minor(dev) != 0)
		panic("a12dctty: bogus");

	return a12dc_tty[0];
}

int
a12dccnattach()
{
	static struct consdev a12dccons = {
		NULL, NULL, a12dccngetc, a12dccnputc, a12dccnpollc, NULL,
		    NODEV, CN_NORMAL
	};

	a12dccons.cn_dev = makedev(cdevsw_lookup_major(&a12dc_cdevsw), 0);

	cn_tab = &a12dccons;
	return 0;
}

int
a12dccngetc(dev)
	dev_t dev;
{
	for(;;)
		continue;
}

void
a12dccnputc(dev, c)
	dev_t dev;
	int c;
{
	a12cdrputc(c);
}

void
a12dccnpollc(dev, on)
	dev_t dev;
	int on;
{
}

static void
a12cdrputc(int c)
{
	int	s = spla12dc();

	checkinit();
        while(msgetput(MSP(0),CHANNEL_MONITOR,c)) {
		/*if(check_cdr_ok)
			check_cdr();*/
		DELAY(100);
        }
	splx(s);
}

#if 0
static void
check_cdr()
{
	int   bpchar,cnumber;
	static int srom_word_address,
		kmsg;
	int	push_check_ok;
	int	ipl;

        if(!get_bc_any(0,&bpchar,&cnumber))
                return;
        switch(cnumber) {
            default:
                printf("Unknown cdr channel %d",cnumber);
                break;
            case CHANNEL_KDATA:
		if(!kmsg)
			printf("Ignoring new kernel\n");;
		kmsg = 1;
		break;
            case CHANNEL_KMARK:
		kmsg = 0;
		break;
#ifndef _KERNEL	/* NetBSD kernel, that is. "if defined: rtmon kernel" */
            case CHANNEL_SROM_A:
		srom_word_address = bpchar;
		break;
            case CHANNEL_SROM_D:
		push_check_ok = check_cdr_ok;
		check_cdr_ok  = 0;
		ipl = swpipl(7);
		write_ethernet_srom(srom_word_address,bpchar);
		++srom_word_address;
		(void)swpipl(ipl);
		check_cdr_ok = push_check_ok;
		break;

            case CHANNEL_MULTI:
                logmchar(bpchar,"<");
                SerialByteReceived(bpchar);
                break;
            case CHANNEL_MONITOR:
		if (bpchar == CPX_PANIC) 
			/* TJF - Could kill all processes and then panic? */
			hrhpanic("Panic in cooperating CPU.",0);
		else 	cpxchar(bpchar);
                break;
#endif
	    case CHANNEL_CONSOLE:
#ifdef A12CON /* XXX this can be done much better */
		virtual_keyboard_byte(bpchar);
#else
		ipl7putc(bpchar);
#endif
		break;
        }
}
#endif

#if 0
static int
get_bc_any(int mn, int *c, int *chan) {
mstate_type *ms = &a12_mstate[mn];

	(void)mcgetput(mn, 0, -1);
	if(ms->rxsv==rx_busy) {
		*c    = ms->c2b_char & 0xff;
		*chan = ms->c2b_channel;
		ms->rxsv = rx_idle;
		return 1;
	}
	return 0;
}

static int
get_bc_char(int mn, int chan) {
int	newc,newchan;

	if(get_bc_any(mn,&newc,&newchan)) {
		if(newchan!=chan)
			printf("receiver busy on dumb channel %d", newchan);
		return newc;
	}
	return -1;
}
#endif

static int
msgetput(register mstate_type *ms, int channel, int c)
{
int	i,t;
int	ipl;

	if(c==-2 || ms->up==0) {
		if(c!=-2)
			a12_worry(10);
		ms->txsv = tx_idle;
		ms->rxsv = rx_idle;
		ms->cdr  = 0;
		return 0;
	}
	if(!(0<=channel && channel<64)) {
		a12_worry(7);
		return 0;
	}
	ipl = spla12dc();
	for(i=0; i<20; ++i) {
		switch(ms->txsv) {
		    case tx_idle:
			if(c!=-1) {
				ms->lastt = c;
				ms->lasttc = channel;
				ms->cdr = (ms->cdr & (A12C_TXACK | A12C_TXRDY))
					| channel<<8
					| c;
				*ms->xcdr = ms->cdr;
				wmb();
				ms->cdr = ((ms->cdr & A12C_TXACK) &~ A12C_TXRDY)
					| ms->txrdy_out
					| channel<<8
					| c;
				*ms->xcdr = ms->cdr;
				wmb();
				ms->txsv = tx_braaw;
				c = -1;
				continue;
			}
			break;
		    case tx_braaw:
			mb();
			if((*ms->xcdr & A12C_TXACK)==ms->txack_in) {
				ms->txrdy_out ^= A12C_TXRDY;
				ms->txack_in  ^= A12C_TXACK;
				ms->txsv       = tx_idle;
				ms->tbytes++;
				continue;
			}
			break;
		    default:
			(void)splx(ipl);
			a12_worry(5);
			return 0;
		}
		switch(ms->rxsv) {
		    case rx_idle:
			mb();
			t = *ms->xcdr;
			if((t & A12C_TXRDY)==ms->txrdy_in) {
				ms->cdr = (ms->cdr & ~A12C_TXACK)
					| ms->txack_out;
				*ms->xcdr = ms->cdr;
				wmb();
				ms->c2b_char  = t & 0xff;
				ms->lastr     = ms->c2b_char;
				ms->c2b_channel = (t & CHMASK) >> 8;
				ms->lastrc    = ms->c2b_channel;
				ms->txrdy_in ^= A12C_TXRDY;
				ms->txack_out^= A12C_TXACK;
				ms->rxsv      = rx_busy;
				continue;
			}
			break;
		    case rx_busy:
			break;
		    default:
			(void)splx(ipl);
			a12_worry(6);
			return 0;
		}
		if(c==-1)
			break;
	}
	(void)splx(ipl);
	return c!=-1;
}

static void
a12_worry(int worry_number)
{
	a12console_last_unexpected_error = worry_number;
}
