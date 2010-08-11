/*	$Id: flashvar.h,v 1.1.2.4 2010/08/11 13:24:48 uebayasi Exp $	*/

/*-
 * Copyright (c) 2010 Tsubai Masanari.  All rights reserved.
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

#ifndef _DEV_FLASH_FLASHVAR_H_
#define _DEV_FLASH_FLASHVAR_H_

#include <sys/disk.h>

struct flash_product {
	u_char manuId;
	u_char devId1, devId2, devId3;
	u_int size;
	const char *name;
};

struct flash_softc {
	struct device sc_dev;
	struct disk sc_dkdev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_baseioh;
	bus_space_handle_t sc_ioh;
	paddr_t sc_addr;
	u_int sc_size;
	int sc_wordsize;
	int (*sc_program)(struct flash_softc *, u_long, u_long);
	int (*sc_eraseblk)(struct flash_softc *, u_long);
	struct flash_product *sc_product;
	void *sc_phys;
};

void flash_init(struct flash_softc *);
int flash_map(struct flash_softc *, u_long);
void flash_unmap(struct flash_softc *);

dev_type_read(flash_read);
dev_type_ioctl(flash_ioctl);
dev_type_mmap(flash_mmap);

extern struct cfdriver flash_cd;

#endif /* _DEV_FLASH_FLASHVAR_H_ */
