/*	$NetBSD: bluetooth.h,v 1.4 2003/07/08 10:06:29 itojun Exp $	*/

/*
 * Copyright (c) 2002, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) and
 * David Sainty (David.Sainty@dtsp.co.nz).
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

struct btframe_buffer;

struct btframe_channel {
	/*
	 * Allocate and transmit a buffer for transmission to
	 * Bluetooth device
	 */
	u_int8_t* (*bt_alloc)(void*, size_t, struct btframe_buffer **);
	int (*bt_send)(void*, struct btframe_buffer *, size_t);
};

struct btframe_methods {
	int (*bt_open)(void *h, int flag, int mode, struct proc *p);
	int (*bt_close)(void *h, int flag, int mode, struct proc *p);

	/* HCI channels */
	struct btframe_channel bt_control;
	struct btframe_channel bt_acldata;
	struct btframe_channel bt_scodata;

	int (*bt_splraise)(void);

	/*
	 * Attempt to activate flow control.  The driver MAY be able
	 * to block certain events, but it isn't guaranteed, and even
	 * blocked events may still be delivered if one was in
	 * progress at the time.
	 *
	 *
	 * In addition, it is possible for an error to occur
	 * unblocking an event.
	 *
	 * Returns bit map of post-operation blocked/unblocked events.
	 */
#define BT_CBBLOCK_EVENT        1
#define BT_CBBLOCK_ACL_DATA     2
#define BT_CBBLOCK_SCO_DATA     4
        unsigned int (*bt_blockcb)(void *, unsigned int);
        unsigned int (*bt_unblockcb)(void *, unsigned int);
};

struct btframe_callback_methods {
	/* Received data */
	void (*bt_recveventdata)(void *, u_int8_t *, size_t);
	void (*bt_recvacldata)(void *, u_int8_t *, size_t);
	void (*bt_recvscodata)(void *, u_int8_t *, size_t);
};

struct bt_attach_args {
	struct btframe_methods const *bt_methods;
	struct btframe_callback_methods const **bt_cb;
	void *bt_handle;
};

int bt_print(void *aux, const char *pnp);

#define splbt spltty
#define IPL_BT IPL_TTY

#define BTGETW(x) (((u_int8_t const*)(x))[0] | ((u_int8_t const*)(x))[1] << 8)

/* HCI interface constants */

/* Maximum event packet length, including header */
#define BTHCI_EVENT_MIN_LEN		2
#define BTHCI_EVENT_MAX_LEN		(0xff + BTHCI_EVENT_MIN_LEN)
#define BTHCI_EVENT_LEN_OFFT		1
#define BTHCI_EVENT_LEN_LENGTH		1

/* Maximum command packet length, including header */
#define BTHCI_COMMAND_MIN_LEN		3
#define BTHCI_COMMAND_MAX_LEN		(0xff + BTHCI_COMMAND_MIN_LEN)
#define BTHCI_COMMAND_LEN_OFFT		2
#define BTHCI_COMMAND_LEN_LENGTH	1

/* Maximum ACL data packet length, including header */
#define BTHCI_ACL_DATA_MIN_LEN		4
#define BTHCI_ACL_DATA_MAX_LEN		(0xffff + BTHCI_ACL_DATA_MIN_LEN)
#define BTHCI_ACL_DATA_LEN_OFFT		2
#define BTHCI_ACL_DATA_LEN_LENGTH	2

/* HCI consumer interface constants */
#define BTHCI_PKTID_COMMAND		1
#define BTHCI_PKTID_ACL_DATA		2
#define BTHCI_PKTID_SCO_DATA		3
#define BTHCI_PKTID_EVENT		4
