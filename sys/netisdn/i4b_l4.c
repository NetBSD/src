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
 *	i4b_l4.c - kernel interface to userland
 *	-----------------------------------------
 *
 *	$Id: i4b_l4.c,v 1.21 2002/05/02 18:56:55 martin Exp $ 
 *
 * $FreeBSD$
 *
 *      last edit-date: [Fri Jan  5 11:33:47 2001]
 *
 *---------------------------------------------------------------------------*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i4b_l4.c,v 1.21 2002/05/02 18:56:55 martin Exp $");

#include "isdn.h"
#include "irip.h"

#if NISDN > 0

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/mbuf.h>
#include <sys/proc.h>
#include <sys/fcntl.h>
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

#include <netisdn/i4b_global.h>
#include <netisdn/i4b_l3l4.h>
#include <netisdn/i4b_mbuf.h>
#include <netisdn/i4b_l2.h>
#include <netisdn/i4b_l3.h>
#include <netisdn/i4b_l4.h>

unsigned int i4b_l4_debug = L4_DEBUG_DEFAULT;

/*
 * BRIs (in userland sometimes called "controllers", but one controller
 * may have multiple BRIs, for example daic QUAD cards attach four BRIs).
 */
static SLIST_HEAD(, isdn_l3_driver) bri_list = SLIST_HEAD_INITIALIZER(bri_list);
static int next_bri = 0;

/*
 * Attach a new L3 driver instance and return it's BRI identifier
 */
struct isdn_l3_driver *
isdn_attach_bri(const char *devname, const char *cardname, 
    void *l1_token, const struct isdn_l3_driver_functions *l3driver)
{
	int s = splnet();
	int l, bri = next_bri++;
	struct isdn_l3_driver *new_ctrl;

	new_ctrl = malloc(sizeof(*new_ctrl), M_DEVBUF, 0);
	memset(new_ctrl, 0, sizeof *new_ctrl);
	SLIST_INSERT_HEAD(&bri_list, new_ctrl, l3drvq);
	l = strlen(devname);
	new_ctrl->devname = malloc(l+1, M_DEVBUF, 0);
	strcpy(new_ctrl->devname, devname);
	l = strlen(cardname);
	new_ctrl->card_name = malloc(l+1, M_DEVBUF, 0);
	strcpy(new_ctrl->card_name, cardname);

	new_ctrl->l3driver = l3driver;
	new_ctrl->l1_token = l1_token;
	new_ctrl->bri = bri;
	new_ctrl->tei = -1;
	new_ctrl->dl_est = DL_DOWN;
	new_ctrl->bch_state[0] = BCH_ST_FREE;
	new_ctrl->bch_state[1] = BCH_ST_FREE;

	splx(s);

	return new_ctrl;
}

/*
 * Detach a L3 driver instance
 */
int
isdn_detach_bri(struct isdn_l3_driver *l3drv)
{
	struct isdn_l3_driver *sc;
	int s = splnet();
	int bri = l3drv->bri;
	int max;

	i4b_l4_contr_ev_ind(bri, 0);
	SLIST_REMOVE(&bri_list, l3drv, isdn_l3_driver, l3drvq);

	max = -1;
	SLIST_FOREACH(sc, &bri_list, l3drvq)
		if (sc->bri > max)
			max = sc->bri;
	next_bri = max+1;

	free_all_cd_of_bri(bri);

	splx(s);

	free(l3drv, M_DEVBUF);
	printf("BRI %d detached\n", bri);
	return 1;
}

struct isdn_l3_driver *
isdn_find_l3_by_bri(int bri)
{
	struct isdn_l3_driver *sc;

	SLIST_FOREACH(sc, &bri_list, l3drvq)
		if (sc->bri == bri)
			return sc;
	return NULL;
}

int isdn_count_bri(int *mbri)
{
	struct isdn_l3_driver *sc;
	int count = 0;
	int maxbri = -1;

	SLIST_FOREACH(sc, &bri_list, l3drvq) {
		count++;
		if (sc->bri > maxbri)
			maxbri = sc->bri;
	}

	if (mbri)
		*mbri = maxbri;

	return count;
}

void *
isdn_find_softc_by_bri(int bri)
{
	struct isdn_l3_driver *sc = isdn_find_l3_by_bri(bri);
	if (sc == NULL)
		return NULL;
	/*
	 * XXX - hack: do not return a softc for active cards.
	 *       all callers of this expecting l2_softc* results
	 *       should be fixed!
	 */
	if (sc->l3driver->N_DOWNLOAD)
		return NULL;
	return sc->l1_token;
}

/*---------------------------------------------------------------------------*
 *      daemon is attached
 *---------------------------------------------------------------------------*/
void 
i4b_l4_daemon_attached(void)
{
	struct isdn_l3_driver *d;

	int x = splnet();
	SLIST_FOREACH(d, &bri_list, l3drvq)
	{
		d->l3driver->N_MGMT_COMMAND(d, CMR_DOPEN, 0);
	}
	splx(x);
}

/*---------------------------------------------------------------------------*
 *      daemon is detached
 *---------------------------------------------------------------------------*/
void 
i4b_l4_daemon_detached(void)
{
	struct isdn_l3_driver *d;

	int x = splnet();
	SLIST_FOREACH(d, &bri_list, l3drvq)
	{
		d->l3driver->N_MGMT_COMMAND(d, CMR_DCLOSE, 0);
	}
	splx(x);
}

/*
 * B-channel layer 4 drivers and their registry.
 * (Application drivers connecting to a B-channel)
 */
static int i4b_link_bchandrvr(call_desc_t *cd);
static void i4b_unlink_bchandrvr(call_desc_t *cd);
static void i4b_l4_setup_timeout(call_desc_t *cd);
static void i4b_idle_check_fix_unit(call_desc_t *cd);
static void i4b_idle_check_var_unit(call_desc_t *cd);
static void i4b_l4_setup_timeout_fix_unit(call_desc_t *cd);
static void i4b_l4_setup_timeout_var_unit(call_desc_t *cd);
static time_t i4b_get_idletime(call_desc_t *cd);

static int next_l4_driver_id = 0;

struct l4_driver_desc {
	SLIST_ENTRY(l4_driver_desc) l4drvq;
	char name[L4DRIVER_NAME_SIZ];
	int driver_id;
	const struct isdn_l4_driver_functions *driver;
	int units;
};
static SLIST_HEAD(, l4_driver_desc) l4_driver_registry
    = SLIST_HEAD_INITIALIZER(l4_driver_registry);

int isdn_l4_driver_attach(const char *name, int units, const struct isdn_l4_driver_functions *driver)
{
	struct l4_driver_desc * new_driver;

	new_driver = malloc(sizeof(struct l4_driver_desc), M_DEVBUF, 0);
	memset(new_driver, 0, sizeof(struct l4_driver_desc));
	strncpy(new_driver->name, name, L4DRIVER_NAME_SIZ);
	new_driver->name[L4DRIVER_NAME_SIZ-1] = 0;
	new_driver->driver_id =	next_l4_driver_id++;
	new_driver->driver = driver;
	new_driver->units = units;
	SLIST_INSERT_HEAD(&l4_driver_registry, new_driver, l4drvq);
	return new_driver->driver_id;
}

int isdn_l4_driver_detatch(const char *name)
{
	/* XXX - not yet implemented */
	return 0;
}

const struct isdn_l4_driver_functions *isdn_l4_find_driver(const char *name, int unit)
{
	struct l4_driver_desc * d;
	SLIST_FOREACH(d, &l4_driver_registry, l4drvq)
		if (strcmp(d->name, name) == 0) {
			return d->driver;
		}
	return NULL;
}

int isdn_l4_find_driverid(const char *name)
{
	struct l4_driver_desc * d;
	SLIST_FOREACH(d, &l4_driver_registry, l4drvq)
		if (strcmp(d->name, name) == 0) {
			return d->driver_id;
		}
	return -1;
}

const struct isdn_l4_driver_functions *isdn_l4_get_driver(int driver_id, int unit)
{
	struct l4_driver_desc * d;
	SLIST_FOREACH(d, &l4_driver_registry, l4drvq)
		if (d->driver_id == driver_id) {
			return d->driver;
		}
	return NULL;
}

/*---------------------------------------------------------------------------*
 *	send MSG_PDEACT_IND message to userland
 *---------------------------------------------------------------------------*/
void
i4b_l4_pdeact(int controller, int numactive)
{
	struct isdn_l3_driver *d = isdn_find_l3_by_bri(controller);
	struct mbuf *m;
	int i;
	call_desc_t *cd;
	
	for(i=0; i < num_call_desc; i++)
	{
		if(call_desc[i].cdid != CDID_UNUSED && call_desc[i].bri == controller)
		{
			cd = &call_desc[i];
			
			if(cd->timeout_active)
			{
				STOP_TIMER(cd->idle_timeout_handle, i4b_idle_check, cd);
			}
			
			if (cd->l4_driver != NULL && cd->l4_driver_softc != NULL)
			{
				(*cd->l4_driver->line_disconnected)(cd->l4_driver_softc, (void *)cd);
				i4b_unlink_bchandrvr(cd);
			}
		
			if((cd->channelid == CHAN_B1) || (cd->channelid == CHAN_B2))
			{
				d->bch_state[cd->channelid] = BCH_ST_FREE;
			}

			cd->cdid = CDID_UNUSED;
		}
	}
	
	if((m = i4b_Dgetmbuf(sizeof(msg_pdeact_ind_t))) != NULL)
	{
		msg_pdeact_ind_t *md = (msg_pdeact_ind_t *)m->m_data;

		md->header.type = MSG_PDEACT_IND;
		md->header.cdid = -1;

		md->controller = controller;
		md->numactive = numactive;

		i4bputqueue_hipri(m);		/* URGENT !!! */
	}
}

/*---------------------------------------------------------------------------*
 *	send MSG_L12STAT_IND message to userland
 *---------------------------------------------------------------------------*/
void
i4b_l4_l12stat(int controller, int layer, int state)
{
	struct mbuf *m;

	if((m = i4b_Dgetmbuf(sizeof(msg_l12stat_ind_t))) != NULL)
	{
		msg_l12stat_ind_t *md = (msg_l12stat_ind_t *)m->m_data;

		md->header.type = MSG_L12STAT_IND;
		md->header.cdid = -1;

		md->controller = controller;
		md->layer = layer;
		md->state = state;

		i4bputqueue(m);
	}
}

/*---------------------------------------------------------------------------*
 *	send MSG_TEIASG_IND message to userland
 *---------------------------------------------------------------------------*/
void
i4b_l4_teiasg(int controller, int tei)
{
	struct isdn_l3_driver *d = isdn_find_l3_by_bri(controller);
	struct mbuf *m;

	if((m = i4b_Dgetmbuf(sizeof(msg_teiasg_ind_t))) != NULL)
	{
		msg_teiasg_ind_t *md = (msg_teiasg_ind_t *)m->m_data;

		md->header.type = MSG_TEIASG_IND;
		md->header.cdid = -1;

		md->controller = controller;
		md->tei = d->tei;

		i4bputqueue(m);
	}
}

/*---------------------------------------------------------------------------*
 *	send MSG_DIALOUT_IND message to userland
 *---------------------------------------------------------------------------*/
void
i4b_l4_dialout(int driver, int driver_unit)
{
	struct mbuf *m;

	if((m = i4b_Dgetmbuf(sizeof(msg_dialout_ind_t))) != NULL)
	{
		msg_dialout_ind_t *md = (msg_dialout_ind_t *)m->m_data;

		md->header.type = MSG_DIALOUT_IND;
		md->header.cdid = -1;

		md->driver = driver;
		md->driver_unit = driver_unit;	

		i4bputqueue(m);
	}
}

/*---------------------------------------------------------------------------*
 *	send MSG_DIALOUTNUMBER_IND message to userland
 *---------------------------------------------------------------------------*/
void
i4b_l4_dialoutnumber(int driver, int driver_unit, int cmdlen, char *cmd)
{
	struct mbuf *m;

	if((m = i4b_Dgetmbuf(sizeof(msg_dialoutnumber_ind_t))) != NULL)
	{
		msg_dialoutnumber_ind_t *md = (msg_dialoutnumber_ind_t *)m->m_data;

		md->header.type = MSG_DIALOUTNUMBER_IND;
		md->header.cdid = -1;

		md->driver = driver;
		md->driver_unit = driver_unit;

		if(cmdlen > TELNO_MAX)
			cmdlen = TELNO_MAX;

		md->cmdlen = cmdlen;
		memcpy(md->cmd, cmd, cmdlen);
		i4bputqueue(m);
	}
}

/*---------------------------------------------------------------------------*
 *	send MSG_NEGOTIATION_COMPL message to userland
 *---------------------------------------------------------------------------*/
void
i4b_l4_negcomplete(call_desc_t *cd)
{
	struct mbuf *m;

	if((m = i4b_Dgetmbuf(sizeof(msg_negcomplete_ind_t))) != NULL)
	{
		msg_negcomplete_ind_t *md = (msg_negcomplete_ind_t *)m->m_data;

		md->header.type = MSG_NEGCOMP_IND;
		md->header.cdid = cd->cdid;

		i4bputqueue(m);
	}
}

/*---------------------------------------------------------------------------*
 *	send MSG_IFSTATE_CHANGED_IND message to userland
 *---------------------------------------------------------------------------*/
void
i4b_l4_ifstate_changed(call_desc_t *cd, int new_state)
{
	struct mbuf *m;

	if((m = i4b_Dgetmbuf(sizeof(msg_ifstatechg_ind_t))) != NULL)
	{
		msg_ifstatechg_ind_t *md = (msg_ifstatechg_ind_t *)m->m_data;

		md->header.type = MSG_IFSTATE_CHANGED_IND;
		md->header.cdid = cd->cdid;
		md->state = new_state;

		i4bputqueue(m);
	}
}

/*---------------------------------------------------------------------------*
 *	send MSG_DRVRDISC_REQ message to userland
 *---------------------------------------------------------------------------*/
void
i4b_l4_drvrdisc(int cdid)
{
	struct mbuf *m;

	if((m = i4b_Dgetmbuf(sizeof(msg_drvrdisc_req_t))) != NULL)
	{
		msg_drvrdisc_req_t *md = (msg_drvrdisc_req_t *)m->m_data;

		md->header.type = MSG_DRVRDISC_REQ;
		md->header.cdid = cdid;

		i4bputqueue(m);
	}
}

/*---------------------------------------------------------------------------*
 *	send MSG_ACCT_IND message to userland
 *---------------------------------------------------------------------------*/
void
i4b_l4_accounting(int cdid, int accttype, int ioutbytes,
		int iinbytes, int ro, int ri, int outbytes, int inbytes)
{
	struct mbuf *m;

	if((m = i4b_Dgetmbuf(sizeof(msg_accounting_ind_t))) != NULL)
	{
		msg_accounting_ind_t *md = (msg_accounting_ind_t *)m->m_data;

		md->header.type = MSG_ACCT_IND;
		md->header.cdid = cdid;

		md->accttype = accttype;
		md->ioutbytes = ioutbytes;
		md->iinbytes = iinbytes;
		md->outbps = ro;
		md->inbps = ri;
		md->outbytes = outbytes;
		md->inbytes = inbytes;
		
		i4bputqueue(m);
	}
}

/*---------------------------------------------------------------------------*
 *	send MSG_CONNECT_IND message to userland
 *---------------------------------------------------------------------------*/
void
i4b_l4_connect_ind(call_desc_t *cd)
{
	struct mbuf *m;

	if((m = i4b_Dgetmbuf(sizeof(msg_connect_ind_t))) != NULL)
	{
		msg_connect_ind_t *mp = (msg_connect_ind_t *)m->m_data;

		mp->header.type = MSG_CONNECT_IND;
		mp->header.cdid = cd->cdid;

		mp->controller = cd->bri;
		mp->channel = cd->channelid;
		mp->bprot = cd->bprot;

		cd->dir = DIR_INCOMING;

		if(strlen(cd->dst_telno) > 0)
			strcpy(mp->dst_telno, cd->dst_telno);
		else
			strcpy(mp->dst_telno, TELNO_EMPTY);

		if(strlen(cd->src_telno) > 0)
			strcpy(mp->src_telno, cd->src_telno);
		else
			strcpy(mp->src_telno, TELNO_EMPTY);
		mp->type_plan = cd->type_plan;
		memcpy(mp->src_subaddr, cd->src_subaddr, sizeof(mp->src_subaddr));
		memcpy(mp->dest_subaddr, cd->dest_subaddr, sizeof(mp->dest_subaddr));
			
		strcpy(mp->display, cd->display);

		mp->scr_ind = cd->scr_ind;
		mp->prs_ind = cd->prs_ind;		
		
		T400_start(cd);
		
		i4bputqueue(m);
	}
}

/*---------------------------------------------------------------------------*
 *	send MSG_CONNECT_ACTIVE_IND message to userland
 *---------------------------------------------------------------------------*/
void
i4b_l4_connect_active_ind(call_desc_t *cd)
{
	int s;
	struct mbuf *m;

	s = splnet();

	cd->last_active_time = cd->connect_time = SECOND;

	NDBGL4(L4_TIMO, "last_active/connect_time=%ld", (long)cd->connect_time);
	
	i4b_link_bchandrvr(cd);

	update_controller_leds(cd->l3drv);

	(*cd->l4_driver->line_connected)(cd->l4_driver_softc, cd);

	i4b_l4_setup_timeout(cd);
	
	splx(s);	
	
	if((m = i4b_Dgetmbuf(sizeof(msg_connect_active_ind_t))) != NULL)
	{
		msg_connect_active_ind_t *mp = (msg_connect_active_ind_t *)m->m_data;

		mp->header.type = MSG_CONNECT_ACTIVE_IND;
		mp->header.cdid = cd->cdid;
		mp->controller = cd->bri;
		mp->channel = cd->channelid;
		if(cd->datetime[0] != '\0')
			strcpy(mp->datetime, cd->datetime);
		else
			mp->datetime[0] = '\0';
		i4bputqueue(m);
	}
}

/*---------------------------------------------------------------------------*
 *	send MSG_DISCONNECT_IND message to userland
 *---------------------------------------------------------------------------*/
void
i4b_l4_disconnect_ind(call_desc_t *cd)
{
	struct isdn_l3_driver *d;
	struct mbuf *m;

	if(cd->timeout_active)
		STOP_TIMER(cd->idle_timeout_handle, i4b_idle_check, cd);

	if (cd->l4_driver != NULL && cd->l4_driver_softc != NULL)
	{
		(*cd->l4_driver->line_disconnected)(cd->l4_driver_softc, (void *)cd);
		i4b_unlink_bchandrvr(cd);
	}

	d = cd->l3drv;

	if((cd->channelid == CHAN_B1) || (cd->channelid == CHAN_B2))
	{
		d->bch_state[cd->channelid] = BCH_ST_FREE;
		i4b_l2_channel_set_state(cd->bri, cd->channelid, BCH_ST_FREE);
	}
	else
	{
		/* no error, might be hunting call for callback */
		NDBGL4(L4_MSG, "channel free not B1/B2 but %d!", cd->channelid);
	}
	update_controller_leds(d);
	
	if((m = i4b_Dgetmbuf(sizeof(msg_disconnect_ind_t))) != NULL)
	{
		msg_disconnect_ind_t *mp = (msg_disconnect_ind_t *)m->m_data;

		mp->header.type = MSG_DISCONNECT_IND;
		mp->header.cdid = cd->cdid;
		mp->cause = cd->cause_in;

		i4bputqueue(m);
	}
}

/*---------------------------------------------------------------------------*
 *	send MSG_IDLE_TIMEOUT_IND message to userland
 *---------------------------------------------------------------------------*/
void
i4b_l4_idle_timeout_ind(call_desc_t *cd)
{
	struct mbuf *m;

	if((m = i4b_Dgetmbuf(sizeof(msg_idle_timeout_ind_t))) != NULL)
	{
		msg_idle_timeout_ind_t *mp = (msg_idle_timeout_ind_t *)m->m_data;

		mp->header.type = MSG_IDLE_TIMEOUT_IND;
		mp->header.cdid = cd->cdid;

		i4bputqueue(m);
	}
}

/*---------------------------------------------------------------------------*
 *	send MSG_CHARGING_IND message to userland
 *---------------------------------------------------------------------------*/
void
i4b_l4_charging_ind(call_desc_t *cd)
{
	struct mbuf *m;

	if((m = i4b_Dgetmbuf(sizeof(msg_charging_ind_t))) != NULL)
	{
		msg_charging_ind_t *mp = (msg_charging_ind_t *)m->m_data;

		mp->header.type = MSG_CHARGING_IND;
		mp->header.cdid = cd->cdid;
		mp->units_type = cd->units_type;

/*XXX*/		if(mp->units_type == CHARGE_CALC)
			mp->units = cd->cunits;
		else
			mp->units = cd->units;

		i4bputqueue(m);
	}
}

/*---------------------------------------------------------------------------*
 *	send MSG_STATUS_IND message to userland
 *---------------------------------------------------------------------------*/
void
i4b_l4_status_ind(call_desc_t *cd)
{
}

/*---------------------------------------------------------------------------*
 *	send MSG_ALERT_IND message to userland
 *---------------------------------------------------------------------------*/
void
i4b_l4_alert_ind(call_desc_t *cd)
{
	struct mbuf *m;

	if((m = i4b_Dgetmbuf(sizeof(msg_alert_ind_t))) != NULL)
	{
		msg_alert_ind_t *mp = (msg_alert_ind_t *)m->m_data;

		mp->header.type = MSG_ALERT_IND;
		mp->header.cdid = cd->cdid;

		i4bputqueue(m);
	}
}

/*---------------------------------------------------------------------------*
 *	send MSG_INFO_IND message to userland
 *---------------------------------------------------------------------------*/
void
i4b_l4_info_ind(call_desc_t *cd)
{
}

/*---------------------------------------------------------------------------*
 *	send MSG_INFO_IND message to userland
 *---------------------------------------------------------------------------*/
void
i4b_l4_proceeding_ind(call_desc_t *cd)
{
	struct mbuf *m;

	if((m = i4b_Dgetmbuf(sizeof(msg_proceeding_ind_t))) != NULL)
	{
		msg_proceeding_ind_t *mp = (msg_proceeding_ind_t *)m->m_data;

		mp->header.type = MSG_PROCEEDING_IND;
		mp->header.cdid = cd->cdid;
		mp->controller = cd->bri;
		mp->channel = cd->channelid;
		i4bputqueue(m);
	}
}

/*---------------------------------------------------------------------------*
 *    send MSG_PACKET_IND message to userland
 *---------------------------------------------------------------------------*/
void
i4b_l4_packet_ind(int driver, int driver_unit, int dir, struct mbuf *pkt)
{
	struct mbuf *m;
	int len = pkt->m_pkthdr.len;
	unsigned char *ip = pkt->m_data;

	if((m = i4b_Dgetmbuf(sizeof(msg_packet_ind_t))) != NULL)
	{
		msg_packet_ind_t *mp = (msg_packet_ind_t *)m->m_data;

		mp->header.type = MSG_PACKET_IND;
		mp->header.cdid = -1;
		mp->driver = driver;
		mp->driver_unit = driver_unit;
		mp->direction = dir;
		memcpy(mp->pktdata, ip,
			len <MAX_PACKET_LOG ? len : MAX_PACKET_LOG);
		i4bputqueue(m);
	}
}

/*---------------------------------------------------------------------------*
 *    send MSG_CONTR_EV_IND message to userland
 *---------------------------------------------------------------------------*/
void
i4b_l4_contr_ev_ind(int controller, int attach)
{
	struct mbuf *m;

	if((m = i4b_Dgetmbuf(sizeof(msg_ctrl_ev_ind_t))) != NULL)
	{
		msg_ctrl_ev_ind_t *ev = (msg_ctrl_ev_ind_t *)m->m_data;

		ev->header.type = MSG_CONTR_EV_IND;
		ev->header.cdid = -1;
		ev->controller = controller;
		ev->event = attach;
		i4bputqueue(m);
	}
}

/*---------------------------------------------------------------------------*
 *	link a driver(unit) to a B-channel(controller,unit,channel)
 *---------------------------------------------------------------------------*/
static int
i4b_link_bchandrvr(call_desc_t *cd)
{
	struct isdn_l3_driver *d = cd->l3drv;
	
	if (d == NULL || d->l3driver == NULL || d->l3driver->get_linktab == NULL)
	{
			cd->ilt = NULL;
			return 1;
	}

	cd->ilt = d->l3driver->get_linktab(d->l1_token,
	    cd->channelid);

	cd->l4_driver = isdn_l4_get_driver(cd->bchan_driver_index, cd->bchan_driver_unit);
	if (cd->l4_driver != NULL)
		cd->l4_driver_softc = cd->l4_driver->get_softc(cd->bchan_driver_unit);
	else
		cd->l4_driver_softc = NULL;

	if(cd->l4_driver == NULL || cd->l4_driver_softc == NULL || cd->ilt == NULL)
		return(-1);

	if (d->l3driver->set_l4_driver != NULL)
	{
		d->l3driver->set_l4_driver(d->l1_token,
		    cd->channelid, cd->l4_driver, cd->l4_driver_softc);
	}

	cd->l4_driver->set_linktab(cd->l4_driver_softc, cd->ilt);

	/* activate B channel */
		
	(*cd->ilt->bchannel_driver->bch_config)(cd->ilt->l1token, cd->ilt->channel, cd->bprot, 1);

	return(0);
}

/*---------------------------------------------------------------------------*
 *	unlink a driver(unit) from a B-channel(controller,unit,channel)
 *---------------------------------------------------------------------------*/
static void
i4b_unlink_bchandrvr(call_desc_t *cd)
{
	struct isdn_l3_driver *d = cd->l3drv;

	/*
	 * XXX - what's this *cd manipulation for? Shouldn't we
	 * close the bchannel driver first and then just set ilt to NULL
	 * in *cd?
	 */
	if (d == NULL || d->l3driver == NULL || d->l3driver->get_linktab == NULL)
	{
		cd->ilt = NULL;
		return;
	}
	else
	{
		cd->ilt = d->l3driver->get_linktab(
		    d->l1_token, cd->channelid);
	}
	
	/* deactivate B channel */
		
	(*cd->ilt->bchannel_driver->bch_config)(cd->ilt->l1token, cd->ilt->channel, cd->bprot, 0);
} 

/*---------------------------------------------------------------------------

	How shorthold mode works for OUTGOING connections
	=================================================

	|<---- unchecked-window ------->|<-checkwindow->|<-safetywindow>|

idletime_state:      IST_NONCHK             IST_CHECK       IST_SAFE	
	
	|				|		|		|
  time>>+-------------------------------+---------------+---------------+-...
	|				|		|		|
	|				|<--idle_time-->|<--earlyhup--->|
	|<-----------------------unitlen------------------------------->|

	
	  unitlen - specifies the time a charging unit lasts
	idle_time - specifies the thime the line must be idle at the
		    end of the unit to be elected for hangup
	 earlyhup - is the beginning of a timing safety zone before the
		    next charging unit starts

	The algorithm works as follows: lets assume the unitlen is 100
	secons, idle_time is 40 seconds and earlyhup is 10 seconds.
	The line then must be idle 50 seconds after the begin of the
	current unit and it must then be quiet for 40 seconds. if it
	has been quiet for this 40 seconds, the line is closed 10
	seconds before the next charging unit starts. In case there was
	any traffic within the idle_time, the line is not closed.
	It does not matter whether there was any traffic between second
	0 and second 50 or not.


	How shorthold mode works for INCOMING connections
	=================================================

	it is just possible to specify a maximum idle time for incoming
	connections, after this time of no activity on the line the line
	is closed.
	
---------------------------------------------------------------------------*/	

static time_t
i4b_get_idletime(call_desc_t *cd)
{
	if (cd->l4_driver->get_idletime)
		return cd->l4_driver->get_idletime(cd->l4_driver_softc);
	return cd->last_active_time;
}

/*---------------------------------------------------------------------------*
 *	B channel idle check timeout setup
 *---------------------------------------------------------------------------*/ 
static void
i4b_l4_setup_timeout(call_desc_t *cd)
{
	NDBGL4(L4_TIMO, "%ld: direction %d, shorthold algorithm %d",
		(long)SECOND, cd->dir, cd->shorthold_data.shorthold_algorithm);
	
	cd->timeout_active = 0;
	cd->idletime_state = IST_IDLE;
	
	if((cd->dir == DIR_INCOMING) && (cd->max_idle_time > 0))
	{
		/* incoming call: simple max idletime check */
	
		START_TIMER(cd->idle_timeout_handle, i4b_idle_check, cd, hz/2);
		cd->timeout_active = 1;
		NDBGL4(L4_TIMO, "%ld: incoming-call, setup max_idle_time to %ld", (long)SECOND, (long)cd->max_idle_time);
	}
	else if((cd->dir == DIR_OUTGOING) && (cd->shorthold_data.idle_time > 0))
	{
		switch( cd->shorthold_data.shorthold_algorithm )
		{
			default:	/* fall into the old fix algorithm */
			case SHA_FIXU:
				i4b_l4_setup_timeout_fix_unit( cd );
				break;
				
			case SHA_VARU:
				i4b_l4_setup_timeout_var_unit( cd );
				break;
		}
	}
	else
	{
		NDBGL4(L4_TIMO, "no idle_timeout configured");
	}
}

/*---------------------------------------------------------------------------*
 *	fixed unit algorithm B channel idle check timeout setup
 *---------------------------------------------------------------------------*/
static void
i4b_l4_setup_timeout_fix_unit(call_desc_t *cd)
{
	/* outgoing call */
	
	if((cd->shorthold_data.idle_time > 0) && (cd->shorthold_data.unitlen_time == 0))
	{
		/* outgoing call: simple max idletime check */
		
		START_TIMER(cd->idle_timeout_handle, i4b_idle_check, cd, hz/2);
		cd->timeout_active = 1;
		NDBGL4(L4_TIMO, "%ld: outgoing-call, setup idle_time to %ld",
			(long)SECOND, (long)cd->shorthold_data.idle_time);
	}
	else if((cd->shorthold_data.unitlen_time > 0) && (cd->shorthold_data.unitlen_time > (cd->shorthold_data.idle_time + cd->shorthold_data.earlyhup_time)))
	{
		/* outgoing call: full shorthold mode check */
		
		START_TIMER(cd->idle_timeout_handle, i4b_idle_check, cd, hz*(cd->shorthold_data.unitlen_time - (cd->shorthold_data.idle_time + cd->shorthold_data.earlyhup_time)));
		cd->timeout_active = 1;
		cd->idletime_state = IST_NONCHK;
		NDBGL4(L4_TIMO, "%ld: outgoing-call, start %ld sec nocheck window", 
			(long)SECOND, (long)(cd->shorthold_data.unitlen_time - (cd->shorthold_data.idle_time + cd->shorthold_data.earlyhup_time)));

		if(cd->aocd_flag == 0)
		{
			cd->units_type = CHARGE_CALC;
			cd->cunits++;
			i4b_l4_charging_ind(cd);
		}
	}
	else
	{
		/* parms somehow got wrong .. */
		
		NDBGL4(L4_ERR, "%ld: ERROR: idletime[%ld]+earlyhup[%ld] > unitlength[%ld]!",
			(long)SECOND, (long)cd->shorthold_data.idle_time, (long)cd->shorthold_data.earlyhup_time, (long)cd->shorthold_data.unitlen_time);
	}
}

/*---------------------------------------------------------------------------*
 *	variable unit algorithm B channel idle check timeout setup
 *---------------------------------------------------------------------------*/
static void
i4b_l4_setup_timeout_var_unit(call_desc_t *cd)
{
	/* outgoing call: variable unit idletime check */
		
	/*
	 * start checking for an idle connect one second before the end of the unit.
	 * The one second takes into account of rounding due to the driver only
	 * using the seconds and not the uSeconds of the current time
	 */
	cd->idletime_state = IST_CHECK;	/* move directly to the checking state */

	START_TIMER(cd->idle_timeout_handle, i4b_idle_check, cd, hz * (cd->shorthold_data.unitlen_time - 1) );
	cd->timeout_active = 1;
	NDBGL4(L4_TIMO, "%ld: outgoing-call, var idle time - setup to %ld",
		(long)SECOND, (long)cd->shorthold_data.unitlen_time);
}


/*---------------------------------------------------------------------------*
 *	B channel idle check timeout function
 *---------------------------------------------------------------------------*/ 
void
i4b_idle_check(call_desc_t *cd)
{
	int s;

	if(cd->cdid == CDID_UNUSED)
		return;
	
	s = splnet();

	/* failsafe */

	if(cd->timeout_active == 0)
	{
		NDBGL4(L4_ERR, "ERROR: timeout_active == 0 !!!");
	}
	else
	{	
		cd->timeout_active = 0;
	}
	
	/* incoming connections, simple idletime check */

	if(cd->dir == DIR_INCOMING)
	{
		if((i4b_get_idletime(cd) + cd->max_idle_time) <= SECOND)
		{
			struct isdn_l3_driver *d = cd->l3drv;
			NDBGL4(L4_TIMO, "%ld: incoming-call, line idle timeout, disconnecting!", (long)SECOND);
			d->l3driver->N_DISCONNECT_REQUEST(cd,
					(CAUSET_I4B << 8) | CAUSE_I4B_NORMAL);
			i4b_l4_idle_timeout_ind(cd);
		}
		else
		{
			NDBGL4(L4_TIMO, "%ld: incoming-call, activity, last_active=%ld, max_idle=%ld", (long)SECOND, (long)i4b_get_idletime(cd), (long)cd->max_idle_time);

			START_TIMER(cd->idle_timeout_handle, i4b_idle_check, cd, hz/2);
			cd->timeout_active = 1;
		}
	}

	/* outgoing connections */

	else if(cd->dir == DIR_OUTGOING)
	{
		switch( cd->shorthold_data.shorthold_algorithm )
		{
			case SHA_FIXU:
				i4b_idle_check_fix_unit( cd );
				break;
			case SHA_VARU:
				i4b_idle_check_var_unit( cd );
				break;
			default:
				NDBGL4(L4_TIMO, "%ld: bad value for shorthold_algorithm of %d",
					(long)SECOND, cd->shorthold_data.shorthold_algorithm);
				i4b_idle_check_fix_unit( cd );
				break;
		}
	}
	splx(s);
}

/*---------------------------------------------------------------------------*
 *	fixed unit algorithm B channel idle check timeout function
 *---------------------------------------------------------------------------*/
static void
i4b_idle_check_fix_unit(call_desc_t *cd)
{
	struct isdn_l3_driver *d = cd->l3drv;

	/* simple idletime calculation */

	if((cd->shorthold_data.idle_time > 0) && (cd->shorthold_data.unitlen_time == 0))
	{
		if((i4b_get_idletime(cd) + cd->shorthold_data.idle_time) <= SECOND)
		{
			NDBGL4(L4_TIMO, "%ld: outgoing-call-st, idle timeout, disconnecting!", (long)SECOND);
			d->l3driver->N_DISCONNECT_REQUEST(cd, (CAUSET_I4B << 8) | CAUSE_I4B_NORMAL);
			i4b_l4_idle_timeout_ind(cd);
		}
		else
		{
			NDBGL4(L4_TIMO, "%ld: outgoing-call-st, activity, last_active=%ld, max_idle=%ld",
					(long)SECOND, (long)i4b_get_idletime(cd), (long)cd->shorthold_data.idle_time);
			START_TIMER(cd->idle_timeout_handle, i4b_idle_check, cd, hz/2);
			cd->timeout_active = 1;
		}
	}

	/* full shorthold mode calculation */

	else if((cd->shorthold_data.unitlen_time > 0)
	         && (cd->shorthold_data.unitlen_time > (cd->shorthold_data.idle_time + cd->shorthold_data.earlyhup_time)))
	{
		switch(cd->idletime_state)
		{

		case IST_NONCHK:	/* end of non-check time */

			START_TIMER(cd->idle_timeout_handle, i4b_idle_check, cd, hz*(cd->shorthold_data.idle_time));
			cd->idletimechk_start = SECOND;
			cd->idletime_state = IST_CHECK;
			cd->timeout_active = 1;
			NDBGL4(L4_TIMO, "%ld: outgoing-call, idletime check window reached!", (long)SECOND);
			break;

		case IST_CHECK:		/* end of idletime chk */
			if((i4b_get_idletime(cd) > cd->idletimechk_start) &&
			   (i4b_get_idletime(cd) <= SECOND))
			{	/* activity detected */
				START_TIMER(cd->idle_timeout_handle, i4b_idle_check, cd, hz*(cd->shorthold_data.earlyhup_time));
				cd->timeout_active = 1;
				cd->idletime_state = IST_SAFE;
				NDBGL4(L4_TIMO, "%ld: outgoing-call, activity at %ld, wait earlyhup-end", (long)SECOND, (long)i4b_get_idletime(cd));
			}
			else
			{	/* no activity, hangup */
				NDBGL4(L4_TIMO, "%ld: outgoing-call, idle timeout, last activity at %ld", (long)SECOND, (long)i4b_get_idletime(cd));
				d->l3driver->N_DISCONNECT_REQUEST(cd, (CAUSET_I4B << 8) | CAUSE_I4B_NORMAL);
				i4b_l4_idle_timeout_ind(cd);
				cd->idletime_state = IST_IDLE;
			}
			break;

		case IST_SAFE:	/* end of earlyhup time */

			START_TIMER(cd->idle_timeout_handle, i4b_idle_check, cd, hz*(cd->shorthold_data.unitlen_time - (cd->shorthold_data.idle_time+cd->shorthold_data.earlyhup_time)));
			cd->timeout_active = 1;
			cd->idletime_state = IST_NONCHK;

			if(cd->aocd_flag == 0)
			{
				cd->units_type = CHARGE_CALC;
				cd->cunits++;
				i4b_l4_charging_ind(cd);
			}
			
			NDBGL4(L4_TIMO, "%ld: outgoing-call, earlyhup end, wait for idletime start", (long)SECOND);
			break;

		default:
			NDBGL4(L4_ERR, "outgoing-call: invalid idletime_state value!");
			cd->idletime_state = IST_IDLE;
			break;
		}
	}
}

/*---------------------------------------------------------------------------*
 *	variable unit algorithm B channel idle check timeout function
 *---------------------------------------------------------------------------*/
static void
i4b_idle_check_var_unit(call_desc_t *cd)
{
	switch(cd->idletime_state)
	{

	/* see if there has been any activity within the last idle_time seconds */
	case IST_CHECK:
		if( i4b_get_idletime(cd) > (SECOND - cd->shorthold_data.idle_time))
		{	/* activity detected */
#if defined(__FreeBSD_version) && __FreeBSD_version >= 300001
			cd->idle_timeout_handle =
#endif
			/* check again in one second */
			START_TIMER(cd->idle_timeout_handle, i4b_idle_check, cd, hz);
			cd->timeout_active = 1;
			cd->idletime_state = IST_CHECK;
			NDBGL4(L4_TIMO, "%ld: outgoing-call, var idle timeout - activity at %ld, continuing", (long)SECOND, (long)i4b_get_idletime(cd));
		}
		else
		{	/* no activity, hangup */
			struct isdn_l3_driver *d = cd->l3drv;
			NDBGL4(L4_TIMO, "%ld: outgoing-call, var idle timeout - last activity at %ld", (long)SECOND, (long)i4b_get_idletime(cd));
			d->l3driver->N_DISCONNECT_REQUEST(cd, (CAUSET_I4B << 8) | CAUSE_I4B_NORMAL);
			i4b_l4_idle_timeout_ind(cd);
			cd->idletime_state = IST_IDLE;
		}
		break;

	default:
		NDBGL4(L4_ERR, "outgoing-call: var idle timeout invalid idletime_state value!");
		cd->idletime_state = IST_IDLE;
		break;
	}
}

#endif /* NISDN > 0 */
