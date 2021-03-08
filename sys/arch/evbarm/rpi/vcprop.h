/*	$NetBSD: vcprop.h,v 1.20 2021/03/08 13:53:08 mlelstv Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

/*
 * Mailbox property interface
 */

#ifndef	_EVBARM_RPI_VCPROP_H_
#define	_EVBARM_RPI_VCPROP_H_

#include "opt_vcprop.h"

#include <sys/endian.h>

struct vcprop_tag {
	uint32_t vpt_tag;
#define	VCPROPTAG_NULL			0x00000000
#define	VCPROPTAG_GET_FIRMWAREREV	0x00000001
#define	VCPROPTAG_GET_FIRMWAREVARIANT	0x00000002
#define	VCPROPTAG_GET_FIRMWAREHASH	0x00000003
#define	VCPROPTAG_GET_BOARDMODEL	0x00010001
#define	VCPROPTAG_GET_BOARDREVISION	0x00010002
#define	VCPROPTAG_GET_MACADDRESS	0x00010003
#define	VCPROPTAG_GET_BOARDSERIAL	0x00010004
#define	VCPROPTAG_GET_ARMMEMORY		0x00010005
#define	VCPROPTAG_GET_VCMEMORY		0x00010006
#define	VCPROPTAG_GET_CLOCKS		0x00010007

#define VCPROPTAG_GET_POWERSTATE	0x00020001
#define VCPROPTAG_GET_POWERTIMING	0x00020002
#define VCPROPTAG_SET_POWERSTATE	0x00028001

#define	VCPROPTAG_GET_CLOCKSTATE	0x00030001
#define	VCPROPTAG_SET_CLOCKSTATE	0x00038001
#define	VCPROPTAG_GET_CLOCKRATE		0x00030002
#define	VCPROPTAG_SET_CLOCKRATE		0x00038002
#define	VCPROPTAG_GET_MIN_CLOCKRATE	0x00030007
#define	VCPROPTAG_GET_MAX_CLOCKRATE	0x00030004
#define	VCPROPTAG_GET_TURBO		0x00030009
#define	VCPROPTAG_SET_TURBO		0x00038009

#define VCPROPTAG_GET_STC		0x0003000b
#define	VCPROPTAG_GET_THROTTLED		0x00030046
#define	VCPROPTAG_GET_CLOCK_MEASURED	0x00030047
#define	VCPROPTAG_NOTIFY_REBOOT		0x00030048

#define VCPROPTAG_GET_VOLTAGE		0x00030003
#define VCPROPTAG_SET_VOLTAGE		0x00038003
#define VCPROPTAG_GET_MIN_VOLTAGE	0x00030008
#define VCPROPTAG_GET_MAX_VOLTAGE	0x00030005

#define VCPROPTAG_GET_TEMPERATURE	0x00030006
#define VCPROPTAG_GET_MAX_TEMPERATURE	0x0003000a

#define VCPROPTAG_GET_DOMAIN_STATE	0x00030030
#define VCPROPTAG_SET_DOMAIN_STATE	0x00038030

#define VCPROPTAG_GET_GPIO_STATE	0x00030041
#define VCPROPTAG_SET_GPIO_STATE	0x00038041
#define VCPROPTAG_GET_GPIO_CONFIG	0x00030041
#define VCPROPTAG_SET_GPIO_CONFIG	0x00038041
#define VCPROPTAG_GET_PERIPH_REG	0x00030045
#define VCPROPTAG_SET_PERIPH_REG	0x00038045

#define VCPROPTAG_GET_OTP		0x00030021
#define VCPROPTAG_SET_OTP		0x00038021

#define VCPROPTAG_SET_SDHOST_CLOCK	0x00038042

#define VCPROPTAG_GET_POE_HAT_VAL	0x00030049
#define VCPROPTAG_SET_POE_HAT_VAL	0x00030050

#define VCPROPTAG_NOTIFY_XHCI_RESET	0x00030058

#define	VCPROPTAG_GET_CMDLINE		0x00050001
#define	VCPROPTAG_GET_DMACHAN		0x00060001

#define	VCPROPTAG_ALLOCATE_BUFFER	0x00040001
#define	VCPROPTAG_RELEASE_BUFFER	0x00048001
#define	VCPROPTAG_BLANK_SCREEN		0x00040002
#define	VCPROPTAG_GET_FB_RES		0x00040003
#define	VCPROPTAG_TST_FB_RES		0x00044003
#define	VCPROPTAG_SET_FB_RES		0x00048003
#define	VCPROPTAG_GET_FB_VRES		0x00040004
#define	VCPROPTAG_TST_FB_VRES		0x00044004
#define	VCPROPTAG_SET_FB_VRES		0x00048004
#define	VCPROPTAG_GET_FB_DEPTH		0x00040005
#define	VCPROPTAG_TST_FB_DEPTH		0x00044005
#define	VCPROPTAG_SET_FB_DEPTH		0x00048005
#define	VCPROPTAG_GET_FB_PIXEL_ORDER	0x00040006
#define	VCPROPTAG_TST_FB_PIXEL_ORDER	0x00044006
#define	VCPROPTAG_SET_FB_PIXEL_ORDER	0x00048006
#define	VCPROPTAG_GET_FB_ALPHA_MODE	0x00040007
#define	VCPROPTAG_TST_FB_ALPHA_MODE	0x00044007
#define	VCPROPTAG_SET_FB_ALPHA_MODE	0x00048007
#define	VCPROPTAG_GET_FB_PITCH		0x00040008
#define	VCPROPTAG_GET_VIRTUAL_OFFSET	0x00040009
#define	VCPROPTAG_TST_VIRTUAL_OFFSET	0x00044009
#define	VCPROPTAG_SET_VIRTUAL_OFFSET	0x00048009
#define	VCPROPTAG_GET_OVERSCAN		0x0004000a
#define	VCPROPTAG_TST_OVERSCAN		0x0004400a
#define	VCPROPTAG_SET_OVERSCAN		0x0004800a
#define	VCPROPTAG_GET_PALETTE		0x0004000b
#define	VCPROPTAG_TST_PALETTE		0x0004400b
#define	VCPROPTAG_SET_PALETTE		0x0004800b
#define	VCPROPTAG_GET_FB_LAYER		0x0004000c
#define	VCPROPTAG_TST_FB_LAYER		0x0004400c
#define	VCPROPTAG_SET_FB_LAYER		0x0004800c
#define	VCPROPTAG_GET_TRANSFORM		0x0004000d
#define	VCPROPTAG_TST_TRANSFORM		0x0004400d
#define	VCPROPTAG_SET_TRANSFORM		0x0004800d
#define	VCPROPTAG_GET_VSYNC		0x0004000e
#define	VCPROPTAG_TST_VSYNC		0x0004400e
#define	VCPROPTAG_SET_VSYNC		0x0004800e
#define	VCPROPTAG_GET_TOUCHBUF		0x0004000f
#define	VCPROPTAG_GET_SET_BACKLIGHT	0x0004800f
#define	VCPROPTAG_GET_GPIOVIRTBUF	0x00040010
#define	VCPROPTAG_SET_GPIOVIRTBUF	0x00048020
#define	VCPROPTAG_GET_NUM_DISPLAYS	0x00040013
#define	VCPROPTAG_SET_DISPLAYNUM	0x00048013
#define	VCPROPTAG_GET_DISPLAY_SETTINGS	0x00040014
#define	VCPROPTAG_GET_DISPLAYID		0x00040016

#define	VCPROPTAG_VCHIQ_INIT		0x00048010

#define	VCPROPTAG_SET_PLANE		0x00048015
#define	VCPROPTAG_GET_TIMING		0x00040017
#define	VCPROPTAG_SET_TIMING		0x00048017
#define	VCPROPTAG_GET_DISPLAY_CFG	0x00048018

#define	VCPROPTAG_GET_EDID_BLOCK	0x00030020
#define	VCPROPTAG_GET_EDID_BLOCK_DISP	0x00030021

#define	VCPROPTAG_ALLOCMEM		0x0003000c
#define	VCPROPTAG_LOCKMEM		0x0003000d
#define	VCPROPTAG_UNLOCKMEM		0x0003000e
#define	VCPROPTAG_RELEASEMEM		0x0003000f
#define	VCPROPTAG_EXECUTE_CODE		0x00030010
#define	VCPROPTAG_EXECUTE_QPU		0x00030011
#define	VCPROPTAG_SET_ENABLE_QPU	0x00030012
#define	VCPROPTAG_GET_DISPMANX_HANDLE	0x00030014

#define	VCPROPTAG_SET_CURSOR_INFO	0x00008010
#define	VCPROPTAG_SET_CURSOR_STATE	0x00008011

	uint32_t vpt_len;
	uint32_t vpt_rcode;
#define	VCPROPTAG_REQUEST	(0U << 31)
#define	VCPROPTAG_RESPONSE	(1U << 31)

};

#define VCPROPTAG_LEN(x) (sizeof((x)) - sizeof(struct vcprop_tag))

#define VCPROP_INIT_REQUEST(req)					\
	do {								\
		memset(&(req), 0, sizeof((req)));			\
		(req).vb_hdr.vpb_len = htole32(sizeof((req)));		\
		(req).vb_hdr.vpb_rcode = htole32(VCPROP_PROCESS_REQUEST);\
		(req).end.vpt_tag = htole32(VCPROPTAG_NULL);			\
	} while (0)
#define VCPROP_INIT_TAG(s, t)						\
	do {								\
		(s).tag.vpt_tag = htole32(t);				\
		(s).tag.vpt_rcode = htole32(VCPROPTAG_REQUEST);		\
		(s).tag.vpt_len = htole32(VCPROPTAG_LEN(s));		\
	} while (0)

struct vcprop_memory {
	uint32_t base;
	uint32_t size;
};

#define	VCPROP_MAXMEMBLOCKS 4
struct vcprop_tag_memory {
	struct vcprop_tag tag;
	struct vcprop_memory mem[VCPROP_MAXMEMBLOCKS];
};

struct vcprop_tag_fwrev {
	struct vcprop_tag tag;
	uint32_t rev;
};

struct vcprop_tag_boardmodel {
	struct vcprop_tag tag;
	uint32_t model;
};

struct vcprop_tag_boardrev {
	struct vcprop_tag tag;
	uint32_t rev;
};

#define	VCPROP_REV_PCBREV	__BITS(3,0)
#define	VCPROP_REV_MODEL	__BITS(11,4)
#define	 RPI_MODEL_A		0
#define	 RPI_MODEL_B		1
#define	 RPI_MODEL_A_PLUS	2
#define	 RPI_MODEL_B_PLUS	3
#define	 RPI_MODEL_B_PI2	4
#define	 RPI_MODEL_ALPHA	5
#define	 RPI_MODEL_COMPUTE	6
#define	 RPI_MODEL_B_PI3	8
#define	 RPI_MODEL_ZERO		9
#define	 RPI_MODEL_COMPUTE_PI3	10
#define	 RPI_MODEL_ZERO_W	12
#define	 RPI_MODEL_B_PLUS_PI3	13
#define	 RPI_MODEL_A_PLUS_PI3	14
#define	 RPI_MODEL_CM_PLUS_PI3	16
#define	 RPI_MODEL_B_PI4	17
#define	VCPROP_REV_PROCESSOR	__BITS(15,12)
#define	 RPI_PROCESSOR_BCM2835	0
#define	 RPI_PROCESSOR_BCM2836	1
#define	 RPI_PROCESSOR_BCM2837	2
#define	 RPI_PROCESSOR_BCM2711	3
#define	VCPROP_REV_MANUF	__BITS(19,16)
#define	 RPI_MANUF_SONY         0
#define	 RPI_MANUF_EGOMAN       1
#define	 RPI_MANUF_QISDA        16
#define	 RPI_MANUF_EMBEST       2
#define	 RPI_MANUF_SONYJAPAN    3
#define	VCPROP_REV_MEMSIZE	__BITS(22,20)
#define	 RPI_MEMSIZE_256	0
#define	 RPI_MEMSIZE_512	1
#define	 RPI_MEMSIZE_1024	2
#define	 RPI_MEMSIZE_2048	3
#define	 RPI_MEMSIZE_4096	4
#define	VCPROP_REV_ENCFLAG	__BIT(23)
#define	VCPROP_REV_WARRANTY	__BITS(25,24)

struct vcprop_tag_macaddr {
	struct vcprop_tag tag;
	uint64_t addr;
} __packed;

struct vcprop_tag_boardserial {
	struct vcprop_tag tag;
	uint64_t sn;
} __packed;

#define	VCPROP_CLK_EMMC		1
#define	VCPROP_CLK_UART		2
#define	VCPROP_CLK_ARM		3
#define	VCPROP_CLK_CORE		4
#define	VCPROP_CLK_V3D		5
#define	VCPROP_CLK_H264		6
#define	VCPROP_CLK_ISP		7
#define	VCPROP_CLK_SDRAM	8
#define	VCPROP_CLK_PIXEL	9
#define	VCPROP_CLK_PWM		10
#define	VCPROP_CLK_EMMC2	12

struct vcprop_clock {
	uint32_t pclk;
	uint32_t cclk;
};

#define	VCPROP_MAXCLOCKS 16
struct vcprop_tag_clock {
	struct vcprop_tag tag;
	struct vcprop_clock clk[VCPROP_MAXCLOCKS];
};

#ifndef	VCPROP_MAXCMDLINE
#define	VCPROP_MAXCMDLINE 1024
#endif
struct vcprop_tag_cmdline {
	struct vcprop_tag tag;
	uint8_t cmdline[VCPROP_MAXCMDLINE];
};

struct vcprop_tag_dmachan {
	struct vcprop_tag tag;
	uint32_t mask;
};

struct vcprop_tag_clockstate {
	struct vcprop_tag tag;
	uint32_t id;
	uint32_t state;
};

struct vcprop_tag_clockrate {
	struct vcprop_tag tag;
	uint32_t id;
	uint32_t rate;
	uint32_t noturbo;
};

struct vcprop_tag_sdhostclock {
	struct vcprop_tag tag;
	uint32_t clock;
	uint32_t clock1;
	uint32_t clock2;
};

#define VCPROP_VOLTAGE_CORE	1
#define VCPROP_VOLTAGE_SDRAM_C	2
#define VCPROP_VOLTAGE_SDRAM_P	3
#define VCPROP_VOLTAGE_SDRAM_I	4

struct vcprop_tag_voltage {
	struct vcprop_tag tag;
	uint32_t id;
	uint32_t value;
};

#define VCPROP_TEMP_SOC		0

struct vcprop_tag_temperature {
	struct vcprop_tag tag;
	uint32_t id;
	uint32_t value;
};

#define	VCPROP_POWER_SDCARD	0
#define	VCPROP_POWER_UART0	1
#define	VCPROP_POWER_UART1	2
#define	VCPROP_POWER_USB	3
#define	VCPROP_POWER_I2C0	4
#define	VCPROP_POWER_I2C1	5
#define	VCPROP_POWER_I2C2	6
#define	VCPROP_POWER_SPI	7
#define	VCPROP_POWER_CCP2TX	8

#define VCPROP_DOMAIN_I2C0           1
#define VCPROP_DOMAIN_I2C1           2
#define VCPROP_DOMAIN_I2C2           3
#define VCPROP_DOMAIN_VIDEO_SCALER   4
#define VCPROP_DOMAIN_VPU1           5
#define VCPROP_DOMAIN_HDMI           6
#define VCPROP_DOMAIN_USB            7
#define VCPROP_DOMAIN_VEC            8
#define VCPROP_DOMAIN_JPEG           9
#define VCPROP_DOMAIN_H264           10
#define VCPROP_DOMAIN_V3D            11
#define VCPROP_DOMAIN_ISP            12
#define VCPROP_DOMAIN_UNICAM0        13
#define VCPROP_DOMAIN_UNICAM1        14
#define VCPROP_DOMAIN_CCP2RX         15
#define VCPROP_DOMAIN_CSI2           16
#define VCPROP_DOMAIN_CPI            17
#define VCPROP_DOMAIN_DSI0           18
#define VCPROP_DOMAIN_DSI1           19
#define VCPROP_DOMAIN_TRANSPOSER     20
#define VCPROP_DOMAIN_CCP2TX         21
#define VCPROP_DOMAIN_CDP            22
#define VCPROP_DOMAIN_ARM            23

struct vcprop_tag_powertiming {
	struct vcprop_tag tag;
	uint32_t id;
	uint32_t waitusec;
};

struct vcprop_tag_powerstate {
	struct vcprop_tag tag;
	uint32_t id;
	uint32_t state;
};

struct vcprop_tag_allocbuf {
	struct vcprop_tag tag;
	uint32_t address;	/* alignment for request */
	uint32_t size;
};

#define VCPROP_BLANK_OFF	0
#define VCPROP_BLANK_ON		1

struct vcprop_tag_blankscreen {
	struct vcprop_tag tag;
	uint32_t state;
};

struct vcprop_tag_fbres {
	struct vcprop_tag tag;
	uint32_t width;
	uint32_t height;
};

struct vcprop_tag_fbdepth {
	struct vcprop_tag tag;
	uint32_t bpp;
};

#define VCPROP_PIXEL_BGR	0
#define VCPROP_PIXEL_RGB	1

struct vcprop_tag_fbpixelorder {
	struct vcprop_tag tag;
	uint32_t order;
};

struct vcprop_tag_fbpitch {
	struct vcprop_tag tag;
	uint32_t linebytes;
};

#define VCPROP_ALPHA_ENABLED	0
#define VCPROP_ALPHA_REVERSED	1
#define VCPROP_ALPHA_IGNORED	2

struct vcprop_tag_fbalpha {
	struct vcprop_tag tag;
	uint32_t state;
};

struct vcprop_tag_edidblock {
	struct vcprop_tag tag;
	uint32_t blockno;
	uint32_t status;
	uint8_t data[128];
};

struct vcprop_tag_numdpy {
	struct vcprop_tag tag;
	uint32_t count;
};

struct vcprop_tag_setdpy {
	struct vcprop_tag tag;
	uint32_t display_num;
};

struct vcprop_tag_cursorinfo {
	struct vcprop_tag tag;
	uint32_t width;
	uint32_t height;
	uint32_t format;
	uint32_t pixels;	/* bus address in VC memory */
	uint32_t hotspot_x;
	uint32_t hotspot_y;
};

struct vcprop_tag_cursorstate {
	struct vcprop_tag tag;
	uint32_t enable;	/* 1 - visible */
	uint32_t x;
	uint32_t y;
	uint32_t flags;		/* 0 - display coord. 1 - fb coord. */
};

struct vcprop_tag_allocmem {
	struct vcprop_tag tag;
	uint32_t size;	/* handle returned here */
	uint32_t align;
	uint32_t flags;
/*
 * flag definitions from
 * https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
 */
#define MEM_FLAG_DISCARDABLE	(1 << 0) /* can be resized to 0 at any time. Use for cached data */
#define MEM_FLAG_NORMAL		(0 << 2) /* normal allocating alias. Don't use from ARM */
#define MEM_FLAG_DIRECT		(1 << 2) /* 0xC alias uncached */
#define MEM_FLAG_COHERENT	(2 << 2) /* 0x8 alias. Non-allocating in L2 but coherent */
#define MEM_FLAG_L1_NONALLOCATING (MEM_FLAG_DIRECT | MEM_FLAG_COHERENT) /* Allocating in L2 */
#define MEM_FLAG_ZERO		(1 << 4)  /* initialise buffer to all zeros */
#define MEM_FLAG_NO_INIT	(1 << 5) /* don't initialise (default is initialise to all ones */
#define MEM_FLAG_HINT_PERMALOCK	(1 << 6) /* Likely to be locked for long periods of time. */
};

/* also for unlock and release */
struct vcprop_tag_lockmem {
	struct vcprop_tag tag;
	uint32_t handle;	/* bus address returned here */
};

struct vcprop_tag_vchiqinit {
	struct vcprop_tag tag;
	uint32_t base;
};

struct vcprop_tag_notifyxhcireset {
	struct vcprop_tag tag;
	uint32_t deviceaddress;
};

struct vcprop_buffer_hdr {
	uint32_t vpb_len;
	uint32_t vpb_rcode;
#define	VCPROP_PROCESS_REQUEST 0
#define VCPROP_REQ_SUCCESS	(1U << 31)
#define VCPROP_REQ_EPARSE	(1U << 0)
};

static inline bool
vcprop_buffer_success_p(struct vcprop_buffer_hdr *vpbh)
{

	return le32toh(vpbh->vpb_rcode) & VCPROP_REQ_SUCCESS;
}

static inline bool
vcprop_tag_success_p(struct vcprop_tag *vpbt)
{

	return le32toh(vpbt->vpt_rcode) & VCPROPTAG_RESPONSE;
}

static inline size_t
vcprop_tag_resplen(struct vcprop_tag *vpbt)
{

	return le32toh(vpbt->vpt_rcode) & ~VCPROPTAG_RESPONSE;
}

uint32_t rpi_alloc_mem(uint32_t, uint32_t, uint32_t);
bus_addr_t rpi_lock_mem(uint32_t);
int rpi_unlock_mem(uint32_t);
int rpi_release_mem(uint32_t);

int rpi_fb_set_video(int);

int rpi_fb_movecursor(int, int, int);
int rpi_fb_initcursor(bus_addr_t, int, int);

int rpi_fb_set_pixelorder(uint32_t);
int rpi_fb_get_pixelorder(uint32_t *);

int rpi_set_domain(uint32_t, uint32_t);
int rpi_get_domain(uint32_t, uint32_t *);

int rpi_vchiq_init(uint32_t *);

int rpi_notify_xhci_reset(uint32_t);

#endif	/* _EVBARM_RPI_VCPROP_H_ */

