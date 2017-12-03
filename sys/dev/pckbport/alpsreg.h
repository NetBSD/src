/* $NetBSD: alpsreg.h,v 1.1.4.2 2017/12/03 11:37:30 jdolecek Exp $ */

/*-
 * Copyright (c) 2017 Ryo ONODERA <ryo@tetera.org>
 * Copyright (c) 2008 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
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

#ifndef _DEV_PCKBCPORT_ALPSREG_H
#define _DEV_PCKBCPORT_ALPSREG_H

#include <dev/pckbport/pmsreg.h>

#define ALPS_PROTO_V2 (2)
#define ALPS_PROTO_V7 (7)

struct alps_nibble_command_data {
	uint8_t command;
	uint8_t data;
	int sendparam;
	int recieve;
};

struct alps_nibble_command_data alps_v7_nibble_command_data_arr[] = {
	{ PMS_SET_REMOTE_MODE,	   0, 0, 0 }, /* 0 F0 */
	{ PMS_SET_DEFAULTS,	   0, 0, 0 }, /* 1 F6 */
	{ PMS_SET_SCALE21,	   0, 0, 0 }, /* 2 E7 */
	{ PMS_SET_SAMPLE,	0x0a, 1, 0 }, /* 3 F3 */
	{ PMS_SET_SAMPLE,	0x14, 1, 0 }, /* 4 F3 */
	{ PMS_SET_SAMPLE,	0x28, 1, 0 }, /* 5 F3 */
	{ PMS_SET_SAMPLE,	0x3c, 1, 0 }, /* 6 F3 */
	{ PMS_SET_SAMPLE,	0x50, 1, 0 }, /* 7 F3 */
	{ PMS_SET_SAMPLE,	0x64, 1, 0 }, /* 8 F3 */
	{ PMS_SET_SAMPLE,	0xc8, 1, 0 }, /* 9 F3 */
	{ PMS_SEND_DEV_ID,	   0, 0, 1 }, /* a F2 */
	{ PMS_SET_RES,		0x00, 1, 0 }, /* b E8 */
	{ PMS_SET_RES,		0x01, 1, 0 }, /* c E8 */
	{ PMS_SET_RES,		0x02, 1, 0 }, /* d E8 */
	{ PMS_SET_RES,		0x03, 1, 0 }, /* e E8 */
	{ PMS_SET_SCALE11,	   0, 0, 0 }, /* f E6 */
};

#define ALPS_V7_PACKETID_UNKNOWN	0
#define ALPS_V7_PACKETID_TWOFINGER	1
#define ALPS_V7_PACKETID_MULTIFINGER	2
#define ALPS_V7_PACKETID_NEWPACKET	3
#define ALPS_V7_PACKETID_IDLE		4

#endif /* !_DEV_PCKBCPORT_ALPSREG_H */
