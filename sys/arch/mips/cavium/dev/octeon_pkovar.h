/*	$NetBSD: octeon_pkovar.h,v 1.6 2020/06/23 05:15:33 simonb Exp $	*/

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

/*
 * PKO Registers
 */

#ifndef _OCTEON_PKOVAR_H_
#define _OCTEON_PKOVAR_H_

#include <mips/cavium/dev/octeon_fauvar.h>
#include <mips/cavium/dev/octeon_pkoreg.h>

#define FAU_OP_SIZE_8	0
#define FAU_OP_SIZE_16	1
#define FAU_OP_SIZE_32	2   
#define FAU_OP_SIZE_64	3        

#define PKO_OUTPUT_PORTS_NUM		5
#define PKO_OUTPUT_PORTS_PKTIF_NUM	3
#define PKO_OUTPUT_PORTS_PCIIF_NUM	2
#define	PKO_MEM_QUEUE_PTRS_ILLEGAL_PID	63

/* XXX */
struct octpko_cmdptr_desc {
	uint64_t	cmdptr;
	uint64_t	cmdptr_idx;
};

/* XXX */
struct octpko_softc {
	int			sc_port;
	bus_space_tag_t		sc_regt;
	bus_space_handle_t	sc_regh;
	struct octpko_cmdptr_desc
				*sc_cmdptr;
	int			sc_cmd_buf_pool;
	size_t			sc_cmd_buf_size;
};

/* XXX */
struct octpko_attach_args {
	int				aa_port;
	bus_space_tag_t			aa_regt;
	struct octpko_cmdptr_desc	*aa_cmdptr;
	int				aa_cmd_buf_pool;
	size_t				aa_cmd_buf_size;
};

/* XXX */
void			octpko_init(struct octpko_attach_args *,
			    struct octpko_softc **);
int			octpko_enable(struct octpko_softc *);
int			octpko_reset(struct octpko_softc *);
void			octpko_config(struct octpko_softc *);
int			octpko_port_enable(struct octpko_softc *, int);
int			octpko_port_config(struct octpko_softc *);
void			octpko_int_enable(struct octpko_softc *, int);
uint64_t		octpko_int_summary(struct octpko_softc *);
static __inline uint64_t	octpko_cmd_word0(int, int, int, int, int, int,
			    int, int, int, int, int, int, int, int, int, int);
static __inline uint64_t	octpko_cmd_word1(int, int, int, int, paddr_t);


static __inline uint64_t
octpko_cmd_word0(int sz1, int sz0, int s1, int reg1, int s0, int reg0,
    int le, int n2, int q, int r, int g, int ipoffp1, int ii, int df, int segs,
    int totalbytes)
{
	uint64_t cmd =
	    __SHIFTIN(sz1, PKO_CMD_WORD0_SZ1) |
	    __SHIFTIN(sz0, PKO_CMD_WORD0_SZ0) |
	    __SHIFTIN(s1, PKO_CMD_WORD0_S1) |
	    __SHIFTIN(reg1, PKO_CMD_WORD0_REG1) |
	    __SHIFTIN(s0, PKO_CMD_WORD0_S0) |
	    __SHIFTIN(reg0, PKO_CMD_WORD0_REG0) |
	    __SHIFTIN(le, PKO_CMD_WORD0_LE) |
	    __SHIFTIN(n2, PKO_CMD_WORD0_N2) |
	    __SHIFTIN(q, PKO_CMD_WORD0_Q) |
	    __SHIFTIN(r, PKO_CMD_WORD0_R) |
	    __SHIFTIN(g, PKO_CMD_WORD0_G) |
	    __SHIFTIN(ipoffp1, PKO_CMD_WORD0_IPOFFP1) |
	    __SHIFTIN(ii, PKO_CMD_WORD0_II) |
	    __SHIFTIN(df, PKO_CMD_WORD0_DF) |
	    __SHIFTIN(segs, PKO_CMD_WORD0_SEGS) |
	    __SHIFTIN(totalbytes, PKO_CMD_WORD0_TOTALBYTES);

	return cmd;
}

static __inline uint64_t
octpko_cmd_word1(int i, int back, int pool, int size, paddr_t addr)
{
	uint64_t cmd =
	    __SHIFTIN(i, PKO_CMD_WORD1_I) |
	    __SHIFTIN(back, PKO_CMD_WORD1_BACK) |
	    __SHIFTIN(pool, PKO_CMD_WORD1_POOL) |
	    __SHIFTIN(size, PKO_CMD_WORD1_SIZE) |
	    __SHIFTIN(addr, PKO_CMD_WORD1_ADDR);

	return cmd;
}

/* ---- operation primitives */

/* Store Operations */

static __inline void
octpko_op_store(uint64_t args, uint64_t value)
{
	paddr_t addr;

	addr = OCTEON_ADDR_IO_DID(PKO_MAJOR_DID, PKO_SUB_DID) | args;
	/* XXX */
	OCTEON_SYNCW;
	octeon_xkphys_write_8(addr, value);
}

static __inline void
octpko_op_doorbell_write(int pid, int qid, int wdc)
{
	uint64_t args, value;

	args =
	    __SHIFTIN(pid, PKO_DOORBELL_WRITE_PID) |
	    __SHIFTIN(qid, PKO_DOORBELL_WRITE_QID);
	value = __SHIFTIN(wdc, PKO_DOORBELL_WRITE_WDC);
	octpko_op_store(args, value);
}

#endif /* _OCTEON_PKOVAR_H_ */
