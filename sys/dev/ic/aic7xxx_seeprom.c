/*	$NetBSD: aic7xxx_seeprom.c,v 1.1 2000/01/26 06:04:40 thorpej Exp $	*/

/*
 * Product specific probe and attach routines for:
 *      3940, 2940, aic7880, aic7870, aic7860 and aic7850 SCSI controllers
 *
 * These are the SEEPROM-reading functions only.  They were split off from
 * the PCI-specific support by Jason R. Thorpe <thorpej@netbsd.org>.
 *
 * Copyright (c) 1995, 1996 Justin T. Gibbs.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from	Id: aic7870.c,v 1.37 1996/06/08 06:55:55 gibbs Exp
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/queue.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/ic/aic7xxxreg.h>
#include <dev/ic/aic7xxxvar.h>
#include <dev/ic/smc93cx6var.h>

/*
 * Under normal circumstances, these messages are unnecessary
 * and not terribly cosmetic.
 */
#ifdef DEBUG
#define	bootverbose	1
#else
#define	bootverbose	0
#endif

/*
 * Define the format of the aic78X0 SEEPROM registers (16 bits).
 */

struct seeprom_config {

/*
 * SCSI ID Configuration Flags
 */
#define CFXFER		0x0007		/* synchronous transfer rate */
#define CFSYNCH		0x0008		/* enable synchronous transfer */
#define CFDISC		0x0010		/* enable disconnection */
#define CFWIDEB		0x0020		/* wide bus device */
/* UNUSED		0x00C0 */
#define CFSTART		0x0100		/* send start unit SCSI command */
#define CFINCBIOS	0x0200		/* include in BIOS scan */
#define CFRNFOUND	0x0400		/* report even if not found */
/* UNUSED		0xf800 */
  u_int16_t device_flags[16];	/* words 0-15 */

/*
 * BIOS Control Bits
 */
#define CFSUPREM	0x0001		/* support all removeable drives */
#define CFSUPREMB	0x0002		/* support removeable drives for boot only */
#define CFBIOSEN	0x0004		/* BIOS enabled */
/* UNUSED		0x0008 */
#define CFSM2DRV	0x0010		/* support more than two drives */
/* UNUSED		0x0060 */
#define CFEXTEND	0x0080		/* extended translation enabled */
/* UNUSED		0xff00 */
  u_int16_t bios_control;		/* word 16 */

/*
 * Host Adapter Control Bits
 */
/* UNUSED		0x0001 */
#define CFULTRAEN       0x0002          /* Ultra SCSI speed enable (Ultra cards) */
#define CFSTERM		0x0004		/* SCSI low byte termination (non-wide cards) */
#define CFWSTERM	0x0008		/* SCSI high byte termination (wide card) */
#define CFSPARITY	0x0010		/* SCSI parity */
/* UNUSED		0x0020 */
#define CFRESETB	0x0040		/* reset SCSI bus at IC initialization */
/* UNUSED		0xff80 */
  u_int16_t adapter_control;	/* word 17 */

/*
 * Bus Release, Host Adapter ID
 */
#define CFSCSIID	0x000f		/* host adapter SCSI ID */
/* UNUSED		0x00f0 */
#define CFBRTIME	0xff00		/* bus release time */
 u_int16_t brtime_id;		/* word 18 */

/*
 * Maximum targets
 */
#define CFMAXTARG	0x00ff	/* maximum targets */
/* UNUSED		0xff00 */
  u_int16_t max_targets;		/* word 19 */

  u_int16_t res_1[11];		/* words 20-30 */
  u_int16_t checksum;		/* word 31 */
};

int ahc_acquire_seeprom __P((struct seeprom_descriptor *sd));
void ahc_release_seeprom __P((struct seeprom_descriptor *sd));

/*
 * Read the SEEPROM.  Return 0 on failure
 */
void
ahc_load_seeprom(ahc)
	struct	ahc_data *ahc;
{
	struct	seeprom_descriptor sd;
	struct	seeprom_config sc;
	u_short *scarray = (u_short *)&sc;
	u_short	checksum = 0;
	u_char	scsi_conf;
	u_char	host_id;
	int	have_seeprom;
                 
	sd.sd_st = ahc->sc_st;
	sd.sd_sh = ahc->sc_sh;
	sd.sd_offset = SEECTL;
	sd.sd_MS = SEEMS;
	sd.sd_RDY = SEERDY;
	sd.sd_CS = SEECS;
	sd.sd_CK = SEECK;
	sd.sd_DO = SEEDO;
	sd.sd_DI = SEEDI;

	if(bootverbose) 
		printf("%s: Reading SEEPROM...", ahc_name(ahc));
	have_seeprom = ahc_acquire_seeprom(&sd);
	if (have_seeprom) {
		have_seeprom = read_seeprom(&sd,
					    (u_int16_t *)&sc,
					    ahc->flags & AHC_CHNLB,
					    sizeof(sc)/2);
		ahc_release_seeprom(&sd);
		if (have_seeprom) {
			/* Check checksum */
			int i;

			for (i = 0;i < (sizeof(sc)/2 - 1);i = i + 1)
				checksum = checksum + scarray[i];
			if (checksum != sc.checksum) {
				if(bootverbose)
					printf ("checksum error");
				have_seeprom = 0;
			}
			else if(bootverbose)
				printf("done.\n");
		}
	}
	if (!have_seeprom) {
		if(bootverbose)
			printf("\n%s: No SEEPROM availible\n", ahc_name(ahc));
		ahc->flags |= AHC_USEDEFAULTS;
	}
	else {
		/*
		 * Put the data we've collected down into SRAM
		 * where ahc_init will find it.
		 */
		int i;
		int max_targ = sc.max_targets & CFMAXTARG;

	        for(i = 0; i < max_targ; i++){
	                u_char target_settings;
			target_settings = (sc.device_flags[i] & CFXFER) << 4;
			if (sc.device_flags[i] & CFSYNCH)
				target_settings |= SOFS;
			if (sc.device_flags[i] & CFWIDEB)
				target_settings |= WIDEXFER;
			if (sc.device_flags[i] & CFDISC)
				ahc->discenable |= (0x01 << i);
			AHC_OUTB(ahc, TARG_SCRATCH+i, target_settings);
		}
		AHC_OUTB(ahc, DISC_DSB, ~(ahc->discenable & 0xff));
		AHC_OUTB(ahc, DISC_DSB + 1, ~((ahc->discenable >> 8) & 0xff));

		host_id = sc.brtime_id & CFSCSIID;

		scsi_conf = (host_id & 0x7);
		if(sc.adapter_control & CFSPARITY)
			scsi_conf |= ENSPCHK;
		if(sc.adapter_control & CFRESETB)
			scsi_conf |= RESET_SCSI;

		if(ahc->type & AHC_ULTRA) {
			/* Should we enable Ultra mode? */
			if(!(sc.adapter_control & CFULTRAEN))
				/* Treat us as a non-ultra card */
				ahc->type &= ~AHC_ULTRA;
		}
		/* Set the host ID */
		AHC_OUTB(ahc, SCSICONF, scsi_conf);
		/* In case we are a wide card */
		AHC_OUTB(ahc, SCSICONF + 1, host_id);
	}
}

int
ahc_acquire_seeprom(sd)
	struct seeprom_descriptor *sd;
{
	int wait;

	/*
	 * Request access of the memory port.  When access is
	 * granted, SEERDY will go high.  We use a 1 second
	 * timeout which should be near 1 second more than
	 * is needed.  Reason: after the chip reset, there
	 * should be no contention.
	 */
	SEEPROM_OUTB(sd, sd->sd_MS);
	wait = 1000;  /* 1 second timeout in msec */
	while (--wait && ((SEEPROM_INB(sd) & sd->sd_RDY) == 0)) {
		DELAY (1000);  /* delay 1 msec */
        }
	if ((SEEPROM_INB(sd) & sd->sd_RDY) == 0) {
		SEEPROM_OUTB(sd, 0); 
		return (0);
	}         
	return(1);
}

void
ahc_release_seeprom(sd)
	struct seeprom_descriptor *sd;
{
	/* Release access to the memory port and the serial EEPROM. */
	SEEPROM_OUTB(sd, 0);
}
