/* $NetBSD: notifications.h,v 1.1.12.2 2014/08/20 00:05:09 tls Exp $ */

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mihai Chelaru <kefren@NetBSD.org>
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

#ifndef _NOTIFICATIONS_H_
#define _NOTIFICATIONS_H_

/* Notifications codes from RFC5036 3.9 - Status code summary */
#define	NOTIF_SUCCESS			0x00000000
#define	NOTIF_BAD_LDP_ID		0x00000001
#define	NOTIF_BAD_LDP_VER		0x00000002
#define	NOTIF_BAD_PDU_LEN		0x00000003
#define	NOTIF_UNKNOWN_MESSAGE		0x00000004
#define	NOTIF_BAD_MSG_LEN		0x00000005
#define	NOTIF_UNKNOWN_TLV		0x00000006
#define	NOTIF_BAD_TLV_LEN		0x00000007
#define	NOTIF_MALFORMED_TLV_VALUE	0x00000008
#define	NOTIF_HOLD_TIME_EXPIRED		0x00000009
#define	NOTIF_SHUTDOWN			0x0000000A
#define	NOTIF_LOOP_DETECTED		0x0000000B
#define	NOTIF_UNKNOWN_FEC		0x0000000C
#define	NOTIF_NO_ROUTE			0x0000000D
#define	NOTIF_NO_LABEL_RESOURCES	0x0000000E
#define	NOTIF_LABEL_RESOURCES_AVAIL	0x0000000F
#define	NOTIF_SESSION_REJECTED_NO_HELLO	0x00000010
#define	NOTIF_SESSION_REJECTED_ADV_MODE	0x00000011
#define	NOTIF_SESSION_REJECTED_MAX_PDU	0x00000012
#define	NOTIF_SESSION_REJECTED_LRANGE	0x00000013
#define	NOTIF_KEEP_ALIVE_TIMER_EXPIRED	0x00000014
#define	NOTIF_LABEL_REQUEST_ABORTED	0x00000015
#define	NOTIF_MISSING_MESSAGE		0x00000016
#define	NOTIF_UNSUPPORTED_AF		0x00000017
#define	NOTIF_SESSION_REJECTED_BAD_KEEP	0x00000018
#define	NOTIF_INTERNAL_ERROR		0x00000019

#define	NOTIF_FATAL			0x80000000

static const char	*NOTIF_STR[] __unused =
{
	"Success",
	"Bad LDP ID",
	"Bad LDP Version",
	"Bad PDU Length",
	"Unknown message",
	"Bad message length",
	"Unknown TLV",
	"Bad TLV Length",
	"Malformed TLV Value",
	"Hold time expired",
	"Shutdown",
	"Loop detected",
	"Unknown FEC",
	"No route",
	"No label resources",
	"Label resources available",
	"Session rejected: No hello",
	"Session rejected: Parameters Advertising Mode",
	"Session rejected: Max PDU Length",
	"Session rejected: Label range",
	"Keepalive timer expired",
	"Label request aborted",
	"Missing message",
	"Unsupported Address Family",
	"Session rejected: Bad keepalive time",
	"Internal error"
};

struct notification_tlv* build_notification(uint32_t, uint32_t);
int send_notification(const struct ldp_peer *, uint32_t, uint32_t);

#endif	/* !_NOTIFICATIONS_H_ */
