/*	$NetBSD: clvm.h,v 1.1.1.2 2009/12/02 00:25:39 haad Exp $	*/

/*
 * Copyright (C) 2002-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU General Public License v.2.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* Definitions for CLVMD server and clients */

/*
 * The protocol spoken over the cluster and across the local socket.
 */

#ifndef _CLVM_H
#define _CLVM_H

struct clvm_header {
	uint8_t  cmd;	        /* See below */
	uint8_t  flags;	        /* See below */
	uint16_t xid;	        /* Transaction ID */
	uint32_t clientid;	/* Only used in Daemon->Daemon comms */
	int32_t  status;	/* For replies, whether request succeeded */
	uint32_t arglen;	/* Length of argument below. 
				   If >1500 then it will be passed 
				   around the cluster in the system LV */
	char node[1];		/* Actually a NUL-terminated string, node name.
				   If this is empty then the command is 
				   forwarded to all cluster nodes unless 
				   FLAG_LOCAL is also set. */
	char args[1];		/* Arguments for the command follow the 
				   node name, This member is only
				   valid if the node name is empty */
} __attribute__ ((packed));

/* Flags */
#define CLVMD_FLAG_LOCAL        1	/* Only do this on the local node */
#define CLVMD_FLAG_SYSTEMLV     2	/* Data in system LV under my node name */
#define CLVMD_FLAG_NODEERRS     4       /* Reply has errors in node-specific portion */

/* Name of the local socket to communicate between libclvm and clvmd */
//static const char CLVMD_SOCKNAME[]="/var/run/clvmd";
static const char CLVMD_SOCKNAME[] = "\0clvmd";

/* Internal commands & replies */
#define CLVMD_CMD_REPLY    1
#define CLVMD_CMD_VERSION  2	/* Send version around cluster when we start */
#define CLVMD_CMD_GOAWAY   3	/* Die if received this - we are running 
				   an incompatible version */
#define CLVMD_CMD_TEST     4	/* Just for mucking about */

#define CLVMD_CMD_LOCK              30
#define CLVMD_CMD_UNLOCK            31

/* Lock/Unlock commands */
#define CLVMD_CMD_LOCK_LV           50
#define CLVMD_CMD_LOCK_VG           51
#define CLVMD_CMD_LOCK_QUERY	    52

/* Misc functions */
#define CLVMD_CMD_REFRESH	    40
#define CLVMD_CMD_GET_CLUSTERNAME   41
#define CLVMD_CMD_SET_DEBUG	    42
#define CLVMD_CMD_VG_BACKUP	    43
#endif
