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
 *	i4b_l4if.c - Layer 3 interface to Layer 4
 *	-------------------------------------------
 *
 *	$Id: i4b_l4if.c,v 1.4 2002/02/14 16:20:47 drochner Exp $ 
 *
 * $FreeBSD$
 *
 *      last edit-date: [Fri Jan  5 11:33:47 2001]
 *
 *---------------------------------------------------------------------------*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i4b_l4if.c,v 1.4 2002/02/14 16:20:47 drochner Exp $");

#ifdef __FreeBSD__
#include "i4bq931.h"
#else
#define	NI4BQ931	1
#endif
#if NI4BQ931 > 0

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
#include <machine/i4b_cause.h>
#else
#include <netisdn/i4b_debug.h>
#include <netisdn/i4b_ioctl.h>
#include <netisdn/i4b_cause.h>
#endif

#include <netisdn/i4b_isdnq931.h>
#include <netisdn/i4b_l1l2.h>
#include <netisdn/i4b_l3l4.h>
#include <netisdn/i4b_mbuf.h>
#include <netisdn/i4b_global.h>

#include <netisdn/i4b_l2.h>
#include <netisdn/i4b_l3.h>
#include <netisdn/i4b_l3fsm.h>
#include <netisdn/i4b_q931.h>

#include <netisdn/i4b_l4.h>

extern void isic_settrace(int unit, int val);		/*XXX*/
extern int isic_gettrace(int unit);			/*XXX*/

static void n_connect_request(u_int cdid);
static void n_connect_response(u_int cdid, int response, int cause);
static void n_disconnect_request(u_int cdid, int cause);
static void n_alert_request(u_int cdid);
static void n_mgmt_command(int bri, int cmd, void *parm);

/*---------------------------------------------------------------------------*
 *	i4b_mdl_status_ind - status indication from lower layers
 *---------------------------------------------------------------------------*/
int
i4b_mdl_status_ind(int bri, int status, int parm)
{
	struct l2_softc * l2sc = (struct l2_softc*)isdn_find_l2_by_bri(bri);
	int sendup;
	int i;
	
	NDBGL3(L3_MSG, "bri = %d, status = %d, parm = %d", bri, status, parm);

	switch(status)
	{
		case STI_ATTACH:
			NDBGL3(L3_MSG, "STI_ATTACH: attaching bri %d to controller %d", bri, nctrl);
		
			/* init function pointers */
			
			ctrl_desc[nctrl].N_CONNECT_REQUEST = n_connect_request;
			ctrl_desc[nctrl].N_CONNECT_RESPONSE = n_connect_response;
			ctrl_desc[nctrl].N_DISCONNECT_REQUEST = n_disconnect_request;
			ctrl_desc[nctrl].N_ALERT_REQUEST = n_alert_request;
#if 0
			ctrl_desc[nctrl].N_DOWNLOAD = NULL;	/* only used by active cards */
			ctrl_desc[nctrl].N_DIAGNOSTICS = NULL;	/* only used by active cards */
#endif
			ctrl_desc[nctrl].N_MGMT_COMMAND = n_mgmt_command;
		
			/* init type and unit */
			
			ctrl_desc[nctrl].l1_token = l2sc->l1_token;
			ctrl_desc[nctrl].bri = bri;
			ctrl_desc[nctrl].ctrl_type = CTRL_PASSIVE;
			ctrl_desc[nctrl].card_type = parm;
		
			/* state fields */
		
			ctrl_desc[nctrl].dl_est = DL_DOWN;
			ctrl_desc[nctrl].bch_state[CHAN_B1] = BCH_ST_FREE;
			ctrl_desc[nctrl].bch_state[CHAN_B2] = BCH_ST_FREE;	
			ctrl_desc[nctrl].tei = -1;
			
			/* init unit to controller table */
			
			utoc_tab[bri] = nctrl;
			
			/* increment no. of controllers */
			
			nctrl++;

			break;
			
		case STI_L1STAT:
			i4b_l4_l12stat(bri, 1, parm);
			NDBGL3(L3_MSG, "STI_L1STAT: bri %d layer 1 = %s", bri, status ? "up" : "down");
			break;
			
		case STI_L2STAT:
			i4b_l4_l12stat(bri, 2, parm);
			NDBGL3(L3_MSG, "STI_L2STAT: bri %d layer 2 = %s", bri, status ? "up" : "down");
			break;

		case STI_TEIASG:
			ctrl_desc[bri].tei = parm;
			i4b_l4_teiasg(bri, parm);
			NDBGL3(L3_MSG, "STI_TEIASG: bri %d TEI = %d = 0x%02x", bri, parm, parm);
			break;

		case STI_PDEACT:	/* L1 T4 timeout */
			NDBGL3(L3_ERR, "STI_PDEACT: bri %d TEI = %d = 0x%02x", bri, parm, parm);

			sendup = 0;

			for(i=0; i < N_CALL_DESC; i++)
			{
				if(call_desc[i].bri == bri)
                		{
					i4b_l3_stop_all_timers(&(call_desc[i]));
					if(call_desc[i].cdid != CDID_UNUSED)
						sendup++;
				}
			}

			ctrl_desc[utoc_tab[bri]].dl_est = DL_DOWN;
			ctrl_desc[utoc_tab[bri]].bch_state[CHAN_B1] = BCH_ST_FREE;
			ctrl_desc[utoc_tab[bri]].bch_state[CHAN_B2] = BCH_ST_FREE;
			ctrl_desc[utoc_tab[bri]].tei = -1;

			if(sendup)
			{
				i4b_l4_pdeact(bri, sendup);
				call_desc[i].cdid = CDID_UNUSED;
			}
			break;

		case STI_NOL1ACC:	/* no outgoing access to S0 */
			NDBGL3(L3_ERR, "STI_NOL1ACC: bri %d no outgoing access to S0", bri);

			for(i=0; i < N_CALL_DESC; i++)
			{
				if(call_desc[i].bri == bri)
                		{
					if(call_desc[i].cdid != CDID_UNUSED)
					{
						SET_CAUSE_TYPE(call_desc[i].cause_in, CAUSET_I4B);
						SET_CAUSE_VAL(call_desc[i].cause_in, CAUSE_I4B_L1ERROR);
						i4b_l4_disconnect_ind(&(call_desc[i]));
					}
				}
			}

			ctrl_desc[utoc_tab[bri]].dl_est = DL_DOWN;
			ctrl_desc[utoc_tab[bri]].bch_state[CHAN_B1] = BCH_ST_FREE;
			ctrl_desc[utoc_tab[bri]].bch_state[CHAN_B2] = BCH_ST_FREE;
			ctrl_desc[utoc_tab[bri]].tei = -1;
			break;

		default:
			NDBGL3(L3_ERR, "ERROR, bri %d, unknown status value %d!", bri, status);
			break;
	}		
	return(0);
}

/*---------------------------------------------------------------------------*
 *	send command to the lower layers
 *---------------------------------------------------------------------------*/
static void
n_mgmt_command(int bri, int cmd, void *parm)
{
	int i;

	switch(cmd)
	{
		case CMR_DOPEN:
			NDBGL3(L3_MSG, "CMR_DOPEN for bri %d", bri);
			
			for(i=0; i < N_CALL_DESC; i++)
			{
				if(call_desc[i].bri == bri)
                		{
                			call_desc[i].cdid = CDID_UNUSED;
				}
			}

			ctrl_desc[utoc_tab[bri]].dl_est = DL_DOWN;
			ctrl_desc[utoc_tab[bri]].bch_state[CHAN_B1] = BCH_ST_FREE;
			ctrl_desc[utoc_tab[bri]].bch_state[CHAN_B2] = BCH_ST_FREE;
			ctrl_desc[utoc_tab[bri]].tei = -1;
			break;

		case CMR_DCLOSE:
			NDBGL3(L3_MSG, "CMR_DCLOSE for bri %d", bri);
			break;
			
		default:
			NDBGL3(L3_MSG, "unknown cmd %d for bri %d", cmd, bri);
			break;
	}

	i4b_mdl_command_req(bri, cmd, parm);
}

/*---------------------------------------------------------------------------*
 *	handle connect request message from userland
 *---------------------------------------------------------------------------*/
static void
n_connect_request(u_int cdid)
{
	call_desc_t *cd;

	cd = cd_by_cdid(cdid);

	next_l3state(cd, EV_SETUPRQ);	
}

/*---------------------------------------------------------------------------*
 *	handle setup response message from userland
 *---------------------------------------------------------------------------*/
static void
n_connect_response(u_int cdid, int response, int cause)
{
	call_desc_t *cd;
	int chstate;

	cd = cd_by_cdid(cdid);

	T400_stop(cd);
	
	cd->response = response;
	cd->cause_out = cause;

	switch(response)
	{
		case SETUP_RESP_ACCEPT:
			next_l3state(cd, EV_SETACRS);
			chstate = BCH_ST_USED;
			break;
		
		case SETUP_RESP_REJECT:
			next_l3state(cd, EV_SETRJRS);
			chstate = BCH_ST_FREE;
			break;
			
		case SETUP_RESP_DNTCRE:
			next_l3state(cd, EV_SETDCRS);
			chstate = BCH_ST_FREE;
			break;

		default:	/* failsafe */
			next_l3state(cd, EV_SETDCRS);
			chstate = BCH_ST_FREE;
			NDBGL3(L3_ERR, "unknown response, doing SETUP_RESP_DNTCRE");
			break;
	}

	if((cd->channelid == CHAN_B1) || (cd->channelid == CHAN_B2))
	{
		ctrl_desc[cd->bri].bch_state[cd->channelid] = chstate;
		i4b_l2_channel_set_state(cd->bri, cd->channelid, chstate);
	}
	else
	{
		NDBGL3(L3_MSG, "Warning, invalid channelid %d, response = %d\n", cd->channelid, response);
	}
}

/*---------------------------------------------------------------------------*
 *	handle disconnect request message from userland
 *---------------------------------------------------------------------------*/
static void
n_disconnect_request(u_int cdid, int cause)
{
	call_desc_t *cd;

	cd = cd_by_cdid(cdid);

	cd->cause_out = cause;

	next_l3state(cd, EV_DISCRQ);
}

/*---------------------------------------------------------------------------*
 *	handle alert request message from userland
 *---------------------------------------------------------------------------*/
static void
n_alert_request(u_int cdid)
{
	call_desc_t *cd;

	cd = cd_by_cdid(cdid);

	next_l3state(cd, EV_ALERTRQ);
}

#endif /* NI4BQ931 > 0 */
