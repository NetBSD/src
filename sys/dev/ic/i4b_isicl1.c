/*
 * Copyright (c) 1997, 2000 Hellmuth Michaelis. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *---------------------------------------------------------------------------
 *
 *	i4b_l1.c - isdn4bsd layer 1 handler
 *	-----------------------------------
 *
 *	$Id: i4b_isicl1.c,v 1.1.1.1.2.3 2001/01/18 09:23:18 bouyer Exp $ 
 *
 *      last edit-date: [Fri Jan  5 11:36:11 2001]
 *
 *---------------------------------------------------------------------------*/

#include <sys/param.h>
#if defined(__FreeBSD_version) && __FreeBSD_version >= 300001
#include <sys/ioccom.h>
#else
#include <sys/ioctl.h>
#endif
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <machine/stdarg.h>

#ifdef __FreeBSD__
#include <machine/clock.h>
#include <i386/isa/isa_device.h>
#else
#ifndef __bsdi__
#include <machine/bus.h>
#endif
#include <sys/device.h>
#endif

#include <sys/socket.h>
#include <net/if.h>

#if defined(__NetBSD__) && __NetBSD_Version__ >= 104230000
#include <sys/callout.h>
#endif

#ifdef __FreeBSD__
#include <machine/i4b_debug.h>
#include <machine/i4b_ioctl.h>
#include <machine/i4b_trace.h>
#else
#include <netisdn/i4b_debug.h>
#include <netisdn/i4b_ioctl.h>
#include <netisdn/i4b_trace.h>
#endif

#include <dev/ic/i4b_isicl1.h>
#include <dev/ic/i4b_isac.h>
#include <dev/ic/i4b_hscx.h>

#include <netisdn/i4b_l1l2.h>
#include <netisdn/i4b_mbuf.h>
#include <netisdn/i4b_global.h>

unsigned int i4b_l1_debug = L1_DEBUG_DEFAULT;

static int ph_data_req(int, struct mbuf *, int);
static int ph_activate_req(int);

/* from i4btrc driver i4b_trace.c */
extern int get_trace_data_from_l1(int unit, int what, int len, char *buf);

/* from layer 2 */
extern int i4b_ph_data_ind(int unit, struct mbuf *m);
extern int i4b_ph_activate_ind(int unit);
extern int i4b_ph_deactivate_ind(int unit);
extern int i4b_mph_status_ind(int, int, int);

/* layer 1 lme */
static int i4b_mph_command_req(int, int, void*);

/* jump table */
struct i4b_l1l2_func i4b_l1l2_func = {

	/* Layer 1 --> Layer 2 */
	
	(int (*)(int, struct mbuf *))		i4b_ph_data_ind,
	(int (*)(int)) 				i4b_ph_activate_ind,
	(int (*)(int))				i4b_ph_deactivate_ind,

	/* Layer 2 --> Layer 1 */

	(int (*)(int, struct mbuf *, int))	ph_data_req,
	(int (*)(int))				ph_activate_req,

	/* Layer 1 --> upstream, ISDN trace data */

	(int (*)(i4b_trace_hdr_t *, int, u_char *))	get_trace_data_from_l1,

	/* Driver control and status information */

	(int (*)(int, int, int))		i4b_mph_status_ind,
	(int (*)(int, int, void*))		i4b_mph_command_req,
};
 
/*---------------------------------------------------------------------------*
 *
 *	L2 -> L1: PH-DATA-REQUEST
 *	=========================
 *
 *	parms:
 *		unit		physical interface unit number
 *		m		mbuf containing L2 frame to be sent out
 *		freeflag	MBUF_FREE: free mbuf here after having sent
 *						it out
 *				MBUF_DONTFREE: mbuf is freed by Layer 2
 *	returns:
 *		==0	fail, nothing sent out
 *		!=0	ok, frame sent out
 *
 *---------------------------------------------------------------------------*/
static int
ph_data_req(int unit, struct mbuf *m, int freeflag)
{
	u_char cmd;
	int s;
	
#ifdef __FreeBSD__
	struct l1_softc *sc = &l1_sc[unit];
#else
	struct l1_softc *sc = isic_find_sc(unit);
#endif

#ifdef NOTDEF
	NDBGL1(L1_PRIM, "PH-DATA-REQUEST, unit %d, freeflag=%d", unit, freeflag);
#endif

	if(m == NULL)			/* failsafe */
		return (0);

	s = SPLI4B();

	if(sc->sc_I430state == ST_F3)	/* layer 1 not running ? */
	{
		NDBGL1(L1_I_ERR, "still in state F3!");
		ph_activate_req(unit);
	}

	if(sc->sc_state & ISAC_TX_ACTIVE)
	{
		if(sc->sc_obuf2 == NULL)
		{
			sc->sc_obuf2 = m;		/* save mbuf ptr */

			if(freeflag)
				sc->sc_freeflag2 = 1;	/* IRQ must mfree */
			else
				sc->sc_freeflag2 = 0;	/* IRQ must not mfree */

			NDBGL1(L1_I_MSG, "using 2nd ISAC TX buffer, state = %s", isic_printstate(sc));

			if(sc->sc_trace & TRACE_D_TX)
			{
				i4b_trace_hdr_t hdr;
				hdr.unit = unit;
				hdr.type = TRC_CH_D;
				hdr.dir = FROM_TE;
				hdr.count = ++sc->sc_trace_dcount;
				MICROTIME(hdr.time);
				MPH_Trace_Ind(&hdr, m->m_len, m->m_data);
			}
			splx(s);
			return(1);
		}

		NDBGL1(L1_I_ERR, "No Space in TX FIFO, state = %s", isic_printstate(sc));
	
		if(freeflag == MBUF_FREE)
			i4b_Dfreembuf(m);			
	
		splx(s);
		return (0);
	}

	if(sc->sc_trace & TRACE_D_TX)
	{
		i4b_trace_hdr_t hdr;
		hdr.unit = unit;
		hdr.type = TRC_CH_D;
		hdr.dir = FROM_TE;
		hdr.count = ++sc->sc_trace_dcount;
		MICROTIME(hdr.time);
		MPH_Trace_Ind(&hdr, m->m_len, m->m_data);
	}
	
	sc->sc_state |= ISAC_TX_ACTIVE;	/* set transmitter busy flag */

	NDBGL1(L1_I_MSG, "ISAC_TX_ACTIVE set");

	sc->sc_freeflag = 0;		/* IRQ must NOT mfree */
	
	ISAC_WRFIFO(m->m_data, min(m->m_len, ISAC_FIFO_LEN)); /* output to TX fifo */

	if(m->m_len > ISAC_FIFO_LEN)	/* message > 32 bytes ? */
	{
		sc->sc_obuf = m;	/* save mbuf ptr */
		sc->sc_op = m->m_data + ISAC_FIFO_LEN; 	/* ptr for irq hdl */
		sc->sc_ol = m->m_len - ISAC_FIFO_LEN;	/* length for irq hdl */

		if(freeflag)
			sc->sc_freeflag = 1;	/* IRQ must mfree */
		
		cmd = ISAC_CMDR_XTF;
	}
	else
	{
		sc->sc_obuf = NULL;
		sc->sc_op = NULL;
		sc->sc_ol = 0;

		if(freeflag)
			i4b_Dfreembuf(m);

		cmd = ISAC_CMDR_XTF | ISAC_CMDR_XME;
  	}

	ISAC_WRITE(I_CMDR, cmd);
	ISACCMDRWRDELAY();

	splx(s);
	
	return(1);
}

/*---------------------------------------------------------------------------*
 *
 *	L2 -> L1: PH-ACTIVATE-REQUEST
 *	=============================
 *
 *	parms:
 *		unit	physical interface unit number
 *
 *	returns:
 *		==0	
 *		!=0	
 *
 *---------------------------------------------------------------------------*/
static int
ph_activate_req(int unit)
{

#ifdef __FreeBSD__
	struct l1_softc *sc = &l1_sc[unit];
#else
	struct l1_softc *sc = isic_find_sc(unit);
#endif

	NDBGL1(L1_PRIM, "unit %d", unit);
	isic_next_state(sc, EV_PHAR);
	return(0);
}

/*---------------------------------------------------------------------------*
 *	command from the upper layers
 *---------------------------------------------------------------------------*/
static int
i4b_mph_command_req(int unit, int command, void *parm)
{
#ifdef __FreeBSD__
	struct l1_softc *sc = &l1_sc[unit];
#else
	struct l1_softc *sc = isic_find_sc(unit);
#endif
	
	switch(command)
	{
		case CMR_DOPEN:		/* daemon running */
			NDBGL1(L1_PRIM, "unit %d, command = CMR_DOPEN", unit);
			sc->sc_enabled = 1;			
			break;
			
		case CMR_DCLOSE:	/* daemon not running */
			NDBGL1(L1_PRIM, "unit %d, command = CMR_DCLOSE", unit);
			sc->sc_enabled = 0;
			break;

		case CMR_SETTRACE:
			NDBGL1(L1_PRIM, "unit %d, command = CMR_SETTRACE, parm = %p", unit, parm);
			sc->sc_trace = (int)(unsigned long)parm;
			break;

		default:
			NDBGL1(L1_ERROR, "ERROR, unknown command = %d, unit = %d, parm = %p", command, unit, parm);
			break;
	}

	return(0);
}
