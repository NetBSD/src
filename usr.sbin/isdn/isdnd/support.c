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
 *	i4b daemon - misc support routines
 *	----------------------------------
 *
 *	$Id: support.c,v 1.7 2002/04/10 23:35:08 martin Exp $ 
 *
 * $FreeBSD$
 *
 *      last edit-date: [Wed Oct  4 18:24:27 2000]
 *
 *---------------------------------------------------------------------------*/

#include "isdnd.h"

static int isvalidtime(struct cfg_entry *cep);

static SLIST_HEAD(, isdn_ctrl_state) isdn_ctrl_list =
    SLIST_HEAD_INITIALIZER(isdn_ctrl_list);

static SIMPLEQ_HEAD(, cfg_entry) cfg_entry_list =
    SIMPLEQ_HEAD_INITIALIZER(cfg_entry_list);
	
/*---------------------------------------------------------------------------*
 *	find an active entry by driver type and driver unit
 *---------------------------------------------------------------------------*/
struct cfg_entry *
find_active_entry_by_driver(int drivertype, int driverunit)
{
	struct cfg_entry *cep = NULL;
	int i;

	SIMPLEQ_FOREACH(cep, &cfg_entry_list, cfgq) {

		if(!((cep->usrdevice == drivertype) &&
		     (cep->usrdeviceunit == driverunit)))
		{
			continue;
		}

		/* check time interval */
		
		if(isvalidtime(cep) == 0)
		{
			DBGL(DL_MSG, (log(LL_DBG, "find_active_entry_by_driver: entry %d, time not valid!", i)));
			continue;
		}
		
		/* found */
		
		if(cep->cdid == CDID_UNUSED)
		{
			DBGL(DL_MSG, (log(LL_DBG, "find_active_entry_by_driver: entry %d [%s%d], cdid=CDID_UNUSED !",
				cep->index, cep->usrdevicename, driverunit)));
			return(NULL);
		}
		else if(cep->cdid == CDID_RESERVED)
		{
			DBGL(DL_MSG, (log(LL_DBG, "find_active_entry_by_driver: entry %d [%s%d], cdid=CDID_RESERVED!",
				cep->index, cep->usrdevicename, driverunit)));
			return(NULL);
		}
		return(cep);
	}
	return(NULL);
}

/*---------------------------------------------------------------------------*
 *	find entry by drivertype and driverunit and setup for dialing out
 *---------------------------------------------------------------------------*/
struct cfg_entry *
find_by_device_for_dialout(int drivertype, int driverunit)
{
	struct cfg_entry *cep;

	SIMPLEQ_FOREACH(cep, &cfg_entry_list, cfgq) {

		/* compare driver type and unit */

		if(!((cep->usrdevice == drivertype) &&
		     (cep->usrdeviceunit == driverunit)))
		{
			continue;
		}

		/* check time interval */
		
		if(isvalidtime(cep) == 0)
		{
			DBGL(DL_MSG, (log(LL_DBG, "find_by_device_for_dialout: entry %d, time not valid!", cep->index)));
			continue;
		}
		
		/* found, check if already reserved */
		
		if(cep->cdid == CDID_RESERVED)
		{
			DBGL(DL_MSG, (log(LL_DBG, "find_by_device_for_dialout: entry %d, cdid reserved!", cep->index)));
			return(NULL);
		}

		/* check if this entry is already in use ? */
		
		if(cep->cdid != CDID_UNUSED)	
		{
			DBGL(DL_MSG, (log(LL_DBG, "find_by_device_for_dialout: entry %d, cdid in use", cep->index)));
			return(NULL);
		}

		if((setup_dialout(cep)) == GOOD)
		{
			/* found an entry to be used for calling out */
		
			DBGL(DL_MSG, (log(LL_DBG, "find_by_device_for_dialout: found entry %d!", cep->index)));
			return(cep);
		}
		else
		{
			DBGL(DL_MSG, (log(LL_DBG, "find_by_device_for_dialout: entry %d, setup_dialout() failed!", cep->index)));
			return(NULL);
		}
	}

	DBGL(DL_MSG, (log(LL_DBG, "find_by_device_for_dialout: no entry found!")));
	return(NULL);
}

/*---------------------------------------------------------------------------*
 *	find entry by drivertype and driverunit and setup for dialing out
 *---------------------------------------------------------------------------*/
struct cfg_entry *
find_by_device_for_dialoutnumber(int drivertype, int driverunit, int cmdlen, char *cmd)
{
	struct cfg_entry *cep;
	int j;

	SIMPLEQ_FOREACH(cep, &cfg_entry_list, cfgq) {

		/* compare driver type and unit */

		if(!((cep->usrdevice == drivertype) &&
		     (cep->usrdeviceunit == driverunit)))
		{
			continue;
		}

		/* check time interval */
		
		if(isvalidtime(cep) == 0)
		{
			DBGL(DL_MSG, (log(LL_DBG, "find_by_device_for_dialoutnumber: entry %d, time not valid!", cep->index)));
			continue;
		}

		/* found, check if already reserved */
		
		if(cep->cdid == CDID_RESERVED)
		{
			DBGL(DL_MSG, (log(LL_DBG, "find_by_device_for_dialoutnumber: entry %d, cdid reserved!", cep->index)));
			return(NULL);
		}

		/* check if this entry is already in use ? */
		
		if(cep->cdid != CDID_UNUSED)	
		{
			DBGL(DL_MSG, (log(LL_DBG, "find_by_device_for_dialoutnumber: entry %d, cdid in use", cep->index)));
			return(NULL);
		}

		/* check number and copy to cep->remote_numbers[] */
		
		for(j = 0; j < cmdlen; j++)
		{
			if(!(isdigit(*(cmd+j))))
			{
				DBGL(DL_MSG, (log(LL_DBG, "find_by_device_for_dialoutnumber: entry %d, dial string contains non-digit at pos %d", cep->index, j)));
				return(NULL);
			}
			/* fill in number to dial */
			cep->remote_numbers[0].number[j] = *(cmd+j);
		}				
		cep->remote_numbers[0].number[j] = '\0';
		cep->remote_numbers_count = 1;

		if((setup_dialout(cep)) == GOOD)
		{
			/* found an entry to be used for calling out */
		
			DBGL(DL_MSG, (log(LL_DBG, "find_by_device_for_dialoutnumber: found entry %d!", cep->index)));
			return(cep);
		}
		else
		{
			DBGL(DL_MSG, (log(LL_DBG, "find_by_device_for_dialoutnumber: entry %d, setup_dialout() failed!", cep->index)));
			return(NULL);
		}
	}

	DBGL(DL_MSG, (log(LL_DBG, "find_by_device_for_dialoutnumber: no entry found!")));
	return(NULL);
}

/*---------------------------------------------------------------------------*
 *	find entry by drivertype and driverunit and setup for dialing out
 *---------------------------------------------------------------------------*/
int
setup_dialout(struct cfg_entry *cep)
{
	struct isdn_ctrl_state *ctrl;

	if (cep->isdncontroller < 0) {
		/* we are free to choose a controller */
		for (ctrl = get_first_ctrl_state(); ctrl; ctrl = NEXT_CTRL(ctrl)) {
			if (get_controller_state(ctrl) != CTRL_UP)
				continue;
			switch(cep->isdnchannel) {
			case CHAN_B1:
			case CHAN_B2:
				if (ret_channel_state(ctrl, cep->isdnchannel) != CHAN_IDLE)
					continue;
				break;

			case CHAN_ANY:
				if (ret_channel_state(ctrl, CHAN_B1) != CHAN_IDLE &&
				   ret_channel_state(ctrl, CHAN_B2) != CHAN_IDLE)
					continue;
				break;
			}
			/* this controller looks ok */
			break;
		}
	} else {
		/* fixed controller in config, use that */
		ctrl = find_ctrl_state(cep->isdncontroller);
	}

	if (ctrl == NULL)
		return (ERROR);

	/* check controller operational */

	if (get_controller_state(ctrl) != CTRL_UP)
	{
		DBGL(DL_MSG, (log(LL_DBG, "setup_dialout: entry %s, controller is down", cep->name)));
		return(ERROR);
	}

	cep->isdncontrollerused = ctrl->bri;

	/* check channel available */

	switch(cep->isdnchannel)
	{
		case CHAN_B1:
		case CHAN_B2:
			if (ret_channel_state(ctrl, cep->isdnchannel) != CHAN_IDLE)
			{
				DBGL(DL_MSG, (log(LL_DBG, "setup_dialout: entry %s, channel not free", cep->name)));
				return(ERROR);
			}
			cep->isdnchannelused = cep->isdnchannel;
			break;

		case CHAN_ANY:
			if (ret_channel_state(ctrl, CHAN_B1) != CHAN_IDLE &&
			   ret_channel_state(ctrl, CHAN_B2) != CHAN_IDLE)
			{
				DBGL(DL_MSG, (log(LL_DBG, "setup_dialout: entry %s, no channel free", cep->name)));
				return(ERROR);
			}
			cep->isdnchannelused = CHAN_ANY;
			break;

		default:
			DBGL(DL_MSG, (log(LL_DBG, "setup_dialout: entry %s, channel undefined", cep->name)));
			return(ERROR);
			break;
	}

	DBGL(DL_MSG, (log(LL_DBG, "setup_dialout: entry %s ok!", cep->name)));

	/* preset disconnect cause */
	
	SET_CAUSE_TYPE(cep->disc_cause, CAUSET_I4B);
	SET_CAUSE_VAL(cep->disc_cause, CAUSE_I4B_NORMAL);
	
	return(GOOD);
}

/*---------------------------------------------------------------------------*
 *	find entry by drivertype and driverunit
 *---------------------------------------------------------------------------*/
struct cfg_entry *
get_cep_by_driver(int drivertype, int driverunit)
{
	struct cfg_entry *cep;

	SIMPLEQ_FOREACH(cep, &cfg_entry_list, cfgq) {

		if(!((cep->usrdevice == drivertype) &&
		     (cep->usrdeviceunit == driverunit)))
		{
			continue;
		}

		/* check time interval */
		
		if(isvalidtime(cep) == 0)
		{
			DBGL(DL_MSG, (log(LL_DBG, "get_cep_by_driver: entry %d, time not valid!", cep->index)));
			continue;
		}		

		DBGL(DL_MSG, (log(LL_DBG, "get_cep_by_driver: found entry %d!", cep->index)));
		return(cep);
	}
	return(NULL);
}

/*---------------------------------------------------------------------------*
 *	find a matching entry for an incoming call
 *
 *	- not found/no match: log output with LL_CHD and return NULL
 *	- found/match: make entry in free cep, return address
 *---------------------------------------------------------------------------*/
struct cfg_entry *
find_matching_entry_incoming(msg_connect_ind_t *mp, int len)
{
	struct cfg_entry *cep = NULL;
	static const char resvd_type[] = "reserverd";
	static const char no_type[] = "no type";
	static const char * const numbering_types[] = {
		"unknown",
		"international",
		"national",
		"network specific",
		"subscriber",
		"abbreviated",
		resvd_type,
		resvd_type,
		resvd_type
	};
	const char * ntype;

	/* older kernels do not deliver all the information */	
	if (((u_int8_t*)&mp->type_plan - (u_int8_t*)mp + sizeof(mp->type_plan)) <= len) {
		ntype = numbering_types[(mp->type_plan & 0x70)>>4];
	} else {
		ntype = no_type;
	}

	/* check for CW (call waiting) early */

	if(mp->channel == CHAN_NO)
	{
		if(aliasing)
	        {
			char *src_tela = "ERROR-src_tela";
			char *dst_tela = "ERROR-dst_tela";
	
	                src_tela = get_alias(mp->src_telno);
	                dst_tela = get_alias(mp->dst_telno);
	
			log(LL_CHD, "%05d <unknown> CW from %s (%s) to %s (%s) (no channel free)",
				mp->header.cdid, src_tela, ntype, dst_tela, mp->display);
		}
		else
		{
			log(LL_CHD, "%05d <unknown> call waiting from %s (%s) to %s (%s) (no channel free)",
				mp->header.cdid, mp->src_telno, ntype, mp->dst_telno, mp->display);
		}
		return(NULL);
	}
	
	SIMPLEQ_FOREACH(cep, &cfg_entry_list, cfgq) {

		int n;
		struct isdn_ctrl_state *ctrl;

		/* check my number */

		if(strncmp(cep->local_phone_incoming, mp->dst_telno, strlen(cep->local_phone_incoming)))
		{
			DBGL(DL_MSG, (log(LL_DBG, "find_matching_entry_incoming: entry %d, myno %s != incomingno %s",
				cep->index, cep->local_phone_incoming, mp->dst_telno)));
			continue;
		}

		/* check all allowed remote number's for this entry */

		for (n = 0; n < cep->incoming_numbers_count; n++)
		{
			incoming_number_t *in = &cep->remote_phone_incoming[n];
			if(in->number[0] == '*')
				break;
			if(strncmp(in->number, mp->src_telno, strlen(in->number)))
			{
				DBGL(DL_MSG, (log(LL_DBG, "find_matching_entry_incoming: entry %d, remno %s != incomingfromno %s",
					cep->index, in->number, mp->src_telno)));
			}
			else
				break;
		}
		if (n >= cep->incoming_numbers_count)
			continue;
				
		/* check b protocol */

		if(cep->b1protocol != mp->bprot)
		{
			DBGL(DL_MSG, (log(LL_DBG, "find_matching_entry_incoming: entry %d, bprot %d != incomingprot %d",
				cep->index, cep->b1protocol, mp->bprot)));
			continue;
		}

		/* is this entry currently in use ? */

		if(cep->cdid != CDID_UNUSED)
		{
			if(cep->cdid == CDID_RESERVED)
			{
				DBGL(DL_MSG, (log(LL_DBG, "find_matching_entry_incoming: entry %d, cdid is reserved", cep->index)));
			}
			else if (cep->dialin_reaction == REACT_ACCEPT
				 && cep->dialouttype == DIALOUT_CALLEDBACK)
			{
				/*
				 * We might consider doing this even if this is
				 * not a calledback config entry - BUT: there are
				 * severe race conditions and timinig problems
				 * ex. if both sides run I4B with no callback
				 * delay - both may shutdown the outgoing call
				 * and never be able to establish a connection.
				 * In the called-back case this should not happen.
				 */
				DBGL(DL_MSG, (log(LL_DBG, "find_matching_entry_incoming: entry %d, incoming call for callback in progress (cdid %05d)", cep->index, cep->cdid)));

				/* save the current call state, we're going to overwrite it with the
				 * new incoming state below... */
				cep->saved_call.cdid = cep->cdid;
				cep->saved_call.controller = cep->isdncontrollerused;
				cep->saved_call.channel = cep->isdnchannelused;
			}
			else
			{
				DBGL(DL_MSG, (log(LL_DBG, "find_matching_entry_incoming: entry %d, cdid in use", cep->index)));
				continue;	/* yes, next */
			}
		}

		/* check controller value ok */
		ctrl = find_ctrl_state(mp->controller);

		if (ctrl == NULL)
		{
			log(LL_CHD, "%05d %s incoming call with invalid controller %d",
                        	mp->header.cdid, cep->name, mp->controller);
			return(NULL);
		}

		/* check controller marked up */

		if (get_controller_state(ctrl) != CTRL_UP)
		{
			log(LL_CHD, "%05d %s incoming call, controller %d DOWN!",
                        	mp->header.cdid, cep->name, mp->controller);
			return(NULL);
		}

		/* check channel he wants */

		switch(mp->channel)
		{
			case CHAN_B1:
			case CHAN_B2:
				if ((ret_channel_state(ctrl, mp->channel)) != CHAN_IDLE)
				{
					log(LL_CHD, "%05d %s incoming call, channel %s not free!",
	                	        	mp->header.cdid, cep->name, mp->channel == CHAN_B1 ? "B1" : "B2");
					return(NULL);
				}
				break;

			case CHAN_ANY:
				if (ret_channel_state(ctrl, CHAN_B1) != CHAN_IDLE &&
				    ret_channel_state(ctrl, CHAN_B2) != CHAN_IDLE)
				{
					log(LL_CHD, "%05d %s incoming call, no channel free!",
	                	        	mp->header.cdid, cep->name);
					return(NULL);
				}
				break;

			case CHAN_NO:
				log(LL_CHD, "%05d %s incoming call, call waiting (no channel available)!",
                	        	mp->header.cdid, cep->name);
                	        return(NULL);
                	        break;

			default:
				log(LL_CHD, "%05d %s incoming call, ERROR, channel undefined!",
                	        	mp->header.cdid, cep->name);
				return(NULL);
				break;
		}

		/* check time interval */
		
		if(isvalidtime(cep) == 0)
		{
			DBGL(DL_MSG, (log(LL_DBG, "find_matching_entry_incoming: entry %d, time not valid!", cep->index)));
			continue;
		}
		
		/* found a matching entry */

		cep->cdid = mp->header.cdid;
		cep->isdncontrollerused = mp->controller;
		cep->isdnchannelused = mp->channel;
/*XXX*/		cep->disc_cause = 0;
		
		/* cp number to real one used */
		
		strcpy(cep->real_phone_incoming, mp->src_telno);

		/* copy display string */
		
		strcpy(cep->display, mp->display);
		
		/* entry currently down ? */
		
		if(cep->state == ST_DOWN)
		{
			msg_updown_ind_t mui;
			
			/* set interface up */
	
			DBGL(DL_MSG, (log(LL_DBG, "find_matching_entry_incoming: entry %d, ", cep->index)));
	
			mui.driver = cep->usrdevice;
			mui.driver_unit = cep->usrdeviceunit;
			mui.updown = SOFT_ENA;
			
			if((ioctl(isdnfd, I4B_UPDOWN_IND, &mui)) < 0)
			{
				log(LL_ERR, "find_matching_entry_incoming: ioctl I4B_UPDOWN_IND failed: %s", strerror(errno));
				error_exit(1, "find_matching_entry_incoming: ioctl I4B_UPDOWN_IND failed: %s", strerror(errno));
			}

			cep->down_retry_count = 0;
			cep->state = ST_IDLE;
		}
		return(cep);
	}

	if(aliasing)
        {
		char *src_tela = "ERROR-src_tela";
		char *dst_tela = "ERROR-dst_tela";

                src_tela = get_alias(mp->src_telno);
                dst_tela = get_alias(mp->dst_telno);

		log(LL_CHD, "%05d Call from %s (%s) to %s (%s)",
			mp->header.cdid, src_tela, ntype, dst_tela, mp->display);
	}
	else
	{
		log(LL_CHD, "%05d <unknown> incoming call from %s (%s) to %s (%s)",
			mp->header.cdid, mp->src_telno, ntype, mp->dst_telno, mp->display);
	}
	return(NULL);
}

/*---------------------------------------------------------------------------*
 *	return address of ACTIVE config entry by controller and channel
 *---------------------------------------------------------------------------*/
struct cfg_entry *
get_cep_by_cc(int ctrlr, int chan)
{
	struct cfg_entry *cep;
	struct isdn_ctrl_state *cts;

	if((chan != CHAN_B1) && (chan != CHAN_B2))
		return(NULL);

	cts = find_ctrl_state(ctrlr);
	if (cts ==  NULL)
		return(NULL);
		
	SIMPLEQ_FOREACH(cep, &cfg_entry_list, cfgq) {

		if((cep->cdid != CDID_UNUSED)		&&
		   (cep->cdid != CDID_RESERVED)		&&
		   (cep->isdnchannelused == chan)	&&
		   (cep->isdncontrollerused == ctrlr)	&&
		   ((ret_channel_state(cts, chan)) == CHAN_RUN))
		{
			return (cep);
		}
	}
	return(NULL);
}

/*---------------------------------------------------------------------------*
 *	return address of config entry identified by cdid
 *---------------------------------------------------------------------------*/
struct cfg_entry *
get_cep_by_cdid(int cdid)
{
	struct cfg_entry *cep;

	SIMPLEQ_FOREACH(cep, &cfg_entry_list, cfgq) {
		if (cep->cdid == cdid || cep->saved_call.cdid == cdid)
			return(cep);
	}
	return(NULL);
}

/*---------------------------------------------------------------------------*
 *	process AOCD charging messages
 *---------------------------------------------------------------------------*/
void
handle_charge(struct cfg_entry *cep)
{
	time_t now = time(NULL);

	if(cep->aoc_last == 0)		/* no last timestamp yet ? */
	{
		cep->aoc_last = now;	/* add time stamp */
	}
	else if(cep->aoc_now == 0)	/* no current timestamp yet ? */
	{
		cep->aoc_now = now;	/* current timestamp */
	}
	else
	{
		cep->aoc_last = cep->aoc_now;
		cep->aoc_now = now;
		cep->aoc_diff = cep->aoc_now - cep->aoc_last;
		cep->aoc_valid = AOC_VALID;
	}
	
#ifdef USE_CURSES
	if(do_fullscreen)
		display_charge(cep);
#endif

#ifdef I4B_EXTERNAL_MONITOR
	if(do_monitor && accepted)
		monitor_evnt_charge(cep, cep->charge, 0);
#endif

	if(cep->aoc_valid == AOC_VALID)
	{
		if(cep->aoc_diff != cep->unitlength)
		{
			DBGL(DL_MSG, (log(LL_DBG, "handle_charge: AOCD unit length updated %d -> %d secs", cep->unitlength, cep->aoc_diff)));

			cep->unitlength = cep->aoc_diff;

			unitlen_chkupd(cep);
		}
		else
		{
#ifdef NOTDEF
			DBGL(DL_MSG, (log(LL_DBG, "handle_charge: AOCD unit length still %d secs", cep->unitlength)));
#endif
		}
	}
}

/*---------------------------------------------------------------------------*
 *	update kernel idle_time, earlyhup_time and unitlen_time
 *---------------------------------------------------------------------------*/
void
unitlen_chkupd(struct cfg_entry *cep)
{
	msg_timeout_upd_t tupd;

	tupd.cdid = cep->cdid;

	/* init the short hold data based on the shorthold algorithm type */
	
	switch(cep->shorthold_algorithm)
	{
		case SHA_FIXU:
			tupd.shorthold_data.shorthold_algorithm = SHA_FIXU;
			tupd.shorthold_data.unitlen_time = cep->unitlength;
			tupd.shorthold_data.idle_time = cep->idle_time_out;
			tupd.shorthold_data.earlyhup_time = cep->earlyhangup;
			break;

		case SHA_VARU:
			tupd.shorthold_data.shorthold_algorithm = SHA_VARU;
			tupd.shorthold_data.unitlen_time = cep->unitlength;
			tupd.shorthold_data.idle_time = cep->idle_time_out;
			tupd.shorthold_data.earlyhup_time = 0;
			break;
		default:
			log(LL_ERR, "unitlen_chkupd bad shorthold_algorithm %d", cep->shorthold_algorithm );
			return;
			break;			
	}

	if((ioctl(isdnfd, I4B_TIMEOUT_UPD, &tupd)) < 0)
	{
		log(LL_ERR, "ioctl I4B_TIMEOUT_UPD failed: %s", strerror(errno));
		error_exit(1, "ioctl I4B_TIMEOUT_UPD failed: %s", strerror(errno));
	}
}

/*--------------------------------------------------------------------------*
 *	this is intended to be called by do_exit and closes down all
 *	active connections before the daemon exits or is reconfigured.
 *--------------------------------------------------------------------------*/
void
close_allactive(void)
{
	int j;
	struct cfg_entry *cep = NULL;
	struct isdn_ctrl_state *cst;

	j = 0;
	
	SLIST_FOREACH(cst, &isdn_ctrl_list, ctrlq) {

		if((get_controller_state(cst)) != CTRL_UP)
			continue;

		if((ret_channel_state(cst, CHAN_B1)) == CHAN_RUN)
		{
			if((cep = get_cep_by_cc(cst->bri, CHAN_B1)) != NULL)
			{
#ifdef USE_CURSES
				if(do_fullscreen)
					display_disconnect(cep);
#endif
#ifdef I4B_EXTERNAL_MONITOR
				monitor_evnt_disconnect(cep);
#endif
				next_state(cep, EV_DRQ);
				j++;
			}
		}

		if((ret_channel_state(cst, CHAN_B2)) == CHAN_RUN)
		{
			if((cep = get_cep_by_cc(cst->bri, CHAN_B2)) != NULL)
			{
#ifdef USE_CURSES
				if(do_fullscreen)
					display_disconnect(cep);
#endif
#ifdef I4B_EXTERNAL_MONITOR
				monitor_evnt_disconnect(cep);
#endif
				next_state(cep, EV_DRQ);
				j++;				
			}
		}
	}

	if(j)
	{
		log(LL_DMN, "close_allactive: waiting for all connections terminated");
		sleep(5);
	}

	SIMPLEQ_FOREACH(cep, &cfg_entry_list, cfgq) {
		if (cep->autoupdown & AUTOUPDOWN_DONE) {
			struct ifreq ifr;
			int r, s;

			s = socket(AF_INET, SOCK_DGRAM, 0);
			memset(&ifr, 0, sizeof ifr);
			snprintf(ifr.ifr_name, sizeof ifr.ifr_name, "%s%d", cep->usrdevicename, cep->usrdeviceunit);
			r = ioctl(s, SIOCGIFFLAGS, &ifr);
			if (r >= 0) {
				ifr.ifr_flags &= ~IFF_UP;
				ioctl(s, SIOCSIFFLAGS, &ifr);
			}
			close(s);
			cep->autoupdown &= ~AUTOUPDOWN_DONE;
		}
	}

}

/*--------------------------------------------------------------------------*
 *	set an interface up
 *--------------------------------------------------------------------------*/
void
if_up(struct cfg_entry *cep)
{
	msg_updown_ind_t mui;
			
	/* set interface up */
	
	DBGL(DL_MSG, (log(LL_DBG, "if_up: taking %s%d up", cep->usrdevicename, cep->usrdeviceunit)));
	
	mui.driver = cep->usrdevice;
	mui.driver_unit = cep->usrdeviceunit;
	mui.updown = SOFT_ENA;
			
	if((ioctl(isdnfd, I4B_UPDOWN_IND, &mui)) < 0)
	{
		log(LL_ERR, "if_up: ioctl I4B_UPDOWN_IND failed: %s", strerror(errno));
		error_exit(1, "if_up: ioctl I4B_UPDOWN_IND failed: %s", strerror(errno));
	}
	cep->down_retry_count = 0;

#ifdef USE_CURSES
	if(do_fullscreen)
		display_updown(cep, 1);
#endif
#ifdef I4B_EXTERNAL_MONITOR
	monitor_evnt_updown(cep, 1);
#endif
	
}

/*--------------------------------------------------------------------------*
 *	set an interface down
 *--------------------------------------------------------------------------*/
void
if_down(struct cfg_entry *cep)
{
	msg_updown_ind_t mui;
			
	/* set interface up */
	
	DBGL(DL_MSG, (log(LL_DBG, "if_down: taking %s%d down", cep->usrdevicename, cep->usrdeviceunit)));
	
	mui.driver = cep->usrdevice;
	mui.driver_unit = cep->usrdeviceunit;
	mui.updown = SOFT_DIS;
			
	if((ioctl(isdnfd, I4B_UPDOWN_IND, &mui)) < 0)
	{
		log(LL_ERR, "if_down: ioctl I4B_UPDOWN_IND failed: %s", strerror(errno));
		error_exit(1, "if_down: ioctl I4B_UPDOWN_IND failed: %s", strerror(errno));
	}
	cep->went_down_time = time(NULL);
	cep->down_retry_count = 0;

#ifdef USE_CURSES
	if(do_fullscreen)
		display_updown(cep, 0);
#endif
#ifdef I4B_EXTERNAL_MONITOR
	monitor_evnt_updown(cep, 0);
#endif

}

/*--------------------------------------------------------------------------*
 *	send a dial response to (an interface in) the kernel 
 *--------------------------------------------------------------------------*/
void
dialresponse(struct cfg_entry *cep, int dstat)
{
	msg_dialout_resp_t mdr;

	static char *stattab[] = {
		"normal condition",
		"temporary failure",
		"permanent failure",
		"dialout not allowed"
	};

	if(dstat < DSTAT_NONE || dstat > DSTAT_INONLY)
	{
		log(LL_ERR, "dialresponse: dstat out of range %d!", dstat);
		return;
	}
	
	mdr.driver = cep->usrdevice;
	mdr.driver_unit = cep->usrdeviceunit;
	mdr.stat = dstat;
	mdr.cause = cep->disc_cause;	
	
	if((ioctl(isdnfd, I4B_DIALOUT_RESP, &mdr)) < 0)
	{
		log(LL_ERR, "dialresponse: ioctl I4B_DIALOUT_RESP failed: %s", strerror(errno));
		error_exit(1, "dialresponse: ioctl I4B_DIALOUT_RESP failed: %s", strerror(errno));
	}

	DBGL(DL_DRVR, (log(LL_DBG, "dialresponse: sent [%s]", stattab[dstat])));
}

/*--------------------------------------------------------------------------*
 *	screening/presentation indicator
 *--------------------------------------------------------------------------*/
void
handle_scrprs(int cdid, int scr, int prs, char *caller)
{
	/* screening indicator */
	
	if(scr < SCR_NONE || scr > SCR_NET)
	{
		log(LL_ERR, "msg_connect_ind: invalid screening indicator value %d!", scr);
	}
	else
	{
		static char *scrtab[] = {
			"no screening indicator",
			"sreening user provided, not screened",
			"screening user provided, verified & passed",
			"screening user provided, verified & failed",
			"screening network provided", };

		if(extcallattr)
		{
			log(LL_CHD, "%05d %s %s", cdid, caller, scrtab[scr]);
		}
		else
		{
			DBGL(DL_MSG, (log(LL_DBG, "%s - %s", caller, scrtab[scr])));
		}
	}
			
	/* presentation indicator */
	
	if(prs < PRS_NONE || prs > PRS_RESERVED)
	{
		log(LL_ERR, "msg_connect_ind: invalid presentation indicator value %d!", prs);
	}
	else
	{
		static char *prstab[] = {
			"no presentation indicator",
			"presentation allowed",
			"presentation restricted",
			"number not available due to interworking",
			"reserved presentation value" };
			
		if(extcallattr)
		{
			log(LL_CHD, "%05d %s %s", cdid, caller, prstab[prs]);
		}
		else
		{
			DBGL(DL_MSG, (log(LL_DBG, "%s - %s", caller, prstab[prs])));
		}
	}
}

/*--------------------------------------------------------------------------*
 *	check if the time is valid for an entry
 *--------------------------------------------------------------------------*/
static int 
isvalidtime(struct cfg_entry *cep)
{
	time_t t;
	struct tm *tp;

	if(cep->day == 0)
		return(1);

	t = time(NULL);
	tp = localtime(&t);

	if(cep->day & HD)
	{
		if(isholiday(tp->tm_mday, (tp->tm_mon)+1, (tp->tm_year)+1900))
		{
			DBGL(DL_MSG, (log(LL_DBG, "isvalidtime: holiday %d.%d.%d", tp->tm_mday, (tp->tm_mon)+1, (tp->tm_year)+1900)));
			goto dayok;
		}
	}
	
	if(cep->day & (1 << tp->tm_wday))
	{
		DBGL(DL_MSG, (log(LL_DBG, "isvalidtime: day match")));	
		goto dayok;
	}

	return(0);
	
dayok:
	if(cep->fromhr==0 && cep->frommin==0 && cep->tohr==0 && cep->tomin==0)
	{
		DBGL(DL_MSG, (log(LL_DBG, "isvalidtime: no time specified, match!")));
		return(1);
	}

	if(cep->tohr < cep->fromhr)
	{
		/* before 00:00 */
		
		if( (tp->tm_hour > cep->fromhr) ||
		    (tp->tm_hour == cep->fromhr && tp->tm_min > cep->frommin) )
		{
			DBGL(DL_MSG, (log(LL_DBG, "isvalidtime: t<f-1, spec=%02d:%02d-%02d:%02d, curr=%02d:%02d, match!",
				cep->fromhr, cep->frommin,
				cep->tohr, cep->tomin,
				tp->tm_hour, tp->tm_min)));
			
			return(1);
		}

		/* after 00:00 */
		
		if( (tp->tm_hour < cep->tohr) ||
		    (tp->tm_hour == cep->tohr && tp->tm_min < cep->tomin) )
		{
			DBGL(DL_MSG, (log(LL_DBG, "isvalidtime: t<f-2, spec=%02d:%02d-%02d:%02d, curr=%02d:%02d, match!",
				cep->fromhr, cep->frommin,
				cep->tohr, cep->tomin,
				tp->tm_hour, tp->tm_min)));
			
			return(1);
		}
	}
	else if(cep->fromhr == cep->tohr)
	{
		if(tp->tm_min >= cep->frommin && tp->tm_min < cep->tomin)
		{
			DBGL(DL_MSG, (log(LL_DBG, "isvalidtime: f=t, spec=%02d:%02d-%02d:%02d, curr=%02d:%02d, match!",
				cep->fromhr, cep->frommin,
				cep->tohr, cep->tomin,
				tp->tm_hour, tp->tm_min)));
			
			return(1);
		}
	}
	else
	{
		if((tp->tm_hour > cep->fromhr && tp->tm_hour < cep->tohr) ||
		   (tp->tm_hour == cep->fromhr && tp->tm_min >= cep->frommin) ||
		   (tp->tm_hour == cep->tohr && tp->tm_min < cep->tomin) )
		{
			DBGL(DL_MSG, (log(LL_DBG, "isvalidtime: t>f, spec=%02d:%02d-%02d:%02d, curr=%02d:%02d, match!",
				cep->fromhr, cep->frommin,
				cep->tohr, cep->tomin,
				tp->tm_hour, tp->tm_min)));
			return(1);
		}
	}
	DBGL(DL_MSG, (log(LL_DBG, "isvalidtime: spec=%02d:%02d-%02d:%02d, curr=%02d:%02d, no match!",
			cep->fromhr, cep->frommin,
			cep->tohr, cep->tomin,
			tp->tm_hour, tp->tm_min)));

	return(0);	
}

struct cfg_entry *
get_first_cfg_entry()
{
	return (SIMPLEQ_FIRST(&cfg_entry_list));
}

int count_cfg_entries()
{
	int cnt;
	struct cfg_entry *cfe;

	cnt = 0;
	SIMPLEQ_FOREACH(cfe, &cfg_entry_list, cfgq)
		cnt++;

	return (cnt);
}

struct cfg_entry *
find_cfg_entry(int index)
{
	struct cfg_entry *cep;

	SIMPLEQ_FOREACH(cep, &cfg_entry_list, cfgq)
		if (cep->index == index)
			return cep;
	return NULL;
}

int
add_cfg_entry(struct cfg_entry *cfe)
{
	struct cfg_entry *cep;
	int max = -1;

	SIMPLEQ_FOREACH(cep, &cfg_entry_list, cfgq)
		if (cep->index > max)
			max = cep->index;

	cfe->index = max;
	SIMPLEQ_INSERT_TAIL(&cfg_entry_list, cfe, cfgq);
	return max;
}

void
remove_all_cfg_entries()
{
	struct cfg_entry *cep;

	while (!SIMPLEQ_EMPTY(&cfg_entry_list)) {
		cep = SIMPLEQ_FIRST(&cfg_entry_list);
		SIMPLEQ_REMOVE_HEAD(&cfg_entry_list, cep, cfgq);

		if (cep->ppp_expect_name)
		    free(cep->ppp_expect_name);
		if (cep->ppp_expect_password)
		    free(cep->ppp_expect_password);
		if (cep->ppp_send_name)
		    free(cep->ppp_send_name);
		if (cep->ppp_send_password)
		    free(cep->ppp_send_password);
		
		free(cep);
	}
}

struct isdn_ctrl_state * get_first_ctrl_state()
{
	return SLIST_FIRST(&isdn_ctrl_list);
}

int count_ctrl_states()
{
	int cnt = 0;
	struct isdn_ctrl_state *ctrl;

	SLIST_FOREACH(ctrl, &isdn_ctrl_list, ctrlq)
		cnt++;

	return (cnt);
}

void remove_all_ctrl_state()
{
	struct isdn_ctrl_state *ctrl;

	while (!SLIST_EMPTY(&isdn_ctrl_list)) {
		ctrl = SLIST_FIRST(&isdn_ctrl_list);
		SLIST_REMOVE_HEAD(&isdn_ctrl_list, ctrlq);
		free(ctrl);
	}
}

struct isdn_ctrl_state *
find_ctrl_state(int controller)
{
	struct isdn_ctrl_state *ctrl;

	SLIST_FOREACH(ctrl, &isdn_ctrl_list, ctrlq)
		if (ctrl->bri == controller)
			return ctrl;
	return NULL;
}

int
add_ctrl_state(struct isdn_ctrl_state *cstate)
{
	SLIST_INSERT_HEAD(&isdn_ctrl_list, cstate, ctrlq);
	return 0;
}

int
remove_ctrl_state(int controller)
{
	struct isdn_ctrl_state *ctrl = find_ctrl_state(controller);
	struct cfg_entry *cep;

	if (ctrl == NULL)
		return 0;

	if ((get_controller_state(ctrl)) == CTRL_UP) {

		if((ret_channel_state(ctrl, CHAN_B1)) == CHAN_RUN)
		{
			if((cep = get_cep_by_cc(controller, CHAN_B1)) != NULL)
			{
#ifdef USE_CURSES
				if(do_fullscreen)
					display_disconnect(cep);
#endif
#ifdef I4B_EXTERNAL_MONITOR
				monitor_evnt_disconnect(cep);
#endif
				cep->cdid = -1;
				cep->isdncontrollerused = -1;
				cep->isdnchannelused = -1;
				cep->state = ST_IDLE;
			}
		}

		if ((ret_channel_state(ctrl, CHAN_B2)) == CHAN_RUN)
		{
			if((cep = get_cep_by_cc(controller, CHAN_B2)) != NULL)
			{
#ifdef USE_CURSES
				if(do_fullscreen)
					display_disconnect(cep);
#endif
#ifdef I4B_EXTERNAL_MONITOR
				monitor_evnt_disconnect(cep);
#endif
				cep->cdid = -1;
				cep->isdncontrollerused = -1;
				cep->isdnchannelused = -1;
				cep->state = ST_IDLE;
			}
		}
	}

	SLIST_REMOVE(&isdn_ctrl_list, ctrl, isdn_ctrl_state, ctrlq);
	return 0;
}

/* EOF */
