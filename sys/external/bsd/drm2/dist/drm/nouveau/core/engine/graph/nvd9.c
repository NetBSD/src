/*
 * Copyright 2013 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Ben Skeggs <bskeggs@redhat.com>
 */

#include "nvc0.h"
#include "ctxnvc0.h"

/*******************************************************************************
 * PGRAPH register lists
 ******************************************************************************/

const struct nvc0_graph_init
nvd9_graph_init_pd_0[] = {
	{ 0x406024,   1, 0x04, 0x00000000 },
	{ 0x4064f0,   3, 0x04, 0x00000000 },
	{}
};

const struct nvc0_graph_init
nvd9_graph_init_ds_0[] = {
	{ 0x405844,   1, 0x04, 0x00ffffff },
	{ 0x405850,   1, 0x04, 0x00000000 },
	{ 0x405900,   1, 0x04, 0x00002834 },
	{ 0x405908,   1, 0x04, 0x00000000 },
	{ 0x405928,   2, 0x04, 0x00000000 },
	{}
};

const struct nvc0_graph_init
nvd9_graph_init_prop_0[] = {
	{ 0x418408,   1, 0x04, 0x00000000 },
	{ 0x4184a0,   3, 0x04, 0x00000000 },
	{}
};

const struct nvc0_graph_init
nvd9_graph_init_gpm_0[] = {
	{ 0x418c04,   1, 0x04, 0x00000000 },
	{ 0x418c64,   2, 0x04, 0x00000000 },
	{ 0x418c88,   1, 0x04, 0x00000000 },
	{ 0x418cb4,   2, 0x04, 0x00000000 },
	{}
};

const struct nvc0_graph_init
nvd9_graph_init_gpc_unk_1[] = {
	{ 0x418d00,   1, 0x04, 0x00000000 },
	{ 0x418d28,   2, 0x04, 0x00000000 },
	{ 0x418f00,   1, 0x04, 0x00000000 },
	{ 0x418f08,   1, 0x04, 0x00000000 },
	{ 0x418f20,   2, 0x04, 0x00000000 },
	{ 0x418e00,   1, 0x04, 0x00000003 },
	{ 0x418e08,   1, 0x04, 0x00000000 },
	{ 0x418e1c,   2, 0x04, 0x00000000 },
	{}
};

const struct nvc0_graph_init
nvd9_graph_init_tex_0[] = {
	{ 0x419ab0,   1, 0x04, 0x00000000 },
	{ 0x419ac8,   1, 0x04, 0x00000000 },
	{ 0x419ab8,   1, 0x04, 0x000000e7 },
	{ 0x419abc,   2, 0x04, 0x00000000 },
	{ 0x419ab4,   1, 0x04, 0x00000000 },
	{}
};

static const struct nvc0_graph_init
nvd9_graph_init_pe_0[] = {
	{ 0x41980c,   1, 0x04, 0x00000010 },
	{ 0x419810,   1, 0x04, 0x00000000 },
	{ 0x419814,   1, 0x04, 0x00000004 },
	{ 0x419844,   1, 0x04, 0x00000000 },
	{ 0x41984c,   1, 0x04, 0x0000a918 },
	{ 0x419850,   4, 0x04, 0x00000000 },
	{ 0x419880,   1, 0x04, 0x00000002 },
	{}
};

static const struct nvc0_graph_init
nvd9_graph_init_wwdx_0[] = {
	{ 0x419bd4,   1, 0x04, 0x00800000 },
	{ 0x419bdc,   1, 0x04, 0x00000000 },
	{ 0x419bf8,   2, 0x04, 0x00000000 },
	{}
};

static const struct nvc0_graph_init
nvd9_graph_init_tpccs_1[] = {
	{ 0x419d2c,   1, 0x04, 0x00000000 },
	{ 0x419d48,   2, 0x04, 0x00000000 },
	{}
};

const struct nvc0_graph_init
nvd9_graph_init_sm_0[] = {
	{ 0x419e00,   1, 0x04, 0x00000000 },
	{ 0x419ea0,   1, 0x04, 0x00000000 },
	{ 0x419ea4,   1, 0x04, 0x00000100 },
	{ 0x419ea8,   1, 0x04, 0x02001100 },
	{ 0x419eac,   1, 0x04, 0x11100702 },
	{ 0x419eb0,   1, 0x04, 0x00000003 },
	{ 0x419eb4,   4, 0x04, 0x00000000 },
	{ 0x419ec8,   1, 0x04, 0x0e063818 },
	{ 0x419ecc,   1, 0x04, 0x0e060e06 },
	{ 0x419ed0,   1, 0x04, 0x00003818 },
	{ 0x419ed4,   1, 0x04, 0x011104f1 },
	{ 0x419edc,   1, 0x04, 0x00000000 },
	{ 0x419f00,   1, 0x04, 0x00000000 },
	{ 0x419f2c,   1, 0x04, 0x00000000 },
	{}
};

const struct nvc0_graph_init
nvd9_graph_init_fe_1[] = {
	{ 0x40402c,   1, 0x04, 0x00000000 },
	{ 0x4040f0,   1, 0x04, 0x00000000 },
	{ 0x404174,   1, 0x04, 0x00000000 },
	{}
};

static const struct nvc0_graph_pack
nvd9_graph_pack_mmio[] = {
	{ nvc0_graph_init_main_0 },
	{ nvc0_graph_init_fe_0 },
	{ nvc0_graph_init_pri_0 },
	{ nvc0_graph_init_rstr2d_0 },
	{ nvd9_graph_init_pd_0 },
	{ nvd9_graph_init_ds_0 },
	{ nvc0_graph_init_scc_0 },
	{ nvd9_graph_init_prop_0 },
	{ nvc1_graph_init_gpc_unk_0 },
	{ nvc0_graph_init_setup_0 },
	{ nvc0_graph_init_crstr_0 },
	{ nvc1_graph_init_setup_1 },
	{ nvc0_graph_init_zcull_0 },
	{ nvd9_graph_init_gpm_0 },
	{ nvd9_graph_init_gpc_unk_1 },
	{ nvc0_graph_init_gcc_0 },
	{ nvc0_graph_init_tpccs_0 },
	{ nvd9_graph_init_tex_0 },
	{ nvd9_graph_init_pe_0 },
	{ nvc0_graph_init_l1c_0 },
	{ nvd9_graph_init_wwdx_0 },
	{ nvd9_graph_init_tpccs_1 },
	{ nvc0_graph_init_mpc_0 },
	{ nvd9_graph_init_sm_0 },
	{ nvc0_graph_init_be_0 },
	{ nvd9_graph_init_fe_1 },
	{}
};

/*******************************************************************************
 * PGRAPH engine/subdev functions
 ******************************************************************************/

struct nouveau_oclass *
nvd9_graph_oclass = &(struct nvc0_graph_oclass) {
	.base.handle = NV_ENGINE(GR, 0xd9),
	.base.ofuncs = &(struct nouveau_ofuncs) {
		.ctor = nvc0_graph_ctor,
		.dtor = nvc0_graph_dtor,
		.init = nvc0_graph_init,
		.fini = _nouveau_graph_fini,
	},
	.cclass = &nvd9_grctx_oclass,
	.sclass = nvc8_graph_sclass,
	.mmio = nvd9_graph_pack_mmio,
	.fecs.ucode = &nvc0_graph_fecs_ucode,
	.gpccs.ucode = &nvc0_graph_gpccs_ucode,
}.base;
