/*	$NetBSD: igsfb_subr.c,v 1.3 2003/05/30 22:41:52 uwe Exp $ */

/*
 * Copyright (c) 2002 Valeriy E. Ushakov
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

/*
 * Integraphics Systems IGA 168x and CyberPro series.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: igsfb_subr.c,v 1.3 2003/05/30 22:41:52 uwe Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/rasops/rasops.h>

#include <dev/ic/igsfbreg.h>
#include <dev/ic/igsfbvar.h>


static void	igsfb_init_seq(struct igsfb_devconfig *);
static void	igsfb_init_crtc(struct igsfb_devconfig *);
static void	igsfb_init_grfx(struct igsfb_devconfig *);
static void	igsfb_init_attr(struct igsfb_devconfig *);
static void	igsfb_init_ext(struct igsfb_devconfig *);
static void	igsfb_init_dac(struct igsfb_devconfig *);

static void	igsfb_freq_latch(struct igsfb_devconfig *);
static void	igsfb_video_on(struct igsfb_devconfig *);



/*
 * Enable chip.
 */
int
igsfb_enable(iot, iobase, ioflags)
	bus_space_tag_t iot;
	bus_addr_t iobase;
	int ioflags;
{
	bus_space_handle_t vdoh;
	bus_space_handle_t vseh;
	bus_space_handle_t regh;
	int ret;

	ret = bus_space_map(iot, iobase + IGS_VDO, 1, ioflags, &vdoh);
	if (ret != 0) {
		printf("unable to map VDO register\n");
		goto out0;
	}

	ret = bus_space_map(iot, iobase + IGS_VSE, 1, ioflags, &vseh);
	if (ret != 0) {
		printf("unable to map VSE register\n");
		goto out1;
	}

	ret = bus_space_map(iot, iobase + IGS_REG_BASE, IGS_REG_SIZE, ioflags,
			    &regh);
	if (ret != 0) {
		printf("unable to map I/O registers\n");
		goto out2;
	}

	/*
	 * Start decoding i/o space accesses.
	 */
	bus_space_write_1(iot, vdoh, 0, IGS_VDO_ENABLE | IGS_VDO_SETUP);
	bus_space_write_1(iot, vseh, 0, IGS_VSE_ENABLE);
	bus_space_write_1(iot, vdoh, 0, IGS_VDO_ENABLE);

	/*
	 * Start decoding memory space accesses (XXX: move out of here?
	 * we program this register in igsfb_init_ext).
	 * While here, enable coprocessor and select IGS_COP_BASE_B.
	 */
	igs_ext_write(iot, regh, IGS_EXT_BIU_MISC_CTL,
		      (IGS_EXT_BIU_LINEAREN
		       | IGS_EXT_BIU_COPREN | IGS_EXT_BIU_COPASELB));

	bus_space_unmap(iot, regh, IGS_REG_SIZE);
  out2:	bus_space_unmap(iot, vseh, 1);
  out1:	bus_space_unmap(iot, vdoh, 1);
  out0: return (ret);
}


/*
 * Init sequencer.
 * This is common for all video modes.
 */
static void
igsfb_init_seq(dc)
	struct igsfb_devconfig *dc;
{
	bus_space_tag_t iot = dc->dc_iot;
	bus_space_handle_t ioh = dc->dc_ioh;

	/* start messing with sequencer */
	igs_seq_write(iot, ioh, IGS_SEQ_RESET, 0);

	igs_seq_write(iot, ioh, 1, 0x01); /* 8 dot clock */
	igs_seq_write(iot, ioh, 2, 0x0f); /* enable all maps */
	igs_seq_write(iot, ioh, 3, 0x00); /* character generator */
	igs_seq_write(iot, ioh, 4, 0x0e); /* memory mode */

	/* this selects color mode among other things */
	bus_space_write_1(iot, ioh, IGS_MISC_OUTPUT_W, 0xef);

	/* normal sequencer operation */
	igs_seq_write(iot, ioh, IGS_SEQ_RESET,
		      IGS_SEQ_RESET_SYNC | IGS_SEQ_RESET_ASYNC);
}

/*
 * Init CRTC to 640x480 8bpp at 60Hz
 */
static void
igsfb_init_crtc(dc)
	struct igsfb_devconfig *dc;
{
	bus_space_tag_t iot = dc->dc_iot;
	bus_space_handle_t ioh = dc->dc_ioh;

	igs_crtc_write(iot, ioh, 0x00, 0x5f);
	igs_crtc_write(iot, ioh, 0x01, 0x4f);
	igs_crtc_write(iot, ioh, 0x02, 0x50);
	igs_crtc_write(iot, ioh, 0x03, 0x80);
	igs_crtc_write(iot, ioh, 0x04, 0x52);
	igs_crtc_write(iot, ioh, 0x05, 0x9d);
	igs_crtc_write(iot, ioh, 0x06, 0x0b);
	igs_crtc_write(iot, ioh, 0x07, 0x3e);

	/* next block is almost constant, only bit 6 in reg 9 differs */
	igs_crtc_write(iot, ioh, 0x08, 0x00);
	igs_crtc_write(iot, ioh, 0x09, 0x40); /* <- either 0x40 or 0x60 */
	igs_crtc_write(iot, ioh, 0x0a, 0x00);
	igs_crtc_write(iot, ioh, 0x0b, 0x00);
	igs_crtc_write(iot, ioh, 0x0c, 0x00);
	igs_crtc_write(iot, ioh, 0x0d, 0x00);
	igs_crtc_write(iot, ioh, 0x0e, 0x00);
	igs_crtc_write(iot, ioh, 0x0f, 0x00);

	igs_crtc_write(iot, ioh, 0x10, 0xe9);
	igs_crtc_write(iot, ioh, 0x11, 0x8b);
	igs_crtc_write(iot, ioh, 0x12, 0xdf);
	igs_crtc_write(iot, ioh, 0x13, 0x50);
	igs_crtc_write(iot, ioh, 0x14, 0x00);
	igs_crtc_write(iot, ioh, 0x15, 0xe6);
	igs_crtc_write(iot, ioh, 0x16, 0x04);
	igs_crtc_write(iot, ioh, 0x17, 0xc3);

	igs_crtc_write(iot, ioh, 0x18, 0xff);
}


/*
 * Init graphics controller.
 * This is common for all video modes.
 */
static void
igsfb_init_grfx(dc)
	struct igsfb_devconfig *dc;
{
	bus_space_tag_t iot = dc->dc_iot;
	bus_space_handle_t ioh = dc->dc_ioh;

	igs_grfx_write(iot, ioh, 0, 0x00);
	igs_grfx_write(iot, ioh, 1, 0x00);
	igs_grfx_write(iot, ioh, 2, 0x00);
	igs_grfx_write(iot, ioh, 3, 0x00);
	igs_grfx_write(iot, ioh, 4, 0x00);
	igs_grfx_write(iot, ioh, 5, 0x60); /* SRMODE, MODE256 */
	igs_grfx_write(iot, ioh, 6, 0x05); /* 64k @ a0000, GRAPHICS */
	igs_grfx_write(iot, ioh, 7, 0x0f); /* color compare all */
	igs_grfx_write(iot, ioh, 8, 0xff); /* bitmask = all bits mutable */
}


/*
 * Init attribute controller.
 * This is common for all video modes.
 */
static void
igsfb_init_attr(dc)
	struct igsfb_devconfig *dc;
{
	bus_space_tag_t iot = dc->dc_iot;
	bus_space_handle_t ioh = dc->dc_ioh;
	int i;

	igs_attr_flip_flop(iot, ioh);	/* reset attr flip-flop to address */

	for (i = 0; i < 16; ++i)	/* crt palette */
		igs_attr_write(iot, ioh, i, i);

	igs_attr_write(iot, ioh, 0x10, 0x01); /* select graphic mode */
	igs_attr_write(iot, ioh, 0x11, 0x00); /* crt overscan color */
	igs_attr_write(iot, ioh, 0x12, 0x0f); /* color plane enable */
	igs_attr_write(iot, ioh, 0x13, 0x00);
	igs_attr_write(iot, ioh, 0x14, 0x00);
}


/*
 * When done with ATTR controller, call this to unblank the screen.
 */
static void
igsfb_video_on(dc)
	struct igsfb_devconfig *dc;
{
	bus_space_tag_t iot = dc->dc_iot;
	bus_space_handle_t ioh = dc->dc_ioh;

	igs_attr_flip_flop(iot, ioh);
	bus_space_write_1(iot, ioh, IGS_ATTR_IDX, 0x20);
	bus_space_write_1(iot, ioh, IGS_ATTR_IDX, 0x20);
}


/*
 * Latch VCLK (b0/b1) and MCLK (b2/b3) values.
 */
static void
igsfb_freq_latch(dc)
	struct igsfb_devconfig *dc;
{
	bus_space_tag_t iot = dc->dc_iot;
	bus_space_handle_t ioh = dc->dc_ioh;

	bus_space_write_1(iot, ioh, IGS_EXT_IDX, 0xb9);
	bus_space_write_1(iot, ioh, IGS_EXT_PORT, 0x80);
	bus_space_write_1(iot, ioh, IGS_EXT_PORT, 0x00);
}


static void
igsfb_init_ext(dc)
	struct igsfb_devconfig *dc;
{
	bus_space_tag_t iot = dc->dc_iot;
	bus_space_handle_t ioh = dc->dc_ioh;
	int is_cyberpro = (dc->dc_id >= 0x2000);

	igs_ext_write(iot, ioh, 0x10, 0x10); /* IGS_EXT_START_ADDR enable */
	igs_ext_write(iot, ioh, 0x12, 0x00); /* IGS_EXT_IRQ_CTL disable  */
	igs_ext_write(iot, ioh, 0x13, 0x00); /* MBZ for normal operation */

	igs_ext_write(iot, ioh, 0x31, 0x00); /* segment write ptr */
	igs_ext_write(iot, ioh, 0x32, 0x00); /* segment read ptr */

	/* IGS_EXT_BIU_MISC_CTL: linearen, copren, copaselb, segon */
	igs_ext_write(iot, ioh, 0x33, 0x1d);

	/* sprite location */
	igs_ext_write(iot, ioh, 0x50, 0x00);
	igs_ext_write(iot, ioh, 0x51, 0x00);
	igs_ext_write(iot, ioh, 0x52, 0x00);
	igs_ext_write(iot, ioh, 0x53, 0x00);
	igs_ext_write(iot, ioh, 0x54, 0x00);
	igs_ext_write(iot, ioh, 0x55, 0x00);
	igs_ext_write(iot, ioh, 0x56, 0x00); /* sprite control */

	/* IGS_EXT_GRFX_MODE */
	igs_ext_write(iot, ioh, 0x57, 0x01); /* raster fb */

	/* overscan R/G/B */
	igs_ext_write(iot, ioh, 0x58, 0x00);
	igs_ext_write(iot, ioh, 0x59, 0x00);
	igs_ext_write(iot, ioh, 0x5A, 0x00);

	/*
	 * Video memory size &c.  We rely on firmware to program
	 * BUS_CTL(30), MEM_CTL1(71), MEM_CTL2(72) appropriately.
	 */

	/* ext memory ctl0 */
	igs_ext_write(iot, ioh, 0x70, 0x0B); /* enable fifo, seq */

	/* ext hidden ctl1 */
	igs_ext_write(iot, ioh, 0x73, 0x30); /* XXX: krups: 0x20 */

	/* ext fifo control */
	igs_ext_write(iot, ioh, 0x74, 0x10); /* XXX: krups: 0x1b */
	igs_ext_write(iot, ioh, 0x75, 0x10); /* XXX: krups: 0x1e */

	igs_ext_write(iot, ioh, 0x76, 0x00); /* ext seq. */
	igs_ext_write(iot, ioh, 0x7A, 0xC8); /* ext. hidden ctl */

	/* ext graphics ctl: GCEXTPATH.  krups 1, nettrom 1, docs 3 */
	igs_ext_write(iot, ioh, 0x90, 0x01);

	if (is_cyberpro)	/* select normal vclk/mclk registers */
	    igs_ext_write(iot, ioh, 0xBF, 0x00);

	igs_ext_write(iot, ioh, 0xB0, 0xD2); /* VCLK = 25.175MHz */
	igs_ext_write(iot, ioh, 0xB1, 0xD3);
	igs_ext_write(iot, ioh, 0xB2, 0xDB); /* MCLK = 75MHz*/
	igs_ext_write(iot, ioh, 0xB3, 0x54);
	igsfb_freq_latch(dc);

	if (is_cyberpro)
	    igs_ext_write(iot, ioh, 0xF8, 0x04); /* XXX: ??? */

	/* 640x480 8bpp at 60Hz */
	igs_ext_write(iot, ioh, 0x11, 0x00);
	igs_ext_write(iot, ioh, 0x77, 0x01); /* 8bpp, indexed */
	igs_ext_write(iot, ioh, 0x14, 0x51);
	igs_ext_write(iot, ioh, 0x15, 0x00);
}


static void
igsfb_init_dac(dc)
	struct igsfb_devconfig *dc;
{
	bus_space_tag_t iot = dc->dc_iot;
	bus_space_handle_t ioh = dc->dc_ioh;
	u_int8_t reg;

	/* RAMDAC address 2 select */
	reg = igs_ext_read(iot, ioh, IGS_EXT_SPRITE_CTL);
	igs_ext_write(iot, ioh, IGS_EXT_SPRITE_CTL,
		      reg | IGS_EXT_SPRITE_DAC_PEL);

	/* VREFEN, DAC8 */
	bus_space_write_1(iot, ioh, IGS_DAC_CMD, 0x06);

	/* restore */
	igs_ext_write(iot, ioh, IGS_EXT_SPRITE_CTL, reg);

	bus_space_write_1(iot, ioh, IGS_PEL_MASK, 0xff);
}


void
igsfb_1024x768_8bpp_60Hz(dc)
	struct igsfb_devconfig *dc;
{
	bus_space_tag_t iot = dc->dc_iot;
	bus_space_handle_t ioh = dc->dc_ioh;

	igs_crtc_write(iot, ioh, 0x11, 0x00); /* write enable CRTC 0..7 */

	igs_crtc_write(iot, ioh, 0x00, 0xa3);
	igs_crtc_write(iot, ioh, 0x01, 0x7f);
	igs_crtc_write(iot, ioh, 0x02, 0x7f); /* krups: 80 */
	igs_crtc_write(iot, ioh, 0x03, 0x85); /* krups: 84 */
	igs_crtc_write(iot, ioh, 0x04, 0x84); /* krups: 88 */
	igs_crtc_write(iot, ioh, 0x05, 0x95); /* krups: 99 */
	igs_crtc_write(iot, ioh, 0x06, 0x24);
	igs_crtc_write(iot, ioh, 0x07, 0xfd);

	/* next block is almost constant, only bit 6 in reg 9 differs */
	igs_crtc_write(iot, ioh, 0x08, 0x00);
	igs_crtc_write(iot, ioh, 0x09, 0x60); /* <- either 0x40 or 0x60 */
	igs_crtc_write(iot, ioh, 0x0a, 0x00);
	igs_crtc_write(iot, ioh, 0x0b, 0x00);
	igs_crtc_write(iot, ioh, 0x0c, 0x00);
	igs_crtc_write(iot, ioh, 0x0d, 0x00);
	igs_crtc_write(iot, ioh, 0x0e, 0x00);
	igs_crtc_write(iot, ioh, 0x0f, 0x00);

	igs_crtc_write(iot, ioh, 0x10, 0x06);
	igs_crtc_write(iot, ioh, 0x11, 0x8c);
	igs_crtc_write(iot, ioh, 0x12, 0xff);
	igs_crtc_write(iot, ioh, 0x13, 0x80); /* depends on BPP */
	igs_crtc_write(iot, ioh, 0x14, 0x0f);
	igs_crtc_write(iot, ioh, 0x15, 0x02);
	igs_crtc_write(iot, ioh, 0x16, 0x21);
	igs_crtc_write(iot, ioh, 0x17, 0xe3);
	igs_crtc_write(iot, ioh, 0x18, 0xff);

	igs_ext_write(iot, ioh, 0xB0, 0xE2); /* VCLK */
	igs_ext_write(iot, ioh, 0xB1, 0x58);
#if 1
	/* XXX: hmm, krups does this */
	igs_ext_write(iot, ioh, 0xB2, 0xE2); /* MCLK */
	igs_ext_write(iot, ioh, 0xB3, 0x58);
#endif
	igsfb_freq_latch(dc);

	igs_ext_write(iot, ioh, 0x11, 0x00);
	igs_ext_write(iot, ioh, 0x77, 0x01); /* 8bpp, indexed */
	igs_ext_write(iot, ioh, 0x14, 0x81);
	igs_ext_write(iot, ioh, 0x15, 0x00);
}


/*
 * igs-video-init from krups prom
 */
void
igsfb_hw_setup(dc)
	struct igsfb_devconfig *dc;
{

	igsfb_init_seq(dc);
	igsfb_init_crtc(dc);
	igsfb_init_attr(dc);
	igsfb_init_grfx(dc);
	igsfb_init_ext(dc);
	igsfb_init_dac(dc);

	igsfb_1024x768_8bpp_60Hz(dc);
	igsfb_video_on(dc);
}
