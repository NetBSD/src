/*	$NetBSD: bmcons.c,v 1.7 2000/06/29 07:59:30 mrg Exp $	*/
/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Sony Corp. and Kazumasa Utashiro of Software Research Associates, Inc.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
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
 * from: $Hdr: cons.c,v 4.300 91/06/09 06:34:41 root Rel41 $ SONY
 *
 *	@(#)bmcons.c	8.1 (Berkeley) 6/10/93
 */

/*
 * console driver
 */
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/map.h>
#include <sys/buf.h>
#include <sys/clist.h>
#include <sys/file.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/pte.h>
#include <machine/cpu.h>
#include <machine/autoconf.h>
#include "fb.h"

#include <newsmips/dev/sccparam.h>

#include <dev/cons.h>

#define BMMAJOR 22

extern void to_monitor();
extern void bmcons_putc();
extern int cnmctl();
extern void auto_dimmer();
extern int vt100_write();
extern int bitmap_get_param();
extern int bitmap_set_param();

static void bmcnparam(struct tty *, struct termios *);

static void bmcnprobe __P((struct consdev *));
static void bmcninit __P((struct consdev *));
static int  bmcngetc __P((dev_t));
static void bmcnputc __P((dev_t, int));
static void bmcnpollc __P((dev_t, int));

struct consdev consdev_bm = {
	bmcnprobe,
	bmcninit,
	bmcngetc,
	bmcnputc,
	bmcnpollc,
	NULL,
};

static void
bmcnprobe(cn)
	struct consdev *cn;
{
}


static void
bmcninit(cn)
	struct consdev *cn;
{
	cn->cn_dev = makedev(BMMAJOR, 0);
	cn->cn_pri = CN_INTERNAL;
}

static int
bmcngetc(dev)
	dev_t dev;
{
	printf("bmcngetc\n");
	to_monitor(8);	/* XXX */
	return 0;
}

static void
bmcnputc(dev, c)
	dev_t dev;
	int c;
{
	bmcons_putc(c);
}

static void
bmcnpollc(dev, on)
	dev_t dev;
	int on;
{
	printf("bmcnpollc\n");
}

#define	CN_RXE		RXE
#define	CN_TXE		TXE
#define	CN_ON		(RXE|TXE|RTS|DTR)
#define	CN_OFF		0
#define	CN_RTS		RTS
#define	CN_DTR		DTR
#define	CN_CTS		CTS
#define	CN_DCD		DCD
#define	CN_DSR		DSR
#define	CN_RI		RI
#define	CN_BRK		XBREAK

/*
 * Local variables for the driver
 */

#define	splcons	spltty

static char bmcn_active[1];
static char bmcn_stopped[1];
static struct tty bmcn_tty[1];

void	ttrstrt __P((void *));
void	bmcnrint __P((char));
void	bmcnxint __P((void));

static void bmcnstart __P((struct tty*));
/*static int bmcnsint __P((int)); */
static int bmcn_init __P((void));
static void bmcn_enable __P((void));
static void bmcn_start __P((void));
static void bmcn_output __P((struct tty*, int));
static void bmcn_stop __P((int));
static int bmcn_get_param __P((void));
static void bmcn_set_param __P((int));

#if 0
static int dmtocn __P((int));
static int cntodm __P((int));
#endif

void
bmcnattach()
{
	struct tty *tp;
	extern struct clist scode_buf;
	extern struct clist keyboard_buf;

	tp = &bmcn_tty[0];
	bzero(tp, sizeof(struct tty));
	clalloc(&tp->t_rawq, 1024, 1);
	clalloc(&tp->t_canq, 1024, 1);
	clalloc(&tp->t_outq, 1024, 0);

	clalloc(&scode_buf, 1024, 1);
	clalloc(&keyboard_buf, 1024, 1);

	tp->t_dev = makedev(BMMAJOR, 0);
	tty_attach(tp);
}


/*
 * Open console. Turn on console if this is the first use of it.
 */
/*ARGSUSED*/
int
bmcnopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	register struct tty *tp = &bmcn_tty[0];
	static int firstopen = 1;

	if (firstopen) {
		tty_attach(tp);
		firstopen = 0;
	}

	if (bmcn_active[0] == 0) {
		if (bmcn_init() < 0)
			return (ENXIO);
		bmcn_enable();
		bmcn_active[0] = 1;
	}
	if (tp->t_state & TS_XCLUDE && p->p_ucred->cr_uid != 0)
		return (EBUSY);
	tp->t_oproc = bmcnstart;
	/*
	 * If this is first open, initialze tty state to default.
	 */
	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttychars(tp);
		if (tp->t_ispeed == 0) {
			tp->t_iflag = TTYDEF_IFLAG;
			tp->t_oflag = TTYDEF_OFLAG;
			tp->t_cflag = TTYDEF_CFLAG;
			tp->t_lflag = TTYDEF_LFLAG;
			tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		}
		bmcnparam(tp, &tp->t_termios);
		ttsetwater(tp);
	}
	/*
	 * Wait receiver and status interrupt
	 */
	(void) cnmctl(CN_ON, DMSET);
	tp->t_state |= TS_CARR_ON;
	return ((*linesw[tp->t_line].l_open)(dev, tp));
}

/*
 * Close console.
 */
/*ARGSUSED*/
int
bmcnclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	register struct tty *tp = &bmcn_tty[0];

	(*linesw[tp->t_line].l_close)(tp, flag);
	(void) cnmctl(CN_BRK, DMBIC);
	ttyclose(tp);
	return (0);
}

/*ARGSUSED*/
int
bmcnread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register struct tty *tp = &bmcn_tty[0];

	return ((*linesw[tp->t_line].l_read)(tp, uio, flag));
}

/*ARGSUSED*/
int
bmcnwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	register struct tty *tp = &bmcn_tty[0];

	return ((*linesw[tp->t_line].l_write)(tp, uio, flag));
}

struct tty *
bmcntty(dev)
	dev_t dev;
{
	return &bmcn_tty[0];
}


/*
 * console receiver interrupt.
 */
void
_bmcnrint(buf, n)
	register char *buf;
	register int n;
{
	register struct tty *tp = &bmcn_tty[0];
	register int (*rint)();

	if ((tp->t_state & TS_ISOPEN) == 0) {
		wakeup((caddr_t)&tp->t_rawq);
		bmcn_enable();
		return;
	}
	/*
	 * Loop fetching characters from the silo for console
	 * until there are no more in the silo.
	 */
	rint = linesw[tp->t_line].l_rint;
	while (--n >= 0)
		(*rint)(*buf++, tp);
	bmcn_enable();
}

/*
 * Ioctl for console.
 */
/*ARGSUSED*/
int
bmcnioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	register struct tty *tp = &bmcn_tty[0];
	int error;

	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);
	error = ttioctl(tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);

	switch (cmd) {

	case TIOCSBRK:
		(void) cnmctl(CN_BRK, DMBIS);
		break;

	case TIOCCBRK:
		(void) cnmctl(CN_BRK, DMBIC);
		break;

	case TIOCSDTR:
		(void) cnmctl(CN_DTR|CN_RTS, DMBIS);
		break;

	case TIOCCDTR:
		(void) cnmctl(CN_DTR|CN_RTS, DMBIC);
		break;

#if 0
	case TIOCMSET:
		(void) cnmctl(dmtocn(*(int *)data), DMSET);
		break;

	case TIOCMBIS:
		(void) cnmctl(dmtocn(*(int *)data), DMBIS);
		break;

	case TIOCMBIC:
		(void) cnmctl(dmtocn(*(int *)data), DMBIC);
		break;

	case TIOCMGET:
		*(int *)data = cntodm(cnmctl(0, DMGET));
		break;
#endif

	default:
		return (ENOTTY);
	}
	return (0);
}

int
bmcnpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{

	return (ttpoll(makedev(major(dev), 0), events, p));
}

#if 0
static int
dmtocn(bits)
	register int bits;
{
	register int b;

	b = 0;
	if (bits & DML_LE)  b |= CN_TXE|CN_RXE;
	if (bits & DML_DTR) b |= CN_DTR;
	if (bits & DML_RTS) b |= CN_RTS;
	if (bits & DML_CTS) b |= CN_CTS;
	if (bits & DML_CAR) b |= CN_DCD;
	if (bits & DML_RNG) b |= CN_RI;
	if (bits & DML_DSR) b |= CN_DSR;
	return(b);
}

static int
cntodm(bits)
	register int bits;
{
	register int b;

	b = 0;
	if (bits & (CN_TXE|CN_RXE)) b |= DML_LE;
	if (bits & CN_DTR) b |= DML_DTR;
	if (bits & CN_RTS) b |= DML_RTS;
	if (bits & CN_CTS) b |= DML_CTS;
	if (bits & CN_DCD) b |= DML_CAR;
	if (bits & CN_RI)  b |= DML_RNG;
	if (bits & CN_DSR) b |= DML_DSR;
	return(b);
}
#endif
 
/*
 * Set parameters from open or stty into the console hardware
 * registers.
 */
static void
bmcnparam(tp, t)
	register struct tty *tp;
	register struct termios *t;
{
	register int param;
	register int cflag = t->c_cflag;
	int s;

	/*
	 * Block interrupts so parameters will be set
	 * before the line interrupts.
	 */
	s = splcons();
	if ((tp->t_ispeed)==0) {
		tp->t_cflag |= HUPCL;
		(void) cnmctl(CN_OFF, DMSET);
		(void) splx(s);
		return;
	}

	param = bmcn_get_param() &
		~(CHAR_SIZE|PARITY|EVEN|STOPBIT|BAUD_RATE|NOCHECK);
	if ((cflag & CREAD) == 0)
		param &= ~RXE;
	switch (cflag & CSIZE) {
	    case CS5: break;
	    case CS6: param |= C6BIT; break;
	    case CS7: param |= C7BIT; break;
	    case CS8: param |= C8BIT; break;
	}
	if (cflag & PARENB)
		param |= PARITY;
	if ((cflag & PARODD) == 0)
		param |= EVEN;
	if ((tp->t_iflag & INPCK) == 0)
		param |= NOCHECK;
	if (cflag & CSTOPB)
		param |= STOP2;
	else
		param |= STOP1;
	bmcn_set_param(param);
	(void) splx(s);
}

/*
 * console transmitter interrupt.
 * Restart the idle line.
 */
void
_bmcnxint(count)
	int count;
{
	register struct tty *tp = &bmcn_tty[0];
	int s;

	bmcn_stopped[0] = 0;
	tp->t_state &= ~TS_BUSY;
	s = splcons();
	if (tp->t_state & TS_FLUSH)
		tp->t_state &= ~TS_FLUSH;
	else
		ndflush(&tp->t_outq, count);
	(void) splx(s);
	if (tp->t_line)
		(*linesw[tp->t_line].l_start)(tp);
	else
		bmcnstart(tp);
}

/*
 * Start (restart) transmission on the console.
 */
static void
bmcnstart(tp)
	register struct tty *tp;
{
	register int nch;
	int s;

	/*
	 * Must hold interrupts in following code to prevent
	 * state of the tp from changing.
	 */
	s = splcons();
	/*
	 * If it's currently active, or delaying, no need to do anything.
	 */
	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
		goto out;
	/*
	 * If ther are still characters in the IOP,
	 * just reenable transmit.
	 */
	if (bmcn_stopped[0]) {
		bmcn_stopped[0] = 0;
		bmcn_start();
		goto out;
	}
	/*
	 * If there are sleepers, and output has drained below low
	 * water mark, wake up the sleepers.
	 */
	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (tp->t_state & TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)&tp->t_outq);
		}
		selwakeup(&tp->t_wsel);
	}
	/*
	 * Now restart transmission unless the output queue is
	 * empty.
	 */
	if (tp->t_outq.c_cc == 0)
		goto out;
	if (tp->t_flags & (RAW|LITOUT))
		nch = ndqb(&tp->t_outq, 0);
	else {
		nch = ndqb(&tp->t_outq, 0200);
		/*
		 * If first thing on queue is a delay process it.
		 */
		if (nch == 0) {
			nch = getc(&tp->t_outq);
			callout_reset(&tp->t_rstrt_ch, (nch&0x7f)+6,
			    ttrstrt, tp);
			tp->t_state |= TS_TIMEOUT;
			goto out;
		}
	}
	/*
	 * If characters to transmit, restart transmission.
	 */
	if (nch) {
		tp->t_state |= TS_BUSY;
		bmcn_output(tp, nch);
	}
out:
	(void) splx(s);
}

/*
 * Stop output on a line, e.g. for ^S/^Q or output flush.
 */
/*ARGSUSED*/
void
bmcnstop(tp, flag)
	register struct tty *tp;
	int flag;
{
	register int s;

	/*
	 * Block input/output interrupts while messing with state.
	 */
	s = splcons();
	if (tp->t_state & TS_BUSY) {
		bmcn_stop(0);
		bmcn_stopped[0] = 1;
		if ((tp->t_state & TS_TTSTOP) == 0) {
			tp->t_state |= TS_FLUSH;
			bmcn_stop(1);
		}
	}
	(void) splx(s);
}

/*
 * console modem control
 */
int
cnmctl(bits, how)
	int bits, how;
{
	register int mbits;
	int s;

	bits &= (RXE|TXE|RTS|DTR|XBREAK);

	s = splcons();

	mbits = bmcn_get_param();
	switch (how) {
	case DMSET:
		mbits = mbits & ~(RXE|TXE|RTS|DTR|XBREAK) | bits;
		break;

	case DMBIS:
		mbits |= bits;
		break;

	case DMBIC:
		mbits &= ~bits;
		break;

	case DMGET:
		(void) splx(s);
		return(mbits);
	}
	bmcn_set_param(mbits);

	(void) splx(s);
	return(mbits);
}

/*
 * console status interrupt
 */
void
_bmcnsint(stat)
	int stat;
{
	register struct tty *tp = &bmcn_tty[0];

	if (stat & OVERRUN_ERROR)
		printf("console: fifo overflow\n");
	if (stat & RBREAK)
		(*linesw[tp->t_line].l_rint)
		    (tp->t_flags & RAW ? '\0' : tp->t_cc[VINTR], tp);
}

/*
 * console control interrupt
 */
void
bmcncint()
{
	printf("bmcncint:\n");
}

/*
 * Machine dependent functions
 *
 *	cn_init()
 *	cnrint()
 *	cnxint()
 *	cnsint()
 *	cn_enable()
 *	cn_output()
 *	cn_start()
 *	cn_stop()
 *	cn_get_param()
 *	cn_set_param()
 */
#ifdef IPC_MRX
#include <news3400/newsipc/newsipc.h>
#include <news3400/mrx/h/cio.h>
#include <news3400/mrx/h/console.h>

#ifdef mips
#define ipc_phys(x)	K0_TT0(x)
#define ipc_log(x)	TT0_K0(x)
#else
#define ipc_phys(x)	(caddr_t)((int)(x) & ~0x80000000)
#define ipc_log(x)	(caddr_t)((int)(x) | 0x80000000)
#endif

#if NFB > 0
extern char *ext_fnt_addr[];
extern char *ext_fnt24_addr[];
#endif /* NFB > 0 */

int	port_cnrecv;
int	port_cnxmit;
int	port_cnstat;
int	port_cnctrl;
int	port_cnfont;
int	port_cnrecv_iop;
int	port_cnxmit_iop;
int	port_cnstat_iop;
int	port_cnctrl_iop;

int	cnfont();

cn_init()
{
	struct cons_ctrl_req req;
	int *reply;

	port_cnrecv = port_create("@cnrecv", cnrint, -1);
	port_cnxmit = port_create("@cnxmit", cnxint, -1);
	port_cnctrl = port_create("@cnctrl", NULL, 0);
	port_cnstat = port_create("@cnstat", cnsint, -1);
	port_cnfont = port_create("@cnfont", cnfont, -1);

	/* use NULL action port */
	port_cnrecv_iop = object_query("cons_input");
	port_cnxmit_iop = object_query("cons_output");
	port_cnctrl_iop = object_query("cons_ctrl");
	port_cnstat_iop = object_query("cons_stat");
	req.cons_func = CIO_ASKDEVICE;
	msg_send(port_cnctrl_iop, port_cnctrl, &req, sizeof(req), 0);
	msg_recv(port_cnctrl, NULL, &reply, NULL, 0);
	tty00_is_console = *reply;
	msg_free(port_cnctrl);
#if NFB > 0
	req.cons_func = CIO_SET16FNT;
	req.cons_addr = (char *)ipc_phys(ext_fnt_addr);
	msg_send(port_cnctrl_iop, port_cnctrl, &req, sizeof(req), 0);
	msg_recv(port_cnctrl, NULL, NULL, NULL, 0);
	req.cons_func = CIO_SET24FNT;
	req.cons_addr = (char *)ipc_phys(ext_fnt24_addr);
	msg_send(port_cnctrl_iop, port_cnctrl, &req, sizeof(req), 0);
	msg_recv(port_cnctrl, NULL, NULL, NULL, 0);
#endif
	return (0);
}

cn_enable()
{
	int len;

	len = MAX_CIO;
	msg_send(port_cnrecv_iop, port_cnrecv, &len, sizeof(len), 0);
}

cnrint(port)
	int port;
{
	int len;
	char *buf;

	msg_recv(port, NULL, &buf, &len, 0);
#ifdef mips
	_cnrint((char *)MACH_CACHED_TO_UNCACHED(buf), len);
#else
	dcia();
	_cnrint(buf, len);
#endif
	msg_free(port);
}

cnxint(port)
	int port;
{
	int *len;

	msg_recv(port, NULL, &len, NULL, 0);
	_cnxint(*len);
}

cn_start()
{
	int func;

	func = CIO_START;
	msg_send(port_cnctrl_iop, 0, &func, sizeof(func), 0);
}

cn_output(tp, n)
	struct tty *tp;
	int n;
{

	msg_send(port_cnxmit_iop, port_cnxmit, tp->t_outq.c_cf,
	    imin(n, MAX_CIO), 0);
}

cn_stop(flush)
	int flush;
{
	int	func;

	func = flush ? CIO_FLUSH : CIO_STOP;
	msg_send(port_cnctrl_iop, 0, &func, sizeof(func), 0);
}

cnsint(port)
	int port;
{
	int *stat;

	msg_recv(port, NULL, &stat, NULL, 0);
	_cnsint(*stat);
	msg_send(port_cnstat_iop, port_cnstat, NULL, 0, 0);
}

cn_get_param()
{
	struct cons_ctrl_req req;
	int *reply, param;

	req.cons_func = CIO_GETPARAMS;
	/* message length 8 means 2 * sizeof(int) : func and status */
	msg_send(port_cnctrl_iop, port_cnctrl, &req, 8, 0);
	msg_recv(port_cnctrl, NULL, &reply, NULL, 0);
	param = *reply;
	msg_free(port_cnctrl);

	return (param);
}

cn_set_param(param)
	int param;
{
	struct cons_ctrl_req req;

	req.cons_func = CIO_SETPARAMS;
	req.cons_status = param;

	/* message length 8 means 2 * sizeof(int) : func and status */
	msg_send(port_cnctrl_iop, 0, &req, 8, 0);
}

cnfont(port)
	int port;
{
	int *func;

	msg_recv(port, NULL, &func, NULL, 0);
#if NFB > 0
	switch (*func) {

	case FONT_JISROMAN:
		font_jisroman();
		font_jisroman24();
		break;

	case FONT_ASCII:
		font_ascii();
		font_ascii24();
		break;
	}
#endif /* NFB > 0 */
	msg_free(port);
}
#endif /* IPC_MRX */

#ifdef CPU_SINGLE
#include <machine/framebuf.h>
#include <newsmips/dev/fbdefs.h>

int lastcount;
int start_dimmer = 1;

static int
bmcn_init()
{

	if (start_dimmer) {
		auto_dimmer();
		start_dimmer = 0;
	}
	return (0);
}

static void
bmcn_enable()
{

	/* nothing to do */
}

void
bmcnrint(code)
	char code;
{
	_bmcnrint(&code, 1);
}

void
bmcnxint()
{
	_bmcnxint(lastcount);
}

static void
bmcn_start()
{
	/* nothing to do */
}

static void
bmcn_output(tp, n)
	struct tty *tp;
	int n;
{

	lastcount = vt100_write(0, tp->t_outq.c_cf, n);
	bmcnxint();
}

static void
bmcn_stop(flush)
	int flush;
{

	/* nothing to do */
}

#if 0 /* not used */
static void
bmcnsint(param)
	int param;
{
	_bmcnsint(param);
}
#endif

static int
bmcn_get_param()
{

	return (bitmap_get_param());
}

static void
bmcn_set_param(param)
	int param;
{

	bitmap_set_param(param);
}
#endif /* CPU_SINGLE */
