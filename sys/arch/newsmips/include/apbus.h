/*	$NetBSD: apbus.h,v 1.1 1999/12/22 05:53:21 tsubai Exp $	*/

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
	int			apbc_ctlno;
	int			apbc_mu;
	int			apbc_unknown2;
	void			*apbc_sladdr;

	int			apbc_unknown4;
	int			apbc_hwbase;
	char			*apbc_softc;
	void			*apbc_ent7;

	int			apbc_unknown8;
	int			apbc_sl;

	struct apbus_ctl	*apbc_slave0;
	int			apbc_slave0no;
	int			apbc_unknown12;
	int			apbc_unknown13;

	struct apbus_ctl	*apbc_slave1;
	int			apbc_slave1no;
	int			apbc_unknown16;
	int			apbc_unknown17;

	struct apbus_ctl	*apbc_slave2;
	int			apbc_slave2no;
	int			apbc_unknown20;
	int			apbc_unknown21;

	struct apbus_ctl	*apbc_parent;
	int			apbc_parentno;
	int			apbc_unknown24;
	int			apbc_unknown25;

	struct apbus_ctl	*apbc_link;
	int			apbc_unknown27;
	int			apbc_unknown28;
	int			apbc_unknown29;
	int			apbc_unknown30;
	int			apbc_unknown31;
};

struct apbus_device {
	char			*apbd_name;
	char			*apbd_vendor;
	unsigned int		apbd_atr;
#define APBD_CHAR	0x00000001
#define APBD_STDIN	0x00000002
#define APBD_STDOUT	0x00000004
	unsigned int		apbd_rev;
	void*			apbd_driver;
	struct {
		int		(*func0)();
		int		(*func1)();
		int		(*func2)();
		int		(*func3)();
		int		(*func4)();
		int		(*func5)();
		int		(*func6)();
		int		(*func7)();
		int		(*func8)();
		int		(*func9)();
		int		(*func10)();
		int		(*func11)();
		int		(*func12)();
		int		(*func13)();
		int		(*func14)();
		int		(*func15)();
	} apbd_call;
	struct apbus_ctl	*apbd_ctl;
	struct apbus_device	*apbd_link;
	unsigned int		apbd_unknwon;
};

struct apbus_bus {
	unsigned int		apbb_no;
	void			*apbb_unknown1;
	void			*apbb_unknown2;
	void			*apbb_unknown3;
	struct apbus_bus	*apbb_link;
	unsigned int		apbb_unknown5;
	unsigned int		apbb_unknown6;
	unsigned int		apbb_unknown7;
	unsigned int		apbb_unknown8;
};

struct apbus_vector {
	unsigned short	state;
	unsigned short	mask;
	unsigned int	(*handler)();
};

struct apbus_sysinfo {
	int			apbsi_revision;
	int			(*apbsi_call)();
	unsigned int		apbsi_error1;
	void *			apbsi_progstart;

	void *			apbsi_progend;
	struct apbus_device	*apbsi_dev;
	struct apbus_bus	*apbsi_bus;
	unsigned int		apbsi_error2;

	unsigned int		apbsi_basel;
	unsigned int		apbsi_baseh;
	unsigned int		apbsi_memsize;
	unsigned int		apbsi_tmpsize;

	unsigned int		apbsi_freesize;
	unsigned char		*apbsi_sram;
	void			*apbsi_io;
	void			*apbsi_alloc;

	void			*apbsi_cmdtable;
	unsigned int		apbsi_none0;
	struct apbus_vector	apbsi_vector[8];
	void			(*apbsi_trap)();
	unsigned int		apbsi_romversion;
	unsigned char		apbsi_none1[112];
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
