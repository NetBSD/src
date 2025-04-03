/*	$NetBSD: if_gscanreg.h,v 1.1 2025/04/03 16:33:48 bouyer Exp $	*/

/*
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Manuel Bouyer.
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

/* control messages */
#define GSCAN_SET_HOST_FORMAT		0 /* out */

struct gscan_set_hf {
	uint32_t hf_byte_order; /* set to 0x0000beef */
} __packed;

#define GSCAN_SET_BITTIMING		1 /* out */

struct gscan_bt {
	uint32_t bt_prop_seg;
	uint32_t bt_phase_seg1;
	uint32_t bt_phase_seg2;
	uint32_t bt_swj;
	uint32_t bt_brp;
} __packed;

#define GSCAN_SET_MODE			2 /* out */

struct gscan_set_mode {
	uint32_t mode_mode;
#define MODE_RESET 0
#define MODE_START 1
	uint32_t mode_flags;
#define FLAGS_LISTEN_ONLY	0x01
#define FLAGS_LOOPBACK		0x02
#define FLAGS_TRIPLE_SAMPLE	0x04
#define FLAGS_ONE_SHOT		0x08
} __packed;

#define GSCAN_BERR			3 /* ??? */
#define GSCAN_GET_BT_CONST		4 /* in  */

struct gscan_bt_const {
	uint32_t btc_features;
#define FEAT_LISTEN_ONLY	0x01
#define FEAT_LOOPBACK		0x02
#define FEAT_TRIPLE_SAMPLE	0x04
#define FEAT_ONE_SHOT		0x08
#define FEAT_HW_TS		0x10
#define FEAT_IDENTIFY		0x20
	uint32_t btc_fclk;
	uint32_t btc_tseg1_min;
	uint32_t btc_tseg1_max;
	uint32_t btc_tseg2_min;
	uint32_t btc_tseg2_max;
	uint32_t btc_swj_max;
	uint32_t btc_brp_min;
	uint32_t btc_brp_max;
	uint32_t btc_brp_inc;
} __packed;

#define GSCAN_GET_DEVICE_CONFIG		5 /* in  */

struct gscan_config {
	uint8_t _pad[3];
	uint8_t conf_count;
	uint32_t sw_version;
	uint32_t hw_version;
} __packed;

#define GSCAN_TIMESTAMP			6 /* ??? */
#define GSCAN_SET_IDENTIFY		7 /* out */

struct gscan_ident {
	uint32_t ident_state; /* 0 = LED off, 1 = LED on */
} __packed;

/*
 * CAN frame on in and out endpoints */
struct gscan_frame {
	uint32_t gsframe_echo_id;
	uint32_t gsframe_can_id;
	uint8_t  gsframe_can_dlc;
	uint8_t  gsframe_channel;
	uint8_t  gsframe_flags;
	uint8_t  _res;
	uint8_t  gsframe_can_data[8];
} __packed;

#define GSFRAME_FLAG_OVER 0x01
