/*	$NetBSD: ess.c,v 1.1 1998/06/08 17:49:42 tv Exp $	*/

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

#if 0
static char *rcsid = "@(#) $RCSfile: ess.c,v $ $Revision: 1.1 $ (SHARK) $Date: 1998/06/08 17:49:42 $";
#endif

/*
**++
**
**  ess.c
**
**  FACILITY:
**
**	DIGITAL Network Appliance Reference Design (DNARD)
**
**  MODULE DESCRIPTION:
**
**      This module contains the device driver for the ESS
**      Technologies 1888/1887/888 sound chip. The code in sbdsp.c was
**	used as a reference point when implementing this driver.
**
**  AUTHORS:
**
**	Blair Fidler	Software Engineering Australia
**			Gold Coast, Australia.
**
**  CREATION DATE:  
**
**	March 10, 1997.
**
**  MODIFICATION HISTORY:
**
**--
*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/pio.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/mulaw.h>

#include <dev/isa/isavar.h>
#include <arm32/isa/isadmavar.h>

#include <dev/isa/essvar.h>
#include <dev/isa/essreg.h>

#ifdef AUDIO_DEBUG
extern void Dprintf __P((const char *, ...));
#define DPRINTF(x)	if (essdebug) Dprintf x
int	essdebug = 0;
#else
#define DPRINTF(x)
#endif

struct {
	u_int wdsp;
	u_int rdsp;
} eserr;

struct cfattach ess_ca = {
	sizeof(struct ess_softc), ess_probe, ess_attach
};

struct cfdriver ess_cd = {
	NULL, "ess", DV_DULL
};

static char *essmodel[] = {
	"unsupported",
	"1888",
	"1887",
	"888"
};

struct audio_device ess_device = {
	"ESS Technology",
	"x",
	"ess"
};

int	ess_config_addr __P((struct ess_softc *));
void	ess_config_intr __P((struct ess_softc *));
void	ess_identify __P((struct ess_softc *));

int	ess_reset __P((struct ess_softc *));
void	ess_set_gain __P((struct ess_softc *, int, int));
int	ess_set_in_ports __P((struct ess_softc *, int));
void	ess_speaker_on __P((struct ess_softc *));
void	ess_speaker_off __P((struct ess_softc *));
u_int	ess_srtotc __P((u_int));
u_int	ess_srtofc __P((u_int));
u_char	ess_get_dsp_status __P((struct ess_softc *));
u_char	ess_dsp_read_ready __P((struct ess_softc *));
u_char	ess_dsp_write_ready __P((struct ess_softc *sc));
int	ess_rdsp __P((struct ess_softc *));
int	ess_wdsp __P((struct ess_softc *, u_char));
u_char	ess_read_x_reg __P((struct ess_softc *, u_char));
int	ess_write_x_reg __P((struct ess_softc *, u_char, u_char));
void	ess_clear_xreg_bits __P((struct ess_softc *, u_char, u_char));
void	ess_set_xreg_bits __P((struct ess_softc *, u_char, u_char));
u_char	ess_read_mix_reg __P((struct ess_softc *, u_char));
void	ess_write_mix_reg __P((struct ess_softc *, u_char, u_char));
void	ess_clear_mreg_bits __P((struct ess_softc *, u_char, u_char));
void	ess_set_mreg_bits __P((struct ess_softc *, u_char, u_char));


/*
 * Define our interface to the higher level audio driver.
 */

struct audio_hw_if ess_hw_if = {
	ess_open,
	ess_close,
	NULL,
	ess_query_encoding,
	ess_set_params,
	ess_round_blocksize,
	ess_set_out_port,
	ess_get_out_port,
	ess_set_in_port,
	ess_get_in_port,
	ess_commit_settings,
	ess_dma_output,
	ess_dma_input,
	ess_halt_output,
	ess_halt_input,
	ess_cont_output,
	ess_cont_input,
	ess_speaker_ctl,
	ess_getdev,
	ess_setfd,
	ess_set_port,
	ess_get_port,
	ess_query_devinfo,
	1,	/* full-duplex */
	0
};

#ifdef AUDIO_DEBUG
void ess_printsc __P((struct ess_softc *));
void ess_dump_mixer __P((struct ess_softc *));

void
ess_printsc(sc)
	struct ess_softc *sc;
{
	int i;
    
	printf("open %d iobase 0x%x outport %u inport %u speaker %s\n",
	       (int)sc->sc_open, sc->sc_iobase, sc->out_port,
	       sc->in_port, sc->spkr_state ? "on" : "off");

	printf("play: dmachan %d irq %d nintr %lu intr %p arg %p\n",
	       sc->sc_out.drq, sc->sc_out.irq, sc->sc_out.nintr,
	       sc->sc_out.intr, sc->sc_out.arg);

	printf("record: dmachan %d irq %d nintr %lu intr %p arg %p\n",
	       sc->sc_in.drq, sc->sc_in.irq, sc->sc_in.nintr,
	       sc->sc_in.intr, sc->sc_in.arg);

	printf("gain:");
	for (i = 0; i < ESS_NDEVS; i++)
		printf(" %u,%u", sc->gain[i][ESS_LEFT], sc->gain[i][ESS_RIGHT]);
	printf("\n");
}

void
ess_dump_mixer(struct ess_softc *sc)
{
	int gain, left, right;
	int src;
	int stereo;

	printf("ESS_DAC_PLAY_VOL: mix reg 0x%02x=0x%02x\n",
	       0x7C, ess_read_mix_reg(sc, 0x7C));
	printf("ESS_MIC_PLAY_VOL: mix reg 0x%02x=0x%02x\n",
	       0x1A, ess_read_mix_reg(sc, 0x1A));
	printf("ESS_LINE_PLAY_VOL: mix reg 0x%02x=0x%02x\n",
	       0x3E, ess_read_mix_reg(sc, 0x3E));
	printf("ESS_SYNTH_PLAY_VOL: mix reg 0x%02x=0x%02x\n",
	       0x36, ess_read_mix_reg(sc, 0x36));
	printf("ESS_CD_PLAY_VOL: mix reg 0x%02x=0x%02x\n",
	       0x38, ess_read_mix_reg(sc, 0x38));
	printf("ESS_AUXB_PLAY_VOL: mix reg 0x%02x=0x%02x\n",
	       0x3A, ess_read_mix_reg(sc, 0x3A));
	printf("ESS_MASTER_VOL: mix reg 0x%02x=0x%02x\n",
	       0x32, ess_read_mix_reg(sc, 0x32));
	printf("ESS_PCSPEAKER_VOL: mix reg 0x%02x=0x%02x\n",
	       0x3C, ess_read_mix_reg(sc, 0x3C));
	printf("ESS_DAC_REC_VOL: mix reg 0x%02x=0x%02x\n",
	       0x69, ess_read_mix_reg(sc, 0x69));
	printf("ESS_MIC_REC_VOL: mix reg 0x%02x=0x%02x\n",
	       0x68, ess_read_mix_reg(sc, 0x68));
	printf("ESS_LINE_REC_VOL: mix reg 0x%02x=0x%02x\n",
	       0x6E, ess_read_mix_reg(sc, 0x6E));
	printf("ESS_SYNTH_REC_VOL: mix reg 0x%02x=0x%02x\n",
	       0x6B, ess_read_mix_reg(sc, 0x6B));
	printf("ESS_CD_REC_VOL: mix reg 0x%02x=0x%02x\n",
	       0x6A, ess_read_mix_reg(sc, 0x6A));
	printf("ESS_AUXB_REC_VOL: mix reg 0x%02x=0x%02x\n",
	       0x6C, ess_read_mix_reg(sc, 0x6C));
	printf("ESS_RECORD_VOL: x reg 0x%02x=0x%02x\n",
	       0xB4, ess_read_x_reg(sc, 0xB4));
	printf("Audio 1 play vol (unused): mix reg 0x%02x=0x%02x\n",
	       0x14, ess_read_mix_reg(sc, 0x14));

	printf("ESS_MIC_PREAMP: x reg 0x%02x=0x%02x\n",
	       ESS_XCMD_PREAMP_CTRL, ess_read_x_reg(sc, ESS_XCMD_PREAMP_CTRL));
	printf("ESS_RECORD_MONITOR: x reg 0x%02x=0x%02x\n",
	       ESS_XCMD_AUDIO_CTRL, ess_read_x_reg(sc, ESS_XCMD_AUDIO_CTRL));
	printf("Record source: mix reg 0x%02x=0x%02x, 0x%02x=0x%02x\n",
	       0x1c, ess_read_mix_reg(sc, 0x1c),
	       0x7a, ess_read_mix_reg(sc, 0x7a));
}

#endif

/*
 * Configure the ESS chip for the desired audio base address.
 */
int
ess_config_addr(sc)
	struct ess_softc *sc;
{
	register int iobase = sc->sc_iobase;
	register bus_space_tag_t iot = sc->sc_iot;

#ifdef ESS_AMODE_LOW
	/*
	 * Configure using the Read-Sequence-Key method.  This method
	 * is used when the AMODE line is tied low, which is the case
	 * for the evaluation board, but not for the Shark.  First we
	 * read a magic sequence of registers, then we read from the
	 * desired base addresses.  See page 21 of ES1887 data sheet
	 * for details.
	 */

	bus_space_handle_t ioh;

	/*
	 * Get a mapping for the configuration key registers.
	 */
	if (bus_space_map(iot, ESS_CONFIG_KEY_BASE, ESS_CONFIG_KEY_PORTS,
			  0, &ioh))
	{
		printf("ess: can't map configuration key registers\n");
		return -1;
	}

	/*
	 * Read the magic key sequence.
	 */
	bus_space_read_1(iot, ioh, 0);
	bus_space_read_1(iot, ioh, 0);
	bus_space_read_1(iot, ioh, 0);

	bus_space_read_1(iot, ioh, 2);
	bus_space_read_1(iot, ioh, 0);
	bus_space_read_1(iot, ioh, 2);
	bus_space_read_1(iot, ioh, 0);
	bus_space_read_1(iot, ioh, 0);
	bus_space_read_1(iot, ioh, 2);
	bus_space_read_1(iot, ioh, 0);

	/*
	 * Unmap the configuration key registers.
	 */
	bus_space_unmap(iot, ioh, ESS_CONFIG_KEY_PORTS);


	/*
	 * Get a mapping for the audio base address.
	 */
	if (bus_space_map(iot, iobase, 1, 0, &ioh))
	{
		printf("ess: can't map audio base address (0x%x)\n", iobase);
		return -1;
	}

	/*
	 * Read from the audio base address.
	 */
	bus_space_read_1(iot, ioh, 0);

	/*
	 * Unmap the audio base address
	 */
	bus_space_unmap(iot, ioh, 1);
#else
	/*
	 * Configure using the System Control Register method.  This
	 * method is used when the AMODE line is tied high, which is
	 * the case for the Shark, but not for the evaluation board.
	 */

	bus_space_handle_t scr_access_ioh;
	bus_space_handle_t scr_ioh;
	u_short scr_value = 0;

	/*
	 * Set the SCR bit to enable audio.
	 */
	scr_value |= ESS_SCR_AUDIO_ENABLE;

	/*
	 * Set the SCR bits necessary to select the specified audio
	 * base address.
	 */
	switch(iobase)
	{
	case 0x220:
		scr_value |= ESS_SCR_AUDIO_220;
		break;

	case 0x230:
		scr_value |= ESS_SCR_AUDIO_230;
		break;
	case 0x240:
		scr_value |= ESS_SCR_AUDIO_240;
		break;
	case 0x250:
		scr_value |= ESS_SCR_AUDIO_250;
		break;
	default:
		printf("ess: configured iobase 0x%x invalid\n", iobase);
		return -1;
		break;
	}

	/*
	 * Get a mapping for the System Control Register (SCR) access
	 * registers and the SCR data registers.
	 */
	if (bus_space_map(iot, ESS_SCR_ACCESS_BASE, ESS_SCR_ACCESS_PORTS,
			  0, &scr_access_ioh))
	{
		printf("ess: can't map SCR access registers\n");
		return -1;
	}
	if (bus_space_map(iot, ESS_SCR_BASE, ESS_SCR_PORTS,
			  0, &scr_ioh))
	{
		printf("ess: can't map SCR registers\n");
		bus_space_unmap(iot, scr_access_ioh, ESS_SCR_ACCESS_PORTS);
		return -1;
	}

	/*
	 * Unlock the SCR.
	 */
	bus_space_write_1(iot, scr_access_ioh, ESS_SCR_UNLOCK, 0);

	/*
	 * Write the base address information into SCR[0].
	 */
	bus_space_write_1(iot, scr_ioh, ESS_SCR_INDEX, 0);
	bus_space_write_1(iot, scr_ioh, ESS_SCR_DATA, scr_value);
	
	/*
	 * Lock the SCR.
	 */
	bus_space_write_1(iot, scr_access_ioh, ESS_SCR_LOCK, 0);

	/*
	 * Unmap the SCR access ports and the SCR data ports.
	 */
	bus_space_unmap(iot, scr_access_ioh, ESS_SCR_ACCESS_PORTS);
	bus_space_unmap(iot, scr_ioh, ESS_SCR_PORTS);
#endif

	return 0;
}


/*
 * Configure the ESS chip for the desired IRQ and DMA channels.
 */
void
ess_config_intr(sc)
	struct ess_softc *sc;
{
	/*
	 * Configure Audio 1 (record) for the appropriate IRQ line.
	 */
	switch(sc->sc_in.irq)
	{
	case 9:
	    ess_wdsp(sc, 0xb1);
	    ess_wdsp(sc, 0x50);
	    break;
	case 5:
	    ess_wdsp(sc, 0xb1);
	    ess_wdsp(sc, 0x54);
	    break;
	case 7:
	    ess_wdsp(sc, 0xb1);
	    ess_wdsp(sc, 0x58);
	    break;
	case 10:
	    ess_wdsp(sc, 0xb1);
	    ess_wdsp(sc, 0x5c);
	    break;
	default:
	    printf("ess: configured irq chan %d not supported for Audio 1\n", sc->sc_in.irq);
	    break;
	}
	    

	/* REVISIT: tidy up following which enables DMA, IRQ for Audio 2 */
	/* note that for 1888/888 IRQ must be 15 and DRQ must be 5 */
	ess_set_mreg_bits(sc, ESS_MREG_AUDIO2_CTRL2, 0x60);

#if 0
	/*
	 * REVISIT: Only the ES1887 supports register 0x7d - for all
	 *          other chips, DRQ is hardwired to 5 anyway.
	 */
	/* REVISIT: tidy up following which hardcodes DRQ to 5 */
	if (sc->sc_model = ESS_1887)
	{
		ess_set_mreg_bits(sc, 0x7d, 0x03);
	}

	/* REVISIT: Only the ES1887 supports register 0x7f */
	/* REVISIT: tidy up following which hardcodes IRQ to 15 for Audio 2*/
	ess_set_mreg_bits(sc, 0x7f, 0x03);
	ess_clear_mreg_bits(sc, 0x7f, 0x0c);
#endif
	
	/*
	 * Configure Audio 1 (record) for DMA on the appropriate
	 * channel.
	 */
	/*
	 * REVISIT: Does bit 4 really need to be set? Reading the data
	 *          sheet, it seems that this is only necessary for
	 *          compatibility mode.
	 */
	switch(sc->sc_in.drq)
	{
	case 0:
	    ess_wdsp(sc, 0xb2);
	    ess_wdsp(sc, 0x54);
	    break;
	case 1:
	    ess_wdsp(sc, 0xb2);
	    ess_wdsp(sc, 0x58);
	    break;
	case 3:
	    ess_wdsp(sc, 0xb2);
	    ess_wdsp(sc, 0x5c);
	    break;
	default:
	    printf("essdsp: configured dma chan %d not supported for Audio 1\n", sc->sc_in.drq);
	    break;
	}
}


/*
 * Determine the model of ESS chip we are talking to.  Currently we
 * only support ES1888, ES1887 and ES888.  The method of determining
 * the chip is based on the information on page 27 of the ES1887 data
 * sheet. 
 *
 * This routine sets the values of sc->sc_model and sc->sc_version.
 */
void
ess_identify(sc)
	struct ess_softc *sc;
{
	u_char reg1;
	u_char reg2;
	u_char reg3;

	sc->sc_model = ESS_UNSUPPORTED;
	sc->sc_version = 0;


	/*
	 * 1. Check legacy ID bytes.  These should be 0x68 0x8n, where
	 *    n >= 8 for an ES1887 or an ES888.  Other values indicate
	 *    earlier (unsupported) chips.
	 */
	ess_wdsp(sc, ESS_ACMD_LEGACY_ID);

	if ((reg1 = ess_rdsp(sc)) != 0x68)
	{
		printf("ess: First ID byte wrong (0x%02x)\n", reg1);
		return;
	}

	reg2 = ess_rdsp(sc);
	if (((reg2 & 0xf0) != 0x80) ||
	    ((reg2 & 0x0f) < 8))
	{
		printf("ess: Second ID byte wrong (0x%02x)\n", reg2);
		return;
	}

	/*
	 * Store the ID bytes as the version.
	 */
	sc->sc_version = (reg1 << 8) + reg2;


	/*
	 * 2. Verify we can change bit 2 in mixer register 0x64.  This
	 *    should be possible on all supported chips.
	 */
	reg1 = ess_read_mix_reg(sc, 0x64);
	reg2 = reg1 ^ 0x04;  /* toggle bit 2 */
	
	ess_write_mix_reg(sc, 0x64, reg2);
	
	if (ess_read_mix_reg(sc, 0x64) != reg2)
	{
		printf("ess: Hardware error (unable to toggle bit 2 of mixer register 0x64)\n");
		return;
	}

	/*
	 * Restore the original value of mixer register 0x64.
	 */
	ess_write_mix_reg(sc, 0x64, reg1);


	/*
	 * 3. Verify we can change the value of mixer register 0x70.
	 *    This should be possible on all supported chips.
	 */
	reg1 = ess_read_mix_reg(sc, 0x70);
	reg2 = reg1 ^ 0xff;  /* toggle all bits */

	ess_write_mix_reg(sc, 0x70, reg2);
	
	if (ess_read_mix_reg(sc, 0x70) != reg2)
	{
		printf("ess: Harware error (unable to change mixer register 0x70)\n");
		return;
	}

	/*
	 * It is not necessary to restore the value of mixer register 0x70.
	 */


	/*
	 * 4. Determine if we can change bit 5 in mixer register 0x64.
	 *    This determines whether we have an ES1887:
	 *
	 *    - can change indicates ES1887
	 *    - can't change indicates ES1888 or ES888
	 */
	reg1 = ess_read_mix_reg(sc, 0x64);
	reg2 = reg1 ^ 0x20;  /* toggle bit 5 */
	
	ess_write_mix_reg(sc, 0x64, reg2);
	
	if (ess_read_mix_reg(sc, 0x64) == reg2)
	{
		sc->sc_model = ESS_1887;

		/*
		 * Restore the original value of mixer register 0x64.
		 */
		ess_write_mix_reg(sc, 0x64, reg1);
	}
	else
	{
		/*
		 * 5. Determine if we can change the value of mixer
		 *    register 0x69 independently of mixer register
		 *    0x68. This determines which chip we have:
		 *
		 *    - can modify idependently indicates ES888
		 *    - register 0x69 is an alias of 0x68 indicates ES1888
		 */
		reg1 = ess_read_mix_reg(sc, 0x68);
		reg2 = ess_read_mix_reg(sc, 0x69);
		reg3 = reg2 ^ 0xff;  /* toggle all bits */

		/*
		 * Write different values to each register.
		 */
		ess_write_mix_reg(sc, 0x68, reg2);
		ess_write_mix_reg(sc, 0x69, reg3);

		if (ess_read_mix_reg(sc, 0x68) == reg2)
		{
			sc->sc_model = ESS_888;
		}
		else
		{
			sc->sc_model = ESS_1888;
		}
		
		/*
		 * Restore the original value of the registers.
		 */
		ess_write_mix_reg(sc, 0x68, reg1);
		ess_write_mix_reg(sc, 0x69, reg2);
	}

	return;
}


/*
 * Probe / attach routines.
 */

/*
 * Probe for the ESS hardware.
 */
int
#ifdef	__BROKEN_INDIRECT_CONFIG
ess_probe(struct device *parent, void *match, void *aux)
#else
ess_probe(struct device *parent, struct cfdata *cf, void *aux)
#endif
{
#ifdef	__BROKEN_INDIRECT_CONFIG
	register struct ess_softc *sc = match;
#else
	struct ess_softc tempsc;
	register struct ess_softc *sc = &tempsc;
#endif
	register struct isa_attach_args *ia = aux;

	if (!ESS_BASE_VALID(ia->ia_iobase)) {
		printf("ess: configured iobase 0x%x invalid\n", ia->ia_iobase);
		return 0;
	}


	/*
	 * Copy the appropriate ISA attach arguments to the ess softc
	 * structure.
	 */
	sc->sc_iot = ia->ia_iot;
	sc->sc_iobase = ia->ia_iobase;
	/* REVISIT: how do we properly get IRQ/DRQ for record channel */
	sc->sc_in.irq = 9;
	sc->sc_in.drq = 0;
	sc->sc_in.mode = ESS_DMA_SIZE(sc->sc_in.drq);
	sc->sc_out.irq = ia->ia_irq;
	sc->sc_out.drq = ia->ia_drq;
	sc->sc_out.mode = ESS_DMA_SIZE(sc->sc_out.drq);
	sc->sc_ic = ia->ia_ic;

	/*
	 * Configure the ESS chip for the desired audio base address.
	 */
	if (ess_config_addr(sc) != 0)
	{
		return 0;
	}

	/*
	 * Map the device-specific address space. If the mapping
	 * fails, then we can't talk to the device.
	 */
	if (bus_space_map(sc->sc_iot, sc->sc_iobase,
			  ia->ia_iosize, 0, &(sc->sc_ioh)))
	{
		printf("ess: can't map I/O space for device\n");
		return 0;
	}

	/*
	 * Reset the chip.
	 */
	if (ess_reset(sc) < 0) {
		DPRINTF(("ess: couldn't reset chip\n"));
		return 0;
	}

	/*
	 * Identify the ESS chip, and check that it is supported.
	 */
	ess_identify(sc);

	if (sc->sc_model == ESS_UNSUPPORTED)
	{
		DPRINTF(("ess: Unsupported model\n"));
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, ia->ia_iosize);
		return 0;
	}

	/*
	 * Check that requested DMA channels are valid and different.
	 */
	if (!ESS_DRQ1_VALID(sc->sc_in.drq,sc->sc_model))
	{
		printf("ess: record dma chan %d invalid\n", sc->sc_in.drq);
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, ia->ia_iosize);
		return 0;
	}
	if (!ESS_DRQ2_VALID(sc->sc_out.drq,sc->sc_model))
	{
		printf("ess: play dma chan %d invalid\n", sc->sc_out.drq);
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, ia->ia_iosize);
		return 0;
	}
	if (sc->sc_in.drq == sc->sc_out.drq)
	{
		printf("ess: play and record dma chan both %d\n",
		       sc->sc_in.drq);
	}
	
	/*
	 * Check that requested IRQ lines are valid and differen.
	 */
	if (!ESS_IRQ1_VALID(sc->sc_in.irq,sc->sc_model))
	{
		printf("ess: record irq %d invalid\n", sc->sc_in.irq);
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, ia->ia_iosize);
		return 0;
	}
	if (!ESS_IRQ2_VALID(sc->sc_out.irq,sc->sc_model))
	{
		printf("ess: play irq %d invalid\n", sc->sc_out.irq);
		bus_space_unmap(sc->sc_iot, sc->sc_ioh, ia->ia_iosize);
		return 0;
	}
	if (sc->sc_in.irq == sc->sc_out.irq)
	{
		printf("ess: play and record irq both %d\n",
		       sc->sc_in.irq);
	}

#ifndef	__BROKEN_INDIRECT_CONFIG
	/*
	 * Unmap the device-specific address space before returning a
	 * successful probe.  This is necessary because we can no
	 * longer pass the necessary mapping tag and handle to
	 * ess_attach via the softc structure.  Instead, ess_attach
	 * must re-map the device ports and initialise the softc
	 * structure itself.
	 */
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, ia->ia_iosize);
#endif

	return 1;
}


/*
 * Attach hardware to driver, attach hardware driver to audio
 * pseudo-device driver .
 */
void
ess_attach(struct device *parent,
	   struct device *self,
	   void *aux)
{
	register struct ess_softc *sc = (struct ess_softc *)self;
	struct isa_attach_args *ia = (struct isa_attach_args *)aux;
	int err;
	struct audio_params xparams;
        int i;
        u_int v;

#ifndef	__BROKEN_INDIRECT_CONFIG
	/*
	 * Map the device-specific address space.  If the mapping
	 * fails, then we panic because we've already successfully
	 * mapped these ports when ess_probe was called.
	 */
	if (bus_space_map(ia->ia_iot, ia->ia_iobase,
			  ia->ia_iosize, 0, &(sc->sc_ioh)))
	{
		panic("ess_attach: can't map I/O space for device\n");
	}

	/*
	 * Copy the appropriate ISA attach arguments to the ess softc
	 * structure.
	 */
	sc->sc_iot = ia->ia_iot;
	sc->sc_iobase = ia->ia_iobase;
	/* REVISIT: how do we properly get IRQ/DRQ for record channel */
	sc->sc_in.irq = 9;
	sc->sc_in.drq = 0;
	sc->sc_in.mode = ESS_DMA_SIZE(sc->sc_in.drq);
	sc->sc_out.irq = ia->ia_irq;
	sc->sc_out.drq = ia->ia_drq;
	sc->sc_out.mode = ESS_DMA_SIZE(sc->sc_out.drq);
	sc->sc_ic = ia->ia_ic;

	/*
	 * Identify the ESS chip again to fill in the sc_model and
	 * sc_version fields of ess softc structure.
	 */
	ess_identify(sc);

#endif
	/*
	 * Establish interrupt handlers for Audio 1 (record) and
	 * Audio 2 (playback).  (This must be done before configuring
	 * the chip for interrupts, otherwise we migt get a stray
	 * interrupt.
	 */
	sc->sc_out.ih = isa_intr_establish(ia->ia_ic, sc->sc_out.irq,
					   IST_LEVEL, IPL_AUDIO,
					   ess_intr_output, sc);
	sc->sc_in.ih = isa_intr_establish(ia->ia_ic, sc->sc_in.irq,
					  IST_LEVEL, IPL_AUDIO,
					  ess_intr_input, sc);

	printf(" ESS Technology ES%s [version 0x%04x]\n", essmodel[sc->sc_model], sc->sc_version);
	
	/* 
	 * Set record and play parameters to default values defined in
	 * generic audio driver.
	 */
        ess_set_params(sc, AUMODE_RECORD, &audio_default, &xparams);
        ess_set_params(sc, AUMODE_PLAY,   &audio_default, &xparams);

	/*
	 * Do a hardware reset on the mixer.
	 */
	ess_write_mix_reg(sc, ESS_MIX_RESET, ESS_MIX_RESET);

	/*
	 * Set volume of Audio 1 to zero and disable Audio 1 DAC input
	 * to playback mixer, since playback is always through Audio 2.
	 */
	ess_write_mix_reg(sc, 0x14, 0);
	ess_wdsp(sc, ESS_ACMD_DISABLE_SPKR);

	/*
	 * Set hardware record source to use output of the record
	 * mixer. We do the selection of record source in software by
	 * setting the gain of the unused sources to zero. (See
	 * ess_set_in_ports.)
	 */
	ess_set_mreg_bits(sc, 0x1c, 0x07);
	ess_clear_mreg_bits(sc, 0x7a, 0x10);
	ess_set_mreg_bits(sc, 0x7a, 0x08);

	/*
	 * Set gain on each mixer device to a sensible value.
	 * Devices not normally used are turned off, and other devices
	 * are set to 75% volume.
	 */
	for (i = 0; i < ESS_NDEVS; i++) {
		switch(i) {
		case ESS_MIC_PLAY_VOL:
		case ESS_LINE_PLAY_VOL:
		case ESS_CD_PLAY_VOL:
		case ESS_AUXB_PLAY_VOL:
		case ESS_DAC_REC_VOL:
		case ESS_LINE_REC_VOL:
		case ESS_SYNTH_REC_VOL:
		case ESS_CD_REC_VOL:
		case ESS_AUXB_REC_VOL:
			v = 0;
			break;
		default:
			v = ESS_4BIT_GAIN(AUDIO_MAX_GAIN * 3 / 4);
			break;
		}
		sc->gain[i][ESS_LEFT] = sc->gain[i][ESS_RIGHT] = v;
		ess_set_gain(sc, i, 1);
	}

	/*
	 * Set the default input and output devices.
	 */
	ess_set_in_port(sc, ESS_MIC_REC_VOL);
	ess_set_out_port(sc, ESS_MASTER_VOL);

	/*
	 * Disable the speaker until the device is opened.
	 */
	ess_speaker_off(sc);
	sc->spkr_state = SPKR_OFF;

	sprintf(ess_device.name, "ES%s", essmodel[sc->sc_model]);
	sprintf(ess_device.version, "0x%04x", sc->sc_version);

	if ((err = audio_hardware_attach(&ess_hw_if, sc)) != 0)
		printf("ess: could not attach to audio pseudo-device driver (%d)\n", err);
}

/*
 * Various routines to interface to higher level audio driver
 */

int
ess_open(dev_t dev,
	 int flags)
{
	struct ess_softc *sc;
	int unit = AUDIOUNIT(dev);

        DPRINTF(("ess_open: sc=0x%x\n", sc));
    
	if (unit >= ess_cd.cd_ndevs)
		return ENODEV;
    
	sc = ess_cd.cd_devs[unit];

	if (!sc || sc->sc_open != 0 || ess_reset(sc) != 0)
		return ENXIO;

	/*
	 * Configure the ESS chip for the desired IRQ and DMA channel.
	 */
	ess_config_intr(sc);

	sc->sc_open = 1;
	sc->sc_mintr = 0;

	/*
	 * Leave most things as they were; users must change things if
	 * the previous process didn't leave it they way they wanted.
	 * Looked at another way, it's easy to set up a configuration
	 * in one program and leave it for another to inherit.
	 */
	DPRINTF(("ess_open: opened\n"));

	return 0;
}

void
ess_close(void *addr)
{
	/* REVISIT: currently just copied from sbdsp.c */
	struct ess_softc *sc = addr;

        DPRINTF(("ess_close: sc=%p\n", sc));

	sc->sc_open = 0;
	ess_speaker_off(sc);
	sc->spkr_state = SPKR_OFF;
	sc->sc_in.intr = 0;
	sc->sc_out.intr = 0;
	sc->sc_mintr = 0;
	ess_halt_output(sc);
	ess_halt_input(sc);

	DPRINTF(("ess_close: closed\n"));
}

int
ess_getdev(void *addr,
	   struct audio_device *retp)
{
	*retp = ess_device;
	return 0;
}

int
ess_query_encoding(void *addr,
		   struct audio_encoding *fp)
{
	struct ess_softc *sc = addr;

	/*
	 * REVISIT: Review the following, esp to check if there should
	 *          be other cases added to the switch.
	 */
	switch (fp->index) {
	case 0:
		strcpy(fp->name, AudioEulinear);
		fp->encoding = AUDIO_ENCODING_ULINEAR;
		fp->precision = 8;
		fp->flags = 0;
		return 0;
	case 1:
		strcpy(fp->name, AudioEmulaw);
		fp->encoding = AUDIO_ENCODING_ULAW;
		fp->precision = 8;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return 0;
	case 2:
		strcpy(fp->name, AudioElinear);
		fp->encoding = AUDIO_ENCODING_LINEAR;
		fp->precision = 8;
		fp->flags = 0;
		return 0;
        case 3:
		strcpy(fp->name, AudioElinear_le);
		fp->encoding = AUDIO_ENCODING_LINEAR_LE;
		fp->precision = 16;
		fp->flags = 0;
		return 0;
	case 4:
		strcpy(fp->name, AudioEulinear_le);
		fp->encoding = AUDIO_ENCODING_ULINEAR_LE;
		fp->precision = 16;
		fp->flags = 0;
		return 0;
	case 5:
		strcpy(fp->name, AudioElinear_be);
		fp->encoding = AUDIO_ENCODING_LINEAR_BE;
		fp->precision = 16;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return 0;
	case 6:
		strcpy(fp->name, AudioEulinear_be);
		fp->encoding = AUDIO_ENCODING_ULINEAR_BE;
		fp->precision = 16;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return 0;
	default:
		return EINVAL;
	}
	return 0;
}

int
ess_set_params(void *addr,
	       int mode,
	       struct audio_params *p,
	       struct audio_params *q)
{
	struct ess_softc *sc = addr;
	void (*swcode) __P((void *, u_char *buf, int cnt));

	switch (mode)
	{
	case AUMODE_PLAY:
		if (ess_set_out_sr(sc, p->sample_rate, p->channels) != 0 ||
		    ess_set_out_precision(sc, p->precision) != 0 ||
		    ess_set_out_channels(sc, p->channels) != 0)
		{
			return EINVAL;
		}
		sc->sc_out.encoding = p->encoding;
		break;

	case AUMODE_RECORD:
		if (ess_set_in_sr(sc, p->sample_rate, p->channels) != 0 ||
		    ess_set_in_precision(sc, p->precision) != 0 ||
		    ess_set_in_channels(sc, p->channels) != 0)
		{
			return EINVAL;
		}
		sc->sc_in.encoding = p->encoding;
		break;

	default:
		return EINVAL;
		break;
	}


	swcode = 0;

	/*
	 * REVISIT: could abstract some of following around the
	 *          setting of signed/unsigned samples
	 */
	switch (p->encoding) {
	case AUDIO_ENCODING_LINEAR_BE:
		if (p->precision == 16)
			swcode = swap_bytes;
		/* fall into */
	case AUDIO_ENCODING_LINEAR_LE:
		if (mode == AUMODE_PLAY)
			ess_set_mreg_bits(sc, ESS_MREG_AUDIO2_CTRL2,
					  ESS_AUDIO2_CTRL2_FIFO_SIGNED);
		else
			ess_set_xreg_bits(sc, ESS_XCMD_AUDIO1_CTRL1, 
					  ESS_AUDIO1_CTRL1_FIFO_SIGNED);
		break;
	case AUDIO_ENCODING_ULINEAR_BE:
		if (p->precision == 16)
			swcode = swap_bytes;
		/* fall into */
	case AUDIO_ENCODING_ULINEAR_LE:
		if (mode == AUMODE_PLAY)
			ess_clear_mreg_bits(sc, ESS_MREG_AUDIO2_CTRL2,
					    ESS_AUDIO2_CTRL2_FIFO_SIGNED);
		else
			ess_clear_xreg_bits(sc, ESS_XCMD_AUDIO1_CTRL1,
					    ESS_AUDIO1_CTRL1_FIFO_SIGNED);
		break;
	case AUDIO_ENCODING_ULAW:
		swcode = mode == AUMODE_PLAY ? 
			mulaw_to_ulinear8 : ulinear8_to_mulaw;
		if (mode == AUMODE_PLAY)
			ess_clear_mreg_bits(sc, ESS_MREG_AUDIO2_CTRL2,
					    ESS_AUDIO2_CTRL2_FIFO_SIGNED);
		else
			ess_clear_xreg_bits(sc, ESS_XCMD_AUDIO1_CTRL1,
					    ESS_AUDIO1_CTRL1_FIFO_SIGNED);
		break;
	default:
		return EINVAL;
	}
	p->sw_code = swcode;


	/* Update setting for the other mode. */
	/* REVISIT: what the hell is this supposed to do. what is q anyway? */
	q->encoding = p->encoding;
	q->channels = p->channels;
	q->precision = p->precision;

	/*
	 * REVISIT: does the following apply to ESS?
	 * Should wait for chip to be idle.
	 */
	sc->sc_in.active = 0;
	sc->sc_out.active = 0;

	return 0;
}
int
ess_set_in_sr(void *addr,
	      u_long sr,
	      int channels)
{
	register struct ess_softc *sc = addr;

	/* REVISIT: check range? */

	/*
	 * Program the sample rate and filter clock for the record
	 * channel (Audio 1).
	 */
	ess_write_x_reg(sc, ESS_XCMD_SAMPLE_RATE, ess_srtotc(sr));
	ess_write_x_reg(sc, ESS_XCMD_FILTER_CLOCK, ess_srtofc(sr));

	sc->sc_in.rate = sr;

	return 0;
}

int
ess_set_out_sr(void *addr,
	       u_long sr,
	       int channels)
{
	register struct ess_softc *sc = addr;

	/* REVISIT: check range? */

	/*
	 * Program the sample rate and filter clock for the playback
	 * channel (Audio 2).
	 */
	ess_write_mix_reg(sc, 0x70, ess_srtotc(sr));
	ess_write_mix_reg(sc, 0x72, ess_srtofc(sr));

	sc->sc_out.rate = sr;

	return 0;
}

int
ess_set_in_precision(void *addr,
		     u_int precision)
{
	register struct ess_softc *sc = addr;
	int error = 0;

	/*
	 * REVISIT: Should we set DMA transfer type to 2-byte or
	 *          4-byte demand? This would probably better be done
	 *          when configuring the DMA channel. See xreg 0xB9.
	 */
	switch (precision)
	{
	case 8:
		ess_clear_xreg_bits(sc, ESS_XCMD_AUDIO1_CTRL1,
				    ESS_AUDIO1_CTRL1_FIFO_SIZE);
		break;

	case 16:
		ess_set_xreg_bits(sc, ESS_XCMD_AUDIO1_CTRL1,
				  ESS_AUDIO1_CTRL1_FIFO_SIZE);
		break;

	default:
		error = EINVAL;
		break;
	}

	sc->sc_in.precision = precision;
	return error;
}

int
ess_set_out_precision(void *addr,
		      u_int precision)
{
	register struct ess_softc *sc = addr;
	int error = 0;
	switch (precision)
	{
	case 8:
		ess_clear_mreg_bits(sc, ESS_MREG_AUDIO2_CTRL2,
				    ESS_AUDIO2_CTRL2_FIFO_SIZE);
		break;

	case 16:
		ess_set_mreg_bits(sc, ESS_MREG_AUDIO2_CTRL2,
				  ESS_AUDIO2_CTRL2_FIFO_SIZE);
		break;

	default:
		error = EINVAL;
		break;
	}

	/*
	 * REVISIT: This actually sets transfer size to 16
	 *          bits. Should this really be hard-coded? This would
	 *          probably better be done when configuring the DMA
	 *          channel.
	 */
	ess_set_mreg_bits(sc, ESS_MREG_AUDIO2_CTRL1,
			  ESS_AUDIO2_CTRL1_XFER_SIZE);

#if 0
	/*
	 * REVISIT: Should we set DMA transfer type to 2-byte,
	 *          4-byte, or 8-byte demand? (Following does 8-byte.)
	 *          This would probably better be done when
	 *          configuring the DMA channel.
	 */
	ess_set_mreg_bits(sc, ESS_MREG_AUDIO2_CTRL1,
			  0xc0);
#endif
	if (!error)
	{
		sc->sc_out.precision = precision;
	}
	return error;
}

int
ess_set_in_channels(void *addr,
		    int channels)
{
	register struct ess_softc *sc = addr;
	int error = 0;

	switch(channels)
	{
	case 1:
		ess_set_xreg_bits(sc, ESS_XCMD_AUDIO_CTRL,
				  ESS_AUDIO_CTRL_MONO);
		ess_clear_xreg_bits(sc, ESS_XCMD_AUDIO_CTRL,
				    ESS_AUDIO_CTRL_STEREO);
		break;

	case 2:
		ess_set_xreg_bits(sc, ESS_XCMD_AUDIO_CTRL,
				  ESS_AUDIO_CTRL_STEREO);
		ess_clear_xreg_bits(sc, ESS_XCMD_AUDIO_CTRL,
				    ESS_AUDIO_CTRL_MONO);
		break;

	default:
		error = EINVAL;
		break;
	}

	sc->sc_in.active = 0;
	sc->sc_in.channels = channels;

	return error;
}

int
ess_set_out_channels(void *addr,
		     int channels)
{
	register struct ess_softc *sc = addr;
	int error = 0;

	switch(channels)
	{
	case 1:
		ess_clear_mreg_bits(sc, ESS_MREG_AUDIO2_CTRL2,
				    ESS_AUDIO2_CTRL2_CHANNELS);
		break;

	case 2:
		ess_set_mreg_bits(sc, ESS_MREG_AUDIO2_CTRL2,
				  ESS_AUDIO2_CTRL2_CHANNELS);
		break;

	default:
		error = EINVAL;
		break;
	}

	sc->sc_out.active = 0;
	sc->sc_out.channels = channels;
	
	return error;
}

int
ess_dma_output(void *addr,
	       void *p,
	       int cc,
	       void (*intr) __P((void *)),
	       void *arg)
{
	register struct ess_softc *sc = addr;

#ifdef AUDIO_DEBUG
	if (essdebug > 1)
		Dprintf("ess_dma_output: cc=%d 0x%x (0x%x)\n", cc, intr, arg);
#endif

	if (sc->sc_out.channels == 2 && (cc & 1))
	{
		DPRINTF(("stereo playback odd bytes (%d)\n", cc));
		return EIO;
	}

	isa_dmastart(DMAMODE_WRITE, p, cc, sc->sc_out.drq);
	sc->sc_out.active = 1;
	sc->sc_out.intr = intr;
	sc->sc_out.arg = arg;
	sc->sc_out.dmaflags = DMAMODE_WRITE;
	sc->sc_out.dmaaddr = p;

	if (sc->sc_out.dmacnt != cc)
	{
		sc->sc_out.dmacnt = cc;

		/*
		 * If doing 16-bit DMA transfers, then the number of
		 * transfers required is half the number of bytes to
		 * be transferred.
		 */
		if (sc->sc_out.mode == ESS_MODE_16BIT)
		{
			cc >>= 1;
		}

		/*
		 * Program transfer count registers with 2's
		 * complement of count.
		 */
		cc = ~cc;	/* 1's complement */
		cc++;		/* 2's complement */
		ess_write_mix_reg(sc, ESS_MREG_XFER_COUNTLO, cc);
		ess_write_mix_reg(sc, ESS_MREG_XFER_COUNTHI, cc >> 8);
	}

/* REVISIT: is it really necessary to clear then set these bits to get
the next lot of DMA to happen?  Would it be sufficient to set the bits
the first time round and leave it at that? (No, because the chip automatically clears the FIFO_ENABLE bit after the DMA is complete.)
*/
	ess_set_mreg_bits(sc, ESS_MREG_AUDIO2_CTRL1,
			  ESS_AUDIO2_CTRL1_DAC_ENABLE);/* REVISIT: once only */
	ess_set_mreg_bits(sc, ESS_MREG_AUDIO2_CTRL1,
			  ESS_AUDIO2_CTRL1_FIFO_ENABLE);
#if 1
/* REVISIT: seems like the 888 and 1888 have an interlock that
 * prevents audio2 channel from working if audio1 channel is not
 * connected to the FIFO.
 */
	ess_set_xreg_bits(sc, 0xB7, 0x80);
#endif
	return 0;

}

int
ess_dma_input(void *addr,
	      void *p,
	      int cc,
	      void (*intr) __P((void *)),
	      void *arg)
{
	register struct ess_softc *sc = addr;

#ifdef AUDIO_DEBUG
	if (essdebug > 1)
		Dprintf("ess_dma_input: cc=%d 0x%x (0x%x)\n", cc, intr, arg);
#endif
	/* REVISIT: Hack to enable Audio1 FIFO connection to CODEC. */
	ess_set_xreg_bits(sc, 0xB7, 0x80);

	if (sc->sc_in.channels == 2 && (cc & 1))
	{
		DPRINTF(("stereo record odd bytes (%d)\n", cc));
		return EIO;
	}

	isa_dmastart(DMAMODE_READ, p, cc, sc->sc_in.drq);
	sc->sc_in.active = 1;
	sc->sc_in.intr = intr;
	sc->sc_in.arg = arg;
	sc->sc_in.dmaflags = DMAMODE_READ;
	sc->sc_in.dmaaddr = p;

	if (sc->sc_in.dmacnt != cc)
	{
		sc->sc_in.dmacnt = cc;

		/*
		 * If doing 16-bit DMA transfers, then the number of
		 * transfers required is half the number of bytes to
		 * be transferred.
		 */
		if (sc->sc_in.mode == ESS_MODE_16BIT)
		{
			cc >>= 1;
		}

		/*
		 * Program transfer count registers with 2's
		 * complement of count.
		 */
		cc = ~cc;	/* 1's complement */
		cc++;		/* 2's complement */
		ess_write_x_reg(sc, ESS_XCMD_XFER_COUNTLO, cc);
		ess_write_x_reg(sc, ESS_XCMD_XFER_COUNTHI, cc >> 8);
	}

/* REVISIT: is it really necessary to clear then set these bits to get
the next lot of DMA to happen?  Would it be sufficient to set the bits
the first time round and leave it at that? (No, because the chip automatically clears the FIFO_ENABLE bit after the DMA is complete.)
*/
	ess_set_xreg_bits(sc, ESS_XCMD_AUDIO1_CTRL2,
			  ESS_AUDIO1_CTRL2_DMA_READ |  /* REVISIT: once only */
			  ESS_AUDIO1_CTRL2_ADC_ENABLE |/* REVISIT: once only */
			  ESS_AUDIO1_CTRL2_FIFO_ENABLE);

	return 0;

}

int
ess_halt_output(void *addr)
{
	struct ess_softc *sc = addr;

	DPRINTF(("ess_halt_output: sc=%p\n", sc));

	ess_clear_mreg_bits(sc, ESS_MREG_AUDIO2_CTRL2,
			    ESS_AUDIO2_CTRL2_DMA_ENABLE);
	return 0;
}

int
ess_halt_input(void *addr)
{
	struct ess_softc *sc = addr;

	DPRINTF(("ess_halt_input: sc=%p\n", sc));

	ess_clear_xreg_bits(sc, ESS_XCMD_AUDIO1_CTRL2,
			    ESS_AUDIO1_CTRL2_FIFO_ENABLE);
	return 0;
}

int
ess_cont_output(void *addr)
{
	struct ess_softc *sc = addr;

	DPRINTF(("ess_cont_output: sc=%p\n", sc));

	ess_set_mreg_bits(sc, ESS_MREG_AUDIO2_CTRL2,
			  ESS_AUDIO2_CTRL2_DMA_ENABLE);
	return 0;
}

int
ess_cont_input(void *addr)
{
	struct ess_softc *sc = addr;

	DPRINTF(("ess_cont_output: sc=%p\n", sc));

	ess_clear_xreg_bits(sc, ESS_XCMD_AUDIO1_CTRL2,
			    ESS_AUDIO1_CTRL2_FIFO_ENABLE);
	return 0;
}

int
ess_speaker_ctl(void *addr,
		int newstate)
{
	struct ess_softc *sc = addr;

	if ((newstate == SPKR_ON) && (sc->spkr_state == SPKR_OFF))
	{
		ess_speaker_on(sc);
		sc->spkr_state = SPKR_ON;
	}
	if ((newstate == SPKR_OFF) && (sc->spkr_state == SPKR_ON))
	{
		ess_speaker_off(sc);
		sc->spkr_state = SPKR_OFF;
	}
	return 0;
}

int
ess_intr_output(arg)
	void *arg;
{
	register struct ess_softc *sc = arg;
	u_char x;

#ifdef AUDIO_DEBUG
	if (essdebug > 1)
		Dprintf("ess_intr_output: intr=0x%x\n", sc->sc_out.intr);
#endif

	/* clear interrupt on Audio channel 2*/
	ess_clear_mreg_bits(sc, ESS_MREG_AUDIO2_CTRL2, 0x80);

	sc->sc_out.nintr++;

	if (sc->sc_out.intr != 0) {
		isa_dmadone(sc->sc_out.dmaflags, sc->sc_out.dmaaddr,
			    sc->sc_out.dmacnt, sc->sc_out.drq);
		(*sc->sc_out.intr)(sc->sc_out.arg);
	}
	else
		return 1;	/* revisit: was 0 */

	return 1;
}

int
ess_intr_input(arg)
	void *arg;
{
	register struct ess_softc *sc = arg;
	u_char x;

#ifdef AUDIO_DEBUG
	if (essdebug > 1)
		Dprintf("ess_intr_input: intr=0x%x\n", sc->sc_in.intr);
#endif

	/*
	 * Disable DMA for Audio 1; it will be enabled again the next
	 * time ess_dma_input is called. Note that for single DMAs,
	 * this bit must be toggled for each DMA. For auto-initialize
	 * DMAs, this bit should be left high.
	 */
	ess_clear_xreg_bits(sc, ESS_XCMD_AUDIO1_CTRL2,
			    ESS_AUDIO1_CTRL2_FIFO_ENABLE);

	/* clear interrupt on Audio channel 1*/
	x = bus_space_read_1(sc->sc_iot, sc->sc_ioh, ESS_CLEAR_INTR);

	sc->sc_in.nintr++;

	if (sc->sc_in.intr != 0) {
		isa_dmadone(sc->sc_in.dmaflags, sc->sc_in.dmaaddr,
			    sc->sc_in.dmacnt, sc->sc_in.drq);
		(*sc->sc_in.intr)(sc->sc_in.arg);
	}
	else
		return 0;

	return 1;
}

int
ess_round_blocksize(addr, blk)
	void *addr;
	int blk;
{
	register struct ess_softc *sc = addr;
	/* REVISIT: need to do a proper implementation for ESS */
	return blk;

	sc->sc_in.dmacnt = 0;
	sc->sc_out.dmacnt = 0;

	/* Higher speeds need bigger blocks to avoid popping and silence gaps. */
	if (blk < NBPG/4 || blk > NBPG/2) {
		if (sc->sc_out.rate > 8000 || sc->sc_in.rate > 8000)
				blk = NBPG/2;
	}
	/* don't try to DMA too much at once, though. */
	if (blk > NBPG)
		blk = NBPG;

	if (sc->sc_out.channels == 2)
		return (blk & ~1); /* must be even to preserve stereo separation */
	else
		return (blk);	/* Anything goes :-) */
}

int
ess_set_out_port(void *addr,
		 int port)
{
	struct ess_softc *sc = addr;
	
	sc->out_port = port; /* Just record it */

	return 0;
}

int
ess_get_out_port(void *addr)
{
	struct ess_softc *sc = addr;

	return sc->out_port;
}


int
ess_set_in_port(void *addr,
		int port)
{
	return ess_set_in_ports(addr, 1 << port);
}

int
ess_get_in_port(void *addr)
{
	struct ess_softc *sc = addr;

	return sc->in_port;
}
int
ess_commit_settings(void *addr)
{
	/*
	 * REVISIT: What adjustments do we need to make here?
	 *          (Probably none since everything is now done via
	 *          ess_set_params.)
	 */
	return 0;
}

int
ess_setfd(addr, flag)
	void *addr;
	int flag;
{
	/* REVISIT: what do we do once we can do full-duplex? */
	return ENOTTY;
}

int
ess_set_port(void *addr,
		   mixer_ctrl_t *cp)
{
	struct ess_softc *sc = addr;
	int lgain, rgain;
    
	DPRINTF(("ess_set_port: port=%d num_channels=%d\n",
		 cp->dev, cp->un.value.num_channels));

	switch (cp->dev)
	{
	/*
	 * The following mixer ports are all stereo. If we get a
	 * single-channel gain value passed in, then we duplicate it
	 * to both left and right channels.
	 */
	case ESS_MASTER_VOL:
	case ESS_DAC_PLAY_VOL:
	case ESS_MIC_PLAY_VOL:
	case ESS_LINE_PLAY_VOL:
	case ESS_SYNTH_PLAY_VOL:
	case ESS_CD_PLAY_VOL:
	case ESS_AUXB_PLAY_VOL:
	case ESS_DAC_REC_VOL:
	case ESS_MIC_REC_VOL:
	case ESS_LINE_REC_VOL:
	case ESS_SYNTH_REC_VOL:
	case ESS_CD_REC_VOL:
	case ESS_AUXB_REC_VOL:
	case ESS_RECORD_VOL:
		if (cp->type != AUDIO_MIXER_VALUE)
			return EINVAL;

		switch (cp->un.value.num_channels) {
		case 1:
			lgain = rgain = ESS_4BIT_GAIN(
			  cp->un.value.level[AUDIO_MIXER_LEVEL_MONO]);
			break;
		case 2:
			lgain = ESS_4BIT_GAIN(
			  cp->un.value.level[AUDIO_MIXER_LEVEL_LEFT]);
			rgain = ESS_4BIT_GAIN(
			  cp->un.value.level[AUDIO_MIXER_LEVEL_RIGHT]);
			break;
		default:
			return EINVAL;
		}

		sc->gain[cp->dev][ESS_LEFT]  = lgain;
		sc->gain[cp->dev][ESS_RIGHT] = rgain;

		ess_set_gain(sc, cp->dev, 1);
		break;


	/*
	 * The PC speaker port is mono. If we get a stereo gain value
	 * passed in, then we return EINVAL.
	 */
	case ESS_PCSPEAKER_VOL:
		if (cp->un.value.num_channels != 1)
			return EINVAL;

		sc->gain[cp->dev][ESS_LEFT]  = sc->gain[cp->dev][ESS_RIGHT] =
		  ESS_3BIT_GAIN(cp->un.value.level[AUDIO_MIXER_LEVEL_MONO]);
		ess_set_gain(sc, cp->dev, 1);
		break;


	case ESS_MIC_PREAMP:
		if (cp->type != AUDIO_MIXER_ENUM)
			return EINVAL;

		if (cp->un.ord)
		{
			/* Enable microphone preamp */
			ess_set_xreg_bits(sc, ESS_XCMD_PREAMP_CTRL,
					  ESS_PREAMP_CTRL_ENABLE);
		}
		else
		{
			/* Disable microphone preamp */
			ess_clear_xreg_bits(sc, ESS_XCMD_PREAMP_CTRL,
					  ESS_PREAMP_CTRL_ENABLE);
		}
		break;

	case ESS_RECORD_SOURCE:
		if (cp->type == AUDIO_MIXER_SET)
		{
			return ess_set_in_ports(sc, cp->un.mask);
		}
		else
		{
			return EINVAL;
		}
		break;

	case ESS_RECORD_MONITOR:
		if (cp->type != AUDIO_MIXER_ENUM)
			return EINVAL;

		if (cp->un.ord)
		{
			/* Enable monitor */
			ess_set_xreg_bits(sc, ESS_XCMD_AUDIO_CTRL,
					  ESS_AUDIO_CTRL_MONITOR);
		}
		else
		{
			/* Disable monitor */
			ess_clear_xreg_bits(sc, ESS_XCMD_AUDIO_CTRL,
					    ESS_AUDIO_CTRL_MONITOR);
		}
		break;

	default:
		return EINVAL;
	}

	return 0;
}

int
ess_get_port(void *addr,
	     mixer_ctrl_t *cp)
{
	struct ess_softc *sc = addr;
    
	DPRINTF(("ess_get_port: port=%d\n", cp->dev));

	switch (cp->dev)
	{
	case ESS_DAC_PLAY_VOL:
	case ESS_MIC_PLAY_VOL:
	case ESS_LINE_PLAY_VOL:
	case ESS_SYNTH_PLAY_VOL:
	case ESS_CD_PLAY_VOL:
	case ESS_AUXB_PLAY_VOL:
	case ESS_MASTER_VOL:
	case ESS_PCSPEAKER_VOL:
	case ESS_DAC_REC_VOL:
	case ESS_MIC_REC_VOL:
	case ESS_LINE_REC_VOL:
	case ESS_SYNTH_REC_VOL:
	case ESS_CD_REC_VOL:
	case ESS_AUXB_REC_VOL:
	case ESS_RECORD_VOL:
		if (cp->dev == ESS_PCSPEAKER_VOL &&
		    cp->un.value.num_channels != 1)
			return EINVAL;

		switch (cp->un.value.num_channels)
		{
		case 1:
			cp->un.value.level[AUDIO_MIXER_LEVEL_MONO] = 
				sc->gain[cp->dev][ESS_LEFT];
			break;
		case 2:
			cp->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = 
				sc->gain[cp->dev][ESS_LEFT];
			cp->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = 
				sc->gain[cp->dev][ESS_RIGHT];
			break;
		default:
			return EINVAL;
		}
		break;

	case ESS_MIC_PREAMP:
		cp->un.ord = (ess_read_x_reg(sc, ESS_XCMD_PREAMP_CTRL) &
			      ESS_PREAMP_CTRL_ENABLE) ? 1 : 0;
		break;

	case ESS_RECORD_SOURCE:
		cp->un.mask = sc->in_mask;
		break;

	case ESS_RECORD_MONITOR:
		cp->un.ord = (ess_read_x_reg(sc, ESS_XCMD_AUDIO_CTRL) &
			      ESS_AUDIO_CTRL_MONITOR) ? 1 : 0;
		break;

	default:
		return EINVAL;
	}

	return 0;
}

int
ess_query_devinfo(void *addr,
			mixer_devinfo_t *dip)
{
	struct ess_softc *sc = addr;

	DPRINTF(("ess_query_devinfo: model=%d index=%d\n", 
		 sc->sc_model, dip->index));

	/*
	 * REVISIT: There are some slight differences between the
	 *          mixers on the different ESS chips, which can
	 *          be sorted out using the chip model rather than a
	 *          separate mixer model.
	 *          This is currently coded assuming an ES1887; we
	 *          need to work out which bits are not applicable to
	 *          the other models (1888 and 888).
	 */
	switch (dip->index) {
	case ESS_DAC_PLAY_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = ESS_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNdac);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;

	case ESS_MIC_PLAY_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = ESS_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNmicrophone);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;

	case ESS_LINE_PLAY_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = ESS_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNline);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;

	case ESS_SYNTH_PLAY_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = ESS_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNfmsynth);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;

	case ESS_CD_PLAY_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = ESS_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNcd);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;

	case ESS_AUXB_PLAY_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = ESS_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, "auxb");
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;

	case ESS_INPUT_CLASS:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = ESS_INPUT_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCInputs);
		return 0;


	case ESS_MASTER_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = ESS_OUTPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNmaster);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;

	case ESS_PCSPEAKER_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = ESS_OUTPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, "pc_speaker");
		dip->un.v.num_channels = 1;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;

	case ESS_OUTPUT_CLASS:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = ESS_OUTPUT_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCOutputs);
		return 0;


	case ESS_DAC_REC_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = ESS_RECORD_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNdac);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;

	case ESS_MIC_REC_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = ESS_RECORD_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = ESS_MIC_PREAMP;
		strcpy(dip->label.name, AudioNmicrophone);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;

	case ESS_LINE_REC_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = ESS_RECORD_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNline);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;

	case ESS_SYNTH_REC_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = ESS_RECORD_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNfmsynth);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;

	case ESS_CD_REC_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = ESS_RECORD_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNcd);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;

	case ESS_AUXB_REC_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = ESS_RECORD_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, "auxb");
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;

	case ESS_MIC_PREAMP:
		dip->type = AUDIO_MIXER_ENUM;
		dip->mixer_class = ESS_RECORD_CLASS;
		dip->prev = ESS_MIC_REC_VOL;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNenhanced);
		dip->un.e.num_mem = 2;
		strcpy(dip->un.e.member[0].label.name, AudioNoff);
		dip->un.e.member[0].ord = 0;
		strcpy(dip->un.e.member[1].label.name, AudioNon);
		dip->un.e.member[1].ord = 1;
		return 0;

	case ESS_RECORD_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = ESS_RECORD_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNrecord);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;

	case ESS_RECORD_SOURCE:
		dip->mixer_class = ESS_RECORD_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNsource);
		dip->type = AUDIO_MIXER_SET;
		dip->un.s.num_mem = 6;
		strcpy(dip->un.s.member[0].label.name, AudioNdac);
		dip->un.s.member[0].mask = 1 << ESS_DAC_REC_VOL;
		strcpy(dip->un.s.member[1].label.name, AudioNmicrophone);
		dip->un.s.member[1].mask = 1 << ESS_MIC_REC_VOL;
		strcpy(dip->un.s.member[2].label.name, AudioNline);
		dip->un.s.member[2].mask = 1 << ESS_LINE_REC_VOL;
		strcpy(dip->un.s.member[3].label.name, AudioNfmsynth);
		dip->un.s.member[3].mask = 1 << ESS_SYNTH_REC_VOL;
		strcpy(dip->un.s.member[4].label.name, AudioNcd);
		dip->un.s.member[4].mask = 1 << ESS_CD_REC_VOL;
		strcpy(dip->un.s.member[5].label.name, "auxb");
		dip->un.s.member[5].mask = 1 << ESS_AUXB_REC_VOL;
		return 0;

	case ESS_RECORD_CLASS:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = ESS_RECORD_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCRecord);
		return 0;


	case ESS_RECORD_MONITOR:
		dip->mixer_class = ESS_MONITOR_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNmonitor);
		dip->type = AUDIO_MIXER_ENUM;
		dip->un.e.num_mem = 2;
		strcpy(dip->un.e.member[0].label.name, AudioNoff);
		dip->un.e.member[0].ord = 0;
		strcpy(dip->un.e.member[1].label.name, AudioNon);
		dip->un.e.member[1].ord = 1;
		return 0;

	case ESS_MONITOR_CLASS:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = ESS_MONITOR_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCMonitor);
		return 0;
	}

	return ENXIO;
}
/* ============================================
 * Generic functions for ess, not used by audio h/w i/f
 * =============================================
 */

/*
 * Reset the chip.
 * Return non-zero if the chip isn't detected.
 */
int
ess_reset(struct ess_softc *sc)
{
	/* REVISIT: currently just copied from sbdsp.c */
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	sc->sc_in.intr = 0;
	sc->sc_in.dmacnt = 0;
	if (sc->sc_in.active) {
		isa_dmaabort(sc->sc_in.drq);
		sc->sc_in.active = 0;
	}

	sc->sc_out.intr = 0;
	sc->sc_out.dmacnt = 0;
	if (sc->sc_out.active) {
		isa_dmaabort(sc->sc_out.drq);
		sc->sc_out.active = 0;
	}

	/*
	 * See SBK, section 11.3.
	 * We pulse a reset signal into the card.
	 * Gee, what a brilliant hardware design.
	 */
	/* REVISIT: need to properly document the use of 3 below */
	bus_space_write_1(iot, ioh, ESS_DSP_RESET, 3);
	delay(10);
	bus_space_write_1(iot, ioh, ESS_DSP_RESET, 0);
	delay(30);
	if (ess_rdsp(sc) != ESS_MAGIC)
		return -1;

	/*
	 * Enable access to the ESS extension commands, which are
	 * disabled by each reset.
	 */
	ess_wdsp(sc, ESS_ACMD_ENABLE_EXT);

	return 0;
}

void
ess_set_gain(struct ess_softc *sc,
		   int port,
		   int on)
{
	int gain, left, right;
	int mix;
	int src;
	int stereo;

	/*
	 * Most gain controls are found in the mixer registers and
	 * are stereo. Any that are not, must set mix and stereo as
	 * required.
	 */
	mix = 1;
	stereo = 1;

	switch (port) {
	case ESS_MASTER_VOL:
		src = 0x32;
		break;
	case ESS_DAC_PLAY_VOL:
		src = 0x7C;
		break;
	case ESS_MIC_PLAY_VOL:
		src = 0x1A;
		break;
	case ESS_LINE_PLAY_VOL:
		src = 0x3E;
		break;
	case ESS_SYNTH_PLAY_VOL:
		src = 0x36;
		break;
	case ESS_CD_PLAY_VOL:
		src = 0x38;
		break;
	case ESS_AUXB_PLAY_VOL:
		src = 0x3A;
		break;
	case ESS_PCSPEAKER_VOL:
		src = 0x3C;
		stereo = 0;
		break;
	case ESS_DAC_REC_VOL:
		src = 0x69;
		break;
	case ESS_MIC_REC_VOL:
		src = 0x68;
		break;
	case ESS_LINE_REC_VOL:
		src = 0x6E;
		break;
	case ESS_SYNTH_REC_VOL:
		src = 0x6B;
		break;
	case ESS_CD_REC_VOL:
		src = 0x6A;
		break;
	case ESS_AUXB_REC_VOL:
		src = 0x6C;
		break;
	case ESS_RECORD_VOL:
		src = 0xB4;
		mix = 0;
		break;
	default:
		return;
	}

	if (on)
	{
		left = sc->gain[port][ESS_LEFT];
		right = sc->gain[port][ESS_RIGHT];
	}
	else
	{
		left = right = 0;
	}

	if (stereo)
	{
		gain = ESS_STEREO_GAIN(left, right);
	}
	else
	{
		gain = ESS_MONO_GAIN(left);
	}

	if (mix)
	{
		ess_write_mix_reg(sc, src, gain);
	}
	else
	{
		ess_write_x_reg(sc, src, gain);
	}
}

int
ess_set_in_ports(struct ess_softc *sc,
		 int mask)
{
	mixer_devinfo_t di;
	int i;
	int on;
	int port;
	int tmp;

	DPRINTF(("ess_set_in_ports: mask=0x%x\n", mask));

	/*
	 * Get the device info for the record source control,
	 * including the list of available sources.
	 */
	di.index = ESS_RECORD_SOURCE;
	if (ess_query_devinfo(sc, &di))
		return EINVAL;

	/*
	 * Set or disable the record volume control for each of the
	 * possible sources.
	 */
	for (i = 0; i < di.un.s.num_mem; i++)
	{
		/*
		 * Calculate the source port number from its mask.
		 */
		tmp = di.un.s.member[i].mask >> 1;
		for (port = 0; tmp; port++)
		{
			tmp >>= 1;
		}

		/*
		 * Set the source gain:
		 *	to the current value if source is enabled
		 *	to zero if source is disabled
		 */
		ess_set_gain(sc, port, mask & di.un.s.member[i].mask);
	}

	sc->in_mask = mask;

	/*
	 * We have to fake a single port since the upper layer expects
	 * one only. We choose the lowest numbered port that is enabled.
	 */
	for(i = 0; i < ESS_NPORT; i++) {
		if (mask & (1 << i)) {
			sc->in_port = i;
			break;
		}
	}

	return 0;
}

void
ess_speaker_on(struct ess_softc *sc)
{
	/*
	 * Disable mute on left- and right-master volume.
	 */
	ess_clear_mreg_bits(sc, 0x60, 0x40);
	ess_clear_mreg_bits(sc, 0x62, 0x40);
}

void
ess_speaker_off(struct ess_softc *sc)
{
	/*
	 * Enable mute on left- and right-master volume.
	 */
	ess_set_mreg_bits(sc, 0x60, 0x40);
	ess_set_mreg_bits(sc, 0x62, 0x40);
}

/*
 * Calculate the time constant for the requested sampling rate.
 */
u_int
ess_srtotc(u_int rate)
{
	u_int tc;

/*
 * REVISIT: Should we clamp the rate?
 */
#if 0
	if (rate < ESS_MINRATE)
	{
		rate = ESS_MINRATE;
	}
	else if (rate > ESS_MAXRATE)
	{
		rate = ESS_MAXRATE;
	}
#endif

	/*
	 * The following formulae are from the ESS data sheet.
	 */
	if (rate < 22050)
	{
		tc = 128 - 397700L / rate;
	}
	else
	{
		tc = 256 - 795500L / rate;
	}

	return (tc);
}


/*
 * Calculate the filter constant for the reuqested sampling rate.
 */
u_int
ess_srtofc(u_int rate)
{
	/*
	 * The following formula is derived from the information in
	 * the ES1887 data sheet, based on a roll-off frequency of
	 * 87%.
	 */
	return (256 - 200279L / rate);
}


/*
 * Return the status of the DSP.
 */
u_char
ess_get_dsp_status(struct ess_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	return (bus_space_read_1(iot, ioh, ESS_DSP_RW_STATUS));
}


/*
 * Return the read status of the DSP:	1 -> DSP ready for reading
 *					0 -> DSP not ready for reading
 */
u_char
ess_dsp_read_ready(struct ess_softc *sc)
{
	return (((ess_get_dsp_status(sc) & ESS_DSP_READ_MASK) ==
		 ESS_DSP_READ_READY) ? 1 : 0);
}


/*
 * Return the write status of the DSP:	1 -> DSP ready for writing
 *					0 -> DSP not ready for writing
 */
u_char
ess_dsp_write_ready(struct ess_softc *sc)
{
	return (((ess_get_dsp_status(sc) & ESS_DSP_WRITE_MASK) ==
		 ESS_DSP_WRITE_READY) ? 1 : 0);
}


/*
 * Read a byte from the DSP.
 */
int
ess_rdsp(struct ess_softc *sc)

{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	register int i;

	for (i = ESS_READ_TIMEOUT; i > 0; --i)
	{
		if (ess_dsp_read_ready(sc))
		{
#if 1
			return bus_space_read_1(iot, ioh, ESS_DSP_READ);
#else
			i = bus_space_read_1(iot, ioh, ESS_DSP_READ);
			printf("ess_rdsp() = 0x%02x\n", i);
			return i;
#endif
		}
		else
		{
			delay(10);
		}
	}

	++eserr.rdsp;
	DPRINTF(("ess_rdsp: timed out\n"));
	return -1;
}

/*
 * Write a byte to the DSP.
 */
int
ess_wdsp(struct ess_softc *sc,
	 u_char v)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	register int i;
#if 0
printf("ess_wdsp(0x%02x)\n", v);
#endif
	for (i = ESS_WRITE_TIMEOUT; i > 0; --i)
	{
		if (ess_dsp_write_ready(sc))
		{
			bus_space_write_1(iot, ioh, ESS_DSP_WRITE, v);
			return 0;
		}
		else
		{
			delay(10);
		}
	}

	++eserr.wdsp;
	DPRINTF(("ess_wdsp(0x%02x): timed out\n", v));
	return -1;
}

/*
 * Write a value to one of the ESS extended registers.
 */
int
ess_write_x_reg(struct ess_softc *sc,
		u_char reg,
		u_char val)
{
	int error;

	if ((error = ess_wdsp(sc, reg)) == 0)
	{
		error = ess_wdsp(sc, val);
	}

	return error;
}

/*
 * Read the value of one of the ESS extended registers.
 */
u_char
ess_read_x_reg(struct ess_softc *sc,
	       u_char reg)
{
	int error;

	if ((error = ess_wdsp(sc, 0xC0)) == 0)
	{
		error = ess_wdsp(sc, reg);
	}
	if (error)
		DPRINTF(("Error reading extended register 0x%02x\n", reg));
/* REVISIT: what if an error is returned above? */
	return ess_rdsp(sc);
}

void
ess_clear_xreg_bits(struct ess_softc *sc,
		    u_char reg,
		    u_char mask)
{
	if (ess_write_x_reg(sc, reg, ess_read_x_reg(sc, reg) & ~mask) == -1)
		DPRINTF(("Error clearing bits in extended register 0x%02x\n",
			 reg));
}

void
ess_set_xreg_bits(struct ess_softc *sc,
		  u_char reg,
		  u_char mask)
{
	if (ess_write_x_reg(sc, reg, ess_read_x_reg(sc, reg) | mask) == -1)
		DPRINTF(("Error setting bits in extended register 0x%02x\n",
			 reg));
}



/*
 * Write a value to one of the ESS mixer registers.
 */
void
ess_write_mix_reg(struct ess_softc *sc,
		  u_char reg,
		  u_char val)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int s;

	s = splaudio();

	/*
	 * Select the register to be written.
	 */
	bus_space_write_1(iot, ioh, ESS_MIX_REG_SELECT, reg);

	/*
	 * Write the desired value.
	 */
	bus_space_write_1(iot, ioh, ESS_MIX_REG_DATA, val);

	splx(s);
}

/*
 * Read the value of one of the ESS mixer registers.
 */
u_char
ess_read_mix_reg(struct ess_softc *sc,
		 u_char reg)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int s;
	u_char val;

	s = splaudio();

	/*
	 * Select the register to be read.
	 */
	bus_space_write_1(iot, ioh, ESS_MIX_REG_SELECT, reg);

	/*
	 * Read the current value.
	 */
	val = bus_space_read_1(iot, ioh, ESS_MIX_REG_DATA);

	splx(s);
	return val;
}

void
ess_clear_mreg_bits(struct ess_softc *sc,
		    u_char reg,
		    u_char mask)
{
	ess_write_mix_reg(sc, reg, ess_read_mix_reg(sc, reg) & ~mask);
}

void
ess_set_mreg_bits(struct ess_softc *sc,
		  u_char reg,
		  u_char mask)
{
	ess_write_mix_reg(sc, reg, ess_read_mix_reg(sc, reg) | mask);
}
