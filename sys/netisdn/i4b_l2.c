/* $NetBSD: i4b_l2.c,v 1.10 2002/03/29 15:01:27 martin Exp $ */

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
 *      i4b_l2.c - ISDN layer 2 (Q.921)
 *	-------------------------------
 *
 *	$Id: i4b_l2.c,v 1.10 2002/03/29 15:01:27 martin Exp $ 
 *
 * $FreeBSD$
 *
 *      last edit-date: [Fri Jan  5 11:33:47 2001]
 *
 *---------------------------------------------------------------------------*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i4b_l2.c,v 1.10 2002/03/29 15:01:27 martin Exp $");

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

#include <netisdn/i4b_l2.h>
#include <netisdn/i4b_l1l2.h>
#include <netisdn/i4b_isdnq931.h>
#include <netisdn/i4b_mbuf.h>
#include <netisdn/i4b_global.h>

#include <netisdn/i4b_l2.h>
#include <netisdn/i4b_l2fsm.h>

int i4b_dl_establish_ind(int);
int i4b_dl_establish_cnf(int);
int i4b_dl_release_ind(int);
int i4b_dl_release_cnf(int);
int i4b_dl_data_ind(int, struct mbuf *);
int i4b_dl_unit_data_ind(int, struct mbuf *);

/* this layers debug level */

unsigned int i4b_l2_debug = L2_DEBUG_DEFAULT;

/*---------------------------------------------------------------------------*
 *	DL_ESTABLISH_REQ from layer 3
 *---------------------------------------------------------------------------*/
int i4b_dl_establish_req(l2_softc_t *l2sc)
{
	NDBGL2(L2_PRIM, "bri %d", l2sc->bri);
	i4b_l1_activate(l2sc);
	i4b_next_l2state(l2sc, EV_DLESTRQ);
	return(0);
}

/*---------------------------------------------------------------------------*
 *	DL_RELEASE_REQ from layer 3
 *---------------------------------------------------------------------------*/
int i4b_dl_release_req(l2_softc_t *l2sc)
{
	NDBGL2(L2_PRIM, "bri %d", l2sc->bri);
	i4b_next_l2state(l2sc, EV_DLRELRQ);
	return(0);	
}

/*---------------------------------------------------------------------------*
 *	DL UNIT DATA REQUEST from Layer 3
 *---------------------------------------------------------------------------*/
int i4b_dl_unit_data_req(l2_softc_t *l2sc, struct mbuf *m)
{
#ifdef NOTDEF
	NDBGL2(L2_PRIM, "bri %d", l2sc->bri);
#endif
	return(0);
}

/*---------------------------------------------------------------------------*
 *	DL DATA REQUEST from Layer 3
 *---------------------------------------------------------------------------*/
int i4b_dl_data_req(l2_softc_t *l2sc, struct mbuf *m)
{
	switch(l2sc->Q921_state)
	{
		case ST_AW_EST:
		case ST_MULTIFR:
		case ST_TIMREC:
		
		        if(IF_QFULL(&l2sc->i_queue))
		        {
		        	NDBGL2(L2_ERROR, "i_queue full!!");
		        	i4b_Dfreembuf(m);
		        }
		        else
		        {
		        	int s;

		        	s = splnet();
				IF_ENQUEUE(&l2sc->i_queue, m);
				splx(s);

				i4b_i_frame_queued_up(l2sc);
			}
			break;
			
		default:
			NDBGL2(L2_ERROR, "bri %d ERROR in state [%s], freeing mbuf", l2sc->bri, i4b_print_l2state(l2sc));
			i4b_Dfreembuf(m);
			break;
	}		
	return(0);
}

/*---------------------------------------------------------------------------*
 *	isdn_layer2_activate_ind - link activation/deactivation indication from layer 1
 *---------------------------------------------------------------------------*/
int
isdn_layer2_activate_ind(struct l2_softc *l2sc, int event_activate)
{
	if (event_activate) {
		l2sc->ph_active = PH_ACTIVE;
	} else {
		l2sc->ph_active = PH_INACTIVE;
	}
	return(0);
}

/*---------------------------------------------------------------------------*
 *	i4b_l2_unit_init - place layer 2 unit into known state
 *---------------------------------------------------------------------------*/
static void
i4b_l2_unit_init(l2_softc_t *l2sc)
{
	int s;

	s = splnet();
	l2sc->Q921_state = ST_TEI_UNAS;
	l2sc->tei_valid = TEI_INVALID;
	l2sc->vr = 0;
	l2sc->vs = 0;
	l2sc->va = 0;
	l2sc->ack_pend = 0;
	l2sc->rej_excpt = 0;
	l2sc->peer_busy = 0;
	l2sc->own_busy = 0;
	l2sc->l3initiated = 0;

	l2sc->rxd_CR = 0;
	l2sc->rxd_PF = 0;
	l2sc->rxd_NR = 0;
	l2sc->RC = 0;
	l2sc->iframe_sent = 0;
		
	l2sc->postfsmfunc = NULL;

	if(l2sc->ua_num != UA_EMPTY)
	{
		i4b_Dfreembuf(l2sc->ua_frame);
		l2sc->ua_num = UA_EMPTY;
		l2sc->ua_frame = NULL;
	}

	i4b_T200_stop(l2sc);
	i4b_T202_stop(l2sc);
	i4b_T203_stop(l2sc);

	splx(s);	
}

/*---------------------------------------------------------------------------*
 *	isdn_layer2_status_ind - status indication upward
 *---------------------------------------------------------------------------*/
int
isdn_layer2_status_ind(l2_softc_t *l2sc, int status, int parm)
{
	int s;
	int sendup = 1;
	
	s = splnet();

	NDBGL1(L1_PRIM, "bri %d, status=%d, parm=%d", l2sc->bri, status, parm);

	switch(status)
	{
		case STI_ATTACH:
			if (parm == 0) 	/* detach */
				break;

			l2sc->i_queue.ifq_maxlen = IQUEUE_MAXLEN;
			l2sc->ua_frame = NULL;
			memset(&l2sc->stat, 0, sizeof(lapdstat_t));			
			i4b_l2_unit_init(l2sc);
			
			/* initialize the callout handles for timeout routines */
			callout_init(&l2sc->T200_callout);
			callout_init(&l2sc->T202_callout);
			callout_init(&l2sc->T203_callout);
			callout_init(&l2sc->IFQU_callout);

			break;

		case STI_L1STAT:	/* state of layer 1 */
			break;
		
		case STI_PDEACT:	/* Timer 4 expired */
/*XXX*/			if((l2sc->Q921_state >= ST_AW_EST) &&
			   (l2sc->Q921_state <= ST_TIMREC))
			{
				NDBGL2(L2_ERROR, "bri %d, persistent deactivation!", l2sc->bri);
				i4b_l2_unit_init(l2sc);
				parm = -1;
			}
			else
			{
				sendup = 0;
			}
			break;

		case STI_NOL1ACC:
			i4b_l2_unit_init(l2sc);
			NDBGL2(L2_ERROR, "bri %d, cannot access S0 bus!", l2sc->bri);
			break;
			
		default:
			NDBGL2(L2_ERROR, "ERROR, bri %d, unknown status message!", l2sc->bri);
			break;
	}
	
	if(sendup)
		i4b_mdl_status_ind(l2sc->bri, status, parm);  /* send up to layer 3 */

	splx(s);
	
	return(0);
}

/*---------------------------------------------------------------------------*
 *	MDL_COMMAND_REQ from layer 3
 *---------------------------------------------------------------------------*/
int i4b_mdl_command_req(int bri, int command, void * parm)
{
	struct l2_softc *sc = (l2_softc_t*)isdn_find_softc_by_bri(bri);

	NDBGL2(L2_PRIM, "bri %d, command=%d, parm=%p", bri, command, parm);

	switch(command)
	{
		case CMR_DOPEN:
			i4b_l2_unit_init(sc);
			/* XXX - enable interrupts */
			break;
		case CMR_DCLOSE:
			/* XXX - disable interrupts */
			break;
	}		

	/* pass down to layer 1 driver */
	sc->driver->mph_command_req(sc->l1_token, command, parm);
	
	return(0);
}

/*---------------------------------------------------------------------------*
 * isdn_layer2_data_ind - process a rx'd frame got from layer 1
 *---------------------------------------------------------------------------*/
int
isdn_layer2_data_ind(l2_softc_t *l2sc, struct mbuf *m)
{
	u_char *ptr = m->m_data;

	if ( (*(ptr + OFF_CNTL) & 0x01) == 0 )
	{
		if(m->m_len < 4)	/* 6 oct - 2 chksum oct */
		{
			l2sc->stat.err_rx_len++;
			NDBGL2(L2_ERROR, "ERROR, I-frame < 6 octetts!");
			i4b_Dfreembuf(m);
			return(0);
		}
		i4b_rxd_i_frame(l2sc, m);
	}
	else if ( (*(ptr + OFF_CNTL) & 0x03) == 0x01 )
	{
		if(m->m_len < 4)	/* 6 oct - 2 chksum oct */
		{
			l2sc->stat.err_rx_len++;
			NDBGL2(L2_ERROR, "ERROR, S-frame < 6 octetts!");
			i4b_Dfreembuf(m);
			return(0);
		}
		i4b_rxd_s_frame(l2sc, m);
	}
	else if ( (*(ptr + OFF_CNTL) & 0x03) == 0x03 )
	{
		if(m->m_len < 3)	/* 5 oct - 2 chksum oct */
		{
			l2sc->stat.err_rx_len++;
			NDBGL2(L2_ERROR, "ERROR, U-frame < 5 octetts!");
			i4b_Dfreembuf(m);
			return(0);
		}
		i4b_rxd_u_frame(l2sc, m);
	}
	else
	{
		l2sc->stat.err_rx_badf++;
		NDBGL2(L2_ERROR, "ERROR, bad frame rx'd - ");
		i4b_print_frame(m->m_len, m->m_data);
		i4b_Dfreembuf(m);
	}
	return(0);
}

int i4b_l2_channel_get_state(int bri, int b_chanid)
{
	l2_softc_t *sc = (l2_softc_t*)isdn_find_softc_by_bri(bri);
	return sc->bchan_state[b_chanid];
}

void i4b_l2_channel_set_state(int bri, int b_chanid, int state)
{
	l2_softc_t *sc = (l2_softc_t*)isdn_find_softc_by_bri(bri);
	sc->bchan_state[b_chanid] = state;
}

/*---------------------------------------------------------------------------*
 *	telephony silence detection
 *---------------------------------------------------------------------------*/

#define TEL_IDLE_MIN (BCH_MAX_DATALEN/2)

int
isdn_bchan_silence(unsigned char *data, int len)
{
	register int i = 0;
	register int j = 0;

	/* count idle bytes */
	
	for(;i < len; i++)
	{
		if((*data >= 0xaa) && (*data <= 0xac))
			j++;
		data++;
	}

#ifdef NOTDEF
	printf("isic_hscx_silence: got %d silence bytes in frame\n", j);
#endif
	
	if(j < (TEL_IDLE_MIN))
		return(0);
	else
		return(1);

}


#endif /* NI4BQ921 > 0 */
