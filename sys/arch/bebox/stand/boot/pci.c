/*	$NetBSD: pci.c,v 1.3 1999/02/15 04:38:06 sakamoto Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Kazuki Sakamoto.
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

#include <stand.h> 
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#define	PCI_CONF_REG(dev,reg)		\
	*(u_int32_t *)(0x80800000 | (1 << dev) | reg)

u_int32_t
pci_conf_read(dev, reg)
	int dev;
	int reg;
{
	if (dev < 11 || dev > 18)
		return 0xffffffff;
	if (reg < 0 || reg > 255)
		return 0xffffffff;

	return (u_int32_t)bswap32(PCI_CONF_REG(dev, reg));
}

pci_conf_write(dev, reg, data)
	int dev;
	int reg;
	u_int32_t data;
{
	if (dev < 11 || dev > 18)
		return;
	if (reg < 0 || reg > 255)
		return;

	PCI_CONF_REG(dev, reg) = (u_int32_t)bswap32(data);
}

pci_init()
{
	int dev;
	u_int32_t data;

	/*
	 * On board NCR53C810A
	 */
	pci_conf_write(12, 0x10, 0x01000001);
	pci_conf_write(12, 0x14, 0x01000000);
	data = pci_conf_read(12, PCI_INTERRUPT_REG);
	pci_conf_write(12, PCI_INTERRUPT_REG, data & 0xffff00ff | 20 << 8);

	/*
	 * remap Video
	 */
	for (dev = 13; dev < 18; dev++) {

		data = pci_conf_read(dev, PCI_CLASS_REG);

		if (PCI_CLASS(data) == PCI_CLASS_DISPLAY) {

			data = pci_conf_read(dev, PCI_COMMAND_STATUS_REG);

			if (data &
			    (PCI_COMMAND_IO_ENABLE | PCI_COMMAND_MEM_ENABLE)) {
				/*
				 * Initialized PCI Video
				 */
				data = pci_conf_read(dev, PCI_ID_REG);

				if (PCI_VENDOR(data) == PCI_VENDOR_S3) {
					pci_conf_write(dev, 0x10, 0x0);
				} else {
					data = pci_conf_read(dev, 0x10);
					pci_conf_write(dev, 0x10,
						data & 0x0fffffff);

					data = pci_conf_read(dev, 0x14);
					pci_conf_write(dev, 0x14,
						data & 0x0fffffff);
				}
			}
		}
	}
}
