/*	$NetBSD: proxyp.c,v 1.2 2021/08/14 16:14:58 christos Exp $	*/

/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2020 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: proxyp.c,v 1.2 2021/08/14 16:14:58 christos Exp $");

#include "portable.h"
#include "slap.h"

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include <lber_types.h>
#include <ac/string.h>
#include <ac/errno.h>

typedef struct {
	uint8_t  sig[12];	/* hex 0d 0a 0d 0a 00 0d 0a 51 55 49 54 0a */
	uint8_t  ver_cmd;	/* protocol version and command */
	uint8_t  fam;		/* protocol family and address */
	uint16_t len;		/* length of address data */
} proxyp_header;

typedef union {
	struct {	/* for TCP/UDP over IPv4, len = 12 */
		uint32_t src_addr;
		uint32_t dst_addr;
		uint16_t src_port;
		uint16_t dst_port;
	} ip4;
	struct {	/* for TCP/UDP over IPv6, len = 36 */
		uint8_t  src_addr[16];
		uint8_t  dst_addr[16];
		uint16_t src_port;
		uint16_t dst_port;
	} ip6;
	struct {	/* for AF_UNIX sockets, len = 216 */
		uint8_t src_addr[108];
		uint8_t dst_addr[108];
	} unx;
} proxyp_addr;

static const uint8_t proxyp_sig[12] = {
	0x0d, 0x0a, 0x0d, 0x0a, 0x00, 0x0d, 0x0a, 0x51, 0x55, 0x49, 0x54, 0x0a,
};

int
proxyp( ber_socket_t sfd, Sockaddr *from ) {
	proxyp_header pph;
	proxyp_addr ppa;
	char peername[LDAP_IPADDRLEN];
	struct berval peerbv = BER_BVC(peername);
	/* Maximum size of header minus static component size is max option size */
	uint8_t proxyp_options[536 - 16];
	int pph_len;
	int ret;

	peername[0] = '\0';

	do {
		ret = tcp_read( SLAP_FD2SOCK( sfd ), &pph, sizeof(pph) );
	} while ( ret == -1 && errno == EINTR );

	if ( ret == -1 ) {
		char ebuf[128];
		int save_errno = errno;
		Debug( LDAP_DEBUG_ANY, "proxyp(%ld): "
				"header read failed %d (%s)\n",
				(long)sfd, save_errno,
				AC_STRERROR_R( save_errno, ebuf, sizeof(ebuf) ) );
		return 0;
	} else if ( ret != sizeof(pph) ) {
		Debug( LDAP_DEBUG_ANY, "proxyp(%ld): "
				"header read insufficient data %d\n",
				(long)sfd, ret );
		return 0;
	}

	if ( memcmp( pph.sig, proxyp_sig, 12 ) != 0 ) {
		Debug( LDAP_DEBUG_ANY, "proxyp(%ld): "
				"invalid header signature\n", (long)sfd );
		return 0;
	}

	if ( ( pph.ver_cmd & 0xF0 ) != 0x20 ) {
		Debug( LDAP_DEBUG_ANY, "proxyp(%ld): "
				"invalid header version %x\n",
				(long)sfd, pph.ver_cmd & 0xF0 );
		return 0;
	}

	pph_len = ntohs( pph.len );
	if ( ( pph.ver_cmd & 0x0F ) == 0x01 ) { /* PROXY command */
		int addr_len;
		switch ( pph.fam ) {
		case 0x11: /* TCPv4 */
			addr_len = sizeof( ppa.ip4 );
			break;
		case 0x21: /* TCPv6 */
			addr_len = sizeof( ppa.ip6 );
			break;
		default:
			Debug( LDAP_DEBUG_ANY, "proxyp(%ld): "
					"unsupported protocol %x\n",
					(long)sfd, pph.fam );
			return 0;
		}

		if ( pph_len < addr_len ) {
			Debug( LDAP_DEBUG_ANY, "proxyp(%ld): "
					"address length %d too small, expecting %d\n",
					(long)sfd, pph_len, addr_len );
			return 0;
		}

		do {
			ret = tcp_read( SLAP_FD2SOCK (sfd), &ppa, addr_len );
		} while ( ret == -1 && errno == EINTR );

		if ( ret == -1 ) {
			char ebuf[128];
			int save_errno = errno;
			Debug( LDAP_DEBUG_ANY, "proxyp(%ld): "
					"address read failed %d (%s)\n",
					(long)sfd, save_errno,
					AC_STRERROR_R( save_errno, ebuf, sizeof(ebuf) ) );
			return 0;
		} else if ( ret != addr_len ) {
			Debug( LDAP_DEBUG_ANY, "proxyp(%ld): "
					"address read insufficient data, expecting %d, read %d\n",
					(long)sfd, addr_len, ret );
			return 0;
		}

		pph_len -= addr_len;
	}

	switch ( pph.ver_cmd & 0x0F ) {
	case 0x01: /* PROXY command */
		switch ( pph.fam ) {
		case 0x11: /* TCPv4 */
			ldap_pvt_sockaddrstr( from, &peerbv );
			Debug( LDAP_DEBUG_STATS, "proxyp(%ld): via %s\n",
					(long)sfd, peername );

			from->sa_in_addr.sin_family = AF_INET;
			from->sa_in_addr.sin_addr.s_addr = ppa.ip4.src_addr;
			from->sa_in_addr.sin_port = ppa.ip4.src_port;
			break;

		case 0x21: /* TCPv6 */
#ifdef LDAP_PF_INET6
			ldap_pvt_sockaddrstr( from, &peerbv );
			Debug( LDAP_DEBUG_STATS, "proxyp(%ld): via %s\n",
					(long)sfd, peername );
			from->sa_in6_addr.sin6_family = AF_INET6;
			memcpy( &from->sa_in6_addr.sin6_addr, ppa.ip6.src_addr,
					sizeof(ppa.ip6.src_addr) );
			from->sa_in6_addr.sin6_port = ppa.ip6.src_port;
#else
			Debug( LDAP_DEBUG_ANY, "proxyp(%ld): "
					"IPv6 proxied addresses disabled\n",
					(long)sfd );
			return 0;
#endif
			break;
		}

		break;

	case 0x00: /* LOCAL command */
		Debug( LDAP_DEBUG_CONNS, "proxyp(%ld): "
				"local connection, ignoring proxy data\n",
				(long)sfd );
		break;

	default:
		Debug( LDAP_DEBUG_ANY, "proxyp(%ld): invalid command %x\n",
				(long)sfd, pph.ver_cmd & 0x0F );
		return 0;
	}

	/* Clear out any options left in proxy packet */
	if ( pph_len > 0 ) {
		if (pph_len > sizeof( proxyp_options ) ) {
			Debug( LDAP_DEBUG_ANY, "proxyp(%ld): "
					"options size %d too big\n",
					(long)sfd, pph_len );
			return 0;
		}

		do {
			ret = tcp_read( SLAP_FD2SOCK (sfd), &proxyp_options, pph_len );
		} while ( ret == -1 && errno == EINTR );

		if ( ret == -1 ) {
			char ebuf[128];
			int save_errno = errno;
			Debug( LDAP_DEBUG_ANY, "proxyp(%ld): "
					"options read failed %d (%s)\n",
					(long)sfd, save_errno,
					AC_STRERROR_R( save_errno, ebuf, sizeof(ebuf) ) );
			return 0;
		} else if ( ret != pph_len ) {
			Debug( LDAP_DEBUG_ANY, "proxyp(%ld): "
					"options read insufficient data, expecting %d, read %d\n",
					(long)sfd, pph_len, ret );
			return 0;
		}
	}

	return 1;
}
