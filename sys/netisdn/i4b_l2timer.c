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
 *	i4b_l2timer.c - layer 2 timer handling
 *	--------------------------------------
 *
 *	$Id: i4b_l2timer.c,v 1.3.2.2 2002/06/23 17:51:28 jdolecek Exp $ 
 *
 * $FreeBSD$
 *
 *      last edit-date: [Fri Jan  5 11:33:47 2001]
 *
 *---------------------------------------------------------------------------*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i4b_l2timer.c,v 1.3.2.2 2002/06/23 17:51:28 jdolecek Exp $");

#ifdef __FreeBSD__
#include "i4bq921.h"
#else
#define NI4BQ921	1
#endif
#if NI4BQ921 > 0

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <net/if.h>

#if defined(__NetBSD__) && __NetBSD_Version__ >= 104230000
#include <sys/callout.h>
#endif

#ifdef __FreeBSD__
#include <machine/i4b_debug.h>
#include <machine/i4b_ioctl.h>
#else
#include <netisdn/i4b_debug.h>
#include <netisdn/i4b_ioctl.h>
#endif

#include <netisdn/i4b_global.h>
#include <netisdn/i4b_l2.h>
#include <netisdn/i4b_l1l2.h>
#include <netisdn/i4b_isdnq931.h>
#include <netisdn/i4b_mbuf.h>
#include <netisdn/i4b_l2fsm.h>
#include <netisdn/i4b_l3l4.h>

/*---------------------------------------------------------------------------*
 *	Q.921 timer T200 timeout function
 *---------------------------------------------------------------------------*/
static void
i4b_T200_timeout(l2_softc_t *l2sc)
{
	NDBGL2(L2_T_ERR, "bri %d, RC = %d", l2sc->drv->bri, l2sc->RC);
	i4b_next_l2state(l2sc, l2sc->drv, EV_T200EXP);
}

/*---------------------------------------------------------------------------*
 *	Q.921 timer T200 start
 *---------------------------------------------------------------------------*/
void
i4b_T200_start(l2_softc_t *l2sc)
{
	if(l2sc->T200 == TIMER_ACTIVE)
		return;
		
	NDBGL2(L2_T_MSG, "bri %d", l2sc->drv->bri);
	l2sc->T200 = TIMER_ACTIVE;

	START_TIMER(l2sc->T200_callout, i4b_T200_timeout, l2sc, T200DEF);
}

/*---------------------------------------------------------------------------*
 *	Q.921 timer T200 stop
 *---------------------------------------------------------------------------*/
void
i4b_T200_stop(l2_softc_t *l2sc)
{
	int s;
	s = splnet();
	if(l2sc->T200 != TIMER_IDLE)
	{
		STOP_TIMER(l2sc->T200_callout, i4b_T200_timeout, l2sc);
		l2sc->T200 = TIMER_IDLE;
	}
	splx(s);
	NDBGL2(L2_T_MSG, "bri %d", l2sc->drv->bri);
}

/*---------------------------------------------------------------------------*
 *	Q.921 timer T200 restart
 *---------------------------------------------------------------------------*/
void
i4b_T200_restart(l2_softc_t *l2sc)
{
	int s;
	s = splnet();
	if(l2sc->T200 != TIMER_IDLE)
	{
		STOP_TIMER(l2sc->T200_callout, i4b_T200_timeout, l2sc);
	}
	else
	{
		l2sc->T200 = TIMER_ACTIVE;
	}

	START_TIMER(l2sc->T200_callout, i4b_T200_timeout, l2sc, T200DEF);
	splx(s);
	NDBGL2(L2_T_MSG, "bri %d", l2sc->drv->bri);
}

/*---------------------------------------------------------------------------*
 *	Q.921 timer T202 timeout function
 *---------------------------------------------------------------------------*/
static void
i4b_T202_timeout(l2_softc_t *l2sc)
{
	NDBGL2(L2_T_ERR, "bri %d, N202 = %d", l2sc->drv->bri, l2sc->N202);
	
	if(--(l2sc->N202))
	{
		(*l2sc->T202func)(l2sc);
	}
}

/*---------------------------------------------------------------------------*
 *	Q.921 timer T202 start
 *---------------------------------------------------------------------------*/
void
i4b_T202_start(l2_softc_t *l2sc)
{
	if (l2sc->N202 == TIMER_ACTIVE)
		return;

	NDBGL2(L2_T_MSG, "bri %d", l2sc->drv->bri);
	l2sc->N202 = N202DEF;	
	l2sc->T202 = TIMER_ACTIVE;

	START_TIMER(l2sc->T202_callout, i4b_T202_timeout, l2sc, T202DEF);
}

/*---------------------------------------------------------------------------*
 *	Q.921 timer T202 stop
 *---------------------------------------------------------------------------*/
void
i4b_T202_stop(l2_softc_t *l2sc)
{
	int s;
	s = splnet();
	if(l2sc->T202 != TIMER_IDLE)
	{
		STOP_TIMER(l2sc->T202_callout, i4b_T202_timeout, l2sc);
		l2sc->T202 = TIMER_IDLE;
	}
	splx(s);
	NDBGL2(L2_T_MSG, "bri %d", l2sc->drv->bri);
}

/*---------------------------------------------------------------------------*
 *	Q.921 timer T203 timeout function
 *---------------------------------------------------------------------------*/
#if I4B_T203_ACTIVE
static void
i4b_T203_timeout(l2_softc_t *l2sc)
{
	NDBGL2(L2_T_ERR, "bri %d", l2sc->bri);
	i4b_next_l2state(l2sc, EV_T203EXP);
}
#endif

/*---------------------------------------------------------------------------*
 *	Q.921 timer T203 start
 *---------------------------------------------------------------------------*/
void
i4b_T203_start(l2_softc_t *l2sc)
{
#if I4B_T203_ACTIVE
	if (l2sc->T203 == TIMER_ACTIVE)
		return;
		
	NDBGL2(L2_T_MSG, "bri %d", l2sc->bri);
	l2sc->T203 = TIMER_ACTIVE;

	START_TIMER(l2sc->T203_callout, i4b_T203_timeout, l2sc, T203DEF);
#endif
}

/*---------------------------------------------------------------------------*
 *	Q.921 timer T203 stop
 *---------------------------------------------------------------------------*/
void
i4b_T203_stop(l2_softc_t *l2sc)
{
#if I4B_T203_ACTIVE
	int s;
	s = splnet();
	if(l2sc->T203 != TIMER_IDLE)
	{
		STOP_TIMER(l2sc->T203_callout, i4b_T203_timeout, l2sc);
		l2sc->T203 = TIMER_IDLE;
	}
	splx(s);
	NDBGL2(L2_T_MSG, "bri %d", l2sc->bri);
#endif
}

/*---------------------------------------------------------------------------*
 *	Q.921 timer T203 restart
 *---------------------------------------------------------------------------*/
void
i4b_T203_restart(l2_softc_t *l2sc)
{
#if I4B_T203_ACTIVE
	int s;
	s = splnet();

	if(l2sc->T203 != TIMER_IDLE)
	{
		STOP_TIMER(l2sc->T203_callout, i4b_T203_timerout, l2sc);
	}
	else
	{
		l2sc->T203 = TIMER_ACTIVE;
	}

	START_TIMER(l2sc->T203_callout, i4b_T203_timerout, l2sc, T203DEF);	
	splx(s);
	NDBGL2(L2_T_MSG, "bri %d", l2sc->bri);
#endif
}

#endif /* NI4BQ921 > 0 */
