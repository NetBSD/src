/*	$NetBSD: octeon_fau.c,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $	*/

/*
 * Copyright (c) 2007 Internet Initiative Japan, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: octeon_fau.c,v 1.1.18.2 2017/12/03 11:36:27 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <mips/locore.h>
#include <mips/cavium/octeonvar.h>
#include <mips/cavium/dev/octeon_faureg.h>
#include <mips/cavium/dev/octeon_fauvar.h>

static inline int64_t	octeon_fau_op_load(uint64_t);
static inline void	octeon_fau_op_iobdma(int, uint64_t);
static inline void	octeon_fau_op_store(uint64_t, int64_t);
static inline int64_t	octeon_fau_op_load_paddr(int, int, int);
static inline void	octeon_fau_op_iobdma_store_data(int, int, int, int, int);
static inline void	octeon_fau_op_store_paddr(int, int, int64_t);


/* ---- utilities */

static inline int64_t
octeon_fau_op_load(uint64_t args)
{
	paddr_t addr;

	addr =
	    ((uint64_t)1 << 48) |
	    ((uint64_t)(CN30XXFAU_MAJORDID & 0x1f) << 43) |
	    ((uint64_t)(CN30XXFAU_SUBDID & 0x7) << 40) |
	    ((uint64_t)(args & 0xfffffffffULL) << 0);
	return octeon_read_csr(addr);
}

static inline void
octeon_fau_op_store(uint64_t args, int64_t value)
{
	paddr_t addr;

	addr =
	    ((uint64_t)1 << 48) |
	    ((uint64_t)(CN30XXFAU_MAJORDID & 0x1f) << 43) |
	    ((uint64_t)(CN30XXFAU_SUBDID & 0x7) << 40) |
	    ((uint64_t)(args & 0xfffffffffULL) << 0);
	octeon_write_csr(addr, value);
}

/* ---- operation primitives */

/*
 * Fetch-and-Add Operations
 */

/* Load Operations */

/* Load Physical Address for FAU Operations */

static inline int64_t
octeon_fau_op_load_paddr(int incval, int tagwait, int reg)
{
	uint64_t args;

	args =
	    ((uint64_t)(incval & 0x3fffff) << 14) |
	    ((uint64_t)(tagwait & 0x1) << 13) |  
	    ((uint64_t)(reg & 0x7ff) << 0);
	return octeon_fau_op_load(args);
}

/* Store Operations */

/* Store Physical Address for FAU Operations */

static inline void
octeon_fau_op_store_paddr(int noadd, int reg, int64_t value)
{
	uint64_t args;

	args =
	    ((uint64_t)(noadd & 0x1) << 13) | 
	    ((uint64_t)(reg & 0x7ff) << 0);
	octeon_fau_op_store(args, value);
}

/* ---- API */

void
octeon_fau_op_init(struct octeon_fau_desc *fd, size_t scroff, size_t regno)
{
	fd->fd_scroff = scroff;
	fd->fd_regno = regno;
}

uint64_t
octeon_fau_op_save(struct octeon_fau_desc *fd)
{
	OCTEON_SYNCIOBDMA/* XXX */;
	return octeon_cvmseg_read_8(fd->fd_scroff);
}

void
octeon_fau_op_restore(struct octeon_fau_desc *fd, uint64_t backup)
{
	octeon_cvmseg_write_8(fd->fd_scroff, backup);
}

int64_t
octeon_fau_op_inc_8(struct octeon_fau_desc *fd, int64_t v)
{
	octeon_fau_op_iobdma_store_data(fd->fd_scroff, v, 0, OCT_FAU_OP_SIZE_64/* XXX */,
	    fd->fd_regno);
	OCTEON_SYNCIOBDMA/* XXX */;
	return octeon_cvmseg_read_8(fd->fd_scroff)/* XXX */;
}

int64_t
octeon_fau_op_incwait_8(struct octeon_fau_desc *fd, int v)
{
	octeon_fau_op_iobdma_store_data(fd->fd_scroff, v, 1, OCT_FAU_OP_SIZE_64/* XXX */,
	    fd->fd_regno);
	/* XXX */
	OCTEON_SYNCIOBDMA/* XXX */;
	/* XXX */
	return octeon_cvmseg_read_8(fd->fd_scroff)/* XXX */;
}

void
octeon_fau_op_add_8(struct octeon_fau_desc *fd, int64_t v)
{
	octeon_fau_op_store_paddr(0, fd->fd_regno, v);
}

void
octeon_fau_op_set_8(struct octeon_fau_desc *fd, int64_t v)
{
	octeon_fau_op_store_paddr(1, fd->fd_regno, v);
}
