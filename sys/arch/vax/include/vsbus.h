/*	$NetBSD: vsbus.h,v 1.6 1999/02/02 18:37:22 ragge Exp $ */
/*
 * Copyright (c) 1996 Ludd, University of Lule}, Sweden.
 * All rights reserved.
 *
 * This code is derived from software contributed to Ludd by Bertram Barth.
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
 *      This product includes software developed at Ludd, University of 
 *      Lule}, Sweden and its contributors.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Generic definitions for the (virtual) vsbus. contains common info
 * used by all VAXstations.
 */
struct confargs {
	char	ca_name[16];		/* device name */
	int	ca_intslot;		/* device interrupt-slot */
	int	ca_intpri;		/* device interrupt "priority" */
	int	ca_intvec;		/* interrup-vector offset */
	int	ca_intbit;		/* bit in interrupt-register */
	int	ca_ioaddr;		/* device hardware I/O address */

	int	ca_aux1;		/* additional info (DMA, etc.) */
	int	ca_aux2;
	int	ca_aux3;
	int	ca_aux4;
	int	ca_aux5;
	int	ca_aux6;
	int	ca_aux7;
	int	ca_aux8;

#define ca_recvslot	ca_intslot	/* DC/DZ: Receiver configuration */
#define ca_recvpri	ca_intpri
#define ca_recvvec	ca_intvec
#define ca_recvbit	ca_intbit
#define ca_xmitslot	ca_aux1		/* DC/DZ: transmitter configuration */
#define ca_xmitpri	ca_aux2		/* DC/DZ:  */
#define ca_xmitvec	ca_aux3
#define ca_xmitbit	ca_aux4
#define ca_dcflags	ca_aux5

#define ca_dareg	ca_aux1		/* SCSI: DMA address register */
#define ca_dcreg	ca_aux2		/* SCSI: DMA byte count register */
#define ca_ddreg	ca_aux3		/* SCSI: DMA transfer direction */
#define ca_dbase	ca_aux4		/* SCSI: DMA buffer address */
#define ca_dsize	ca_aux5		/* SCSI: DMA buffer size */
#define ca_dflag	ca_aux6		/* SCSI: DMA flags (eg. shared) */
#define ca_idval	ca_aux7		/* SCSI: host-ID to use/set */
#define ca_idreg	ca_aux8		/* SCSI: host-ID port register */

#define ca_enaddr	ca_aux1		/* LANCE: Ethernet address in ROM */
#define ca_leflags	ca_aux2
};

struct	vsbus_attach_args {
	int va_type;
};

/*
 * Some chip addresses and constants, same on all VAXstations.
 */
#define NI_ADDR         0x20090000      /* Ethernet address */
#define DZ_CSR          0x200a0000      /* DZ11-compatible chip csr */
#define VS_CLOCK        0x200b0000      /* clock chip address */
#define SCA_REGS        0x200c0000      /* disk device addresses */
#define NI_BASE         0x200e0000      /* LANCE CSRs */
#define NI_IOSIZE       (128 * VAX_NBPG)    /* IO address size */
#define VS_REGS         0x20080000      /* Misc cpu internal regs */

/*
 * Small monochrome graphics framebuffer, present on all machines.
 */
#define	SMADDR		0x30000000
#define	SMSIZE		0x20000		/* Actually 256k, only 128k used */

/*
 * interrupt vector numbers
 */
#define IVEC_BASE       0x20040020
#define IVEC_SR         0x000002C0
#define IVEC_ST         0x000002C4
#define IVEC_NP         0x00000250
#define IVEC_NS         0x00000254
#define IVEC_VF         0x00000244
#define IVEC_VS         0x00000248
#define IVEC_SC         0x000003F8
#define IVEC_DC         0x000003FC

/*
 * Interrupt mask bits.
 */
#define	VS3100_SR	7
#define	VS3100_ST	6
#define	VS3100_NI	5
#define	VS3100_VF	3

#define	VS4000_SR	5
#define	VS4000_ST	4
#define	VS4000_NI	1

caddr_t le_iomem; /* base addr of RAM -- CPU's view */
int	inr_ni, inr_sr, inr_st, inr_vf; /* Interrupt register bit */

void vsbus_intr_enable __P((int));
void vsbus_intr_disable  __P((int));
void vsbus_intr_attach __P((int, void(*)(int), int));

int vsbus_lockDMA __P((struct confargs *));
int vsbus_unlockDMA __P((struct confargs *));

