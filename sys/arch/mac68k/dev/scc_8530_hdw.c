/* 
 * Mach Operating System
 * Copyright (c) 1993-1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * HISTORY
 * $Log: scc_8530_hdw.c,v $
 * Revision 1.1.1.1  1993/09/29 06:09:28  briggs
 * The current MacBSD architecture sub-directory for NetBSD-0.9--more or
 * less.  It needs some help to get to be -current.  Support for minimal
 * hardware on the SE/30, II, IIx, and IIcx exists.
 *
 * Revision 2.9  93/03/26  17:58:01  mrt
 * 	No minor()s, no dev_t.
 * 	[93/03/18            af]
 * 
 * Revision 2.8  93/03/09  10:52:21  danner
 * 	GCC quiet.
 * 	[93/03/07  13:29:58  af]
 * 
 * 	Post-debugging lint.
 * 	[93/03/05            af]
 * 
 * Revision 2.7  93/02/05  08:05:17  danner
 * 	Flamingo, full_modem and isa_console per-line.
 * 	[93/02/04            af]
 * 
 * Revision 2.6  93/01/14  17:21:44  danner
 * 	static/extern cleanups.
 * 	[93/01/14            danner]
 * 
 * 	Proper spl typing.
 * 	[92/11/30            af]
 * 
 * Revision 2.5  92/05/05  10:04:59  danner
 * 	Fixed how the interrupt routine plays with priorities.
 * 	Ask for buffering on all lines.
 * 	[92/05/04  11:15:43  af]
 * 
 * 	Fixed for more than just rconsole-ing.  Two bugs: the chip
 * 	needs one char out to generate the xmit-empty (so it really
 * 	is a xmit-done) interrupt, and the t_addr game was not played
 * 	properly.
 * 	Tested both on maxine and the two serial lines kmin has.
 * 	[92/04/14  11:47:26  af]
 * 
 * Revision 2.4  92/02/19  16:46:10  elf
 * 	Uhmm, lotsa changes.  Basically, got channel B working
 * 	and made it possible to use it as rconsole line.
 * 	Missing modem bitsies only.
 * 	A joint Terri&Sandro feature presentation.
 * 	[92/02/10  17:03:08  af]
 * 
 * Revision 2.3  91/08/28  11:09:53  jsb
 * 	Fixed scc_scan to actually check the tp->state each time,
 * 	we are not notified when the MI code brutally zeroes it
 * 	on close and so we cannot update our software CARrier.
 * 	[91/08/27  16:18:05  af]
 * 
 * Revision 2.2  91/08/24  11:52:54  af
 * 	Created, from the Zilog specs:
 * 	"Z8530 SCC Serial Communications Controller, Product Specification"
 * 	in the "1983/84 Components Data Book" pp 409-429, September 1983
 * 	Zilog, Campbell, CA 95008
 * 	[91/06/28            af]
 * 
 */
#ident "$Id: scc_8530_hdw.c,v 1.1.1.1 1993/09/29 06:09:28 briggs Exp $"
/*
 *	File: scc_8530_hdw.c
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	6/91
 *
 *	Hardware-level operations for the SCC Serial Line Driver
 */

#include <scc.h>
#if	NSCC > 0
#include <bm.h>
#include <platforms.h>

#include <mach_kdb.h>

#include <machine/machparam.h>		/* spl definitions */
#include <sys/types.h>
#include <mach/std_types.h>
#include <device/io_req.h>
#include <device/tty.h>

#include <chips/busses.h>
#include <chips/serial_defs.h>
#include <chips/screen_defs.h>

/* Alignment and padding */
#if	defined(DECSTATION)
/*
 * 3min's padding
 */
typedef struct {
	char			pad0;
	volatile unsigned char	datum;
	char			pad1[2];
} scc_padded1_register_t;

#define	scc_register_t	scc_padded1_register_t
#endif

#if	defined(FLAMINGO)
typedef struct {
	volatile unsigned int	datum;
	unsigned int		pad1;
} scc_padded1_register_t;

#define	scc_register_t	scc_padded1_register_t

#define	scc_set_datum(d,v)	(d) = (volatile unsigned int) (v) << 8, wbflush()
#define	scc_get_datum(d,v)	(v) = ((d) >> 8) & 0xff

#endif

#include <chips/scc_8530.h>	/* needs the above defs */

/*
 * On the 3min keyboard and mouse come in on channels A
 * of the two units.  The MI code expects them at 'lines'
 * 0 and 1, respectively.  So we map here back and forth.
 * Note also the MI code believes unit 0 has four lines.
 */

#define	SCC_KBDUNIT	1
#define	SCC_PTRUNIT	0

mi_to_scc(unitp, linep)
	int	*unitp, *linep;
{
	/* only play games on MI 'unit' 0 */
	if (*unitp) {
		/* e.g. by mapping the first four lines specially */
		*unitp++;
		return;
	}

	/* always get unit=0 (console) and line = 0|1 */
	if (*linep == SCREEN_LINE_KEYBOARD) {
		*unitp = SCC_KBDUNIT;
		*linep = SCC_CHANNEL_A;
	} else if (*linep == SCREEN_LINE_POINTER) {
		*unitp = SCC_PTRUNIT;
		*linep = SCC_CHANNEL_A;
	} else {
		*unitp = (*linep & 1);
		*linep = SCC_CHANNEL_B;
	}
/* line 0 is channel B, line 1 is channel A */
}

#define	NSCC_LINE	2	/* 2 ttys per chip */

/* only care for mapping to ttyno */
scc_to_mi(sccunit, sccline)
{
	if (sccunit > 1)
		return (sccunit * NSCC_LINE + sccline);
	/* only for console (first pair of SCCs): */
	if (sccline == SCC_CHANNEL_A)
		return ((!sccunit) & 1);
	return 2+sccunit;
}


/*
 * Driver status
 */
struct scc_softc {
	scc_regmap_t	*regs;

	/* software copy of some write regs, for reg |= */
	struct softreg {
		unsigned char	wr1;
		unsigned char	wr4;
		unsigned char	wr5;
		unsigned char	wr14;
	} softr[2];	/* per channel */

	unsigned short	breaks;
	unsigned short	fake;	/* missing rs232 bits, channel A */
	char		polling_mode;
	char		softCAR, osoftCAR;
	char		probed_once;

	boolean_t	full_modem;
	boolean_t	isa_console;

} scc_softc_data[NSCC];

typedef struct scc_softc *scc_softc_t;

scc_softc_t	scc_softc[NSCC];

scc_softCAR(unit, line, on)
{
	mi_to_scc(&unit, &line);
	if (on)
		scc_softc[unit]->softCAR |= 1<<line;
	else
		scc_softc[unit]->softCAR &= ~(1 << line);
}


/*
 * BRG formula is:
 *				ClockFrequency
 *	BRGconstant = 	---------------------------  -  2
 *			2 * BaudRate * ClockDivider
 */
/* Speed selections with Pclk=7.3728Mhz, clock x16 */
static
short	scc_speeds[] =
	/* 0   50    75    110  134.5  150  200   300  600 1200 1800 2400 */
	{  0, 4606, 3070, 2093, 1711, 1534, 1150, 766, 382, 190, 126, 94,

	/* 4800 9600 19.2k 38.4k */
	  46,   22,  10,    4};

/*
 * Definition of the driver for the auto-configuration program.
 */

int	scc_probe(), scc_attach(), scc_intr();

caddr_t	scc_std[NSCC] = { 0 };
struct	bus_device *scc_info[NSCC];
struct	bus_driver scc_driver = 
        { scc_probe, 0, scc_attach, 0, scc_std, "scc", scc_info,};

/*
 * Adapt/Probe/Attach functions
 */
boolean_t		scc_uses_modem_control = FALSE;/* patch this with adb */

set_scc_address( sccunit, regs, has_modem, isa_console)
	caddr_t		regs;
	boolean_t	has_modem;
	boolean_t	isa_console;
{
	extern int	scc_probe(), scc_param(), scc_start(),
			scc_putc(), scc_getc(),
			scc_pollc(), scc_mctl(), scc_softCAR();

	scc_std[sccunit] = regs;
	scc_softc_data[sccunit].full_modem = has_modem & scc_uses_modem_control;
	scc_softc_data[sccunit].isa_console = isa_console;

	/* Do this here */
	console_probe		= scc_probe;
	console_param		= scc_param;
	console_start		= scc_start;
	console_putc		= scc_putc;
	console_getc		= scc_getc;
	console_pollc		= scc_pollc;
	console_mctl		= scc_mctl;
	console_softCAR		= scc_softCAR;

}

scc_probe( xxx, ui)
	struct bus_device *ui;
{
	int             sccunit = ui->unit;
	scc_softc_t     scc;
	register int	val;
	register scc_regmap_t	*regs;

	regs = (scc_regmap_t *)scc_std[sccunit];
	if (regs == 0)
		return 0;

	/*
	 * See if we are here
	 */
	if (check_memory(regs, 0)) {
		/* no rides today */
		return 0;
	}

	scc = &scc_softc_data[sccunit];

	if (scc->probed_once++){
		return 1;
	}
	/*
	 * Chip once-only initialization
	 *
	 * NOTE: The wiring we assume is the one on the 3min:
	 *
	 *	out	A-TxD	-->	TxD	keybd or mouse
	 *	in	A-RxD	-->	RxD	keybd or mouse
	 *	out	A-DTR~	-->	DTR	comm
	 *	out	A-RTS~	-->	RTS	comm
	 *	in	A-CTS~	-->	SI	comm
	 *	in	A-DCD~	-->	RI	comm
	 *	in	A-SYNCH~-->	DSR	comm
	 *	out	B-TxD	-->	TxD	comm
	 *	in	B-RxD	-->	RxD	comm
	 *	in	B-RxC	-->	TRxCB	comm
	 *	in	B-TxC	-->	RTxCB	comm
	 *	out	B-RTS~	-->	SS	comm
	 *	in	B-CTS~	-->	CTS	comm
	 *	in	B-DCD~	-->	CD	comm
	 */

	scc_softc[sccunit] = scc;
	scc->regs = regs;

	scc->fake = 1<<SCC_CHANNEL_A;

	{ 
		register int i;
		/* We need this in scc_start only, hence the funny
		   value: we need it non-zero and we want to avoid
		   too much overhead in getting to (scc,regs,line) */
		for (i = 0; i < NSCC_LINE; i++) {
			register struct tty	*tp;

			tp = console_tty[scc_to_mi(sccunit,i)];
			tp->t_addr = (char*)(0x80000000L + (sccunit<<1) + (i&1));
			/* do min buffering */
			tp->t_state |= TS_MIN;
		}
	}

	/* make sure reg pointer is in known state */
	scc_init_reg(regs, SCC_CHANNEL_A);
	scc_init_reg(regs, SCC_CHANNEL_B);

	/* reset chip, fully */
	scc_write_reg(regs, SCC_CHANNEL_A, SCC_WR9, SCC_WR9_HW_RESET);
	delay(50000);/*enough ? */
	scc_write_reg(regs, SCC_CHANNEL_A, SCC_WR9, 0);

	/* program the interrupt vector */
	scc_write_reg(regs, SCC_CHANNEL_A, SCC_WR2, 0xf0);
	scc_write_reg(regs, SCC_CHANNEL_B, SCC_WR2, 0xf0);
	scc_write_reg(regs, SCC_CHANNEL_A, SCC_WR9, SCC_WR9_VIS);

	/* most of the init is in scc_param() */

	/* timing base defaults */
	scc->softr[SCC_CHANNEL_A].wr4 = SCC_WR4_CLK_x16;
	scc->softr[SCC_CHANNEL_B].wr4 = SCC_WR4_CLK_x16;

	/* enable DTR, RTS and SS */
	scc->softr[SCC_CHANNEL_B].wr5 = SCC_WR5_RTS;
	scc->softr[SCC_CHANNEL_A].wr5 = SCC_WR5_RTS | SCC_WR5_DTR;

	/* baud rates */
	val = SCC_WR14_BAUDR_ENABLE|SCC_WR14_BAUDR_SRC;
	scc->softr[SCC_CHANNEL_B].wr14 = val;
	scc->softr[SCC_CHANNEL_A].wr14 = val;

	/* interrupt conditions */
	val =	SCC_WR1_RXI_ALL_CHAR | SCC_WR1_PARITY_IE |
		SCC_WR1_EXT_IE | SCC_WR1_TX_IE;		
	scc->softr[SCC_CHANNEL_A].wr1 = val;
	scc->softr[SCC_CHANNEL_B].wr1 = val;

	/*
	 * After probing, any line that should be active
	 * (keybd,mouse,rcline) is activated via scc_param().
	 */

	scc_set_modem_control(scc, scc->full_modem);

#if defined(KMIN) || defined (FLAMINGO)
	/*
	 * Crock: MI code knows of unit 0 as console, we need
	 * unit 1 as well since the keyboard is there
	 * This is acceptable on maxine, which has to call its
	 * only one chip unit 1 so that rconsole is happy.
	 */
	if (sccunit == 0) {
		struct bus_device d;
		d = *ui;
		d.unit = 1;
		scc_probe( xxx, &d);
	}
#endif
	return 1;
}

boolean_t scc_timer_started = FALSE;

scc_attach(ui)
	register struct bus_device *ui;
{
	int sccunit = ui->unit;
	extern scc_scan();
	extern int tty_inq_size;
	int i;

	/* We only have 4 ttys, but always at 9600
	 * Give em a lot of room (plus dma..)
	 */
	tty_inq_size = 4096;
	if (!scc_timer_started) {
		/* do all of them, before we call scc_scan() */
		/* harmless if done already */
		for (i = 0; i < NSCC*NSCC_LINE; i++)
			ttychars(console_tty[i]);

		scc_timer_started = TRUE;
		scc_scan();
	}

#if	NBM > 0
	if (SCREEN_ISA_CONSOLE() && scc_softc[sccunit]->isa_console) {
		printf("\n sl0: ");
		if (sccunit && rcline == 3) printf("( rconsole )");

		if (sccunit == SCC_KBDUNIT) {
			printf("\n sl1: "); lk201_attach(0, sccunit >> 1);
		} else if (sccunit == SCC_PTRUNIT) {
			printf("\n sl1: "); mouse_attach(0, sccunit >> 1);
		}
	} else
#endif	/*NBM > 0*/
	{
		printf("%s", (sccunit == 1) ?
			"\n sl0: ( alternate console )\n sl1:" :
			"\n sl0:\n sl1:");
	}
}

/*
 * Would you like to make a phone call ?
 */
scc_set_modem_control(scc, on)
	scc_softc_t      scc;
	boolean_t	on;
{
	if (on)
		/* your problem if the hardware then is broke */
		scc->fake = 0;
	else
		scc->fake = 3;
	scc->full_modem = on;
	/* user should do an scc_param() ifchanged */
}

/*
 * Polled I/O (debugger)
 */
scc_pollc(unit, on)
	boolean_t		on;
{
	scc_softc_t		scc;
	int			line = SCREEN_LINE_KEYBOARD,
				sccunit = unit;

	mi_to_scc(&sccunit, &line);

	scc = scc_softc[sccunit];
	if (on) {
		scc->polling_mode++;
#if	NBM > 0
		screen_on_off(unit, TRUE);
#endif	NBM > 0
	} else
		scc->polling_mode--;
}

/*
 * Interrupt routine
 */
int scc_intr_count;

scc_intr(unit,spllevel)
	spl_t	spllevel;
{
	scc_softc_t		scc = scc_softc[unit];
	register scc_regmap_t	*regs = scc->regs;
	register int		rr1, rr2;
	register int		c;

scc_intr_count++;

#if	mips
	splx(spllevel);		/* lower priority */
#endif

next_intr:

	scc_read_reg(regs, SCC_CHANNEL_B, SCC_RR2, rr2);

	rr2 = SCC_RR2_STATUS(rr2);

	/* are we done yet ? */
	if (rr2 == 6) {	/* strange, distinguished value */
		register int rr3;
		scc_read_reg(regs, SCC_CHANNEL_A, SCC_RR3, rr3);
		if (rr3 == 0)
			return;
	}

	if ((rr2 == SCC_RR2_A_XMIT_DONE) || (rr2 == SCC_RR2_B_XMIT_DONE)) {

		register chan = (rr2 == SCC_RR2_A_XMIT_DONE) ?
					SCC_CHANNEL_A : SCC_CHANNEL_B;

		scc_write_reg(regs, SCC_CHANNEL_A, SCC_RR0, SCC_RESET_HIGHEST_IUS);
		c = cons_simple_tint(scc_to_mi(unit,chan), FALSE);

		if (c == -1) {
			/* no more data for this line */

			scc_read_reg(regs, chan, SCC_RR15, c);
			c &= ~SCC_WR15_TX_UNDERRUN_IE;
			scc_write_reg(regs, chan, SCC_WR15, c);

			c = scc->softr[chan].wr1 & ~SCC_WR1_TX_IE;
			scc_write_reg(regs, chan, SCC_WR1, c);
			scc->softr[chan].wr1 = c;

			c = cons_simple_tint(scc_to_mi(unit,chan), TRUE);
			if (c != -1)
				/* funny race, scc_start has been called already */
				scc_write_data(regs, chan, c);
		} else {
			scc_write_data(regs, chan, c);
			/* and leave it enabled */
		}
	}

	else if (rr2 == SCC_RR2_A_RECV_DONE) {
		int	err = 0;

		scc_write_reg(regs, SCC_CHANNEL_A, SCC_RR0, SCC_RESET_HIGHEST_IUS);
		if (scc->polling_mode)
			goto next_intr;

		scc_read_data(regs, SCC_CHANNEL_A, c);
		rr1 = scc_to_mi(unit,SCC_CHANNEL_A);
		cons_simple_rint (rr1, rr1, c, 0);
	}

	else if (rr2 == SCC_RR2_B_RECV_DONE) {
		int	err = 0;

		scc_write_reg(regs, SCC_CHANNEL_A, SCC_RR0, SCC_RESET_HIGHEST_IUS);
		if (scc->polling_mode)
			goto next_intr;

		scc_read_data(regs, SCC_CHANNEL_B, c);
		rr1 = scc_to_mi(unit,SCC_CHANNEL_B);
		cons_simple_rint (rr1, rr1, c, 0);
	}

	else if ((rr2 == SCC_RR2_A_EXT_STATUS) || (rr2 == SCC_RR2_B_EXT_STATUS)) {
		int chan = (rr2 == SCC_RR2_A_EXT_STATUS) ?
			SCC_CHANNEL_A : SCC_CHANNEL_B;
		scc_write_reg(regs, chan, SCC_RR0, SCC_RESET_EXT_IP);
		scc_write_reg(regs, SCC_CHANNEL_A, SCC_RR0, SCC_RESET_HIGHEST_IUS);
		scc_modem_intr(scc, chan);
	}

	else if ((rr2 == SCC_RR2_A_RECV_SPECIAL) || (rr2 == SCC_RR2_B_RECV_SPECIAL)) {
		register int chan = (rr2 == SCC_RR2_A_RECV_SPECIAL) ?
			SCC_CHANNEL_A : SCC_CHANNEL_B;

		scc_read_reg(regs, chan, SCC_RR1, rr1);
		if (rr1 & (SCC_RR1_PARITY_ERR | SCC_RR1_RX_OVERRUN | SCC_RR1_FRAME_ERR)) {
			int err;
			/* map to CONS_ERR_xxx MI error codes */
			err =	((rr1 & SCC_RR1_PARITY_ERR)<<8) |
				((rr1 & SCC_RR1_RX_OVERRUN)<<9) |
				((rr1 & SCC_RR1_FRAME_ERR)<<7);
			scc_write_reg(regs, chan, SCC_RR0, SCC_RESET_ERROR);
			rr1 = scc_to_mi(unit,chan);
			cons_simple_rint(rr1, rr1, 0, err);
		}
		scc_write_reg(regs, SCC_CHANNEL_A, SCC_RR0, SCC_RESET_HIGHEST_IUS);
	}

	goto next_intr;

}

boolean_t
scc_start(tp)
	struct tty *tp;
{
	register scc_regmap_t	*regs;
	register int		chan, temp;
	register struct softreg	*sr;

	temp = (natural_t)tp->t_addr;
	chan = (temp & 1);	/* channel */
	temp = (temp >> 1)&0xff;/* sccunit */
	regs = scc_softc[temp]->regs;
	sr   = &scc_softc[temp]->softr[chan];

	scc_read_reg(regs, chan, SCC_RR15, temp);
	temp |= SCC_WR15_TX_UNDERRUN_IE;
	scc_write_reg(regs, chan, SCC_WR15, temp);

	temp = sr->wr1 | SCC_WR1_TX_IE;
	scc_write_reg(regs, chan, SCC_WR1, temp);
	sr->wr1 = temp;

	/* but we need a first char out or no cookie */
	scc_read_reg(regs, chan, SCC_RR0, temp);
	if (temp & SCC_RR0_TX_EMPTY)
	{
		register char	c;

		c = getc(&tp->t_outq);
		scc_write_data(regs, chan, c);
	}
}

/*
 * Get a char from a specific SCC line
 * [this is only used for console&screen purposes]
 */
scc_getc( unit, line, wait, raw )
	boolean_t	wait;
	boolean_t	raw;
{
	scc_softc_t     scc;
	register scc_regmap_t *regs;
	unsigned char   c;
	int             value, mi_line, rcvalue, from_line;

	mi_line = line;
	mi_to_scc(&unit, &line);

	scc = scc_softc[unit];
	regs = scc->regs;

	/*
	 * wait till something available
	 *
	 * NOTE: we know! that rcline==3
	 */
	if (rcline) rcline = 3;
again:
	rcvalue = 0;
	while (1) {
		scc_read_reg_zero(regs, line, value);
		if (rcline && (mi_line == SCREEN_LINE_KEYBOARD)) {
			scc_read_reg_zero(regs, SCC_CHANNEL_B, rcvalue);
			value |= rcvalue;
		}
		if (((value & SCC_RR0_RX_AVAIL) == 0) && wait)
			delay(10);
		else
			break;
	}

	/*
	 * if nothing found return -1 
	 */
	from_line = (rcvalue & SCC_RR0_RX_AVAIL) ? SCC_CHANNEL_B : line;

	if (value & SCC_RR0_RX_AVAIL) {
		scc_read_reg(regs, from_line, SCC_RR1, value);
		scc_read_data(regs, from_line, c);
	} else {
/*		splx(s);*/
		return -1;
	}

	/*
	 * bad chars not ok 
	 */
	if (value&(SCC_RR1_PARITY_ERR | SCC_RR1_RX_OVERRUN | SCC_RR1_FRAME_ERR)) {
/* scc_state(unit,from_line); */
		scc_write_reg(regs, from_line, SCC_RR0, SCC_RESET_ERROR);
		if (wait) {
			scc_write_reg(regs, SCC_CHANNEL_A, SCC_RR0, SCC_RESET_HIGHEST_IUS);
			goto again;
		}
	}
	scc_write_reg(regs, SCC_CHANNEL_A, SCC_RR0, SCC_RESET_HIGHEST_IUS);
/*	splx(s);*/


#if	NBM > 0
	if ((mi_line == SCREEN_LINE_KEYBOARD) && (from_line == SCC_CHANNEL_A) &&
	    !raw && SCREEN_ISA_CONSOLE() && scc->isa_console)
		return lk201_rint(SCREEN_CONS_UNIT(), c, wait, scc->polling_mode);
	else
#endif	NBM > 0
		return c;
}

/*
 * Put a char on a specific SCC line
 */
scc_putc( unit, line, c )
{
	scc_softc_t      scc;
	register scc_regmap_t *regs;
	spl_t             s = spltty();
	register int	value;

	mi_to_scc(&unit, &line);

	scc = scc_softc[unit];
	regs = scc->regs;

	do {
		scc_read_reg(regs, line, SCC_RR0, value);
		if (value & SCC_RR0_TX_EMPTY)
			break;
		delay(100);
	} while (1);

	scc_write_data(regs, line, c);
/* wait for it to swallow the char ? */

	splx(s);
}

scc_param(tp, line)
	struct tty	*tp;
{
	scc_regmap_t	*regs;
	int		value, sccline, unit;
	struct softreg	*sr;
	scc_softc_t	scc;
 
	line = tp->t_dev;
	/* MI code wants us to handle 4 lines on unit 0 */
	unit = (line < 4) ? 0 : (line / NSCC_LINE);
	sccline = line;
	mi_to_scc(&unit, &sccline);

	if ((scc = scc_softc[unit]) == 0) return;	/* sanity */
	regs = scc->regs;

	sr = &scc->softr[sccline];

	/*
	 * Do not let user fool around with kbd&mouse
	 */
#if	NBM > 0
	if (screen_captures(line)) {
		tp->t_ispeed = tp->t_ospeed = B4800;
		tp->t_flags |= TF_LITOUT;
	}
#endif	NBM > 0

	if (tp->t_ispeed == 0) {
		(void) scc_mctl(tp->t_dev, TM_HUP, DMSET);	/* hang up line */
		return;
	}

	/* reset line */
	value = (sccline == SCC_CHANNEL_A) ? SCC_WR9_RESET_CHA_A : SCC_WR9_RESET_CHA_B;
	scc_write_reg(regs, sccline, SCC_WR9, value);
	delay(25);

	/* stop bits, normally 1 */
	value = sr->wr4 & 0xf0;
	value |= (tp->t_ispeed == B110) ? SCC_WR4_2_STOP : SCC_WR4_1_STOP;
	/* .. and parity */
#if 0
	if ((tp->t_flags & (TF_ODDP | TF_EVENP)) == TF_EVENP)
		value |= SCC_WR4_EVEN_PARITY;
	value |= SCC_WR4_PARITY_ENABLE;
#else
	if ((tp->t_flags & (TF_ODDP | TF_EVENP)) == TF_ODDP)
		value |= SCC_WR4_PARITY_ENABLE;
#endif

	/* set it now, remember it must be first after reset */
	sr->wr4 = value;
	scc_write_reg(regs, sccline, SCC_WR4, value);

	/* vector again */
	scc_write_reg(regs, sccline, SCC_WR2, 0xf0);

	/* we only do 8 bits per char */
	value = SCC_WR3_RX_8_BITS;
	scc_write_reg(regs, sccline, SCC_WR3, value);

	/* clear break, keep rts dtr */
	value = sr->wr5 & (SCC_WR5_DTR|SCC_WR5_RTS);
	value |= SCC_WR5_TX_8_BITS;
	sr->wr5 = value;
	scc_write_reg(regs, sccline, SCC_WR5, value);

	scc_write_reg(regs, sccline, SCC_WR6, 0);
	scc_write_reg(regs, sccline, SCC_WR7, 0);

	scc_write_reg(regs, sccline, SCC_WR9, SCC_WR9_VIS);

	scc_write_reg(regs, sccline, SCC_WR10, 0);

	/* clock config */
	value = SCC_WR11_RCLK_BAUDR | SCC_WR11_XTLK_BAUDR |
		SCC_WR11_TRc_OUT | SCC_WR11_TRcOUT_BAUDR;
	scc_write_reg(regs, sccline, SCC_WR11, value);

	value = scc_speeds[tp->t_ispeed];
	scc_set_timing_base(regs,sccline,value);

	value = sr->wr14;
	scc_write_reg(regs, sccline, SCC_WR14, value);

	value = (scc->full_modem) ?
		SCC_WR15_BREAK_IE | SCC_WR15_CTS_IE | SCC_WR15_DCD_IE :
		SCC_WR15_BREAK_IE;
	scc_write_reg(regs, sccline, SCC_WR15, value);

	/* and now the enables */
	value = SCC_WR3_RX_8_BITS | SCC_WR3_RX_ENABLE;
	scc_write_reg(regs, sccline, SCC_WR3, value);

	value = sr->wr5 | SCC_WR5_TX_ENABLE;
	sr->wr5 = value;
	scc_write_reg(regs, sccline, SCC_WR5, value);

	/* master inter enable */
	scc_write_reg(regs,sccline,SCC_WR9,SCC_WR9_MASTER_IE|SCC_WR9_VIS);

	scc_write_reg(regs, sccline, SCC_WR1, sr->wr1);

}
 
/*
 * Modem control functions
 */
scc_mctl(dev, bits, how)
	int dev;
	int bits, how;
{
	return 0;
#if 0
	register scc_regmap_t *regs;
	int unit, sccline;
	register int tcr, msr, brk, n_tcr, n_brk;
	int b;
	spl_t	s;
	scc_softc_t      sc;

	unit = dev;
	sccline = unit & 3;
	unit >>= 2;
	mi_to_scc(&unit, &sccline);

	/* no modem support on channel A */
/* XXX break on 0&1 */
	if (sccline == SCC_CHANNEL_A)
		return TM_LE | TM_DTR | TM_CTS | TM_CAR | TM_DSR;

	scc = scc_softc[unit];
	regs = scc->regs;
	s = spltty();

....some other day..

	tcr = ((regs->scc_tcr | (scc->fake>>4)) & 0xf00) >> (8 + b*2);
	brk = (scc->breaks >> (8 + (unit&3))) & 1;	/* THE break bit */

	n_tcr = (bits & (TM_RTS | TM_DTR)) >> 1;
	n_brk = (bits & TM_BRK) >> 9;

	/* break transitions, must 'send' a char out */
	bits = (brk ^ n_brk) & 1;

	switch (how) {
	case DMSET:
		tcr = n_tcr;
		brk = n_brk;
		break;

	case DMBIS:
		tcr |= n_tcr;
		brk |= n_brk;
		break;

	case DMBIC:
		tcr &= ~n_tcr;
		brk = 0;
		break;

	case DMGET:
		msr = ((regs->scc_msr | scc->fake) & 0xf0f) >> (b*8);
		(void) splx(s);
		return  (tcr<<1)|/* DTR, RTS */
			((msr&1)<<5)|/* CTS */
			((msr&2)<<7)|/* DSR */
			((msr&0xc)<<4)|/* CD, RNG */
			(brk << 9)|/* BRK */
			TM_LE;
	}
	n_tcr =	(regs->scc_tcr & ~(3 << (8 + b*2))) |
		(tcr << (8 + b*2));

	regs->scc_tcr = n_tcr;
	scc->fake = (scc->fake & 0xf0f) | (n_tcr<<4&0xf000);

	scc->breaks = (scc->breaks & ~(1 << (8 + (unit&3)))) |
		    (brk << (8 + (unit&3)));
	if(bits) scc_putc( unit>>2, unit&3, 0);/* force break, now */
	(void) splx(s);
	return 0;/* useless to compute it */
#endif
}

scc_modem_intr(scc, chan)
{
	/* nothing yet */
}

static check_car(
	register struct tty *tp,
        boolean_t car)
		
{
	if (car) {
#if notyet
		/* cancel modem timeout if need to */
		if (car & (SCC_MSR_CD2 | SCC_MSR_CD3))
			untimeout(scc_hup, (caddr_t)tp);
#endif

		/* I think this belongs in the MI code */
		if (tp->t_state & TS_WOPEN)
			tp->t_state |= TS_ISOPEN;
		/* carrier present */
		if ((tp->t_state & TS_CARR_ON) == 0)
			(void)ttymodem(tp, 1);
	} else if ((tp->t_state&TS_CARR_ON) && ttymodem(tp, 0) == 0)
		scc_mctl( tp->t_dev, TM_DTR, DMBIC);
}

/*
 * Periodically look at the CD signals:
 * they do generate interrupts but we
 * must fake them on channel A.  We might
 * also fake them on channel B.
 */
scc_scan()
{
	register i;
	spl_t s = spltty();

	for (i = 0; i < NSCC; i++) {
		register scc_softc_t	scc;
		register int		car;
		register struct tty	**tpp;

		scc = scc_softc[i];
		if (scc == 0)
			continue;
		car = scc->softCAR | scc->fake;

		tpp = &console_tty[i * NSCC_LINE];

		while (car) {
			if (car & 1)
				check_car(*tpp, 1);
			tpp++;
			car  = car>>1;
		}

	}
	splx(s);
	timeout(scc_scan, (caddr_t)0, 5*hz);
}


#if 0 /* debug */
scc_rreg(unit,chan,n)
{
	int val;
	scc_read_reg(scc_softc[unit]->regs, chan, n, val);
	return val;
}

scc_wreg(unit,chan,n,val)
{
	scc_write_reg(scc_softc[unit]->regs, chan, n, val);
}

scc_state(unit,soft)
{
	printf("{%d intr, A: R0 %x R1 %x R3 %x baudr %x R15 %x}\n",
		scc_intr_count,
		scc_rreg(unit, 1, SCC_RR0),
		scc_rreg(unit, 1, SCC_RR1),
		scc_rreg(unit, 1, SCC_RR3),
		(scc_rreg(unit, 1, SCC_RR13) << 8) | scc_rreg(unit, 1, SCC_RR12),
		scc_rreg(unit, 1, SCC_RR15));
	printf("{B: R0 %x R1 %x R2 %x baudr %x R15 %x}\n",
		scc_rreg(unit, 1, SCC_RR0),
		scc_rreg(unit, 1, SCC_RR1),
		scc_rreg(unit, 1, SCC_RR2),
		(scc_rreg(unit, 1, SCC_RR13) << 8) | scc_rreg(unit, 1, SCC_RR12),
		scc_rreg(unit, 1, SCC_RR15));
	if (soft) {
		struct softreg *sr;
		sr = scc_softc[unit]->softr;
		printf("{B: W1 %x W4 %x W5 %x W14 %x}",
			sr->wr1, sr->wr4, sr->wr5, sr->wr14);
		sr++;
		printf("{A: W1 %x W4 %x W5 %x W14 %x}\n",
			sr->wr1, sr->wr4, sr->wr5, sr->wr14);
	}
}

#endif

#endif	NSCC > 0
