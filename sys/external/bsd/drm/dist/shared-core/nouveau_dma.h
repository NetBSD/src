/*
 * Copyright (C) 2007 Ben Skeggs.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __NOUVEAU_DMA_H__
#define __NOUVEAU_DMA_H__

typedef enum {
	NvSubM2MF	= 0,
} nouveau_subchannel_id_t;

typedef enum {
	NvM2MF		= 0x80039001,
	NvDmaFB		= 0x8003d001,
	NvDmaTT		= 0x8003d002,
	NvNotify0       = 0x8003d003
} nouveau_object_handle_t;

#define NV_MEMORY_TO_MEMORY_FORMAT                                    0x00000039
#define NV_MEMORY_TO_MEMORY_FORMAT_NAME                               0x00000000
#define NV_MEMORY_TO_MEMORY_FORMAT_SET_REF                            0x00000050
#define NV_MEMORY_TO_MEMORY_FORMAT_NOP                                0x00000100
#define NV_MEMORY_TO_MEMORY_FORMAT_NOTIFY                             0x00000104
#define NV_MEMORY_TO_MEMORY_FORMAT_NOTIFY_STYLE_WRITE                 0x00000000
#define NV_MEMORY_TO_MEMORY_FORMAT_NOTIFY_STYLE_WRITE_LE_AWAKEN       0x00000001
#define NV_MEMORY_TO_MEMORY_FORMAT_SET_DMA_NOTIFY                     0x00000180
#define NV_MEMORY_TO_MEMORY_FORMAT_SET_DMA_SOURCE                     0x00000184
#define NV_MEMORY_TO_MEMORY_FORMAT_OFFSET_IN                          0x0000030c

#define NV50_MEMORY_TO_MEMORY_FORMAT                                  0x00005039
#define NV50_MEMORY_TO_MEMORY_FORMAT_UNK200                           0x00000200
#define NV50_MEMORY_TO_MEMORY_FORMAT_UNK21C                           0x0000021c
#define NV50_MEMORY_TO_MEMORY_FORMAT_OFFSET_IN_HIGH                   0x00000238
#define NV50_MEMORY_TO_MEMORY_FORMAT_OFFSET_OUT_HIGH                  0x0000023c

#define BEGIN_RING(subc, mthd, cnt) do {                                       \
	int push_size = (cnt) + 1;                                             \
	if (dchan->push_free) {                                                \
		DRM_ERROR("prior packet incomplete: %d\n", dchan->push_free);  \
		break;                                                         \
	}                                                                      \
	if (dchan->free < push_size) {                                         \
		if (nouveau_dma_wait(dev, push_size)) {                        \
			DRM_ERROR("FIFO timeout\n");                           \
			break;                                                 \
		}                                                              \
	}                                                                      \
	dchan->free -= push_size;                                              \
	dchan->push_free = push_size;                                          \
	OUT_RING(((cnt)<<18) | ((subc)<<15) | mthd);                           \
} while(0)

#define OUT_RING(data) do {                                                    \
	if (dchan->push_free == 0) {                                           \
		DRM_ERROR("no space left in packet\n");                        \
		break;                                                         \
	}                                                                      \
	dchan->pushbuf[dchan->cur++] = (data);                                 \
	dchan->push_free--;                                                    \
} while(0)

#define FIRE_RING() do {                                                       \
	if (dchan->push_free) {                                                \
		DRM_ERROR("packet incomplete: %d\n", dchan->push_free);        \
		break;                                                         \
	}                                                                      \
	if (dchan->cur != dchan->put) {                                        \
		DRM_MEMORYBARRIER();                                           \
		dchan->put = dchan->cur;                                       \
		NV_WRITE(dchan->chan->put, dchan->put << 2);                   \
	}                                                                      \
} while(0)

#endif
