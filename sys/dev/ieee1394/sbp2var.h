/*	$NetBSD: sbp2var.h,v 1.5.2.1 2005/03/04 16:43:12 skrll Exp $	*/

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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

#ifndef _DEV_IEEE1394_SBP2VAR_H
#define _DEV_IEEE1394_SBP2VAR_H

#include <dev/std/ieee1212var.h>
struct sbp2_orb;

struct sbp2_mapping {
	void *laddr;
	u_int64_t fwaddr;
	u_int32_t size;
	u_int8_t rw;
	struct sbp2_orb *orb;
};

/* Need a top level map per bus. */

struct sbp2_map {
	unsigned char datamap[SBP_DATA_SIZE / SBP_DATA_BLOCK_SIZE / CHAR_BIT];
	unsigned int next_data;

	/*
	 * Divide by 4 because this is allocated in quad chunks and divide by
	 * 8 to make it a bitmap.
	 */
	unsigned char addrmap[SBP_ADDR_SIZE / SBP_ADDR_BLOCK_SIZE / CHAR_BIT];
	unsigned int next_addr;

	u_int64_t fifo;
	struct ieee1394_softc *sc_bus; /* fw controller softc */
	int refcnt;
	struct simplelock maplock;
	TAILQ_ENTRY(sbp2_map) map_list;
};

struct sbp2;
struct sbp2_lun;

struct sbp2_status {
	u_int8_t resp;
	u_int8_t sbp_status;
	u_int8_t object;
	u_int8_t bus_error;
	u_int32_t *data;
	u_int16_t datalen;
};

struct sbp2_pagetable {
	struct ieee1394_abuf pt_ent;
	struct ieee1394_abuf pt_resp;
	struct sbp2_mapping pt_map;

	u_int16_t pt_cnt;
	struct ieee1394_abuf *pt_data; /* cbarg == data_mapping */
};

struct sbp2_orb {
	struct sbp2_mapping data_map;

	struct ieee1394_abuf cmd;
	struct ieee1394_abuf data;
	struct ieee1394_abuf resp;

	struct sbp2_pagetable *pt;

	struct sbp2_status status;
	u_int8_t status_rec;
	u_int8_t ack;
	u_int8_t db;
	u_int8_t dback;

	int state;
	struct sbp2 *sbp2;
	struct sbp2_lun *lun;

	void (*cb)(struct sbp2_status *, void *);
	void *cb_arg;

	struct simplelock orb_lock;
	CIRCLEQ_ENTRY(sbp2_orb) orb_list;
};

struct sbp2_lun {

	u_int8_t ordered;
	u_int8_t dev_type;
	u_int16_t lun;

	u_int32_t cmd_spec_id;
	u_int32_t cmd_set;
	u_int32_t cmd_set_rev;

	u_int16_t loginid;
	u_int16_t reconnect;
	u_int64_t cmdreg;

	int login_flag;
	int state;

	struct ieee1394_abuf command;
	struct ieee1394_abuf doorbell;
	struct ieee1394_abuf status;
	TAILQ_ENTRY(sbp2_lun) lun_list;
};

struct sbp2 {
	struct ieee1394_softc *sc; /* node softc */

	u_int8_t speed;
	u_int32_t maxpayload;

	u_int8_t orb_size;
	u_int8_t orb_timeout;

	u_int16_t max_reconnect;

	u_int32_t cmd_spec_id;
	u_int32_t cmd_set;
	u_int32_t cmd_set_rev;
	u_int32_t firmware_rev;

	u_int64_t uniq_id;
	u_int64_t mgmtreg;

	u_int16_t luncnt;

	struct sbp2_map *map;
	struct simplelock orblist_lock;

	TAILQ_HEAD(, sbp2_lun) luns;
	CIRCLEQ_HEAD(, sbp2_orb) orbs;
};

struct sbp2_cmd {
	u_int8_t cmd[SBP2_MAX_ORB];
	int cmdlen;
	u_char *data;
	int datalen;
	u_int16_t lun;
	int rw;
	void (*cb)(struct sbp2_status *, void *);
	void *cb_arg;
};

int sbp2_match(struct p1212_dir *);
struct sbp2 *sbp2_init(struct ieee1394_softc *, struct p1212_dir *);
void sbp2_free(struct sbp2 *);
void *sbp2_runcmd(struct sbp2 *, struct sbp2_cmd *);
void *sbp2_abort(void *);
void sbp2_reset_lun(struct sbp2 *, u_int16_t);

#endif /* _DEV_IEEE1394_SBP2VAR_H */
