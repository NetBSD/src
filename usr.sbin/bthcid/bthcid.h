/*	$NetBSD: bthcid.h,v 1.1 2006/06/19 15:44:56 gdamore Exp $	*/

/*-
 * Copyright (c) 2006 Itronix Inc.
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
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _BTHCID_H_
#define _BTHCID_H_	1

/*
 * Client PIN Request packet
 */
typedef struct {
	bdaddr_t	laddr;			/* local address */
	bdaddr_t	raddr;			/* remote address */
	uint8_t		time;			/* validity (seconds) */
} __attribute__ ((packed)) client_pin_request_t;

/*
 * Client PIN Response packet
 */
typedef struct {
	bdaddr_t	laddr;			/* local address */
	bdaddr_t	raddr;			/* remote address */
	uint8_t		pin[HCI_PIN_SIZE];	/* PIN */
} __attribute__ ((packed)) client_pin_response_t;

/*
 * Default socket name
 */
#define BTHCID_SOCKET_NAME	"/var/run/bthcid"

/*
 * The rest is bthcid daemon internals
 */

#ifdef _BTHCID_

extern	const	char	*key_file;
extern	const	char	*config_file;

/* config.c */
uint8_t		*lookup_pin		(bdaddr_t *, bdaddr_t *);
uint8_t		*lookup_key		(bdaddr_t *, bdaddr_t *);
void		 save_key		(bdaddr_t *, bdaddr_t *, uint8_t *);

/* client.c */
int		 init_control		(const char *, mode_t);
int		 send_client_request	(bdaddr_t *, bdaddr_t *, int);
uint8_t		*lookup_item		(bdaddr_t *, bdaddr_t *);

/* hci.c */
int		 init_hci		(bdaddr_t *);
int		 send_pin_code_reply	(int, struct sockaddr_bt *, bdaddr_t *, uint8_t *);

#endif	/* _BTHCID_ */
#endif	/* _BTHCID_H_ */
