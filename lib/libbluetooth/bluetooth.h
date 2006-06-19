/*	$NetBSD: bluetooth.h,v 1.1 2006/06/19 15:44:36 gdamore Exp $	*/

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
 * $Id: bluetooth.h,v 1.1 2006/06/19 15:44:36 gdamore Exp $
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

/*
 * Bluetooth remote device config information.
 *
 * Entries will be empty if no value was specified
 * in the configuration file.
 */
typedef struct {
	bdaddr_t	 bdaddr;		/* device address */
	int		 type;			/* device type */
	char		*name;			/* NUL terminated string */
	uint8_t		*pin;			/* uint8_t[HCI_PIN_SIZE] */
	uint8_t		*key;			/* uint8_t[HCI_KEY_SIZE] */
	uint8_t		*hid_descriptor;	/* uint8_t[hid_length] */
	int		 hid_length;
	uint16_t	 control_psm;
	uint8_t		 control_channel;
	uint16_t	 interrupt_psm;
	uint8_t		 reconnect_initiate;
	uint8_t		 normally_connectable;
	uint8_t		 battery_power;
} bt_cfgentry_t;

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

typedef struct	  bt_handle           *bt_handle_t;

bt_handle_t	  bt_openconfig       (const char *);
bt_cfgentry_t *   bt_getconfig        (bt_handle_t, const bdaddr_t *);
int		  bt_eachconfig       (bt_handle_t, void (*func)(bt_cfgentry_t *, void *), void *);
void		  bt_freeconfig       (bt_cfgentry_t *);
int		  bt_closeconfig      (bt_handle_t);
int		  bt_printconfig      (FILE *, bt_cfgentry_t *);

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
