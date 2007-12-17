/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _RS6000_IOPLANAR_H_
#define _RS6000_IOPLANAR_H_

#include <machine/bus.h>
#include <machine/mca_machdep.h>

/*
 * Driver attach args
 */
struct ioplanar_dev_attach_args {
	bus_space_tag_t idaa_iot;	/* I/O space tag */
	bus_space_tag_t idaa_memt;	/* MEM space tag */
	mca_chipset_tag_t idaa_mc;
	bus_dma_tag_t idaa_dmat;	/* DMA tag */

	uint16_t idaa_devid;		/* devid of the ioplanar */
	int idaa_device;		/* devnum from below defines */
};

/* Master bus */
struct ioplanar_softc {
	struct device sc_dev;
	mca_chipset_tag_t sc_ic;
	bus_space_tag_t sc_iot;
	bus_space_tag_t sc_memt;
	bus_dma_tag_t sc_dmat;
	uint16_t sc_devid;		/* devid of the ioplanar */
};

/*
 * These are the defines for the various devices we might find on an
 * ioplanar.  Because this is not a real bus, we have to basically tell
 * autoconf what we are looking for specifically, so it can find it.  Each
 * ioplanar has a specific set of devices that live on it, and those will
 * appear in a table.
 */

#define IOP_COM0	1
#define IOP_COM1	2
#define IOP_COM0_2	3
#define IOP_COM1_2	4
#define IOP_COM0_3	5
#define IOP_COM1_3	6
#define IOP_COM2_3	7
#define IOP_MOUSE	10
#define IOP_KBD		11
#define IOP_TABLET	12
#define IOP_KBD_2	13
#define IOP_TABLET_2	14
#define IOP_LPD		20
#define IOP_LPD_2	21
#define IOP_FDC		30
#define IOP_FDC_2	31
#define IOP_FDC_3	32

/*
 * The RS6000 defines a set of minimum IO configuration ports.  These are
 * basic services like serial, parallel, scsi, etc.  Per the definition, all
 * machines must respond on these ports for these services.
 */

#define RSI_COM0	0x30
#define RSI_COM0_LEN	8
#define RSI_COM1	0x38
#define RSI_COM1_LEN	8
#define RSI_COM_DMA	0x40
#define RSI_COM_DMA_LEN	1
#define RSI_MOUSE	0x48
#define RSI_MOUSE_LEN	8
#define RSI_KBD		0x50
#define RSI_KBD_LEN	10
#define RSI_FLOPPY	0x60
#define RSI_FLOPPY_LEN	9
#define RSI_TABLET	0x70
#define RSI_TABLEN_LEN	8
#define RSI_PARALLEL	0x78
#define RSI_PARALLEL_LEN	7
#define RSI_SCSI	0x7f
#define RSI_SCSI_LEN	96
#define RSI_LAN		0xf0
#define RSI_LAN_LEN	11

#endif /* _RS6000_IOPLANAR_H_ */
