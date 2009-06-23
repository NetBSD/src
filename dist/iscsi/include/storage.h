/* $NetBSD: storage.h,v 1.6 2009/06/23 05:11:46 agc Exp $ */

/*
 * Copyright © 2006 Alistair Crooks.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef STORAGE_H_
#define STORAGE_H_

#include "defs.h"

/* Length of a node address (an IEEE 802 address). */
#define NB_UUID_NODE_LEN		6

/*
 * See also:
 *      http://www.opengroup.org/dce/info/draft-leach-uuids-guids-01.txt
 *      http://www.opengroup.org/onlinepubs/009629399/apdxa.htm
 *
 * A DCE 1.1 compatible source representation of UUIDs.
 */
typedef struct nbuuid_t {
        uint32_t        time_low;
        uint16_t        time_mid;
        uint16_t        time_hi_and_version;
        uint8_t         clock_seq_hi_and_reserved;
        uint8_t         clock_seq_low;
        uint8_t         node[NB_UUID_NODE_LEN];
} nbuuid_t;

void    nbuuid_create(nbuuid_t *, uint32_t *);
void    nbuuid_to_string(nbuuid_t *, char **, uint32_t *);

enum {
	DE_EXTENT,
	DE_DEVICE
};

/* a device can be made up of an extent or another device */
typedef struct disc_de_t {
	int32_t		type;		/* device or extent */
	uint64_t	size;		/* size of underlying extent or device */
	union {
		struct disc_extent_t	*xp;	/* pointer to extent */
		struct disc_device_t	*dp;	/* pointer to device */
	} u;
} disc_de_t;

/* this struct describes an extent of storage */
typedef struct disc_extent_t {
	char		*extent;	/* extent name */
	char		*dev;		/* device associated with it */
	uint64_t	sacred;		/* offset of extent from start of dev */
	uint64_t	len;		/* size of extent */
	int		fd;		/* in-core file descriptor */
	int		used;		/* extent has been used in a device */
} disc_extent_t;

DEFINE_ARRAY(extv_t, disc_extent_t);

/* this struct describes a device */
typedef struct disc_device_t {
	char		*dev;		/* device name */
	int		raid;		/* RAID level */
	uint64_t	off;		/* current offset in device */
	uint64_t	len;		/* size of device */
	uint32_t	size;		/* size of device/extent array */
	uint32_t	c;		/* # of entries in device/extents */
	disc_de_t	*xv;		/* device/extent array */
	int		used;		/* device has been used in a device/target */
} disc_device_t;

DEFINE_ARRAY(devv_t, disc_device_t);

enum {
	TARGET_READONLY = 0x01
};

/* this struct describes an iscsi target's associated features */
typedef struct disc_target_t {
	char		*target;	/* target name */
	disc_de_t	 de;		/* pointer to its device */
	uint16_t	 port;		/* port to listen on */
	char		*mask;		/* mask to export it to */
	uint32_t	 flags;		/* any flags */
	uint16_t	 tsih;		/* target session identifying handle */
	char		*iqn;		/* assigned iqn - can be NULL */
} disc_target_t;

DEFINE_ARRAY(targv_t, disc_target_t);

int read_conf_file(const char *, targv_t *, devv_t *, extv_t *);
void write_pid_file(const char *);

#endif /* !STORAGE_H_ */
