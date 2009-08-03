/*	$NetBSD: bluetooth.h,v 1.4 2009/08/03 15:59:42 plunky Exp $	*/

/*-
 * Copyright (c) 2001-2009 Maksim Yevmenkin <m_evmenkin@yahoo.com>
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
 * $FreeBSD: src/lib/libbluetooth/bluetooth.h,v 1.5 2009/04/22 15:50:03 emax Exp $
 */

#ifndef _BLUETOOTH_H_
#define _BLUETOOTH_H_

#include <sys/types.h>
#include <sys/endian.h>
#include <sys/socket.h>

#include <netbt/bluetooth.h>
#include <netbt/hci.h>
#include <netbt/l2cap.h>

#include <netdb.h>
#include <stdio.h>
#include <time.h>

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

/*
 * Bluetooth device access API
 */

struct bt_devinfo {
	char		devname[HCI_DEVNAME_SIZE];
	int		enabled;	/* device is enabled */

	/* device information */
	bdaddr_t	bdaddr;
	uint8_t		features[HCI_FEATURES_SIZE];
	uint16_t	acl_size;	/* max ACL data size */
	uint16_t	acl_pkts;	/* total ACL packet buffers */
	uint16_t	sco_size;	/* max SCO data size */
	uint16_t	sco_pkts;	/* total SCO packet buffers */

	/* flow control */
	uint16_t	cmd_free;	/* available CMD packet buffers */
	uint16_t	acl_free;	/* available ACL packet buffers */
	uint16_t	sco_free;	/* available SCO packet buffers */

	/* statistics */
	uint32_t	cmd_sent;
	uint32_t	evnt_recv;
	uint32_t	acl_recv;
	uint32_t	acl_sent;
	uint32_t	sco_recv;
	uint32_t	sco_sent;
	uint32_t	bytes_recv;
	uint32_t	bytes_sent;

	/* device settings */
	uint16_t	link_policy_info;
	uint16_t	packet_type_info;
	uint16_t	role_switch_info;
};

struct bt_devreq {
	uint16_t	opcode;
	uint8_t		event;
	void		*cparam;
	size_t		clen;
	void		*rparam;
	size_t		rlen;
};

struct bt_devfilter {
	struct hci_filter	packet_mask;
	struct hci_filter	event_mask;
};

struct bt_devinquiry {
	bdaddr_t        bdaddr;
	uint8_t         pscan_rep_mode;
	uint8_t         pscan_period_mode;
	uint8_t         dev_class[3];
	uint16_t        clock_offset;
	int8_t          rssi;
	uint8_t         data[240];
};

/* bt_devopen() flags */
#define	BTOPT_DIRECTION		(1 << 0)
#define	BTOPT_TIMESTAMP		(1 << 1)

/* compatibility */
#define	bt_devclose(s)		close(s)

typedef int (bt_devenum_cb_t)(int, const struct bt_devinfo *, void *);

int	bt_devaddr(const char *, bdaddr_t *);
int	bt_devname(char *, const bdaddr_t *);
int	bt_devopen(const char *, int);
ssize_t	bt_devsend(int, uint16_t, void *, size_t);
ssize_t	bt_devrecv(int, void *, size_t, time_t);
int	bt_devreq(int, struct bt_devreq *, time_t);
int	bt_devfilter(int, const struct bt_devfilter *, struct bt_devfilter *);
void	bt_devfilter_pkt_set(struct bt_devfilter *, uint8_t);
void	bt_devfilter_pkt_clr(struct bt_devfilter *, uint8_t);
int	bt_devfilter_pkt_tst(const struct bt_devfilter *, uint8_t);
void	bt_devfilter_evt_set(struct bt_devfilter *, uint8_t);
void	bt_devfilter_evt_clr(struct bt_devfilter *, uint8_t);
int	bt_devfilter_evt_tst(const struct bt_devfilter *, uint8_t);
int	bt_devinquiry(const char *, time_t, int, struct bt_devinquiry **);
int	bt_devinfo(const char *, struct bt_devinfo *);
int	bt_devenum(bt_devenum_cb_t *, void *);

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
