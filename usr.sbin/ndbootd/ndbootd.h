/*	$NetBSD: ndbootd.h,v 1.3 2002/04/09 02:39:14 thorpej Exp $	*/

/* ndbootd.h - header file for the Sun Network Disk (nd) daemon: */

/*
 * Copyright (c) 2001 Matthew Fredette.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. All advertising materials mentioning features or use of this software
 *      must display the following acknowledgement:
 *        This product includes software developed by Matthew Fredette.
 *   4. The name of Matthew Fredette may not be used to endorse or promote
 *      products derived from this software without specific prior written
 *      permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/* <<Header: /data/home/fredette/project/THE-WEIGHT-CVS/ndbootd/ndbootd.h,v 1.3 2001/05/22 13:13:20 fredette Exp >> */

/*
 * <<Log: ndbootd.h,v >>
 * Revision 1.3  2001/05/22 13:13:20  fredette
 * Ran indent(1) with NetBSD's KNF-approximating profile.
 *
 * Revision 1.2  2001/01/31 17:35:16  fredette
 * Now include param.h.
 *
 * Revision 1.1  2001/01/29 15:12:13  fredette
 * Added.
 *
 */

#ifndef _NDBOOTD_H
#define _NDBOOTD_H

#if 0
static const char _ndbootd_h_rcsid[] = "<<Id: ndbootd.h,v 1.3 2001/05/22 13:13:20 fredette Exp >>";
#endif

/* includes: */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif				/* HAVE_CONFIG_H */
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <net/if.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#if defined(HAVE_SYS_SOCKIO_H)
#include <sys/sockio.h>
#elif defined(HAVE_SYS_SOCKETIO_H)	/* HAVE_SYS_SOCKIO_H,
					 * HAVE_SYS_SOCKETIO_H */
#include <sys/socketio.h>
#endif				/* HAVE_SYS_SOCKETIO_H */
#include <sys/ioctl.h>
#ifdef HAVE_IOCTLS_H
#include <ioctls.h>
#endif				/* HAVE_IOCTLS_H */
#ifdef HAVE_NET_IF_ETHER_H
#include <net/if_ether.h>
#endif				/* HAVE_NET_IF_ETHER_H */
#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif				/* HAVE_NET_ETHERNET_H */
#include <netinet/ip.h>
#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif				/* HAVE_NET_IF_DL_H */
#include <arpa/inet.h>

/* macros: */
#ifdef __STDC__
#define _NDBOOTD_P(x) x
#else				/* !__STDC__ */
#define _NDBOOTD_P(x)
#endif				/* !__STDC__ */
#undef FALSE
#undef TRUE
#define FALSE (0)
#define TRUE (!FALSE)
#ifndef HAVE_STRERROR
#define strerror(e) ((e) < sys_nerr ? sys_errlist[e] : "unknown error")
#endif				/* !HAVE_STRERROR */
#ifndef MAX
#define MAX(a, b) ((a) < (b) ? (a) : (b))
#endif				/* !MAX */
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif				/* !MIN */
#if 1
#define _NDBOOTD_DO_DEBUG
#endif
#ifdef _NDBOOTD_DO_DEBUG
#define _NDBOOTD_DEBUG(x) do { if (_ndbootd_debug) { FILE *fp; int saved_errno; fp = stderr; saved_errno = errno; fprintf(fp, "%s: ", _ndbootd_argv0); errno = saved_errno; fprintf x ; fputc('\n', fp); errno = saved_errno; } } while(0)
#else				/* !_NDBOOTD_DO_DEBUG */
#define _NDBOOTD_DEBUG(x)
#endif				/* !_NDBOOTD_DO_DEBUG */
#define NDBOOTD_PID_FILE "/var/run/ndbootd.pid"

#define NDBOOTD_OFFSETOF(t, m) (((char *) &(((t *) NULL)-> m)) - ((char *) ((t *) NULL)))

#define NDBOOT_OP_READ      (0x01)
#define NDBOOT_OP_WRITE     (0x02)
#define NDBOOT_OP_ERROR     (0x03)
#define NDBOOT_OP_MASK      (0x07)
#define NDBOOT_OP_FLAG_WAIT (1 << 3)
#define NDBOOT_OP_FLAG_DONE (1 << 4)
#define NDBOOT_MAX_PACKET_DATA (1024)
#define NDBOOT_MAX_BYTE_COUNT  (63 * 1024)
#define NDBOOT_WINDOW_SIZE_DEFAULT (6)
#define NDBOOT_BSIZE        (512)
#define NDBOOT_MINOR_NDP0   (0x40)
#undef IPPROTO_ND
#define IPPROTO_ND 77

/* structures: */

/* our network interface: */
struct ndbootd_interface {

	/* the interface: */
	struct ifreq *ndbootd_interface_ifreq;

	/* our Ethernet address: */
	u_int8_t ndbootd_interface_ether[ETHER_ADDR_LEN];

	/* the socket for the interface: */
	int ndbootd_interface_fd;

	/* private data for the raw interface: */
	void *_ndbootd_interface_raw_private;
};
/* the Sun Network Disk (nd) packet format: */
struct ndboot_packet {

	/* the operation code: */
	u_int8_t ndboot_packet_op;

	/* the minor device: */
	u_int8_t ndboot_packet_minor;

	/* any error: */
	int8_t ndboot_packet_error;

	/* the disk version number: */
	int8_t ndboot_packet_disk_version;

	/* the sequence number: */
	int32_t ndboot_packet_sequence;

	/* the disk block number: */
	int32_t ndboot_packet_block_number;

	/* the byte count: */
	int32_t ndboot_packet_byte_count;

	/* the residual byte count: */
	int32_t ndboot_packet_residual_byte_count;

	/* the current byte offset: */
	int32_t ndboot_packet_current_byte_offset;

	/* the current byte count: */
	int32_t ndboot_packet_current_byte_count;
};

/* prototypes: */
int ndbootd_raw_open _NDBOOTD_P((struct ndbootd_interface *));
int ndbootd_raw_read _NDBOOTD_P((struct ndbootd_interface *, void *, size_t));
int ndbootd_raw_write _NDBOOTD_P((struct ndbootd_interface *, void *, size_t));

#endif				/* !_NDBOOTD_H */
