/*	$NetBSD: apbus.h,v 1.2.6.3 2000/12/08 09:28:51 bouyer Exp $	*/

/*-
 * Copyright (C) 1999 SHIMIZU Ryo.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __MACHINE_APBUS__
#define __MACHINE_APBUS__

struct apbus_ctl {
	u_int	apbc_ctlno;
	u_int	apbc_mu;
	u_int	apbc_unknown2;
	void	*apbc_sladdr;

	u_int	apbc_unknown4;
	u_int	apbc_hwbase;
	char	*apbc_softc;
	u_int	*apbc_ent7;

	u_int	apbc_unknown8;
	u_int	apbc_sl;

	struct apbus_ctl *apbc_child0;
	u_int	apbc_child0no[3];

	struct apbus_ctl *apbc_child1;
	u_int	apbc_child1no[3];

	struct apbus_ctl *apbc_child2;
	u_int	apbc_child2no[3];

	struct apbus_ctl *apbc_parent;
	u_int	apbc_parentno[3];

	struct apbus_ctl *apbc_link;
};

struct apbus_dev {
	char	*apbd_name;
	char	*apbd_vendor;
	u_int	apbd_atr;
	u_int	apbd_rev;
	void	*apbd_driver;
	void	*table[16];
	struct apbus_ctl *apbd_ctl;
	struct apbus_dev *apbd_link;
};

struct apbus_sysinfo {
	int	apbsi_revision;
	int	(*apbsi_call)(int, ...);/* apcall entry */
	int	apbsi_errno;		/* errno from apcall? */
	void	*apbsi_bootstart;	/* entry of primary boot */
	void	*apbsi_bootend;
	struct apbus_dev *apbsi_dev;
	struct apbus_bus *apbsi_bus;
	int	apbsi_exterr;		/* ? */

	int	apbsi_pad1[2];
	int	apbsi_memsize;		/* memory size */
	int	apbsi_pad2[24];
	int	apbsi_romversion;
	int	apbsi_pad3[28];
};

/*
 * FYI: result of 'ss -m' command on NEWS5000 rom monitor on my machine...
 *
 * > ss -m
 * Memory use:
 *  diag info:  bf881800
 *    environ:  bf881000
 *     apinfo:  bf880000
 *    sysinfo:  9ff03270	->	struct apbus_sysinfo
 * alloc list:  ffffbff8
 *    max mem:  04000000
 *   free mem:  03ff1678
 *   mem base: 100000000
 *
 */

extern struct apbus_sysinfo *_sip;
void apbus_wbflush __P((void));

#endif /* !__MACHINE_APBUS__ */
