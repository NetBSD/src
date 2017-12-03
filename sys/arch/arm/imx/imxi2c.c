/*	$NetBSD: imxi2c.c,v 1.1.6.3 2017/12/03 11:35:53 jdolecek Exp $	*/

/*
 * Copyright (c) 2012, 2015 Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
__KERNEL_RCSID(0, "$NetBSD: imxi2c.c,v 1.1.6.3 2017/12/03 11:35:53 jdolecek Exp $");

#include "opt_imx.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <dev/i2c/motoi2cvar.h>
#include <dev/i2c/motoi2creg.h>
#include <arm/imx/imxi2cvar.h>

struct clk_div {
	uint8_t ic_val;
	int div;
};

static const struct clk_div imxi2c_clk_div[] = {
	{0x20, 22},   {0x21, 24},   {0x22, 26},   {0x23, 28},
	{0x00, 30},   {0x01, 32},   {0x24, 32},   {0x02, 36},
	{0x25, 36},   {0x26, 40},   {0x03, 42},   {0x27, 44},
	{0x04, 48},   {0x28, 48},   {0x05, 52},   {0x29, 56},
	{0x06, 60},   {0x2A, 64},   {0x07, 72},   {0x2B, 72},
	{0x08, 80},   {0x2C, 80},   {0x09, 88},   {0x2D, 96},
	{0x0A, 104},  {0x2E, 112},  {0x0B, 128},  {0x2F, 128},
	{0x0C, 144},  {0x0D, 160},  {0x30, 160},  {0x0E, 192},
	{0x31, 192},  {0x32, 224},  {0x0F, 240},  {0x33, 256},
	{0x10, 288},  {0x11, 320},  {0x34, 320},  {0x12, 384},
	{0x35, 384},  {0x36, 448},  {0x13, 480},  {0x37, 512},
	{0x14, 576},  {0x15, 640},  {0x38, 640},  {0x16, 768},
	{0x39, 768},  {0x3A, 896},  {0x17, 960},  {0x3B, 1024},
	{0x18, 1152}, {0x19, 1280}, {0x3C, 1280}, {0x1A, 1536},
	{0x3D, 1536}, {0x3E, 1792}, {0x1B, 1920}, {0x3F, 2048},
	{0x1C, 2304}, {0x1D, 2560}, {0x1E, 3072}, {0x1F, 3840},
};

CFATTACH_DECL_NEW(imxi2c, sizeof(struct imxi2c_softc),
    imxi2c_match, imxi2c_attach, NULL, NULL);

static uint8_t
imxi2c_iord1(struct motoi2c_softc *sc, bus_size_t off)
{
	if (off < I2CDFSRR)
		return bus_space_read_2(sc->sc_iot, sc->sc_ioh, off) & 0xff;
	else
		return 0;
}

static void
imxi2c_iowr1(struct motoi2c_softc *sc, bus_size_t off, uint8_t data)
{
	if (off < I2CDFSRR)
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, off, data);
}

int
imxi2c_attach_common(device_t parent, device_t self,
    bus_space_tag_t iot, paddr_t iobase, size_t size, int intr, int flags)
{
	struct imxi2c_softc *sc = device_private(self);
	struct motoi2c_softc *msc = &sc->sc_motoi2c;
	int error;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_dev = self;
	msc->sc_iot = iot;
	error = bus_space_map(msc->sc_iot, iobase, size, 0, &msc->sc_ioh);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		        "failed to map registers (errno=%d)\n", error);
		return 1;
	}

	sc->sc_motoi2c_settings.i2c_adr = MOTOI2C_ADR_DEFAULT;
	sc->sc_motoi2c_settings.i2c_dfsrr = MOTOI2C_DFSRR_DEFAULT;
	msc->sc_iord = imxi2c_iord1;
	msc->sc_iowr = imxi2c_iowr1;

	motoi2c_attach_common(self, msc, &sc->sc_motoi2c_settings);

	return 0;
}

int
imxi2c_set_freq(device_t self, long freq, int speed)
{
	struct imxi2c_softc *sc = device_private(self);
	bool found = false;
	int index;

	for (index = 0; index < __arraycount(imxi2c_clk_div); index++) {
		if (freq / imxi2c_clk_div[index].div < speed) {
			found = true;
			break;
		}
	}

	if (found == false)
		sc->sc_motoi2c_settings.i2c_fdr = 0x1f;
	else
		sc->sc_motoi2c_settings.i2c_fdr = imxi2c_clk_div[index].ic_val;

	return 0;
}
