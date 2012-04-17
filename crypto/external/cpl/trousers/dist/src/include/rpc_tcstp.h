
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2004-2006
 *
 */

#ifndef _RPC_TCSTP_H_
#define _RPC_TCSTP_H_

#include "tcs_tsp.h"

typedef unsigned char  TCSD_PACKET_TYPE;

/* Packet header used for TCSD communication */
struct tcsd_packet_hdr {
	UINT32 packet_size;
	union {
		UINT32 ordinal;
		UINT32 result;
	} u;
	UINT32 num_parms;
	UINT32 type_size;
	UINT32 type_offset;
	UINT32 parm_size;
	UINT32 parm_offset;
} STRUCTURE_PACKING_ATTRIBUTE;

struct tcsd_comm_data {
	BYTE *buf;
	int buf_size;
	struct tcsd_packet_hdr hdr;
} STRUCTURE_PACKING_ATTRIBUTE;

#define TCSD_INIT_TXBUF_SIZE	1024

#endif
