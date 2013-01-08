/*	$NetBSD: vcprop.h,v 1.7 2013/01/08 15:07:32 jmcneill Exp $	*/

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

struct vcprop_tag {
	uint32_t vpt_tag;
#define	VCPROPTAG_NULL			0x00000000
#define	VCPROPTAG_GET_FIRMWAREREV	0x00000001
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

#define VCPROPTAG_GET_VOLTAGE		0x00030003
#define VCPROPTAG_SET_VOLTAGE		0x00038003
#define VCPROPTAG_GET_MIN_VOLTAGE	0x00030008
#define VCPROPTAG_GET_MAX_VOLTAGE	0x00030005

#define VCPROPTAG_GET_TEMPERATURE	0x00030006
#define VCPROPTAG_GET_MAX_TEMPERATURE	0x0003000a

#define	VCPROPTAG_GET_CMDLINE		0x00050001
#define	VCPROPTAG_GET_DMACHAN		0x00060001

#define	VCPROPTAG_ALLOCATE_BUFFER	0x00040001
#define	VCPROPTAG_BLANK_SCREEN		0x00040002
#define	VCPROPTAG_GET_FB_RES		0x00040003
#define	VCPROPTAG_SET_FB_RES		0x00048003
#define	VCPROPTAG_GET_FB_VRES		0x00040004
#define	VCPROPTAG_SET_FB_VRES		0x00048004
#define	VCPROPTAG_GET_FB_DEPTH		0x00040005
#define	VCPROPTAG_SET_FB_DEPTH		0x00048005
#define	VCPROPTAG_GET_FB_PIXEL_ORDER	0x00040006
#define	VCPROPTAG_SET_FB_PIXEL_ORDER	0x00048006
#define	VCPROPTAG_GET_FB_ALPHA_MODE	0x00040007
#define	VCPROPTAG_SET_FB_ALPHA_MODE	0x00048007
#define	VCPROPTAG_GET_FB_PITCH		0x00040008

#define	VCPROPTAG_GET_EDID_BLOCK	0x00030020


	uint32_t vpt_len;
	uint32_t vpt_rcode;
#define	VCPROPTAG_REQUEST	(0U << 31)
#define	VCPROPTAG_RESPONSE	(1U << 31)

};

#define VCPROPTAG_LEN(x) (sizeof((x)) - sizeof(struct vcprop_tag))

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
} ;

struct vcprop_tag_boardrev {
	struct vcprop_tag tag;
	uint32_t rev;
} ;

struct vcprop_tag_macaddr {
	struct vcprop_tag tag;
	uint64_t addr;
};

struct vcprop_tag_boardserial {
	struct vcprop_tag tag;
	uint64_t sn;
};

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

struct vcprop_clock {
	uint32_t pclk;
	uint32_t cclk;
};

#define	VCPROP_MAXCLOCKS 16
struct vcprop_tag_clock {
	struct vcprop_tag tag;
	struct vcprop_clock clk[VCPROP_MAXCLOCKS];
};

#define	VCPROP_MAXCMDLINE 256
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
	uint32_t state;
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

	return (vpbh->vpb_rcode & VCPROP_REQ_SUCCESS);
}

static inline bool
vcprop_tag_success_p(struct vcprop_tag *vpbt)
{

	return (vpbt->vpt_rcode & VCPROPTAG_RESPONSE);
}

static inline size_t
vcprop_tag_resplen(struct vcprop_tag *vpbt)
{

	return (vpbt->vpt_rcode & ~VCPROPTAG_RESPONSE);
}

#endif	/* _EVBARM_RPI_VCPROP_H_ */
