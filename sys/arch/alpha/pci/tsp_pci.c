/* $NetBSD: tsp_pci.c,v 1.9.12.1 2017/12/03 11:35:46 jdolecek Exp $ */

/*-
 * Copyright (c) 1999 by Ross Harvey.  All rights reserved.
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
 *	This product includes software developed by Ross Harvey.
 * 4. The name of Ross Harvey may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ROSS HARVEY ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURP0SE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ROSS HARVEY BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: tsp_pci.c,v 1.9.12.1 2017/12/03 11:35:46 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>

#include <alpha/pci/tsreg.h>
#include <alpha/pci/tsvar.h>

#define tsp_pci() { Generate ctags(1) key. }

void		tsp_attach_hook(device_t, device_t,
		    struct pcibus_attach_args *);
int		tsp_bus_maxdevs(void *, int);
pcitag_t	tsp_make_tag(void *, int, int, int);
void		tsp_decompose_tag(void *, pcitag_t, int *, int *,
		    int *);
pcireg_t	tsp_conf_read(void *, pcitag_t, int);
void		tsp_conf_write(void *, pcitag_t, int, pcireg_t);

void
tsp_pci_init(pci_chipset_tag_t pc, void *v)
{
	pc->pc_conf_v = v;
	pc->pc_attach_hook = tsp_attach_hook;
	pc->pc_bus_maxdevs = tsp_bus_maxdevs;
	pc->pc_make_tag = tsp_make_tag;
	pc->pc_decompose_tag = tsp_decompose_tag;
	pc->pc_conf_read = tsp_conf_read;
	pc->pc_conf_write = tsp_conf_write;
}

void
tsp_attach_hook(device_t parent, device_t self, struct pcibus_attach_args *pba)
{
}

int
tsp_bus_maxdevs(void *cpv, int busno)
{
	return 32;
}

pcitag_t
tsp_make_tag(void *cpv, int b, int d, int f)
{
	return b << 16 | d << 11 | f << 8;
}

void
tsp_decompose_tag(void *cpv, pcitag_t tag, int *bp, int *dp, int *fp)
{
	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x7;
}
/*
 * Tsunami makes this a lot easier than it used to be, automatically
 * generating type 0 or type 1 cycles, and quietly returning -1 with
 * no errors on unanswered probes.
 */
pcireg_t
tsp_conf_read(void *cpv, pcitag_t tag, int offset)
{
	pcireg_t *datap, data;
	struct tsp_config *pcp = cpv;

	if ((unsigned int)offset >= PCI_CONF_SIZE)
		return (pcireg_t) -1;

	datap = S_PAGE(pcp->pc_iobase | P_PCI_CONFIG | tag | (offset & ~3));
	alpha_mb();
	data = *datap;
	alpha_mb();
	return data;
}

void
tsp_conf_write(void *cpv, pcitag_t tag, int offset, pcireg_t data)
{
	pcireg_t *datap;
	struct tsp_config *pcp = cpv;

	if ((unsigned int)offset >= PCI_CONF_SIZE)
		return;

	datap = S_PAGE(pcp->pc_iobase | P_PCI_CONFIG | tag | (offset & ~3));
	alpha_mb();
	*datap = data;
	alpha_mb();
}

#define NTH_STR(n, ...) ((const char *[]){ __VA_ARGS__ }[n])

void
tsp_print_error(unsigned int indent, unsigned long p_error)
{
	char buf[40];

	if (PER_INV(p_error)) {
		IPRINTF(indent, "data invalid\n");
		return;
	}

	if (!PER_ERR(p_error))
		return;

	snprintb(buf, 40,
		 "\177\20"
		 "b\0Error lost\0"
		 "b\1PCI SERR#\0"
		 "b\2PCI PERR#\0"
		 "b\3Delayed completion retry timeout\0"
		 "b\4Invalid S/G page table entry\0"
		 "b\5Address parity error\0"
		 "b\6Target abort\0"
		 "b\7PCI read data parity error\0"
		 "b\10no PCI DEVSEL#\0"
		 "b\11unknown\0"
		 "b\12Uncorrectable ECC\0"
		 "b\13Correctable ECC\0",
		 PER_ERR(p_error));
	IPRINTF(indent, "error    = %s\n", buf);
	
	if (PER_ECC(p_error)) {
		IPRINTF(indent, "address  = 0x%09lx\n", PER_SADR(p_error));
		IPRINTF(indent, "command  = 0x%lx<%s>\n", PER_CMD(p_error),
			NTH_STR(PER_CMD(p_error) & 0x3,
				"DMA read", "DMA RMW", "?", "S/G read"));
		IPRINTF(indent, "syndrome = 0x%02lx\n", PER_SYN(p_error));
	} else {
		IPRINTF(indent, "address  = 0x%08lx, 0x%lx<%s>\n",
			PER_PADR(p_error), PER_TRNS(p_error),
			NTH_STR(PER_TRNS(p_error), "No DAC", "DAC SG Win3",
				"Monster Window", "Monster Window"));
		IPRINTF(indent, "command  = 0x%lx<%s>\n", PER_CMD(p_error),
			NTH_STR(PER_CMD(p_error),
				"PCI IACK", "PCI special cycle",
				"PCI I/O read", "PCI I/O write", "?",
				"PCI PTP write", "PCI memory read",
				"PCI memory write", "PCI CSR write",
				"?", "?", "?", "?", "?", "?", "?"));
	}
}
