/*	$NetBSD: dtf_comms.c,v 1.1 2002/07/05 13:32:04 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This code provides a very simple/basic interface to communicate with
 * a debug host using the "DTF" protocol.
 *
 * XXX: This is not reliable. Please avoid using it!
 */

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/cacheops.h>

#include <sh5/sh5/dtf_comms.h>

#define DTF_USE_VIRT 1

#define	DTF_CID0			0
#define	DTF_CID0_TAG_OPEN		1
#define	DTF_CID0_PROT_TAG_LEN		1
#define	DTF_CID0_PROT_STATUS_LEN	1
#define	DTF_CID0_PROT_SNAME_MAX_LEN	128
#define	DTF_CID0_PROT_SARGS_MAX_LEN	128
#define	DTF_CID0_STATUS_OK		1

#define	DTF_ASEBRK_DTF		0
#define	DTF_ASEBRK_GETARGS	2

static const char DTF_POSIX_NAME[DTF_CID0_PROT_SNAME_MAX_LEN] = "posix";
static const char DTF_POSIX_ARGS[DTF_CID0_PROT_SARGS_MAX_LEN] = "";

struct dtf_trap {
	u_int32_t	dt_magic;
	u_int32_t	dt_params;
};
static paddr_t trap_p;
static struct dtf_trap *trap_v;

struct dtf_params {
	u_int32_t	dp_desc;
	u_int32_t	dp_cid;
	u_int32_t	dp_direction;
};
static paddr_t params_p;
static struct dtf_params *params_v;
#define	DTF_DIRECTION_TARGET2HOST	0
#define	DTF_DIRECTION_HOST2TARGET	1

static paddr_t packet_p;
static u_int8_t *packet_v;

static paddr_t dtf_frob_p;
static paddr_t dtf_fpfreg_p;
static int dtf_up;
static u_int16_t dtf_posix_cid;

static int dtf_startup(void);
static int dtf_get_args(void);
static int dtf_send_packet(void);
static int dtf_recv_packet(u_int16_t cid);
extern void _dtf_trap(paddr_t fpfreg_p, paddr_t trapbuff, paddr_t dtf_frob_p);

void
dtf_init(paddr_t fpfreg_p, paddr_t frob_p, paddr_t dtf_p, vaddr_t dtf_v)
{

#define	DTF_ROUND(v)	(((v) + 0x3f) & ~0x3f)

	dtf_p = DTF_ROUND(dtf_p + sizeof(struct dtf_trap));
	dtf_v = DTF_ROUND(dtf_v + sizeof(struct dtf_trap));

	trap_p = dtf_p;
	trap_v = (struct dtf_trap *)dtf_v;
	dtf_p = DTF_ROUND(dtf_p + sizeof(struct dtf_trap));
	dtf_v = DTF_ROUND(dtf_v + sizeof(struct dtf_trap));

	params_p = dtf_p;
	params_v = (struct dtf_params *)dtf_v;
	dtf_p = DTF_ROUND(dtf_p + sizeof(struct dtf_params));
	dtf_v = DTF_ROUND(dtf_v + sizeof(struct dtf_params));

	packet_p = dtf_p;
	packet_v = (u_int8_t *)dtf_v;

	dtf_fpfreg_p = fpfreg_p;
	dtf_frob_p = frob_p;

	if (dtf_startup() < 0 || dtf_get_args() < 0)
		return;

	dtf_up = 1;
}

int
dtf_transaction(void *buff, int *len)
{
	u_int16_t rlen, cid;
	u_int8_t *packet;
	int s;

	if (dtf_up == 0)
		return (-1);

	s = splhigh();

	packet = dtf_packword(packet_v, (u_int16_t)*len);
	packet = dtf_packword(packet, dtf_posix_cid);
	memcpy(packet, buff, *len);

	if (dtf_send_packet() < 0) {
		splx(s);
		return (-1);
	}

	splx(s);
	s = splhigh();

	/*
	 * Receive the response
	 */
	if (dtf_recv_packet(dtf_posix_cid) < 0) {
		splx(s);
		return (-1);
	}

	packet = dtf_unpackword(packet_v, &rlen);
	packet = dtf_unpackword(packet, &cid);

	if (rlen > DTF_MAX_PACKET_LEN || cid != dtf_posix_cid) {
		splx(s);
		return (-1);
	}

	if (rlen)
		memcpy(buff, packet, (size_t)rlen);
	*len = (int)rlen;

	splx(s);

	return (0);
}

static int
dtf_startup()
{
	u_int8_t *packet;
	u_int16_t len, cid, newcid;
	u_int16_t status;

	len = DTF_CID0_PROT_TAG_LEN +
	      DTF_CID0_PROT_SNAME_MAX_LEN +
	      DTF_CID0_PROT_SARGS_MAX_LEN;

	packet = dtf_packword(packet_v, len);
	packet = dtf_packword(packet, DTF_CID0);
	*packet++ = DTF_CID0_TAG_OPEN;
	packet = dtf_packbytes(packet, DTF_POSIX_NAME,
	    DTF_CID0_PROT_SNAME_MAX_LEN);
	packet = dtf_packbytes(packet, DTF_POSIX_ARGS,
	    DTF_CID0_PROT_SNAME_MAX_LEN);

	if (dtf_send_packet() < 0 || dtf_recv_packet(DTF_CID0) < 0)
		return (-1);

	packet = dtf_unpackword(packet_v, &len);
	packet = dtf_unpackword(packet, &cid);
	status = *packet++;

	if (len < DTF_CID0_PROT_STATUS_LEN || len > DTF_MAX_PACKET_LEN ||
	    cid != DTF_CID0 || status != DTF_CID0_STATUS_OK)
		return (-1);

	(void) dtf_unpackword(packet, &newcid);

	if (newcid == 0)
		return (-1);

	dtf_posix_cid = newcid;

	return (0);
}

static int
dtf_get_args(void)
{
	u_int32_t len;
	u_int8_t *packet;
#if 0
	int i;
#endif

	trap_v->dt_magic = DTF_ASEBRK_GETARGS;
	trap_v->dt_params = (u_int32_t)params_p;
	__cpu_cache_purge((vaddr_t)trap_v, sizeof(struct dtf_trap));

	params_v->dp_desc = (u_int32_t)packet_p;
	__cpu_cache_purge((vaddr_t)params_v, sizeof(struct dtf_params));

	__cpu_cache_invalidate((vaddr_t)packet_v, DTF_MAX_PACKET_LEN);

	_dtf_trap(dtf_fpfreg_p, trap_p, dtf_frob_p);

	packet = dtf_unpackdword(packet_v, &len);

	if (len > DTF_MAX_PACKET_LEN)
		return (-1);

#if 0
	for (i = 0; i < len; i++, packet++) {
		if (*packet != '\0')
			sh5_bootline[i] = *packet;
		else
			sh5_bootline[i] = ' ';
	}

	sh5_bootline[i] = '\0';
#endif
	return (0);
}

static int
dtf_send_packet(void)
{

	__cpu_cache_purge((vaddr_t)packet_v, DTF_MAX_PACKET_LEN);

	trap_v->dt_magic = DTF_ASEBRK_DTF;
	trap_v->dt_params = (u_int32_t)params_p;
	__cpu_cache_purge((vaddr_t)trap_v, sizeof(struct dtf_trap));

	params_v->dp_desc = (u_int32_t)packet_p;
	params_v->dp_cid = 0;
	params_v->dp_direction = DTF_DIRECTION_TARGET2HOST;
	__cpu_cache_purge((vaddr_t)params_v, sizeof(struct dtf_params));

	_dtf_trap(dtf_fpfreg_p, trap_p, dtf_frob_p);

	return (0);
}

static int
dtf_recv_packet(u_int16_t cid)
{

	trap_v->dt_magic = DTF_ASEBRK_DTF;
	trap_v->dt_params = (u_int32_t)params_p;
	__cpu_cache_purge((vaddr_t)trap_v, sizeof(struct dtf_trap));

	params_v->dp_desc = (u_int32_t)packet_p;
	params_v->dp_cid = cid;
	params_v->dp_direction = DTF_DIRECTION_HOST2TARGET;
	__cpu_cache_purge((vaddr_t)params_v, sizeof(struct dtf_params));

	__cpu_cache_invalidate((vaddr_t)packet_v, DTF_MAX_PACKET_LEN);

	_dtf_trap(dtf_fpfreg_p, trap_p, dtf_frob_p);

	return (0);
}


u_int8_t *
dtf_packbytes(u_int8_t *packet, const void *buff, int len)
{
	memcpy(packet, buff, len);

	return (&packet[len]);
}

u_int8_t *
dtf_packword(u_int8_t *packet, u_int16_t w)
{
	*packet++ = (u_int8_t)(w & 0xff);
	*packet++ = (u_int8_t)(w >> 8);

	return (packet);
}

u_int8_t *
dtf_packdword(u_int8_t *packet, u_int32_t dw)
{
	*packet++ = (u_int8_t)dw;
	*packet++ = (u_int8_t)(dw >> 8);
	*packet++ = (u_int8_t)(dw >> 16);
	*packet++ = (u_int8_t)(dw >> 24);

	return (packet);
}

u_int8_t *
dtf_unpackbytes(u_int8_t *packet, void *buff, int len)
{
	memcpy(buff, packet, len);

	return (&packet[len]);
}

u_int8_t *
dtf_unpackword(u_int8_t *packet, u_int16_t *pw)
{
	u_int16_t w;

	w = (u_int16_t) *packet++;
	w |= ((u_int16_t) *packet++) << 8;

	*pw = w;

	return (packet);
}

u_int8_t *
dtf_unpackdword(u_int8_t *packet, u_int32_t *pdw)
{
	u_int32_t dw;

	dw = (u_int32_t) *packet++;
	dw |= ((u_int32_t) *packet++) << 8;
	dw |= ((u_int32_t) *packet++) << 16;
	dw |= ((u_int32_t) *packet++) << 24;

	*pdw = dw;

	return (packet);
}
