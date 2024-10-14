/* $NetBSD: avenc.c,v 1.1.2.3 2024/10/14 16:44:42 martin Exp $ */

/*-
 * Copyright (c) 2024 Jared McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: avenc.c,v 1.1.2.3 2024/10/14 16:44:42 martin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <machine/wii.h>

#include <lib/libkern/libkern.h>	/* hexdump */

#include <dev/i2c/i2cvar.h>

#include "avenc.h"
#include "vireg.h"

#define AVENC_DEBUG		0

#define AVENC_ADDR		0x70

#define AVENC_VOLUME		0x71
#define  AVENC_VOLUME_RIGHT	__BITS(15,8)
#define  AVENC_VOLUME_LEFT	__BITS(7,0)

#define RD1			avenc_read_1
#define RD2			avenc_read_2
#define WR1			avenc_write_1
#define WR2			avenc_write_2
#define WR4			avenc_write_4
#define WRBUF			avenc_write_buf

static i2c_tag_t avenc_tag;
static i2c_addr_t avenc_addr;

static uint8_t
avenc_read_1(i2c_tag_t tag, i2c_addr_t addr, uint8_t reg)
{
	uint8_t val;

	if (iic_smbus_read_byte(tag, addr, reg, &val, 0) != 0) {
		val = 0xff;
	}

	return val;
}

static uint16_t
avenc_read_2(i2c_tag_t tag, i2c_addr_t addr, uint8_t reg)
{
	uint8_t vbuf[2];

	if (iic_exec(tag, I2C_OP_READ_WITH_STOP, addr, &reg, 1,
	    	     vbuf, sizeof(vbuf), 0) != 0) {
		return 0xffff;
	} else {
		return ((uint16_t)vbuf[0] << 8) | vbuf[1];
	}
}

static void
avenc_write_1(i2c_tag_t tag, i2c_addr_t addr, uint8_t reg, uint8_t val)
{
	iic_smbus_write_byte(tag, addr, reg, val, 0);
}

static void
avenc_write_2(i2c_tag_t tag, i2c_addr_t addr, uint8_t reg, uint16_t val)
{
	uint8_t vbuf[2];

	vbuf[0] = (val >> 8) & 0xff;
	vbuf[1] = val & 0xff;

	iic_exec(tag, I2C_OP_WRITE_WITH_STOP, addr, &reg, 1,
		 vbuf, sizeof(vbuf), 0);
}

static void
avenc_write_4(i2c_tag_t tag, i2c_addr_t addr, uint8_t reg, uint32_t val)
{
	uint8_t vbuf[4];

	vbuf[0] = (val >> 24) & 0xff;
	vbuf[1] = (val >> 16) & 0xff;
	vbuf[2] = (val >> 8) & 0xff;
	vbuf[3] = val & 0xff;

	iic_exec(tag, I2C_OP_WRITE_WITH_STOP, addr, &reg, 1,
		 vbuf, sizeof(vbuf), 0);
}

static void
avenc_write_buf(i2c_tag_t tag, i2c_addr_t addr, uint8_t reg, uint8_t *buf,
    size_t buflen)
{
	iic_exec(tag, I2C_OP_WRITE_WITH_STOP, addr, &reg, 1,
		 buf, buflen, 0);
}

void
avenc_get_volume(uint8_t *left, uint8_t *right)
{
	uint16_t val;

	if (avenc_addr == 0) {
		*left = *right = 0;
		return;
	}

	iic_acquire_bus(avenc_tag, 0);
	val = RD2(avenc_tag, avenc_addr, AVENC_VOLUME);
	iic_release_bus(avenc_tag, 0);

	*left = __SHIFTOUT(val, AVENC_VOLUME_LEFT);
	*right = __SHIFTOUT(val, AVENC_VOLUME_RIGHT);
}

void
avenc_set_volume(uint8_t left, uint8_t right)
{
	uint16_t val;

	if (avenc_addr == 0) {
		return;
	}

	val = __SHIFTIN(left, AVENC_VOLUME_LEFT) |
	      __SHIFTIN(right, AVENC_VOLUME_RIGHT);

	iic_acquire_bus(avenc_tag, 0);
	WR2(avenc_tag, avenc_addr, AVENC_VOLUME, val);
	iic_release_bus(avenc_tag, 0);
}

static void
avenc_dump_regs(const char *pfx, i2c_tag_t tag, i2c_addr_t addr)
{
	uint8_t regdump[0x100];
	unsigned int reg;

	iic_acquire_bus(tag, 0);

	for (reg = 0; reg < sizeof(regdump); reg++) {
		regdump[reg] = RD1(tag, addr, reg);
	}

	iic_release_bus(tag, 0);

	hexdump(printf, pfx, regdump, sizeof(regdump));
}

static void
avenc_init(i2c_tag_t tag, i2c_addr_t addr)
{
	uint8_t mvinit[26] = {};
	uint8_t ginit[] = {
		0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00,
		0x10, 0x00, 0x10, 0x00, 0x10, 0x20, 0x40, 0x60,
		0x80, 0xa0, 0xeb, 0x10, 0x00, 0x20, 0x00, 0x40,
		0x00, 0x60, 0x00, 0x80, 0x00, 0xa0, 0x00, 0xeb,
		0x00
	};
	uint8_t video_fmt;

	video_fmt = 0;
	if (__SHIFTOUT(in16(VI_BASE + VI_DCR), VI_DCR_FMT) == VI_DCR_FMT_PAL) {
		video_fmt |= 0x02;
	}
	if ((in16(VI_BASE + VI_VISEL) & VI_VISEL_COMPONENT_CABLE) != 0) {
		video_fmt |= 0x20;
	}

	iic_acquire_bus(tag, 0);
	WR1(tag, addr, 0x6a, 1);
	WR1(tag, addr, 0x65, 3);
	WR1(tag, addr, 0x01, video_fmt);
	WR1(tag, addr, 0x00, 0);
	WR1(tag, addr, 0x02, 7);
	WR2(tag, addr, 0x05, 0);
	WR2(tag, addr, 0x08, 0);
	WR4(tag, addr, 0x7a, 0);
	WRBUF(tag, addr, 0x40, mvinit, sizeof(mvinit));
	WR1(tag, addr, 0x0a, 0);
	WR1(tag, addr, 0x03, 1);
	WRBUF(tag, addr, 0x10, ginit, sizeof(ginit));
	WR1(tag, addr, 0x04, 1);
	WR4(tag, addr, 0x7a, 0);
	WR2(tag, addr, 0x08, 0);
	WR1(tag, addr, 0x03, 1);
	WR1(tag, addr, 0x6e, 0);
	iic_release_bus(tag, 0);

	avenc_set_volume(0x8e, 0x8e);
}


static int
avenc_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;

	return ia->ia_addr == AVENC_ADDR;
}

static void
avenc_attach(device_t parent, device_t self, void *aux)
{
	struct i2c_attach_args *ia = aux;
	i2c_tag_t tag = ia->ia_tag;
	i2c_addr_t addr = ia->ia_addr;

	KASSERT(device_unit(self) == 0);

	aprint_naive("\n");
	aprint_normal(": A/V Encoder\n");

	if (AVENC_DEBUG) {
		avenc_dump_regs("avenc pre ", tag, addr);
	}

	avenc_tag = tag;
	avenc_addr = addr;

	avenc_init(tag, addr);

	if (AVENC_DEBUG) {
		avenc_dump_regs("avenc post", tag, addr);
	}
}

CFATTACH_DECL_NEW(avenc, 0, avenc_match, avenc_attach, NULL, NULL);
