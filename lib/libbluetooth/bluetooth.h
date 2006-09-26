/*	$NetBSD: bluetooth.h,v 1.3 2006/09/26 19:18:19 plunky Exp $	*/

/*
 * bluetooth.h
 *
 * Copyright (c) 2001-2003 Maksim Yevmenkin <m_evmenkin@yahoo.com>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: bluetooth.h,v 1.3 2006/09/26 19:18:19 plunky Exp $
 * $FreeBSD: src/lib/libbluetooth/bluetooth.h,v 1.2 2005/03/17 21:39:44 emax Exp $
 */

#ifndef _BLUETOOTH_H_
#define _BLUETOOTH_H_

#include <sys/types.h>
#include <sys/endian.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netbt/bluetooth.h>
#include <netbt/hci.h>
#include <netbt/l2cap.h>
#include <stdio.h>

__BEGIN_DECLS

/*
 * Interface to the outside world
 */

struct hostent *  bt_gethostbyname    (char const *);
struct hostent *  bt_gethostbyaddr    (char const *, socklen_t, int);
struct hostent *  bt_gethostent       (void);
void              bt_sethostent       (int);
void              bt_endhostent       (void);

struct protoent * bt_getprotobyname   (char const *);
struct protoent * bt_getprotobynumber (int);
struct protoent * bt_getprotoent      (void);
void              bt_setprotoent      (int);
void              bt_endprotoent      (void);

char const *      bt_ntoa             (bdaddr_t const *, char *);
int               bt_aton             (char const *, bdaddr_t *);

int               bt_devaddr          (const char *, bdaddr_t *);
int               bt_devname          (char *, const bdaddr_t *);

/*
 * bthcid(8) PIN Client API
 */

/* Client PIN Request packet */
typedef struct {
	bdaddr_t	laddr;			/* local address */
	bdaddr_t	raddr;			/* remote address */
	uint8_t		time;			/* validity (seconds) */
} __attribute__ ((packed)) bthcid_pin_request_t;

/* Client PIN Response packet */
typedef struct {
	bdaddr_t	laddr;			/* local address */
	bdaddr_t	raddr;			/* remote address */
	uint8_t		pin[HCI_PIN_SIZE];	/* PIN */
} __attribute__ ((packed)) bthcid_pin_response_t;

/* Default socket name */
#define BTHCID_SOCKET_NAME	"/var/run/bthcid"

#ifdef COMPAT_BLUEZ
/*
 * Linux BlueZ compatibility
 */

#define	bacmp(ba1, ba2)	memcmp((ba1), (ba2), sizeof(bdaddr_t))
#define	bacpy(dst, src)	memcpy((dst), (src), sizeof(bdaddr_t))
#define ba2str(ba, str)	bt_ntoa((ba), (str))
#define str2ba(str, ba)	(bt_aton((str), (ba)) == 1 ? 0 : -1)

#define htobs(x)	htole16(x)
#define htobl(x)	htole32(x)
#define btohs(x)	le16toh(x)
#define btohl(x)	le32toh(x)

#define bt_malloc(n)	malloc(n)
#define bt_free(p)	free(p)

#endif	/* COMPAT_BLUEZ */

__END_DECLS

#endif /* ndef _BLUETOOTH_H_ */
