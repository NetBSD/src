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
 *	$Id: i4b_l4if.c,v 1.2.2.3 2002/06/23 17:51:30 jdolecek Exp $ 
 *
 * $FreeBSD$
 *
 *      last edit-date: [Fri Jan  5 11:33:47 2001]
 *
 *---------------------------------------------------------------------------*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i4b_l4if.c,v 1.2.2.3 2002/06/23 17:51:30 jdolecek Exp $");

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
#include <netisdn/i4b_l2.h>
#include <netisdn/i4b_l1l2.h>
#include <netisdn/i4b_l3l4.h>
#include <netisdn/i4b_mbuf.h>
#include <netisdn/i4b_global.h>

#include <netisdn/i4b_l3.h>
#include <netisdn/i4b_l3fsm.h>
#include <netisdn/i4b_q931.h>

#include <netisdn/i4b_l4.h>

void n_connect_request(struct call_desc *cd);
void n_connect_response(struct call_desc *cd, int response, int cause);
void n_disconnect_request(struct call_desc *cd, int cause);
void n_alert_request(struct call_desc *cd);
void n_mgmt_command(struct isdn_l3_driver *drv, int cmd, void *parm);

/*---------------------------------------------------------------------------*
 *	i4b_mdl_status_ind - status indication from lower layers
 *---------------------------------------------------------------------------*/
int
i4b_mdl_status_ind(struct isdn_l3_driver *d, int status, int parm)
{
	int sendup, update_leds = 0;
	int i;
	
	NDBGL3(L3_MSG, "bri = %d, status = %d, parm = %d", d->bri, status, parm);

	switch(status)
	{
		case STI_ATTACH:
			if (parm) {
				NDBGL3(L3_MSG, "STI_ATTACH: attaching bri %d", d->bri);
			} else {
				NDBGL3(L3_MSG, "STI_ATTACH: dettaching bri %d", d->bri);
			}
			break;

		case STI_L1STAT:
			i4b_l4_l12stat(d, 1, parm);
			update_leds = 1;
			NDBGL3(L3_MSG, "STI_L1STAT: bri %d layer 1 = %s", d->bri, status ? "up" : "down");
			break;
			
		case STI_L2STAT:
			i4b_l4_l12stat(d, 2, parm);
			update_leds = 1;
			NDBGL3(L3_MSG, "STI_L2STAT: bri %d layer 2 = %s", d->bri, status ? "up" : "down");
			break;

		case STI_TEIASG:
			d->tei = parm;
			i4b_l4_teiasg(d, parm);
			update_leds = 1;
			NDBGL3(L3_MSG, "STI_TEIASG: bri %d TEI = %d = 0x%02x", d->bri, parm, parm);
			break;

		case STI_PDEACT:	/* L1 T4 timeout */
			NDBGL3(L3_ERR, "STI_PDEACT: bri %d TEI = %d = 0x%02x", d->bri, parm, parm);

			update_leds = 1;
			sendup = 0;

			for(i=0; i < num_call_desc; i++)
			{
				if(call_desc[i].bri == d->bri)
                		{
					i4b_l3_stop_all_timers(&(call_desc[i]));
					if(call_desc[i].cdid != CDID_UNUSED)
						sendup++;
				}
			}

			d->dl_est = DL_DOWN;
			d->bch_state[CHAN_B1] = BCH_ST_FREE;
			d->bch_state[CHAN_B2] = BCH_ST_FREE;
			d->tei = -1;

			if(sendup)
			{
				i4b_l4_pdeact(d, sendup);
				call_desc[i].cdid = CDID_UNUSED;
			}
			break;

		case STI_NOL1ACC:	/* no outgoing access to S0 */
			NDBGL3(L3_ERR, "STI_NOL1ACC: bri %d no outgoing access to S0", d->bri);
			update_leds = 1;

			for(i=0; i < num_call_desc; i++)
			{
				if(call_desc[i].bri == d->bri)
                		{
					if(call_desc[i].cdid != CDID_UNUSED)
					{
						SET_CAUSE_TYPE(call_desc[i].cause_in, CAUSET_I4B);
						SET_CAUSE_VAL(call_desc[i].cause_in, CAUSE_I4B_L1ERROR);
						i4b_l4_disconnect_ind(&(call_desc[i]));
					}
				}
			}
			d->dl_est = DL_DOWN;
			d->bch_state[CHAN_B1] = BCH_ST_FREE;
			d->bch_state[CHAN_B2] = BCH_ST_FREE;
			d->tei = -1;

			break;

		default:
			NDBGL3(L3_ERR, "ERROR, bri %d, unknown status value %d!", d->bri, status);
			break;
	}

	if (update_leds && d != NULL)
		update_controller_leds(d);

	return(0);
}

void
update_controller_leds(struct isdn_l3_driver *d)
{
	int leds = 0;

	if (d->tei != -1)
		leds |= CMRLEDS_TEI;
	if (d->bch_state[CHAN_B1] != BCH_ST_FREE)
		leds |= CMRLEDS_B0;
	if (d->bch_state[CHAN_B2] != BCH_ST_FREE)
		leds |= CMRLEDS_B1;

	d->l3driver->N_MGMT_COMMAND(d, CMR_SETLEDS, (void*)leds);
}

/*---------------------------------------------------------------------------*
 *	send command to the lower layers
 *---------------------------------------------------------------------------*/
void
n_mgmt_command(struct isdn_l3_driver *d, int cmd, void *parm)
{
	int i;

	switch(cmd)
	{
		case CMR_DOPEN:
			NDBGL3(L3_MSG, "CMR_DOPEN for bri %d", d->bri);
			
			for(i=0; i < num_call_desc; i++)
			{
				if(call_desc[i].bri == d->bri)
                		{
                			call_desc[i].cdid = CDID_UNUSED;
				}
			}
			d->dl_est = DL_DOWN;
			d->bch_state[CHAN_B1] = BCH_ST_FREE;
			d->bch_state[CHAN_B2] = BCH_ST_FREE;
			d->tei = -1;

			break;

		case CMR_DCLOSE:
			NDBGL3(L3_MSG, "CMR_DCLOSE for bri %d", d->bri);
			break;
			
		default:
			NDBGL3(L3_MSG, "unknown cmd %d for bri %d", cmd, d->bri);
			break;
	}

	i4b_mdl_command_req(d, cmd, parm);
}

/*---------------------------------------------------------------------------*
 *	handle connect request message from userland
 *---------------------------------------------------------------------------*/
void
n_connect_request(struct call_desc *cd)
{
	next_l3state(cd, EV_SETUPRQ);	
}

/*---------------------------------------------------------------------------*
 *	handle setup response message from userland
 *---------------------------------------------------------------------------*/
void
n_connect_response(struct call_desc *cd, int response, int cause)
{
	struct isdn_l3_driver *d = cd->l3drv;
	int chstate;

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
		d->bch_state[cd->channelid] = chstate;
		i4b_l2_channel_set_state(cd->l3drv, cd->channelid, chstate);
		update_controller_leds(d);
	}
	else
	{
		NDBGL3(L3_MSG, "Warning, invalid channelid %d, response = %d\n", cd->channelid, response);
	}
}

/*---------------------------------------------------------------------------*
 *	handle disconnect request message from userland
 *---------------------------------------------------------------------------*/
void
n_disconnect_request(struct call_desc *cd, int cause)
{
	cd->cause_out = cause;

	next_l3state(cd, EV_DISCRQ);
}

/*---------------------------------------------------------------------------*
 *	handle alert request message from userland
 *---------------------------------------------------------------------------*/
void
n_alert_request(struct call_desc *cd)
{
	next_l3state(cd, EV_ALERTRQ);
}

#endif /* NI4BQ931 > 0 */
