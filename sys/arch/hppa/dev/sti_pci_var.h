/*	$NetBSD: sti_pci_var.h,v 1.2 2025/10/20 09:50:10 macallan Exp $	*/

/*
 * Copyright (c) 2006, 2007 Miodrag Vallat
 *                     2025 Michael Lorenz
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice, this permission notice, and the disclaimer below
 * appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef STI_PCI_VAR_H
#define STI_PCI_VAR_H

int	sti_pci_is_console(struct pci_attach_args *, bus_addr_t *);
int	sti_pci_check_rom(struct sti_softc *, struct pci_attach_args *,
			  bus_space_handle_t *);

#endif
