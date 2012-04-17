/*	$NetBSD: shpcicvar.h,v 1.8.2.1 2012/04/17 00:06:52 yamt Exp $	*/

/*-
 * Copyright (C) 2005 NONAKA Kimihiro <nonaka@netbsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#ifndef	_SH3_SHPCICVAR_H_
#define	_SH3_SHPCICVAR_H_

#include <sys/bus.h>

bus_space_tag_t shpcic_get_bus_io_tag(void);
bus_space_tag_t shpcic_get_bus_mem_tag(void);
bus_dma_tag_t shpcic_get_bus_dma_tag(void);

int shpcic_bus_maxdevs(void *v, int busno);
pcitag_t shpcic_make_tag(void *v, int bus, int device, int function);
void shpcic_decompose_tag(void *v, pcitag_t tag, int *bp, int *dp, int *fp);
pcireg_t shpcic_conf_read(void *v, pcitag_t tag, int reg);
void shpcic_conf_write(void *v, pcitag_t tag, int reg, pcireg_t data);

int shpcic_set_intr_priority(int intr, int level);
void *shpcic_intr_establish(int evtcode, int (*ih_func)(void *), void *ih_arg);
void shpcic_intr_disestablish(void *ih);

/*
 * shpcic io/mem bus space
 */
int shpcic_iomem_map(void *v, bus_addr_t bpa, bus_size_t size, int flags,
    bus_space_handle_t *bshp);
void shpcic_iomem_unmap(void *v, bus_space_handle_t bsh, bus_size_t size);
int shpcic_iomem_subregion(void *v, bus_space_handle_t bsh, bus_size_t offset,
    bus_size_t size, bus_space_handle_t *nbshp);
int shpcic_iomem_alloc(void *v, bus_addr_t rstart, bus_addr_t rend,
    bus_size_t size, bus_size_t alignment, bus_size_t boundary, int flags,
    bus_addr_t *bpap, bus_space_handle_t *bshp);
void shpcic_iomem_free(void *v, bus_space_handle_t bsh, bus_size_t size);
paddr_t shpcic_iomem_mmap(void *v, bus_addr_t addr, off_t off, int prot,
    int flags);

/* read single */
uint8_t shpcic_io_read_1(void *v, bus_space_handle_t bsh, bus_size_t offset);
uint16_t shpcic_io_read_2(void *v, bus_space_handle_t bsh, bus_size_t offset);
uint32_t shpcic_io_read_4(void *v, bus_space_handle_t bsh, bus_size_t offset);
uint8_t shpcic_mem_read_1(void *v, bus_space_handle_t bsh, bus_size_t offset);
uint16_t shpcic_mem_read_2(void *v, bus_space_handle_t bsh, bus_size_t offset);
uint32_t shpcic_mem_read_4(void *v, bus_space_handle_t bsh, bus_size_t offset);

/* read multi */
void shpcic_io_read_multi_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *addr, bus_size_t count);
void shpcic_io_read_multi_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *addr, bus_size_t count);
void shpcic_io_read_multi_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *addr, bus_size_t count);
void shpcic_mem_read_multi_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *addr, bus_size_t count);
void shpcic_mem_read_multi_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *addr, bus_size_t count);
void shpcic_mem_read_multi_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *addr, bus_size_t count);

/* read region */
void shpcic_io_read_region_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *addr, bus_size_t count);
void shpcic_io_read_region_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *addr, bus_size_t count);
void shpcic_io_read_region_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *addr, bus_size_t count);
void shpcic_mem_read_region_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *addr, bus_size_t count);
void shpcic_mem_read_region_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *addr, bus_size_t count);
void shpcic_mem_read_region_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *addr, bus_size_t count);

/* write single */
void shpcic_io_write_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t data);
void shpcic_io_write_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t data);
void shpcic_io_write_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t data);
void shpcic_mem_write_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t data);
void shpcic_mem_write_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t data);
void shpcic_mem_write_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t data);

/* write multi */
void shpcic_io_write_multi_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint8_t *addr, bus_size_t count);
void shpcic_io_write_multi_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *addr, bus_size_t count);
void shpcic_io_write_multi_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *addr, bus_size_t count);
void shpcic_mem_write_multi_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint8_t *addr, bus_size_t count);
void shpcic_mem_write_multi_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *addr, bus_size_t count);
void shpcic_mem_write_multi_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *addr, bus_size_t count);

/* write region */
void shpcic_io_write_region_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint8_t *addr, bus_size_t count);
void shpcic_io_write_region_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *addr, bus_size_t count);
void shpcic_io_write_region_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *addr, bus_size_t count);
void shpcic_mem_write_region_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint8_t *addr, bus_size_t count);
void shpcic_mem_write_region_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *addr, bus_size_t count);
void shpcic_mem_write_region_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *addr, bus_size_t count);

/* set multi */
void shpcic_io_set_multi_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t val, bus_size_t count);
void shpcic_io_set_multi_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t val, bus_size_t count);
void shpcic_io_set_multi_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t val, bus_size_t count);
void shpcic_mem_set_multi_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t val, bus_size_t count);
void shpcic_mem_set_multi_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t val, bus_size_t count);
void shpcic_mem_set_multi_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t val, bus_size_t count);

/* set region */
void shpcic_io_set_region_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t val, bus_size_t count);
void shpcic_io_set_region_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t val, bus_size_t count);
void shpcic_io_set_region_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t val, bus_size_t count);
void shpcic_mem_set_region_1(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t val, bus_size_t count);
void shpcic_mem_set_region_2(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t val, bus_size_t count);
void shpcic_mem_set_region_4(void *v, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t val, bus_size_t count);

/* copy region */
void shpcic_io_copy_region_1(void *v, bus_space_handle_t bsh1,
    bus_size_t off1, bus_space_handle_t bsh2, bus_size_t off2,
    bus_size_t count);
void shpcic_io_copy_region_2(void *v, bus_space_handle_t bsh1,
    bus_size_t off1, bus_space_handle_t bsh2, bus_size_t off2,
    bus_size_t count);
void shpcic_io_copy_region_4(void *v, bus_space_handle_t bsh1,
    bus_size_t off1, bus_space_handle_t bsh2, bus_size_t off2,
    bus_size_t count);
void shpcic_mem_copy_region_1(void *v, bus_space_handle_t bsh1,
    bus_size_t off1, bus_space_handle_t bsh2, bus_size_t off2,
    bus_size_t count);
void shpcic_mem_copy_region_2(void *v, bus_space_handle_t bsh1,
    bus_size_t off1, bus_space_handle_t bsh2, bus_size_t off2,
    bus_size_t count);
void shpcic_mem_copy_region_4(void *v, bus_space_handle_t bsh1,
    bus_size_t off1, bus_space_handle_t bsh2, bus_size_t off2,
    bus_size_t count);

#endif	/* _SH3_SHPCICVAR_H_ */
