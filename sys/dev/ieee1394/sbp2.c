/*	$NetBSD: sbp2.c,v 1.14 2003/02/28 23:31:41 enami Exp $	*/

/*
 * Copyright (c) 2001,2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by James Chacon.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sbp2.c,v 1.14 2003/02/28 23:31:41 enami Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/lock.h>

#include <machine/bus.h>
#include <machine/limits.h>

#include <dev/std/ieee1212reg.h>
#include <dev/std/ieee1212var.h>
#include <dev/ieee1394/ieee1394reg.h>
#include <dev/ieee1394/ieee1394var.h>
#include <dev/ieee1394/sbp2reg.h>
#include <dev/ieee1394/sbp2var.h>
#include <dev/ieee1394/sbpscsireg.h>

static void sbp2_print_data(struct p1212_data *);
static void sbp2_print_dir(struct p1212_dir *);
static void sbp2_login(struct sbp2 *, struct sbp2_lun *);
static void sbp2_login_ans(struct ieee1394_abuf *, int);
static void sbp2_login_resp(struct ieee1394_abuf *, int);
static struct sbp2_lun *sbp2_alloc_lun(u_int32_t, u_int32_t, u_int32_t,
    u_int32_t, u_int32_t);
static void sbp2_logout(struct sbp2 *, struct sbp2_lun *);
static void sbp2_relogin(struct ieee1394_softc *, void *);
static struct sbp2_orb *sbp2_alloc_orb(void);
static void sbp2_free_orb(struct sbp2_orb *);
static void sbp2_alloc_data_mapping(struct sbp2 *, struct sbp2_mapping *,
    u_char *, u_int32_t, u_int8_t);
static void sbp2_free_data_mapping(struct sbp2 *, struct sbp2_mapping *);
static u_int64_t sbp2_alloc_addr(struct sbp2 *);
static void sbp2_free_addr(struct sbp2 *, u_int64_t);
static void sbp2_orb_resp(struct ieee1394_abuf *, int);
static void sbp2_data_resp(struct ieee1394_abuf *, int);
static void sbp2_enable_status(struct ieee1394_abuf *, int);
static void sbp2_null_resp(struct ieee1394_abuf *, int);
static void sbp2_doorbell_reset(struct ieee1394_abuf *, int);
static void sbp2_status_resp(struct sbp2_status *, void *);
static struct sbp2_pagetable *sbp2_alloc_pt(struct uio *, u_int8_t, 
    struct sbp2_orb *);
static void sbp2_pt_resp(struct ieee1394_abuf *, int);
static void sbp2_free_pt(struct sbp2_pagetable *);

#ifdef SBP2_DEBUG
static void sbp2_agent_status(struct ieee1394_abuf *, int);
#endif

static CIRCLEQ_HEAD(, sbp2_orb) sbp2_freeorbs =
    CIRCLEQ_HEAD_INITIALIZER(sbp2_freeorbs);
static struct simplelock sbp2_freeorbs_lock = SIMPLELOCK_INITIALIZER;
static struct sbp2_orb status_orb;
static TAILQ_HEAD(, sbp2_map) sbp2_maps =
    TAILQ_HEAD_INITIALIZER(sbp2_maps);
static struct simplelock sbp2_maps_lock = SIMPLELOCK_INITIALIZER;

#ifdef SBP2_DEBUG
#define DPRINTF(x)      if (sbp2debug) printf x
#define DPRINTFN(n,x)   if (sbp2debug>(n)) printf x
int     sbp2debug = 3;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

static void
sbp2_print_data(struct p1212_data *data)
{
	switch (data->com.key.key_value) {
	case SBP2_KEYVALUE_Command_Set:
		printf("SBP2 Command Set: ");
		if (data->com.key.val == SBPSCSI_COMMAND_SET)
			printf("SCSI 2\n");
		else
			printf("0x%08x\n", data->val);
		break;
	case SBP2_KEYVALUE_Unit_Characteristics:
		printf("SBP2 Unit Characteristics: 0x%08x\n",data->com.key.val);
		break;
	case SBP2_KEYVALUE_Command_Set_Revision:
		printf("SBP2 Command Set Revision: 0x%08x\n",data->com.key.val);
		break;
	case SBP2_KEYVALUE_Command_Set_Spec_Id:
		printf("SBP2 Command Set Spec Id: 0x%08x\n", data->com.key.val);
		break;
	case SBP2_KEYVALUE_Firmware_Revision:
		printf("SBP2 Firmware Revision: 0x%08x\n", data->com.key.val);
		break;
	case SBP2_KEYVALUE_Reconnect_Timeout:
		printf("SBP2 Reconnect Timeout: 0x%08x\n", data->com.key.val);
		break;
	case SBP2_KEYVALUE_Unit_Unique_Id:
		printf("SBP2 Unit Unique Id: 0x%08x\n", data->com.key.val);
		break;
	case P1212_KEYVALUE_Unit_Dependent_Info:
		if (data->com.key.key_type == P1212_KEYTYPE_Immediate) 
			printf("SBP2 Logical Unit Number: 0x%08x\n",
			    data->com.key.val);
		else if (data->com.key.key_type == P1212_KEYTYPE_Offset) 
			printf("SBP2 Management Agent: 0x%08x\n",
			    data->com.key.val);
		break;
	default:
		printf("Unknown SBP2 key and value: 0x%08x 0x%08x\n",
		    data->com.key.key_value, data->com.key.val);
		break;
	}
	return;
}

static void
sbp2_print_dir(struct p1212_dir *dir)
{
	switch(dir->com.key.key_type) {
	case SBP2_KEYVALUE_Logical_Unit_Directory:
		printf("Logical Unit\n");
		break;
	default:
		printf("Unknown SBP2 key and value: 0x%08x 0x%08x\n",
		    dir->com.key.key_value, dir->com.key.val);
		break;
	}
	return;
}

#define VALID_SBP2_IMM(data) { \
if (data->com.key.key_type != P1212_KEYTYPE_Immediate) { \
	DPRINTF(("Unknown SBP2 key and value: 0x%08x 0x%08x\n", \
		    data->com.key.key_value, data->com.key.val)); \
	break; \
} \
} 

struct sbp2 *
sbp2_init(struct ieee1394_softc *sc, struct p1212_dir *udir)
{
	struct sbp2 *sbp2;
	struct sbp2_map *sbp2_map;
	struct p1212_data *data;
	struct p1212_dir *dir;
	struct sbp2_lun *lun;
	u_int32_t *luns;
	int32_t cmd_spec_id, cmd_set, cmd_set_rev;
	int i, luncnt, found;
	
	sbp2 = malloc(sizeof (struct sbp2), M_1394DATA, M_WAITOK|M_ZERO);
	luns = NULL;
	luncnt = 0;
	
	sbp2->sc = sc;
	sc->sc1394_callback.sc1394_reset = sbp2_relogin;
	sc->sc1394_callback.sc1394_resetarg = sbp2;
	simple_lock_init(&sbp2->orblist_lock);
	
	found = 0;
	simple_lock(&sbp2_maps_lock);
	if (!TAILQ_EMPTY(&sbp2_maps)) {
		TAILQ_FOREACH(sbp2_map, &sbp2_maps, map_list) {
			if (sbp2_map->sc_bus ==
			    (struct ieee1394_softc *)sc->sc1394_dev.dv_parent) {
				sbp2_map->refcnt++;
				found = 1;
				sbp2->map = sbp2_map;
				break;
			}
		}
	}
	simple_unlock(&sbp2_maps_lock);
	if (!found) {
		sbp2_map = malloc(sizeof (struct sbp2_map), M_1394DATA,
		    M_WAITOK|M_ZERO);
		simple_lock_init(&sbp2_map->maplock);
		simple_lock(&sbp2_map->maplock);
		simple_lock(&sbp2_maps_lock);
		TAILQ_INSERT_TAIL(&sbp2_maps, sbp2_map, map_list);
		simple_unlock(&sbp2_maps_lock);
		sbp2_map->sc_bus =
		    (struct ieee1394_softc *)sc->sc1394_dev.dv_parent;
		sbp2_map->refcnt = 1;
		sbp2->map = sbp2_map;
		simple_unlock(&sbp2_map->maplock);
		
		status_orb.cmd.ab_addr = sbp2_alloc_addr(sbp2);
		status_orb.cmd.ab_length = SBP2_STATUS_SIZE;
		status_orb.cmd.ab_data = malloc(SBP2_STATUS_SIZE, M_1394DATA,
		    M_WAITOK|M_ZERO);
		status_orb.cmd.ab_req = sc;
		status_orb.cmd.ab_cb = sbp2_orb_resp;
		status_orb.cmd.ab_cbarg = &status_orb;
		status_orb.cmd.ab_tcode = IEEE1394_TCODE_WRITE_REQ_BLOCK;
		status_orb.state = SBP2_ORB_STATUS_STATE;
		status_orb.sbp2 = sbp2;
		sc->sc1394_callback.sc1394_inreg(&status_orb.cmd, 0);
	} 
	
	TAILQ_INIT(&sbp2->luns);
	CIRCLEQ_INIT(&sbp2->orbs);

	TAILQ_FOREACH(data, &udir->data_root, data) {
		switch (data->com.key.key_value) {
		case SBP2_KEYVALUE_Command_Set_Spec_Id:
			VALID_SBP2_IMM(data);
			sbp2->cmd_spec_id = data->com.key.val;
			data->print = sbp2_print_data;
			break;
		case SBP2_KEYVALUE_Command_Set:
			VALID_SBP2_IMM(data);
			sbp2->cmd_set = data->com.key.val;
			data->print = sbp2_print_data;
			break;
		case SBP2_KEYVALUE_Unit_Characteristics:
			VALID_SBP2_IMM(data);
			sbp2->orb_size = (data->com.key.val & 0xff);
			sbp2->orb_timeout = ((data->com.key.val >> 8) & 0xff);
			data->print = sbp2_print_data;
			break;
		case SBP2_KEYVALUE_Command_Set_Revision:
			VALID_SBP2_IMM(data);
			sbp2->cmd_set_rev = data->com.key.val;
			data->print = sbp2_print_data;
			break;
		case SBP2_KEYVALUE_Firmware_Revision:
			VALID_SBP2_IMM(data);
			sbp2->firmware_rev = data->com.key.val;
			data->print = sbp2_print_data;
			break;
		case SBP2_KEYVALUE_Reconnect_Timeout:
			VALID_SBP2_IMM(data);
			sbp2->max_reconnect = (data->com.key.val & 0xffff);
			data->print = sbp2_print_data;
			break;
		case SBP2_KEYVALUE_Logical_Unit_Number:
			/*
			 * This is either lun or management reg offset 
			 * depending on type. 
			 */

			if (data->com.key.key_type == P1212_KEYTYPE_Immediate) {

				/* Save until done with data entries. */
				luncnt++;
				luns = realloc(luns, sizeof(u_int32_t) * luncnt,
				    M_1394DATA, M_WAITOK);
				luns[luncnt - 1] = data->com.key.val;
				data->print = sbp2_print_data;
			}
			if (data->com.key.key_type == P1212_KEYTYPE_Offset) {
				if (data->com.key.val < SBP2_MIN_MGMT_OFFSET) {
					DPRINTF(("Management reg offset too "
					    "small: 0x%08x\n", 
					    data->com.key.val));
					break;
				}
				sbp2->mgmtreg = (data->com.key.val * 4) + 
				    CSR_BASE;
				data->print = sbp2_print_data;
			}
			break;
		case SBP2_KEYVALUE_Unit_Unique_Id:
			if (data->com.key.key_type != P1212_KEYTYPE_Leaf) {
				DPRINTF(("Unknown SBP2 key and value: "
					    "0x%08x 0x%08x\n", \
					    data->com.key.key_value,
					    data->com.key.val)); \
				break;
			}
#ifdef DIAGNOSTIC
			if (data->leafdata == NULL) {
				printf ("sbp2: No leaf data?\n");
				break;
			}
			if (data->leafdata->len != 2) {
				printf ("sbp2: Unique ID must be length 2\n");
				break;
			}
#endif
			memcpy (&sbp2->uniq_id, &data->leafdata->data[0], 4);
			memcpy (&sbp2->uniq_id+4, &data->leafdata->data[1], 4);
			data->print = sbp2_print_data;
			break;
		default:
			break;
		}
	}

	for (i = 0; i < luncnt; i++) {
		lun = sbp2_alloc_lun(luns[i], sbp2->orb_size,
		    sbp2->cmd_spec_id, sbp2->cmd_set, sbp2->cmd_set_rev);
		sbp2->luncnt++;
		TAILQ_INSERT_TAIL(&sbp2->luns, lun, lun_list);
	}
	free(luns, M_1394DATA);
	
	/*
	 * A little complicated since entries can come in any order in a ROM
	 * directory.
	 *
	 * For each logical unit directory, scan it for overriding command set
	 * values. Then go back and for each logical unit found setup a lun
	 * with the appropriate default or overriden cmd_set values.
	 */

	TAILQ_FOREACH(dir, &udir->subdir_root, dir) {
		if (dir->com.key.key_value ==
		    SBP2_KEYVALUE_Logical_Unit_Directory) {
			dir->print = sbp2_print_dir;
			cmd_spec_id = -1;
			cmd_set = -1;
			cmd_set_rev = -1;
			TAILQ_FOREACH(data, &dir->data_root, data) {
				switch(data->com.key.key_value) {
				case SBP2_KEYVALUE_Command_Set_Spec_Id:
					cmd_spec_id = data->com.key.val;
					data->print = sbp2_print_data;
					break;
				case SBP2_KEYVALUE_Command_Set:
					cmd_set = data->com.key.val;
					data->print = sbp2_print_data;
					break;
				case SBP2_KEYVALUE_Command_Set_Revision:
					cmd_set_rev = data->com.key.val;
					data->print = sbp2_print_data;
					break;
				default:
					break;
				}
			}
			if (cmd_spec_id == -1)
				cmd_spec_id = sbp2->cmd_spec_id;
			if (cmd_set == -1)
				cmd_set = sbp2->cmd_set;
			if (cmd_set_rev == -1)
				cmd_set_rev = sbp2->cmd_set_rev;
			
			TAILQ_FOREACH(data, &dir->data_root, data) {
				if (data->com.key.key_value ==
				    SBP2_KEYVALUE_Logical_Unit_Number) {
					lun = sbp2_alloc_lun(data->com.key.val,
					    sbp2->orb_size, cmd_spec_id,
					    cmd_set, cmd_set_rev);
					TAILQ_INSERT_TAIL(&sbp2->luns, lun,
					    lun_list);
					sbp2->luncnt++;
					data->print = sbp2_print_data;
				}
			}
		}
	}
	return sbp2;
}

#undef VALID_SBP2_IMM

static struct sbp2_lun *
sbp2_alloc_lun(u_int32_t luninfo, u_int32_t orb_size, u_int32_t cmd_spec_id,
    u_int32_t cmd_set, u_int32_t cmd_set_rev)
{
	struct sbp2_lun *lun;

	lun = malloc(sizeof(struct sbp2_lun), M_1394DATA, M_WAITOK|M_ZERO);

	lun->cmd_spec_id = cmd_spec_id;
	lun->cmd_set = cmd_set;
	lun->cmd_set_rev = cmd_set_rev;

	lun->lun = luninfo & 0xffff;
	lun->ordered = ((luninfo >> 22) & 0x1);
	lun->dev_type = ((luninfo >> 16) & 0x1f);
	lun->command.ab_data = malloc(orb_size, M_1394DATA, M_WAITOK|M_ZERO);
	lun->doorbell.ab_data = malloc(orb_size, M_1394DATA, M_WAITOK|M_ZERO);
	lun->status.ab_data = malloc(orb_size, M_1394DATA, M_WAITOK|M_ZERO);
	return lun;
}

static void
sbp2_login(struct sbp2 *sbp2, struct sbp2_lun *lun)
{
	struct sbp2_orb *orb;
	u_int64_t respaddr, orbaddr;

	orb = sbp2_alloc_orb();
	
	lun->command.ab_length = 8;
	lun->command.ab_req = sbp2->sc;
	lun->command.ab_cb = sbp2_null_resp;
	lun->command.ab_cbarg = sbp2;
	
	orbaddr = sbp2_alloc_addr(sbp2);
	lun->command.ab_addr = sbp2->mgmtreg;
	lun->command.ab_data[0] = IEEE1394_CREATE_ADDR_HIGH(orbaddr);
	lun->command.ab_data[1] = IEEE1394_CREATE_ADDR_LOW(orbaddr);
	lun->command.ab_tcode = IEEE1394_TCODE_WRITE_REQ_BLOCK;
	
	respaddr = sbp2_alloc_addr(sbp2);
	orb->cmd.ab_req = sbp2->sc;
	orb->cmd.ab_length = SBP2_LOGIN_SIZE;
	orb->cmd.ab_tcode = IEEE1394_TCODE_READ_REQ_BLOCK;
	orb->cmd.ab_data[0] = 0;
	orb->cmd.ab_data[1] = 0;
	orb->cmd.ab_data[2] = IEEE1394_CREATE_ADDR_HIGH(respaddr);
	orb->cmd.ab_data[3] = IEEE1394_CREATE_ADDR_LOW(respaddr);
	orb->cmd.ab_data[4] |= SBP2_LOGIN_SET_EXCLUSIVE;
	orb->cmd.ab_data[4] |= SBP2_ORB_NOTIFY_MASK;

	/* Ask for reasonable reconnect time. */
	orb->cmd.ab_data[4] |= SBP2_LOGIN_SET_RECONNECT(SBP2_RECONNECT);
	orb->cmd.ab_data[4] |= lun->loginid;
	orb->cmd.ab_data[4] = htonl(orb->cmd.ab_data[4]);

	/* Allow max login response. */
	orb->cmd.ab_data[5] = htonl(SBP2_LOGIN_MAX_RESP);
	orb->cmd.ab_data[6] = IEEE1394_CREATE_ADDR_HIGH(status_orb.cmd.ab_addr);
	orb->cmd.ab_data[7] = IEEE1394_CREATE_ADDR_LOW(status_orb.cmd.ab_addr);
	
	orb->cmd.ab_addr = orbaddr;
	orb->cmd.ab_cb = sbp2_login_ans;
	orb->cmd.ab_cbarg = orb;
	orb->sbp2 = sbp2;
	
	lun->doorbell.ab_addr = respaddr;
	lun->doorbell.ab_length = SBP2_LOGIN_MAX_RESP;
	lun->doorbell.ab_data = malloc (SBP2_LOGIN_MAX_RESP, M_1394DATA,
	    M_WAITOK|M_ZERO);
	lun->doorbell.ab_req = sbp2->sc;
	lun->doorbell.ab_cb = sbp2_login_resp;
	lun->doorbell.ab_cbarg = lun;
	lun->doorbell.ab_tcode = IEEE1394_TCODE_WRITE_REQ_BLOCK;
	
	sbp2->sc->sc1394_callback.sc1394_inreg(&orb->cmd, 0);
	sbp2->sc->sc1394_callback.sc1394_inreg(&lun->doorbell, 0);
	sbp2->sc->sc1394_callback.sc1394_write(&lun->command);

	
	simple_lock(&sbp2->orblist_lock);
	orb = sbp2_alloc_orb();
	orb->cmd.ab_addr = sbp2_alloc_addr(sbp2);
	orb->cmd.ab_req = sbp2->sc;
	orb->cmd.ab_tcode = IEEE1394_TCODE_READ_REQ_BLOCK;
	orb->cmd.ab_length = (sbp2->orb_size * 4);
	orb->cmd.ab_data[0] = htonl(SBP2_ORB_NULL_POINTER);
	orb->cmd.ab_data[4] = htonl(SBP2_ORB_FMT_DUMMY_MASK);

	/*
	 * Go ahead and set the notify bit. Some SBP2 devices always
	 * seem to return status on a dummy orb regardless so don't 2nd guess.
	 */

	orb->cmd.ab_data[4] |= htonl(SBP2_ORB_NOTIFY_MASK);
	orb->cmd.ab_subok = 1;
	orb->cmd.ab_cb = sbp2_orb_resp;
	orb->cmd.ab_cbarg = orb;
	orb->sbp2 = sbp2;

	orb->cb = sbp2_status_resp;
	CIRCLEQ_INSERT_HEAD(&sbp2->orbs, orb, orb_list);
	simple_unlock(&sbp2->orblist_lock);
	sbp2->sc->sc1394_callback.sc1394_inreg(&orb->cmd, 0);

	return;
}

static void
sbp2_login_ans(struct ieee1394_abuf *ab, int rcode)
{
	struct ieee1394_softc *sc = ab->ab_req;
	struct sbp2_orb *orb = ab->ab_cbarg;
	
	/* Got a read so allocate the buffer and write out the response. */
	
	if (orb->state != SBP2_ORB_INIT_STATE) {
		orb->cmd.ab_tcode = IEEE1394_TCODE_READ_REQ_BLOCK;
		sbp2_free_orb(orb);
		return;
	}
	
	if (rcode) {
		DPRINTF(("sbp2_login: Bad return code: %d\n", rcode));
		orb->cmd.ab_tcode = IEEE1394_TCODE_READ_REQ_BLOCK;
		sbp2_free_orb(orb);
		return;
	}
	
	orb->cmd.ab_tcode = IEEE1394_TCODE_READ_RESP_BLOCK;
	orb->state = SBP2_ORB_SENT_STATE;
	sc->sc1394_callback.sc1394_write(&orb->cmd);
	return;
}

static void
sbp2_login_resp(struct ieee1394_abuf *ab, int rcode)
{
	struct ieee1394_softc *sc = ab->ab_req;
	struct sbp2_lun *lun = ab->ab_cbarg;
	struct sbp2_orb *orb;
	struct sbp2 *sbp2 = lun->command.ab_cbarg;

#ifdef SBP2_DEBUG
	int i;
#endif
	
	sc->sc1394_callback.sc1394_unreg(&lun->doorbell, 1);

	if (rcode) {
		DPRINTF(("Bad return code: %d\n", rcode));
		return;
	}
	
	DPRINTF(("csr: 0x%016qx\n", (quad_t)ab->ab_addr));
#ifdef SBP2_DEBUG
	if (sbp2debug > 2) 
		for (i = 0; i < (ab->ab_retlen / 4); i++) 
			DPRINTF(("%d: 0x%08x\n", i, ntohl(ab->ab_data[i])));
#endif
	
	lun->cmdreg = SBP2_LOGINRESP_CREATE_CMDREG(ntohl(ab->ab_data[1]),
	    ntohl(ab->ab_data[2]));
	if (SBP2_LOGINRESP_GET_LENGTH(ntohl(ab->ab_data[0])) ==
	    SBP2_LOGIN_MAX_RESP)
		lun->reconnect =
		    SBP2_LOGINRESP_GET_RECONNECT(ntohl(ab->ab_data[3]));
	lun->loginid = SBP2_LOGINRESP_GET_LOGINID(ntohl(ab->ab_data[0]));
	lun->login_flag = SBP2_LOGGED_IN;

	lun->doorbell.ab_addr = lun->cmdreg + SBP2_CMDREG_AGENT_RESET;
	lun->doorbell.ab_length = 4;
	lun->doorbell.ab_req = sc;
	lun->doorbell.ab_cb = sbp2_enable_status;
	lun->doorbell.ab_cbarg = lun;
	lun->doorbell.ab_tcode = IEEE1394_TCODE_WRITE_REQ_QUAD;
	lun->doorbell.ab_data[0] = 0xffffffff;

	/*
	 * Reset the state engine, plug the address of the first orb into
	 * the orb pointer and off we go.
	 */

	lun->command.ab_addr = lun->cmdreg + SBP2_CMDREG_ORB_POINTER;
	lun->command.ab_length = 8;
	lun->command.ab_req = sc;
	lun->command.ab_cb = sbp2_doorbell_reset;
	lun->command.ab_cbarg = lun;
	lun->command.ab_tcode = IEEE1394_TCODE_WRITE_REQ_BLOCK;
	simple_lock(&sbp2->orblist_lock);
	orb = CIRCLEQ_LAST(&sbp2->orbs);
	simple_lock(&orb->orb_lock);
	lun->command.ab_data[0] = IEEE1394_CREATE_ADDR_HIGH(orb->cmd.ab_addr);
	lun->command.ab_data[1] = IEEE1394_CREATE_ADDR_LOW(orb->cmd.ab_addr);
	simple_unlock(&orb->orb_lock);
	simple_unlock(&sbp2->orblist_lock);
	sc->sc1394_callback.sc1394_write(&lun->doorbell);
	sc->sc1394_callback.sc1394_write(&lun->command);
	lun->state = SBP2_STATE_ACTIVE;

	return;
}

static void
sbp2_enable_status(struct ieee1394_abuf *ab, int rcode)
{
	struct sbp2_lun *lun = ab->ab_cbarg;

	lun->doorbell.ab_cb = sbp2_doorbell_reset;
	lun->doorbell.ab_addr = lun->cmdreg +
	    SBP2_CMDREG_UNSOLICITED_STATUS_ENABLE;
	ab->ab_req->sc1394_callback.sc1394_write(&lun->doorbell);
	return;
}

static void
sbp2_doorbell_reset(struct ieee1394_abuf *ab, int rcode)
{
	struct sbp2_lun *lun = ab->ab_cbarg;

	if (lun->cmdreg)
		lun->doorbell.ab_addr = lun->cmdreg + SBP2_CMDREG_DOORBELL;
	return;
}

static void
sbp2_null_resp(struct ieee1394_abuf *ab, int rcode)
{
	return;
}

static void
sbp2_status_resp(struct sbp2_status *status, void *arg)
{
	DPRINTFN(1, ("Got a status response in sbp2_status_resp\n"));
	DPRINTFN(1, ("status: resp 0x%04x, sbp_status 0x%04x\n", status->resp,
		     status->sbp_status));
	
}

int
sbp2_match(struct p1212_dir *udir)
{
        struct p1212_key **keys;

	keys = p1212_find(udir, P1212_KEYTYPE_Immediate,
			  P1212_KEYVALUE_Unit_Spec_Id, 0);
	if (keys && keys[0]->val == SBP2_UNIT_SPEC_ID) {
		keys = p1212_find(udir, P1212_KEYTYPE_Immediate,
				  P1212_KEYVALUE_Unit_Sw_Version, 0);
		if (keys && keys[0]->val == SBP2_UNIT_SW_VERSION)
			return 1;
	}
	return 0;
}

static void
sbp2_logout(struct sbp2 *sbp2, struct sbp2_lun *lun)
{
	DPRINTF(("Called sbp2_logout\n"));
	return;
}

void
sbp2_free(struct sbp2 *sbp2)
{
	struct sbp2_orb *orb;
	struct sbp2_lun *lun;
	
	while (CIRCLEQ_FIRST(&sbp2->orbs) != (void *)&sbp2->orbs) {
		orb = CIRCLEQ_FIRST(&sbp2->orbs);
		CIRCLEQ_REMOVE(&sbp2->orbs, orb, orb_list);
		(void)sbp2_abort(orb);
		free (orb, M_1394DATA);
	}
	while (TAILQ_FIRST(&sbp2->luns) != NULL) {
		lun = TAILQ_FIRST(&sbp2->luns);
		TAILQ_REMOVE(&sbp2->luns, lun, lun_list);
		if (lun->login_flag)
			sbp2_logout(sbp2, lun);
		free (lun, M_1394DATA);
	}
	simple_lock(&sbp2_maps_lock);
	if (!--sbp2->map->refcnt) {
		TAILQ_REMOVE(&sbp2_maps, sbp2->map, map_list);
		free(sbp2->map, M_1394DATA);
	}
	simple_unlock(&sbp2_maps_lock);
	sbp2->sc->sc1394_callback.sc1394_unreg(&status_orb.cmd, 1);
	free (status_orb.cmd.ab_data, M_1394DATA);
}

#ifdef FW_DEBUG
extern int fwdebug;
#endif

void *
sbp2_runcmd(struct sbp2 *sbp2, struct sbp2_cmd *cmd)
{
#ifdef DIAGNOSTIC
	int found = 0;
#endif
	struct sbp2_orb *orb, *toporb;
	struct ieee1394_softc *psc =
	    (struct ieee1394_softc *) sbp2->sc->sc1394_dev.dv_parent;
	struct sbp2_lun *lun;
	struct uio io;
	struct iovec iov;
	u_int32_t t;
	u_int64_t addr;
	
#if defined(FW_DEBUG) && defined(SBP2_DEBUG)
	if (sbp2debug > 2)
		fwdebug = 3;
#endif
	TAILQ_FOREACH(lun, &sbp2->luns, lun_list)
		if (lun->lun == cmd->lun) {
#ifdef DIAGNOSTIC
			found = 1;
#endif
			break;
		}
	
#ifdef DIAGNOSTIC
	if (!found) {
		DPRINTF(("Got a request for an invalid lun: %d\n", cmd->lun));
		return NULL;
	}
	if (cmd->cmdlen % 4) {
		DPRINTF(("cmdlen is not 4 byte aligned: %d\n", cmd->cmdlen));
		return NULL;
	}
	if ((cmd->cmdlen / 4) > (sbp2->orb_size - 5)) {
		DPRINTF(("cmdlen too large: len - %d max - %d\n",
			    cmd->cmdlen / 4, sbp2->orb_size - 5));
		return NULL;
	}
#endif
	if (lun->login_flag != SBP2_LOGGED_IN)
		sbp2_login(sbp2, lun);
	
	orb = sbp2_alloc_orb();

	orb->cmd.ab_addr = sbp2_alloc_addr(sbp2);
	orb->cb = cmd->cb;
	orb->cb_arg = cmd->cb_arg;
	orb->cmd.ab_subok = 1;

	orb->cmd.ab_req = sbp2->sc;
	orb->cmd.ab_tcode = IEEE1394_TCODE_READ_REQ_BLOCK;
	orb->cmd.ab_length = (sbp2->orb_size * 4);
	orb->cmd.ab_data[0] = htonl(SBP2_ORB_NULL_POINTER);
	orb->cmd.ab_data[4] |= SBP2_ORB_NOTIFY_MASK;

	if (cmd->rw == SBP_WRITE)
		orb->cmd.ab_data[4] |= SBP2_ORB_RW_MASK;
	orb->cmd.ab_data[4] |= SBP2_ORB_SET_SPEED(sbp2->sc->sc1394_link_speed);
	orb->cmd.ab_data[4] = htonl(orb->cmd.ab_data[4]);
	
	memcpy(&orb->cmd.ab_data[5], cmd->cmd, cmd->cmdlen);
	
	orb->cmd.ab_cb = sbp2_orb_resp;
	orb->cmd.ab_cbarg = orb;
	orb->lun = lun;
	orb->sbp2 = sbp2;
	
	if (cmd->data) {
		if ((cmd->datalen == 0) || (cmd->datalen > SBP2_MAXPHYS)) {
			/* Handle uio and large data chunks via page tables. */
			if (cmd->datalen) {
				io.uio_iov = &iov;
				io.uio_iovcnt = 1;
				io.uio_offset = 0;
				io.uio_resid = cmd->datalen;
				io.uio_segflg = UIO_SYSSPACE;
				if (cmd->rw == SBP_WRITE)
					io.uio_rw = UIO_WRITE;
				else
					io.uio_rw = UIO_READ;
				io.uio_procp = NULL;
				iov.iov_base = cmd->data;
				iov.iov_len = cmd->datalen;
				orb->pt = sbp2_alloc_pt(&io, cmd->rw, orb);
			} else 
				orb->pt = 
				    sbp2_alloc_pt((struct uio *)cmd->data, 
					cmd->rw, orb);
		 	if (orb->pt == NULL) {
				sbp2_free_orb (orb);
				return NULL;
			}
			orb->cmd.ab_data[2] =
			    IEEE1394_CREATE_ADDR_HIGH(orb->pt->pt_ent.ab_addr);
			orb->cmd.ab_data[2] |=
			    htonl((0xffc0 | psc->sc1394_node_id) << 16);
			orb->cmd.ab_data[3] =
			    IEEE1394_CREATE_ADDR_LOW(orb->pt->pt_ent.ab_addr);
			t = SBP2_ORB_SET_MAXTRANS(sbp2->sc->sc1394_link_speed);
			orb->cmd.ab_data[4] |= htonl(t);
			orb->cmd.ab_data[4] |= htonl(SBP2_ORB_PAGETABLE_MASK);
			orb->cmd.ab_data[4] |= htonl(orb->pt->pt_cnt);
		} else {
			sbp2_alloc_data_mapping(sbp2, &orb->data_map, cmd->data,
		    	    cmd->datalen, cmd->rw);
			orb->data_map.orb = orb;
			orb->data.ab_length = cmd->datalen;
			orb->data.ab_addr = orb->data_map.fwaddr;
			orb->data.ab_cb = sbp2_data_resp;
			orb->data.ab_cbarg = &orb->data_map;
			orb->data.ab_data = (u_int32_t *)cmd->data;
			orb->data.ab_req = sbp2->sc;
			if (cmd->rw == SBP_WRITE)
				orb->data.ab_tcode = 
				    IEEE1394_TCODE_WRITE_REQ_BLOCK;
			else
				orb->data.ab_tcode = 
				    IEEE1394_TCODE_READ_REQ_BLOCK;
			orb->cmd.ab_data[2] =
			    IEEE1394_CREATE_ADDR_HIGH(orb->data.ab_addr);
			orb->cmd.ab_data[2] |=
			    htonl((0xffc0 | psc->sc1394_node_id) << 16);
			orb->cmd.ab_data[3] =
			    IEEE1394_CREATE_ADDR_LOW(orb->data.ab_addr);
			t = SBP2_ORB_SET_MAXTRANS(sbp2->sc->sc1394_link_speed);
			orb->cmd.ab_data[4] |= htonl(t);
			orb->cmd.ab_data[4] |= htonl(cmd->datalen);
			sbp2->sc->sc1394_callback.sc1394_inreg(&orb->data, 1);
		}
	}
	
	simple_lock(&sbp2->orblist_lock);
	toporb = CIRCLEQ_FIRST(&sbp2->orbs);
	simple_lock(&toporb->orb_lock);
	addr = orb->cmd.ab_addr;
	toporb->cmd.ab_data[0] = IEEE1394_CREATE_ADDR_HIGH(addr);
	toporb->cmd.ab_data[1] = IEEE1394_CREATE_ADDR_LOW(addr);
	sbp2->sc->sc1394_callback.sc1394_inreg(&orb->cmd, 0);
	if ((lun->state == SBP2_STATE_SUSPENDED) || 
	    ((lun->state == SBP2_STATE_ACTIVE) && 
	     (toporb->state != SBP2_ORB_INIT_STATE))) {
		DPRINTFN(1, ("Ringing doorbell\n"));
		toporb->state = SBP2_ORB_INIT_STATE;
		toporb->db = 1;
		toporb->dback = 0;
		sbp2->sc->sc1394_callback.sc1394_write(&lun->doorbell);
	} else if (lun->state == SBP2_STATE_DEAD) {
		CIRCLEQ_FOREACH(toporb, &orb->sbp2->orbs, orb_list) {
			toporb->ack = 0;
			toporb->status_rec = 0;
			toporb->db = 0;
			toporb->dback = 0;
		}
		toporb = CIRCLEQ_LAST(&sbp2->orbs);
		lun->doorbell.ab_addr = lun->cmdreg + SBP2_CMDREG_AGENT_RESET;
		lun->doorbell.ab_cb = sbp2_enable_status;
		lun->command.ab_data[0] =
		    IEEE1394_CREATE_ADDR_HIGH(toporb->cmd.ab_addr);
		lun->command.ab_data[1] =
		    IEEE1394_CREATE_ADDR_LOW(toporb->cmd.ab_addr);
		toporb->state = SBP2_ORB_INIT_STATE;
		sbp2->sc->sc1394_callback.sc1394_write(&lun->doorbell);
		sbp2->sc->sc1394_callback.sc1394_write(&lun->command);
	}
	lun->state = SBP2_STATE_ACTIVE;
	
	CIRCLEQ_INSERT_HEAD(&sbp2->orbs, orb, orb_list);
	simple_unlock(&toporb->orb_lock);
	simple_unlock(&sbp2->orblist_lock);

	return orb;
}

#ifdef SBP2_DEBUG
static void
sbp2_agent_status(struct ieee1394_abuf *abuf, int status)
{
	DPRINTF(("sbp2_agent_status: 0x%08x\n", ntohl(abuf->ab_data[0])));
	return;
}
#endif

static void
sbp2_orb_resp(struct ieee1394_abuf *abuf, int status)
{
	struct sbp2_orb *statorb, *orb;
	u_int64_t addr;
	int found = 0;
	u_int32_t t;
	
	orb = abuf->ab_cbarg;
	
	DPRINTFN(1, ("orb addr: 0x%016qx\n", orb->cmd.ab_addr));
        DPRINTFN(1, ("orb next ptr: 0x%08x%08x\n", ntohl(orb->cmd.ab_data[0]), ntohl(orb->cmd.ab_data[1])));
	DPRINTFN(1, ("retlen: %d, length: %d\n", abuf->ab_retlen,
		     abuf->ab_length));

#ifdef SBP2_DEBUG
        if ((sbp2debug > 3) && orb->lun) {
		orb->lun->status.ab_addr = 
		    orb->lun->cmdreg + SBP2_CMDREG_AGENT_STATE;
		orb->lun->status.ab_cb = sbp2_agent_status;
		orb->lun->status.ab_cbarg = orb->lun;
		orb->lun->status.ab_length = 4;
		orb->lun->status.ab_req = orb->sbp2->sc;
		orb->lun->status.ab_tcode = IEEE1394_TCODE_READ_REQ_QUAD;
		orb->sbp2->sc->sc1394_callback.sc1394_read(&orb->lun->status);
        }
#endif
	simple_lock(&orb->orb_lock);

	switch (orb->state) {
	case SBP2_ORB_INIT_STATE:
		if (abuf->ab_tcode == IEEE1394_TCODE_READ_REQ_BLOCK) {
			orb->state = SBP2_ORB_STATUS_STATE;
			abuf->ab_tcode = IEEE1394_TCODE_READ_RESP_BLOCK;
			abuf->ab_length = abuf->ab_retlen;
			abuf->ab_req->sc1394_callback.sc1394_write(&orb->cmd);
			break;
		}

		/* FALL THRU */

	case SBP2_ORB_STATUS_STATE:

		/*
		 * If it's not the fifo addr then just resend the orb out as
		 * the doorbell was rung to reread.
		 */

		if (orb->cmd.ab_addr != status_orb.cmd.ab_addr) {
			/* An ack from a write */
	
			/*
			 * Change engine state once this has been processed 
			 * and it's next pointer is the null pointer. 
			 */

			if ((orb->cmd.ab_data[0] == 
			    ntohl(SBP2_ORB_NULL_POINTER)) &&
		    	    (orb->lun->state == SBP2_STATE_ACTIVE))
				orb->lun->state = SBP2_STATE_SUSPENDED;
			if (orb->ack == 0) 
				orb->ack = 1;
			else if ((orb->db == 1) && (orb->dback == 0)) 
				orb->dback = 1;
			else
				panic ("Unknown packet received!");
		} else {

			/*
			 * The orb passed in is the generic status. Find the 
			 * one it goes with so status can be filled in and 
			 * passed back up.
			 */
		
			addr = ntohl(abuf->ab_data[0]);
			addr &= 0x0000ffff;
			addr = (addr << 32);
			addr |= ntohl(abuf->ab_data[1]);
			simple_lock(&orb->sbp2->orblist_lock);
			CIRCLEQ_FOREACH(statorb, &orb->sbp2->orbs, orb_list) {
				if (addr == statorb->cmd.ab_addr) {
					found = 1;
					break;
				}
			}
			simple_unlock(&orb->sbp2->orblist_lock);
			simple_lock(&statorb->orb_lock);
		
			/* XXX: Need to handle unsolicited correctly. */
			if (SBP2_STATUS_GET_DEAD(ntohl(abuf->ab_data[0]))) {
				DPRINTFN(1, ("Transitioning to dead state\n"));
				statorb->lun->state = SBP2_STATE_DEAD;
			}
			if (!found) {
#ifdef SBP2_DEBUG
				u_int32_t i = ntohl(abuf->ab_data[0]);
#endif
				DPRINTF(("Got a status block for an unknown "
				    "orb addr: 0x%016qx\n", addr));
				DPRINTF(("resp: 0x%x status: 0x%x len: 0x%x\n",
				    SBP2_STATUS_GET_RESP(i),
				    SBP2_STATUS_GET_STATUS(i),
				    SBP2_STATUS_GET_LEN(i) - 1));
				return;
			}

			/*
		 	 * After it's been sent turn this into a dummy orb. 
			 * That way if the engine stalls and has to be 
			 * restarted this orb getting reread won't cause 
			 * duplicate work.
		 	 */

			statorb->cmd.ab_data[4] |= 
				htonl(SBP2_ORB_FMT_DUMMY_MASK);
			statorb->status.resp =
			    SBP2_STATUS_GET_RESP(ntohl(abuf->ab_data[0]));
			statorb->status.sbp_status =
			    SBP2_STATUS_GET_STATUS(ntohl(abuf->ab_data[0]));
			statorb->status.datalen =
			    SBP2_STATUS_GET_LEN(ntohl(abuf->ab_data[0])) - 1;
			if (statorb->status.datalen)
				statorb->status.data = &abuf->ab_data[2];
			if ((statorb->status.resp == 
			    SBP2_STATUS_TRANSPORT_FAIL) &&
			    (statorb->status.sbp_status == 
			    SBP2_STATUS_UNSPEC_ERROR)) {
				t = statorb->status.sbp_status;
				statorb->status.object = 
				    SBP2_STATUS_GET_OBJECT(t);
				statorb->status.bus_error =
				    SBP2_STATUS_GET_BUS_ERROR(t);
			}
			statorb->status_rec = 1;
			simple_unlock(&statorb->orb_lock);
			statorb->cb(&statorb->status, statorb->cb_arg);
			statorb->cb = sbp2_status_resp;
			orb = statorb;
		}
		
		/* If it's not the null pointer orb, free it. */
		simple_lock(&orb->orb_lock);
		if ((orb->cmd.ab_data[0] == ntohl(SBP2_ORB_NULL_POINTER)))
			break;
		/* Check conditions for free'ing an orb */
		if ((orb->status_rec == 0) || (orb->ack == 0) || 
		    ((orb->db == 1) && (orb->dback == 0)))
			break;
		simple_lock(&orb->sbp2->orblist_lock);

		/*
		 * Always leave the last orb on the list so the doorbell can
		 * be rang.
		 */
		
		if (orb != CIRCLEQ_FIRST(&orb->sbp2->orbs)) {
			orb->cmd.ab_tcode = IEEE1394_TCODE_READ_REQ_BLOCK;

			CIRCLEQ_REMOVE(&orb->sbp2->orbs, orb, orb_list);
			simple_unlock(&orb->sbp2->orblist_lock);
			simple_unlock(&orb->orb_lock);
			sbp2_free_orb(orb);
			return;
		}
		simple_unlock(&orb->sbp2->orblist_lock);
		break;
	case SBP2_ORB_FREE_STATE:
		break;
	default:
		panic("Invalid orb state: %d\n", orb->state);
		break;
	}
	simple_unlock(&orb->orb_lock);
}

static void
sbp2_data_resp(struct ieee1394_abuf *abuf, int rcode)
{
	struct sbp2_orb *orb;
	struct sbp2_mapping *data_map;
	u_int32_t offset;
	unsigned char *addr;
	
	switch (abuf->ab_tcode) {

	case IEEE1394_TCODE_WRITE_REQ_BLOCK:
		return;
		break;
	case IEEE1394_TCODE_READ_REQ_BLOCK:
		data_map = abuf->ab_cbarg;
		orb = data_map->orb;
	
		simple_lock(&orb->orb_lock);

		addr = data_map->laddr;
		offset = abuf->ab_retaddr - data_map->fwaddr;

		orb->resp.ab_addr = abuf->ab_retaddr;
		orb->resp.ab_tcode = IEEE1394_TCODE_READ_RESP_BLOCK;
		orb->resp.ab_tlabel = abuf->ab_tlabel;
		orb->resp.ab_length = abuf->ab_retlen;
		orb->resp.ab_req = abuf->ab_req;
		orb->resp.ab_data = (u_int32_t *)(addr + offset);
		orb->resp.ab_cb = sbp2_null_resp;
		orb->resp.ab_cbarg = data_map;
		orb->sbp2->sc->sc1394_callback.sc1394_write(&orb->resp);
		simple_unlock(&orb->orb_lock);
		break;
	default:
		panic("Invalid tcode: 0x%0x\n", abuf->ab_tcode);
	}
}

void *
sbp2_abort(void *handle)
{
	struct sbp2_orb *orb = handle;
	void *arg;
	int remove;
	
	DPRINTF(("Called sbp2_abort\n"));

	remove = 0;
	simple_lock(&orb->sbp2->orblist_lock);
	simple_lock(&orb->orb_lock);
	arg = orb->cb_arg;
	if (orb != CIRCLEQ_FIRST(&orb->sbp2->orbs)) {
		remove = 1;
		CIRCLEQ_REMOVE(&orb->sbp2->orbs, orb, orb_list);
	} else {
		orb->state = SBP2_ORB_INIT_STATE;
		orb->cmd.ab_data[4] |= htonl(SBP2_ORB_FMT_DUMMY_MASK);
	}
	
	simple_unlock(&orb->orb_lock);
	simple_unlock(&orb->sbp2->orblist_lock);
	
	if (remove)
		sbp2_free_orb(orb);
	
	return arg;
}

static void
sbp2_relogin(struct ieee1394_softc *sc, void *arg)
{
	struct sbp2 *sbp2 = arg;

	DPRINTF(("Called sbp2_relogin\n"));
	if (sbp2)
		return;
}

void
sbp2_reset_lun(struct sbp2 *sbp2, u_int16_t lun)
{
	DPRINTF(("Called sbp2_reset_lun\n"));
	return;
}

static struct sbp2_orb *
sbp2_alloc_orb(void)
{
	struct sbp2_orb *orb = NULL;
	int i;
	
	simple_lock(&sbp2_freeorbs_lock);
	if (CIRCLEQ_EMPTY(&sbp2_freeorbs)) {
		DPRINTFN(2, ("Alloc'ing more orbs\n"));
		for (i = 0; i < SBP2_NUM_ALLOC; i++) {
			simple_unlock(&sbp2_freeorbs_lock);
			orb = malloc(sizeof(struct sbp2_orb), M_1394DATA,
			    M_WAITOK|M_ZERO);
			orb->cmd.ab_data = malloc(SBP2_MAX_ORB, M_1394DATA,
			    M_WAITOK|M_ZERO);
			simple_lock_init(&orb->orb_lock);
			simple_lock(&sbp2_freeorbs_lock);
			CIRCLEQ_INSERT_TAIL(&sbp2_freeorbs, orb, orb_list);
		}
		simple_unlock(&sbp2_freeorbs_lock);
		orb = malloc(sizeof(struct sbp2_orb), M_DEVBUF,
		    M_WAITOK|M_ZERO);
		orb->cmd.ab_data = malloc(SBP2_MAX_ORB, M_1394DATA,
		    M_WAITOK|M_ZERO);
		simple_lock_init(&orb->orb_lock);
		return orb;
	} else {
		orb = CIRCLEQ_FIRST(&sbp2_freeorbs);
		CIRCLEQ_REMOVE(&sbp2_freeorbs, orb, orb_list);
	}
	simple_unlock(&sbp2_freeorbs_lock);
	orb->state = SBP2_ORB_INIT_STATE;
	return orb;
}

static void
sbp2_free_orb(struct sbp2_orb *orb)
{
	simple_lock(&orb->orb_lock);
	DPRINTFN(2, ("Freeing orb at addr: 0x%016qx status_rec: 0x%0x\n", 
	    orb->cmd.ab_addr, orb->status_rec));
	orb->sbp2->sc->sc1394_callback.sc1394_unreg(&orb->cmd, 0);
	if (orb->data_map.laddr) {
		orb->sbp2->sc->sc1394_callback.sc1394_unreg(&orb->data, 1);
		sbp2_free_data_mapping(orb->sbp2, &orb->data_map);
	}
	if (orb->pt)
		sbp2_free_pt(orb->pt);

	sbp2_free_addr(orb->sbp2, orb->cmd.ab_addr);

	simple_lock(&sbp2_freeorbs_lock);
	memset(orb->cmd.ab_data, 0, SBP2_MAX_ORB);
	memset(&orb->status, 0, sizeof(struct sbp2_status));
	memset(&orb->data_map, 0, sizeof(struct sbp2_mapping));
	orb->sbp2 = NULL;
	orb->pt = NULL;
	orb->lun = NULL;
	orb->cb = NULL;
	orb->cb_arg = NULL;
	orb->status_rec = 0;
	orb->db = 0;
	orb->dback = 0;
	orb->ack = 0;
	orb->state = SBP2_ORB_FREE_STATE;
	CIRCLEQ_INSERT_TAIL(&sbp2_freeorbs, orb, orb_list);
	simple_unlock(&sbp2_freeorbs_lock);
	simple_unlock(&orb->orb_lock);
}

static void
sbp2_alloc_data_mapping(struct sbp2 *sbp2, struct sbp2_mapping *map,
    u_char *data, u_int32_t datalen, u_int8_t rw)
{
	int byte, bitpos, found;
	u_int32_t size, count, startbyte, startbit;
	unsigned char bit;
	
	size = datalen / SBP_DATA_BLOCK_SIZE;
	if (datalen % SBP_DATA_BLOCK_SIZE)
		size++;

	map->laddr = data;
	map->size = datalen;
	map->rw = rw;
	
	simple_lock(&sbp2->map->maplock);
	count = found = 0;
	startbyte = 0;
	startbit = 0;
	for (byte = 0; byte < sizeof(sbp2->map->datamap); byte++) {
		for (bitpos = 0; bitpos < CHAR_BIT; bitpos++) {
			bit = 0x1 << bitpos;
			if ((sbp2->map->datamap[byte] & bit) == 0) {
				if (++count == size) {
					found = 1;
					break;
				}
			} else {
				count = 0;
				if (bitpos != (CHAR_BIT - 1)) {
					startbyte = byte;
					startbit = bitpos + 1;
				} else {
					startbyte = byte + 1;
					startbit = 0;
				}
			}
		}
		if (found)
			break;
	}

	/* Gets a little complicated to handle crossing bytes on the ends. */
	
	/* Handle the bits on the front end if they start in the middle */
	if (startbit) {
		count = CHAR_BIT - startbit;
		if (size < count)
			count = size;
		for (bitpos = 0; bitpos < count; bitpos++) {
			bit = 0x1 << (bitpos + startbit);
			size--;
			sbp2->map->datamap[startbyte] |= bit;
		}
		startbyte++;
	}

	/* Allocate bytes at a time */
	if (size) {
		count = startbyte + (size / CHAR_BIT);
		for (byte = startbyte; byte < count; byte++) {
			sbp2->map->datamap[byte] = 0xff;
			size -= CHAR_BIT;
		}
		/* If any bits are left allocate them out of the next byte */
		if (size) {
#ifdef DEBUG
			if (size >= CHAR_BIT)
				panic ("Too many bits left to allocate: %d",
				    size);
#endif
			for (bitpos = 0; bitpos < size; bitpos++) {
				bit = 0x1 << bitpos;
				sbp2->map->datamap[byte] |= bit;
			}
		}
	}

	/* Adjust back one if the bits started 1 byte back */
	if (startbit)
		startbyte--;
	map->fwaddr = SBP_DATA_BEG +
	    (((startbyte * CHAR_BIT) + startbit) * SBP_DATA_BLOCK_SIZE);

	simple_unlock(&sbp2->map->maplock);
}

static u_int64_t
sbp2_alloc_addr(struct sbp2 *sbp2)
{
	u_int64_t addr;
	int byte, bitpos, found;
	unsigned char bit;
	
	found = 0;
	simple_lock(&sbp2->map->maplock);

	addr = SBP_ADDR_BEG + (sbp2->map->next_addr * SBP_ADDR_BLOCK_SIZE);
	byte = sbp2->map->next_addr / CHAR_BIT;
	bitpos = sbp2->map->next_addr % CHAR_BIT;
	bit = 0x1 << bitpos;
	
#ifdef DIAGNOSTIC
	if (sbp2->map->addrmap[byte] & bit)
		panic("Already allocated address 0x%016" PRIx64, addr);
#endif
	sbp2->map->addrmap[byte] |= bit;

	for (; byte < (sizeof(sbp2->map->addrmap) - byte); byte++) {
		for (bitpos = 0; bitpos < CHAR_BIT; bitpos++) {
			bit = 0x1 << bitpos;
			if ((sbp2->map->addrmap[byte] & bit) == 0) {
				found = 1;
				break;
			}
		}
		if (found)
			break;
	}
	sbp2->map->next_addr = (byte * CHAR_BIT) + bitpos;
	
	simple_unlock(&sbp2->map->maplock);
	
#ifdef DIAGNOSTIC
	if (sbp2->map->next_addr >= SBP_ADDR_MAX)
		panic("XXX: Used 64k of sbp addr's.\n");
#endif
	return addr;
}

static void
sbp2_free_data_mapping(struct sbp2 *sbp2, struct sbp2_mapping *map)
{
	int byte, bitpos;
	u_int32_t size, count, startbyte, off, startbit;
	unsigned char bit;

	simple_lock(&sbp2->map->maplock);

        size = map->size / SBP_DATA_BLOCK_SIZE;
        if (map->size % SBP_DATA_BLOCK_SIZE)
                size++;
	off = ((int)(map->fwaddr - SBP_DATA_BEG) / SBP_DATA_BLOCK_SIZE);
	
	startbyte = off / CHAR_BIT;
	startbit = off % CHAR_BIT;

	/*
	 * 3 parts. Any bits in the middle of the first byte.
	 * Then bytes until whole bytes are done.
	 * Finally, any left over remaining bits.
	 */

	if (startbit) {
		count = CHAR_BIT - startbit;
		if (size < count)
			count = size;
		for (bitpos = 0; bitpos < count; bitpos++) {
			bit = 0x1 << (bitpos + startbit);
#ifdef DIAGNOSTIC
			if (!(sbp2->map->datamap[startbyte] & bit))
				panic("Freeing addr not allocated: 0x%016"
				    PRIx64, map->fwaddr);
#endif
			bit = ~bit;
			size--;
			sbp2->map->datamap[startbyte] &= bit;
		}
		startbyte++;
	}

	if (size) {
		count = startbyte + (size / CHAR_BIT);
		for (byte = startbyte; byte < count; byte++) {
#ifdef DIAGNOSTIC
			if (!(sbp2->map->datamap[byte]))
				panic("Freeing addr not allocated: 0x%016"
				    PRIx64, map->fwaddr);
#endif
			size -= CHAR_BIT;
			sbp2->map->datamap[byte] = 0;
		}
		/* If any bits are left free them out of the next byte */
		if (size) {
#ifdef DEBUG
			if (size >= CHAR_BIT)
				panic ("Too many bits left to free: %d", size);
#endif
			for (bitpos = 0; bitpos < size; bitpos++) {
				bit = 0x1 << bitpos;
#ifdef DIAGNOSTIC
				if (!(sbp2->map->datamap[byte] & bit))
					panic("Freeing addr not allocated: "
					    "0x%016" PRIx64, map->fwaddr);
#endif
				bit = ~bit;
				sbp2->map->datamap[byte] &= bit;
			}
		}
	}
	if (startbit)
		startbyte--;

	simple_unlock(&sbp2->map->maplock);

	return;
}

static void
sbp2_free_addr(struct sbp2 *sbp2, u_int64_t addr)
{
	int off, byte, bitpos;
	unsigned char bit;

	off = ((int)(addr - SBP_ADDR_BEG) / SBP_ADDR_BLOCK_SIZE);
	
	byte = off / CHAR_BIT;
	bitpos = off % CHAR_BIT;
	bit = 0x1 << bitpos;

	simple_lock(&sbp2->map->maplock);
#ifdef DIAGNOSTIC
	if (!(sbp2->map->addrmap[byte] & bit))
		panic("Freeing addr not allocated: 0x%016" PRIx64, addr);
#endif
	bit = ~bit;
	sbp2->map->addrmap[byte] &= bit;
	if (sbp2->map->next_addr > off)
		sbp2->map->next_addr = off;
	simple_unlock(&sbp2->map->maplock);
}

static struct sbp2_pagetable *
sbp2_alloc_pt(struct uio *io, u_int8_t rw, struct sbp2_orb *orb)
{
	struct sbp2_pagetable *pt;
	struct sbp2_mapping *map;
	struct ieee1394_softc *sc;
	ssize_t len;
	char *addr;
	int i, j, k, cnt;

	pt = malloc (sizeof (*pt), M_1394DATA, M_ZERO | M_WAITOK);

	/* Compute number of entries. */

	for (i = 0; i < io->uio_iovcnt; i++) {
		if (io->uio_iov[i].iov_len <= SBP2_PHYS_SEGMENT)
			pt->pt_cnt++;
		else {
			pt->pt_cnt += 
			    io->uio_iov[i].iov_len / SBP2_PHYS_SEGMENT;
			if (io->uio_iov[i].iov_len % SBP2_PHYS_SEGMENT)
				pt->pt_cnt++;
		}
#ifdef DEBUG
		/* Overflow'd 16 bits. */
		if (pt->pt_cnt == 0) {
			DPRINTFN(1, ("pt_cnt overflow\n"));
			free (pt, M_1394DATA);
			return NULL;
		}
#endif
	}

	sc = orb->sbp2->sc;

	pt->pt_ent.ab_data = malloc (SBP2_PTENT_SIZE * pt->pt_cnt, M_1394DATA, 
	    M_ZERO | M_WAITOK);
	sbp2_alloc_data_mapping(orb->sbp2, &pt->pt_map, 
	    (char *)pt->pt_ent.ab_data, SBP2_PTENT_SIZE * pt->pt_cnt, SBP_READ);

	pt->pt_ent.ab_addr = pt->pt_map.fwaddr;

	pt->pt_ent.ab_length = SBP2_PTENT_SIZE * pt->pt_cnt;
	pt->pt_ent.ab_tcode = IEEE1394_TCODE_READ_REQ_BLOCK;
	pt->pt_ent.ab_req = sc;
	pt->pt_ent.ab_cb = sbp2_pt_resp;
	pt->pt_ent.ab_cbarg = orb;
	pt->pt_data = malloc (sizeof (struct ieee1394_abuf) * pt->pt_cnt,
	    M_1394DATA, M_ZERO | M_WAITOK);

	j = 0;
	for (i = 0; i < io->uio_iovcnt; i++) {
		if (io->uio_iov[i].iov_len <= SBP2_PHYS_SEGMENT) {
			cnt = 1;
		} else {
			cnt = io->uio_iov[i].iov_len / SBP2_PHYS_SEGMENT;
			if (io->uio_iov[i].iov_len % SBP2_PHYS_SEGMENT)
				cnt++;
		}
		len = io->uio_iov[i].iov_len;
		for (k = 0; k < cnt; k++) {
#ifdef DIAGNOSTIC
			if (len == 0)
				panic ("len is zero");
#endif
			map = malloc (sizeof (struct sbp2_mapping), M_1394DATA, 
			    M_ZERO | M_WAITOK);
			pt->pt_data[j].ab_cbarg = map;
			addr = (char *)io->uio_iov[i].iov_base + 
			    (k * SBP2_PHYS_SEGMENT);
			if (len > SBP2_PHYS_SEGMENT) {
				len -= SBP2_PHYS_SEGMENT;
				sbp2_alloc_data_mapping(orb->sbp2, map, addr,
			    	    SBP2_PHYS_SEGMENT, rw);
				pt->pt_data[j].ab_length = SBP2_PHYS_SEGMENT;
				pt->pt_ent.ab_data[j * SBP2_PTENT_SIZEQ] =
			    	    SBP2_PT_MAKELEN(SBP2_PHYS_SEGMENT);
			} else {
				sbp2_alloc_data_mapping(orb->sbp2, map, addr,
			    	    len, rw);
				pt->pt_data[j].ab_length = len;
				pt->pt_ent.ab_data[j * SBP2_PTENT_SIZEQ] =
				    SBP2_PT_MAKELEN(len);
				len -= len;
			}
			pt->pt_data[j].ab_addr = map->fwaddr;
			pt->pt_data[j].ab_req = sc;
			if (rw == SBP_WRITE)
				pt->pt_data[j].ab_tcode = 
				    IEEE1394_TCODE_WRITE_REQ_BLOCK;
			else
				pt->pt_data[j].ab_tcode = 
				    IEEE1394_TCODE_READ_REQ_BLOCK;
			map->orb = orb;
			pt->pt_data[j].ab_cb = sbp2_data_resp;
			pt->pt_data[j].ab_data = (u_int32_t *)addr;
			pt->pt_ent.ab_data[j * SBP2_PTENT_SIZEQ] |=
			    IEEE1394_CREATE_ADDR_HIGH(map->fwaddr);
			pt->pt_ent.ab_data[(j * SBP2_PTENT_SIZEQ) + 1] =
			    IEEE1394_CREATE_ADDR_LOW(map->fwaddr);
			sc->sc1394_callback.sc1394_inreg(&pt->pt_data[j], 1);
			j++;
		} 
	}
	sc->sc1394_callback.sc1394_inreg(&pt->pt_ent, 1);
	return pt;
}

static void 
sbp2_pt_resp(struct ieee1394_abuf *abuf, int rcode)
{
	struct sbp2_orb *orb = abuf->ab_cbarg;
	struct sbp2_pagetable *pt = orb->pt;
	u_int32_t offset;

	if (rcode) {
		DPRINTF(("sbp2_pt_resp: Bad return code: %d\n", rcode));
		return;
        }

	simple_lock(&orb->orb_lock);

	/*
	 * The target is allowed to read these 1 entry at a time so construct
	 * responses in a different abuf to allow for this.
	 */

	offset = abuf->ab_retaddr - abuf->ab_addr;
	pt->pt_resp.ab_addr = abuf->ab_retaddr;
	pt->pt_resp.ab_tcode = IEEE1394_TCODE_READ_RESP_BLOCK;
	pt->pt_resp.ab_tlabel = abuf->ab_tlabel;
	pt->pt_resp.ab_length = abuf->ab_retlen;
	pt->pt_resp.ab_req = abuf->ab_req;
	pt->pt_resp.ab_data = (u_int32_t *)((u_int8_t *)abuf->ab_data + offset);
	pt->pt_resp.ab_cb = sbp2_null_resp;
	pt->pt_resp.ab_cbarg = orb;
	abuf->ab_req->sc1394_callback.sc1394_write(&pt->pt_resp);
	simple_unlock(&orb->orb_lock);
	return;
}

static void
sbp2_free_pt(struct sbp2_pagetable *pt)
{
	struct sbp2_orb *orb = pt->pt_ent.ab_cbarg;
	struct ieee1394_softc *sc = orb->sbp2->sc;
	int i;

	sc->sc1394_callback.sc1394_unreg(&pt->pt_ent, 1);
	sbp2_free_data_mapping(orb->sbp2, &pt->pt_map);

	for (i = 0; i < pt->pt_cnt; i++) {
		sc->sc1394_callback.sc1394_unreg(&pt->pt_data[i], 1);
		sbp2_free_data_mapping(orb->sbp2, pt->pt_data[i].ab_cbarg);
		free (pt->pt_data[i].ab_cbarg, M_1394DATA);
	}
	free (pt->pt_data, M_1394DATA);
	free (pt->pt_ent.ab_data, M_1394DATA);
	free (pt, M_1394DATA);
}
