/*
 * Copyright 2007 Matthieu CASTET <castet.matthieu@free.fr>
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "drmP.h"
#include "drm.h"
#include "nouveau_drm.h"
#include "nouveau_drv.h"

#define NV10_FIFO_NUMBER 32

struct pipe_state {
	uint32_t pipe_0x0000[0x040/4];
	uint32_t pipe_0x0040[0x010/4];
	uint32_t pipe_0x0200[0x0c0/4];
	uint32_t pipe_0x4400[0x080/4];
	uint32_t pipe_0x6400[0x3b0/4];
	uint32_t pipe_0x6800[0x2f0/4];
	uint32_t pipe_0x6c00[0x030/4];
	uint32_t pipe_0x7000[0x130/4];
	uint32_t pipe_0x7400[0x0c0/4];
	uint32_t pipe_0x7800[0x0c0/4];
};

static int nv10_graph_ctx_regs [] = {
NV10_PGRAPH_CTX_SWITCH1,
NV10_PGRAPH_CTX_SWITCH2,
NV10_PGRAPH_CTX_SWITCH3,
NV10_PGRAPH_CTX_SWITCH4,
NV10_PGRAPH_CTX_SWITCH5,
NV10_PGRAPH_CTX_CACHE1,	/* 8 values from 0x400160 to 0x40017c */
NV10_PGRAPH_CTX_CACHE2,	/* 8 values from 0x400180 to 0x40019c */
NV10_PGRAPH_CTX_CACHE3,	/* 8 values from 0x4001a0 to 0x4001bc */
NV10_PGRAPH_CTX_CACHE4,	/* 8 values from 0x4001c0 to 0x4001dc */
NV10_PGRAPH_CTX_CACHE5,	/* 8 values from 0x4001e0 to 0x4001fc */
0x00400164,
0x00400184,
0x004001a4,
0x004001c4,
0x004001e4,
0x00400168,
0x00400188,
0x004001a8,
0x004001c8,
0x004001e8,
0x0040016c,
0x0040018c,
0x004001ac,
0x004001cc,
0x004001ec,
0x00400170,
0x00400190,
0x004001b0,
0x004001d0,
0x004001f0,
0x00400174,
0x00400194,
0x004001b4,
0x004001d4,
0x004001f4,
0x00400178,
0x00400198,
0x004001b8,
0x004001d8,
0x004001f8,
0x0040017c,
0x0040019c,
0x004001bc,
0x004001dc,
0x004001fc,
NV10_PGRAPH_CTX_USER,
NV04_PGRAPH_DMA_START_0,
NV04_PGRAPH_DMA_START_1,
NV04_PGRAPH_DMA_LENGTH,
NV04_PGRAPH_DMA_MISC,
NV10_PGRAPH_DMA_PITCH,
NV04_PGRAPH_BOFFSET0,
NV04_PGRAPH_BBASE0,
NV04_PGRAPH_BLIMIT0,
NV04_PGRAPH_BOFFSET1,
NV04_PGRAPH_BBASE1,
NV04_PGRAPH_BLIMIT1,
NV04_PGRAPH_BOFFSET2,
NV04_PGRAPH_BBASE2,
NV04_PGRAPH_BLIMIT2,
NV04_PGRAPH_BOFFSET3,
NV04_PGRAPH_BBASE3,
NV04_PGRAPH_BLIMIT3,
NV04_PGRAPH_BOFFSET4,
NV04_PGRAPH_BBASE4,
NV04_PGRAPH_BLIMIT4,
NV04_PGRAPH_BOFFSET5,
NV04_PGRAPH_BBASE5,
NV04_PGRAPH_BLIMIT5,
NV04_PGRAPH_BPITCH0,
NV04_PGRAPH_BPITCH1,
NV04_PGRAPH_BPITCH2,
NV04_PGRAPH_BPITCH3,
NV04_PGRAPH_BPITCH4,
NV10_PGRAPH_SURFACE,
NV10_PGRAPH_STATE,
NV04_PGRAPH_BSWIZZLE2,
NV04_PGRAPH_BSWIZZLE5,
NV04_PGRAPH_BPIXEL,
NV10_PGRAPH_NOTIFY,
NV04_PGRAPH_PATT_COLOR0,
NV04_PGRAPH_PATT_COLOR1,
NV04_PGRAPH_PATT_COLORRAM, /* 64 values from 0x400900 to 0x4009fc */
0x00400904,
0x00400908,
0x0040090c,
0x00400910,
0x00400914,
0x00400918,
0x0040091c,
0x00400920,
0x00400924,
0x00400928,
0x0040092c,
0x00400930,
0x00400934,
0x00400938,
0x0040093c,
0x00400940,
0x00400944,
0x00400948,
0x0040094c,
0x00400950,
0x00400954,
0x00400958,
0x0040095c,
0x00400960,
0x00400964,
0x00400968,
0x0040096c,
0x00400970,
0x00400974,
0x00400978,
0x0040097c,
0x00400980,
0x00400984,
0x00400988,
0x0040098c,
0x00400990,
0x00400994,
0x00400998,
0x0040099c,
0x004009a0,
0x004009a4,
0x004009a8,
0x004009ac,
0x004009b0,
0x004009b4,
0x004009b8,
0x004009bc,
0x004009c0,
0x004009c4,
0x004009c8,
0x004009cc,
0x004009d0,
0x004009d4,
0x004009d8,
0x004009dc,
0x004009e0,
0x004009e4,
0x004009e8,
0x004009ec,
0x004009f0,
0x004009f4,
0x004009f8,
0x004009fc,
NV04_PGRAPH_PATTERN,	/* 2 values from 0x400808 to 0x40080c */
0x0040080c,
NV04_PGRAPH_PATTERN_SHAPE,
NV03_PGRAPH_MONO_COLOR0,
NV04_PGRAPH_ROP3,
NV04_PGRAPH_CHROMA,
NV04_PGRAPH_BETA_AND,
NV04_PGRAPH_BETA_PREMULT,
0x00400e70,
0x00400e74,
0x00400e78,
0x00400e7c,
0x00400e80,
0x00400e84,
0x00400e88,
0x00400e8c,
0x00400ea0,
0x00400ea4,
0x00400ea8,
0x00400e90,
0x00400e94,
0x00400e98,
0x00400e9c,
NV10_PGRAPH_WINDOWCLIP_HORIZONTAL, /* 8 values from 0x400f00 to 0x400f1c */
NV10_PGRAPH_WINDOWCLIP_VERTICAL,   /* 8 values from 0x400f20 to 0x400f3c */
0x00400f04,
0x00400f24,
0x00400f08,
0x00400f28,
0x00400f0c,
0x00400f2c,
0x00400f10,
0x00400f30,
0x00400f14,
0x00400f34,
0x00400f18,
0x00400f38,
0x00400f1c,
0x00400f3c,
NV10_PGRAPH_XFMODE0,
NV10_PGRAPH_XFMODE1,
NV10_PGRAPH_GLOBALSTATE0,
NV10_PGRAPH_GLOBALSTATE1,
NV04_PGRAPH_STORED_FMT,
NV04_PGRAPH_SOURCE_COLOR,
NV03_PGRAPH_ABS_X_RAM,	/* 32 values from 0x400400 to 0x40047c */
NV03_PGRAPH_ABS_Y_RAM,	/* 32 values from 0x400480 to 0x4004fc */
0x00400404,
0x00400484,
0x00400408,
0x00400488,
0x0040040c,
0x0040048c,
0x00400410,
0x00400490,
0x00400414,
0x00400494,
0x00400418,
0x00400498,
0x0040041c,
0x0040049c,
0x00400420,
0x004004a0,
0x00400424,
0x004004a4,
0x00400428,
0x004004a8,
0x0040042c,
0x004004ac,
0x00400430,
0x004004b0,
0x00400434,
0x004004b4,
0x00400438,
0x004004b8,
0x0040043c,
0x004004bc,
0x00400440,
0x004004c0,
0x00400444,
0x004004c4,
0x00400448,
0x004004c8,
0x0040044c,
0x004004cc,
0x00400450,
0x004004d0,
0x00400454,
0x004004d4,
0x00400458,
0x004004d8,
0x0040045c,
0x004004dc,
0x00400460,
0x004004e0,
0x00400464,
0x004004e4,
0x00400468,
0x004004e8,
0x0040046c,
0x004004ec,
0x00400470,
0x004004f0,
0x00400474,
0x004004f4,
0x00400478,
0x004004f8,
0x0040047c,
0x004004fc,
NV03_PGRAPH_ABS_UCLIP_XMIN,
NV03_PGRAPH_ABS_UCLIP_XMAX,
NV03_PGRAPH_ABS_UCLIP_YMIN,
NV03_PGRAPH_ABS_UCLIP_YMAX,
0x00400550,
0x00400558,
0x00400554,
0x0040055c,
NV03_PGRAPH_ABS_UCLIPA_XMIN,
NV03_PGRAPH_ABS_UCLIPA_XMAX,
NV03_PGRAPH_ABS_UCLIPA_YMIN,
NV03_PGRAPH_ABS_UCLIPA_YMAX,
NV03_PGRAPH_ABS_ICLIP_XMAX,
NV03_PGRAPH_ABS_ICLIP_YMAX,
NV03_PGRAPH_XY_LOGIC_MISC0,
NV03_PGRAPH_XY_LOGIC_MISC1,
NV03_PGRAPH_XY_LOGIC_MISC2,
NV03_PGRAPH_XY_LOGIC_MISC3,
NV03_PGRAPH_CLIPX_0,
NV03_PGRAPH_CLIPX_1,
NV03_PGRAPH_CLIPY_0,
NV03_PGRAPH_CLIPY_1,
NV10_PGRAPH_COMBINER0_IN_ALPHA,
NV10_PGRAPH_COMBINER1_IN_ALPHA,
NV10_PGRAPH_COMBINER0_IN_RGB,
NV10_PGRAPH_COMBINER1_IN_RGB,
NV10_PGRAPH_COMBINER_COLOR0,
NV10_PGRAPH_COMBINER_COLOR1,
NV10_PGRAPH_COMBINER0_OUT_ALPHA,
NV10_PGRAPH_COMBINER1_OUT_ALPHA,
NV10_PGRAPH_COMBINER0_OUT_RGB,
NV10_PGRAPH_COMBINER1_OUT_RGB,
NV10_PGRAPH_COMBINER_FINAL0,
NV10_PGRAPH_COMBINER_FINAL1,
0x00400e00,
0x00400e04,
0x00400e08,
0x00400e0c,
0x00400e10,
0x00400e14,
0x00400e18,
0x00400e1c,
0x00400e20,
0x00400e24,
0x00400e28,
0x00400e2c,
0x00400e30,
0x00400e34,
0x00400e38,
0x00400e3c,
NV04_PGRAPH_PASSTHRU_0,
NV04_PGRAPH_PASSTHRU_1,
NV04_PGRAPH_PASSTHRU_2,
NV10_PGRAPH_DIMX_TEXTURE,
NV10_PGRAPH_WDIMX_TEXTURE,
NV10_PGRAPH_DVD_COLORFMT,
NV10_PGRAPH_SCALED_FORMAT,
NV04_PGRAPH_MISC24_0,
NV04_PGRAPH_MISC24_1,
NV04_PGRAPH_MISC24_2,
NV03_PGRAPH_X_MISC,
NV03_PGRAPH_Y_MISC,
NV04_PGRAPH_VALID1,
NV04_PGRAPH_VALID2,
};

static int nv17_graph_ctx_regs [] = {
NV10_PGRAPH_DEBUG_4,
0x004006b0,
0x00400eac,
0x00400eb0,
0x00400eb4,
0x00400eb8,
0x00400ebc,
0x00400ec0,
0x00400ec4,
0x00400ec8,
0x00400ecc,
0x00400ed0,
0x00400ed4,
0x00400ed8,
0x00400edc,
0x00400ee0,
0x00400a00,
0x00400a04,
};

struct graph_state {
	int nv10[sizeof(nv10_graph_ctx_regs)/sizeof(nv10_graph_ctx_regs[0])];
	int nv17[sizeof(nv17_graph_ctx_regs)/sizeof(nv17_graph_ctx_regs[0])];
	struct pipe_state pipe_state;
};

static void nv10_graph_save_pipe(struct nouveau_channel *chan) {
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct graph_state* pgraph_ctx = chan->pgraph_ctx;
	struct pipe_state *fifo_pipe_state = &pgraph_ctx->pipe_state;
	int i;
#define PIPE_SAVE(addr) \
	do { \
		NV_WRITE(NV10_PGRAPH_PIPE_ADDRESS, addr); \
		for (i=0; i < sizeof(fifo_pipe_state->pipe_##addr)/sizeof(fifo_pipe_state->pipe_##addr[0]); i++) \
			fifo_pipe_state->pipe_##addr[i] = NV_READ(NV10_PGRAPH_PIPE_DATA); \
	} while (0)

	PIPE_SAVE(0x4400);
	PIPE_SAVE(0x0200);
	PIPE_SAVE(0x6400);
	PIPE_SAVE(0x6800);
	PIPE_SAVE(0x6c00);
	PIPE_SAVE(0x7000);
	PIPE_SAVE(0x7400);
	PIPE_SAVE(0x7800);
	PIPE_SAVE(0x0040);
	PIPE_SAVE(0x0000);

#undef PIPE_SAVE
}

static void nv10_graph_load_pipe(struct nouveau_channel *chan) {
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct graph_state* pgraph_ctx = chan->pgraph_ctx;
	struct pipe_state *fifo_pipe_state = &pgraph_ctx->pipe_state;
	int i;
	uint32_t xfmode0, xfmode1;
#define PIPE_RESTORE(addr) \
	do { \
		NV_WRITE(NV10_PGRAPH_PIPE_ADDRESS, addr); \
		for (i=0; i < sizeof(fifo_pipe_state->pipe_##addr)/sizeof(fifo_pipe_state->pipe_##addr[0]); i++) \
			NV_WRITE(NV10_PGRAPH_PIPE_DATA, fifo_pipe_state->pipe_##addr[i]); \
	} while (0)


	nouveau_wait_for_idle(dev);
	/* XXX check haiku comments */
	xfmode0 = NV_READ(NV10_PGRAPH_XFMODE0);
	xfmode1 = NV_READ(NV10_PGRAPH_XFMODE1);
	NV_WRITE(NV10_PGRAPH_XFMODE0, 0x10000000);
	NV_WRITE(NV10_PGRAPH_XFMODE1, 0x00000000);
	NV_WRITE(NV10_PGRAPH_PIPE_ADDRESS, 0x000064c0);
	for (i = 0; i < 4; i++)
		NV_WRITE(NV10_PGRAPH_PIPE_DATA, 0x3f800000);
	for (i = 0; i < 4; i++)
		NV_WRITE(NV10_PGRAPH_PIPE_DATA, 0x00000000);

	NV_WRITE(NV10_PGRAPH_PIPE_ADDRESS, 0x00006ab0);
	for (i = 0; i < 3; i++)
		NV_WRITE(NV10_PGRAPH_PIPE_DATA, 0x3f800000);

	NV_WRITE(NV10_PGRAPH_PIPE_ADDRESS, 0x00006a80);
	for (i = 0; i < 3; i++)
		NV_WRITE(NV10_PGRAPH_PIPE_DATA, 0x00000000);

	NV_WRITE(NV10_PGRAPH_PIPE_ADDRESS, 0x00000040);
	NV_WRITE(NV10_PGRAPH_PIPE_DATA, 0x00000008);


	PIPE_RESTORE(0x0200);
	nouveau_wait_for_idle(dev);

	/* restore XFMODE */
	NV_WRITE(NV10_PGRAPH_XFMODE0, xfmode0);
	NV_WRITE(NV10_PGRAPH_XFMODE1, xfmode1);
	PIPE_RESTORE(0x6400);
	PIPE_RESTORE(0x6800);
	PIPE_RESTORE(0x6c00);
	PIPE_RESTORE(0x7000);
	PIPE_RESTORE(0x7400);
	PIPE_RESTORE(0x7800);
	PIPE_RESTORE(0x4400);
	PIPE_RESTORE(0x0000);
	PIPE_RESTORE(0x0040);
	nouveau_wait_for_idle(dev);

#undef PIPE_RESTORE
}

static void nv10_graph_create_pipe(struct nouveau_channel *chan) {
	struct graph_state* pgraph_ctx = chan->pgraph_ctx;
	struct pipe_state *fifo_pipe_state = &pgraph_ctx->pipe_state;
	uint32_t *fifo_pipe_state_addr;
	int i;
#define PIPE_INIT(addr) \
	do { \
		fifo_pipe_state_addr = fifo_pipe_state->pipe_##addr; \
	} while (0)
#define PIPE_INIT_END(addr) \
	do { \
		if (fifo_pipe_state_addr != \
				sizeof(fifo_pipe_state->pipe_##addr)/sizeof(fifo_pipe_state->pipe_##addr[0]) + fifo_pipe_state->pipe_##addr) \
			DRM_ERROR("incomplete pipe init for 0x%x :  %p/%p\n", addr, fifo_pipe_state_addr, \
					sizeof(fifo_pipe_state->pipe_##addr)/sizeof(fifo_pipe_state->pipe_##addr[0]) + fifo_pipe_state->pipe_##addr); \
	} while (0)
#define NV_WRITE_PIPE_INIT(value) *(fifo_pipe_state_addr++) = value

	PIPE_INIT(0x0200);
	for (i = 0; i < 48; i++)
		NV_WRITE_PIPE_INIT(0x00000000);
	PIPE_INIT_END(0x0200);

	PIPE_INIT(0x6400);
	for (i = 0; i < 211; i++)
		NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x3f800000);
	NV_WRITE_PIPE_INIT(0x40000000);
	NV_WRITE_PIPE_INIT(0x40000000);
	NV_WRITE_PIPE_INIT(0x40000000);
	NV_WRITE_PIPE_INIT(0x40000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x3f800000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x3f000000);
	NV_WRITE_PIPE_INIT(0x3f000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x3f800000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x3f800000);
	NV_WRITE_PIPE_INIT(0x3f800000);
	NV_WRITE_PIPE_INIT(0x3f800000);
	NV_WRITE_PIPE_INIT(0x3f800000);
	PIPE_INIT_END(0x6400);

	PIPE_INIT(0x6800);
	for (i = 0; i < 162; i++)
		NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x3f800000);
	for (i = 0; i < 25; i++)
		NV_WRITE_PIPE_INIT(0x00000000);
	PIPE_INIT_END(0x6800);

	PIPE_INIT(0x6c00);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0xbf800000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	PIPE_INIT_END(0x6c00);

	PIPE_INIT(0x7000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x7149f2ca);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x7149f2ca);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x7149f2ca);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x7149f2ca);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x7149f2ca);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x7149f2ca);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x7149f2ca);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x00000000);
	NV_WRITE_PIPE_INIT(0x7149f2ca);
	for (i = 0; i < 35; i++)
		NV_WRITE_PIPE_INIT(0x00000000);
	PIPE_INIT_END(0x7000);

	PIPE_INIT(0x7400);
	for (i = 0; i < 48; i++)
		NV_WRITE_PIPE_INIT(0x00000000);
	PIPE_INIT_END(0x7400);

	PIPE_INIT(0x7800);
	for (i = 0; i < 48; i++)
		NV_WRITE_PIPE_INIT(0x00000000);
	PIPE_INIT_END(0x7800);

	PIPE_INIT(0x4400);
	for (i = 0; i < 32; i++)
		NV_WRITE_PIPE_INIT(0x00000000);
	PIPE_INIT_END(0x4400);

	PIPE_INIT(0x0000);
	for (i = 0; i < 16; i++)
		NV_WRITE_PIPE_INIT(0x00000000);
	PIPE_INIT_END(0x0000);

	PIPE_INIT(0x0040);
	for (i = 0; i < 4; i++)
		NV_WRITE_PIPE_INIT(0x00000000);
	PIPE_INIT_END(0x0040);

#undef PIPE_INIT
#undef PIPE_INIT_END
#undef NV_WRITE_PIPE_INIT
}

static int nv10_graph_ctx_regs_find_offset(struct drm_device *dev, int reg)
{
	int i;
	for (i = 0; i < sizeof(nv10_graph_ctx_regs)/sizeof(nv10_graph_ctx_regs[0]); i++) {
		if (nv10_graph_ctx_regs[i] == reg)
			return i;
	}
	DRM_ERROR("unknow offset nv10_ctx_regs %d\n", reg);
	return -1;
}

static int nv17_graph_ctx_regs_find_offset(struct drm_device *dev, int reg)
{
	int i;
	for (i = 0; i < sizeof(nv17_graph_ctx_regs)/sizeof(nv17_graph_ctx_regs[0]); i++) {
		if (nv17_graph_ctx_regs[i] == reg)
			return i;
	}
	DRM_ERROR("unknow offset nv17_ctx_regs %d\n", reg);
	return -1;
}

int nv10_graph_load_context(struct nouveau_channel *chan)
{
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct graph_state* pgraph_ctx = chan->pgraph_ctx;
	int i;

	for (i = 0; i < sizeof(nv10_graph_ctx_regs)/sizeof(nv10_graph_ctx_regs[0]); i++)
		NV_WRITE(nv10_graph_ctx_regs[i], pgraph_ctx->nv10[i]);
	if (dev_priv->chipset>=0x17) {
		for (i = 0; i < sizeof(nv17_graph_ctx_regs)/sizeof(nv17_graph_ctx_regs[0]); i++)
			NV_WRITE(nv17_graph_ctx_regs[i], pgraph_ctx->nv17[i]);
	}

	nv10_graph_load_pipe(chan);

	return 0;
}

int nv10_graph_save_context(struct nouveau_channel *chan)
{
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct graph_state* pgraph_ctx = chan->pgraph_ctx;
	int i;

	for (i = 0; i < sizeof(nv10_graph_ctx_regs)/sizeof(nv10_graph_ctx_regs[0]); i++)
		pgraph_ctx->nv10[i] = NV_READ(nv10_graph_ctx_regs[i]);
	if (dev_priv->chipset>=0x17) {
		for (i = 0; i < sizeof(nv17_graph_ctx_regs)/sizeof(nv17_graph_ctx_regs[0]); i++)
			pgraph_ctx->nv17[i] = NV_READ(nv17_graph_ctx_regs[i]);
	}

	nv10_graph_save_pipe(chan);

	return 0;
}

void nouveau_nv10_context_switch(struct drm_device *dev)
{
	struct drm_nouveau_private *dev_priv;
	struct nouveau_engine *engine;
	struct nouveau_channel *next, *last;
	int chid;

	if (!dev) {
		DRM_DEBUG("Invalid drm_device\n");
		return;
	}
	dev_priv = dev->dev_private;
	if (!dev_priv) {
		DRM_DEBUG("Invalid drm_nouveau_private\n");
		return;
	}
	if (!dev_priv->fifos) {
		DRM_DEBUG("Invalid drm_nouveau_private->fifos\n");
		return;
	}
	engine = &dev_priv->Engine;

	chid = (NV_READ(NV04_PGRAPH_TRAPPED_ADDR) >> 20) &
		(engine->fifo.channels - 1);
	next = dev_priv->fifos[chid];

	if (!next) {
		DRM_ERROR("Invalid next channel\n");
		return;
	}

	chid = (NV_READ(NV10_PGRAPH_CTX_USER) >> 24) & 
		(engine->fifo.channels - 1);
	last = dev_priv->fifos[chid];

	if (!last) {
		DRM_INFO("WARNING: Invalid last channel, switch to %x\n",
		          next->id);
	} else {
		DRM_DEBUG("NV: PGRAPH context switch interrupt channel %x -> %x\n",
		         last->id, next->id);
	}

	NV_WRITE(NV04_PGRAPH_FIFO,0x0);
	if (last) {
		nouveau_wait_for_idle(dev);
		nv10_graph_save_context(last);
	}

	nouveau_wait_for_idle(dev);

	NV_WRITE(NV10_PGRAPH_CTX_CONTROL, 0x10000000);

	nouveau_wait_for_idle(dev);

	nv10_graph_load_context(next);

	NV_WRITE(NV10_PGRAPH_CTX_CONTROL, 0x10010100);
	NV_WRITE(NV10_PGRAPH_FFINTFC_ST2, NV_READ(NV10_PGRAPH_FFINTFC_ST2)&0xCFFFFFFF);
	NV_WRITE(NV04_PGRAPH_FIFO,0x1);
}

#define NV_WRITE_CTX(reg, val) do { \
	int offset = nv10_graph_ctx_regs_find_offset(dev, reg); \
	if (offset > 0) \
		pgraph_ctx->nv10[offset] = val; \
	} while (0)

#define NV17_WRITE_CTX(reg, val) do { \
	int offset = nv17_graph_ctx_regs_find_offset(dev, reg); \
	if (offset > 0) \
		pgraph_ctx->nv17[offset] = val; \
	} while (0)

int nv10_graph_create_context(struct nouveau_channel *chan) {
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct graph_state* pgraph_ctx;

	DRM_DEBUG("nv10_graph_context_create %d\n", chan->id);

	chan->pgraph_ctx = pgraph_ctx = drm_calloc(1, sizeof(*pgraph_ctx),
					      DRM_MEM_DRIVER);

	if (pgraph_ctx == NULL)
		return -ENOMEM;

	/* mmio trace suggest that should be done in ddx with methods/objects */
#if 0
	uint32_t tmp, vramsz;
	/* per channel init from ddx */
	tmp = NV_READ(NV10_PGRAPH_SURFACE) & 0x0007ff00;
	/*XXX the original ddx code, does this in 2 steps :
	 * tmp = NV_READ(NV10_PGRAPH_SURFACE) & 0x0007ff00;
	 * NV_WRITE(NV10_PGRAPH_SURFACE, tmp);
	 * tmp = NV_READ(NV10_PGRAPH_SURFACE) | 0x00020100;
	 * NV_WRITE(NV10_PGRAPH_SURFACE, tmp);
	 */
	tmp |= 0x00020100;
	NV_WRITE_CTX(NV10_PGRAPH_SURFACE, tmp);

	vramsz = drm_get_resource_len(dev, 0) - 1;
	NV_WRITE_CTX(NV04_PGRAPH_BOFFSET0, 0);
	NV_WRITE_CTX(NV04_PGRAPH_BOFFSET1, 0);
	NV_WRITE_CTX(NV04_PGRAPH_BLIMIT0 , vramsz);
	NV_WRITE_CTX(NV04_PGRAPH_BLIMIT1 , vramsz);

	NV_WRITE_CTX(NV04_PGRAPH_PATTERN_SHAPE, 0x00000000);
	NV_WRITE_CTX(NV04_PGRAPH_BETA_AND     , 0xFFFFFFFF);

	NV_WRITE_CTX(NV03_PGRAPH_ABS_UCLIP_XMIN, 0);
	NV_WRITE_CTX(NV03_PGRAPH_ABS_UCLIP_YMIN, 0);
	NV_WRITE_CTX(NV03_PGRAPH_ABS_UCLIP_XMAX, 0x7fff);
	NV_WRITE_CTX(NV03_PGRAPH_ABS_UCLIP_YMAX, 0x7fff);
#endif

	NV_WRITE_CTX(0x00400e88, 0x08000000);
	NV_WRITE_CTX(0x00400e9c, 0x4b7fffff);
	NV_WRITE_CTX(NV03_PGRAPH_XY_LOGIC_MISC0, 0x0001ffff);
	NV_WRITE_CTX(0x00400e10, 0x00001000);
	NV_WRITE_CTX(0x00400e14, 0x00001000);
	NV_WRITE_CTX(0x00400e30, 0x00080008);
	NV_WRITE_CTX(0x00400e34, 0x00080008);
	if (dev_priv->chipset>=0x17) {
		/* is it really needed ??? */
		NV17_WRITE_CTX(NV10_PGRAPH_DEBUG_4, NV_READ(NV10_PGRAPH_DEBUG_4));
		NV17_WRITE_CTX(0x004006b0, NV_READ(0x004006b0));
		NV17_WRITE_CTX(0x00400eac, 0x0fff0000);
		NV17_WRITE_CTX(0x00400eb0, 0x0fff0000);
		NV17_WRITE_CTX(0x00400ec0, 0x00000080);
		NV17_WRITE_CTX(0x00400ed0, 0x00000080);
	}
	NV_WRITE_CTX(NV10_PGRAPH_CTX_USER, chan->id << 24);

	nv10_graph_create_pipe(chan);
	return 0;
}

void nv10_graph_destroy_context(struct nouveau_channel *chan)
{
	struct drm_device *dev = chan->dev;
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	struct nouveau_engine *engine = &dev_priv->Engine;
	struct graph_state* pgraph_ctx = chan->pgraph_ctx;
	int chid;

	drm_free(pgraph_ctx, sizeof(*pgraph_ctx), DRM_MEM_DRIVER);
	chan->pgraph_ctx = NULL;

	chid = (NV_READ(NV10_PGRAPH_CTX_USER) >> 24) & (engine->fifo.channels - 1);

	/* This code seems to corrupt the 3D pipe, but blob seems to do similar things ????
	 */
#if 0
	/* does this avoid a potential context switch while we are written graph
	 * reg, or we should mask graph interrupt ???
	 */
	NV_WRITE(NV04_PGRAPH_FIFO,0x0);
	if (chid == chan->id) {
		DRM_INFO("cleanning a channel with graph in current context\n");
		nouveau_wait_for_idle(dev);
		DRM_INFO("reseting current graph context\n");
		/* can't be call here because of dynamic mem alloc */
		//nv10_graph_create_context(chan);
		nv10_graph_load_context(chan);
	}
	NV_WRITE(NV04_PGRAPH_FIFO, 0x1);
#else
	if (chid == chan->id) {
		DRM_INFO("cleanning a channel with graph in current context\n");
	}
#endif
}

int nv10_graph_init(struct drm_device *dev) {
	struct drm_nouveau_private *dev_priv = dev->dev_private;
	int i;

	NV_WRITE(NV03_PMC_ENABLE, NV_READ(NV03_PMC_ENABLE) &
			~NV_PMC_ENABLE_PGRAPH);
	NV_WRITE(NV03_PMC_ENABLE, NV_READ(NV03_PMC_ENABLE) |
			 NV_PMC_ENABLE_PGRAPH);

	NV_WRITE(NV03_PGRAPH_INTR   , 0xFFFFFFFF);
	NV_WRITE(NV03_PGRAPH_INTR_EN, 0xFFFFFFFF);

	NV_WRITE(NV04_PGRAPH_DEBUG_0, 0xFFFFFFFF);
	NV_WRITE(NV04_PGRAPH_DEBUG_0, 0x00000000);
	NV_WRITE(NV04_PGRAPH_DEBUG_1, 0x00118700);
	//NV_WRITE(NV04_PGRAPH_DEBUG_2, 0x24E00810); /* 0x25f92ad9 */
	NV_WRITE(NV04_PGRAPH_DEBUG_2, 0x25f92ad9);
	NV_WRITE(NV04_PGRAPH_DEBUG_3, 0x55DE0830 |
				      (1<<29) |
				      (1<<31));
	if (dev_priv->chipset>=0x17) {
		NV_WRITE(NV10_PGRAPH_DEBUG_4, 0x1f000000);
		NV_WRITE(0x004006b0, 0x40000020);
	}
	else
		NV_WRITE(NV10_PGRAPH_DEBUG_4, 0x00000000);

	/* copy tile info from PFB */
	for (i=0; i<NV10_PFB_TILE__SIZE; i++) {
		NV_WRITE(NV10_PGRAPH_TILE(i), NV_READ(NV10_PFB_TILE(i)));
		NV_WRITE(NV10_PGRAPH_TLIMIT(i), NV_READ(NV10_PFB_TLIMIT(i)));
		NV_WRITE(NV10_PGRAPH_TSIZE(i), NV_READ(NV10_PFB_TSIZE(i)));
		NV_WRITE(NV10_PGRAPH_TSTATUS(i), NV_READ(NV10_PFB_TSTATUS(i)));
	}

	NV_WRITE(NV10_PGRAPH_CTX_SWITCH1, 0x00000000);
	NV_WRITE(NV10_PGRAPH_CTX_SWITCH2, 0x00000000);
	NV_WRITE(NV10_PGRAPH_CTX_SWITCH3, 0x00000000);
	NV_WRITE(NV10_PGRAPH_CTX_SWITCH4, 0x00000000);
	NV_WRITE(NV10_PGRAPH_CTX_CONTROL, 0x10010100);
	NV_WRITE(NV10_PGRAPH_STATE      , 0xFFFFFFFF);
	NV_WRITE(NV04_PGRAPH_FIFO       , 0x00000001);

	return 0;
}

void nv10_graph_takedown(struct drm_device *dev)
{
}
