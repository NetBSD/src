/*	$NetBSD: cd_scsi.c,v 1.29 2003/09/05 09:04:26 mycroft Exp $	*/

/*-
 * Copyright (c) 1998, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Originally written by Julian Elischer (julian@tfs.com)
 * for TRW Financial Systems for use under the MACH(2.5) operating system.
 *
 * TRW Financial Systems, in accordance with their agreement with Carnegie
 * Mellon University, makes this software available to CMU to distribute
 * or use in any manner that they see fit as long as this message is kept with
 * the software. For this reason TFS also grants any other persons or
 * organisations permission to use or modify this software.
 *
 * TFS supplies this software to be publicly redistributed
 * on the understanding that TFS is not responsible for the correct
 * functioning of this software in any circumstances.
 *
 * Ported to run under 386BSD by Julian Elischer (julian@tfs.com) Sept 1992
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cd_scsi.c,v 1.29 2003/09/05 09:04:26 mycroft Exp $");

#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/buf.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <sys/cdio.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipi_cd.h>
#include <dev/scsipi/scsi_cd.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/cdvar.h>

int	cd_scsibus_match __P((struct device *, struct cfdata *, void *));
void	cd_scsibus_attach __P((struct device *, struct device *, void *));

CFATTACH_DECL(cd_scsibus, sizeof(struct cd_softc),
    cd_scsibus_match, cd_scsibus_attach, cddetach, cdactivate);

struct scsipi_inquiry_pattern cd_scsibus_patterns[] = {
	{T_CDROM, T_REMOV,
	 "",         "",                 ""},
	{T_WORM, T_REMOV,
	 "",         "",                 ""},
#if 0
	{T_CDROM, T_REMOV, /* more luns */
	 "PIONEER ", "CD-ROM DRM-600  ", ""},
#endif
};

int	cd_scsibus_setchan __P((struct cd_softc *, int, int, int, int, int));
int	cd_scsibus_getvol __P((struct cd_softc *, struct ioc_vol *, int));
int	cd_scsibus_setvol __P((struct cd_softc *, const struct ioc_vol *,
	    int));
int	cd_scsibus_set_pa_immed __P((struct cd_softc *, int));
int	cd_scsibus_load_unload __P((struct cd_softc *, int, int));
int	cd_scsibus_setblksize __P((struct cd_softc *));

const struct cd_ops cd_scsibus_ops = {
	cd_scsibus_setchan,
	cd_scsibus_getvol,
	cd_scsibus_setvol,
	cd_scsibus_set_pa_immed,
	cd_scsibus_load_unload,
	cd_scsibus_setblksize,
};

int
cd_scsibus_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct scsipibus_attach_args *sa = aux;
	int priority;

	if (scsipi_periph_bustype(sa->sa_periph) != SCSIPI_BUSTYPE_SCSI)
		return (0);

	(void)scsipi_inqmatch(&sa->sa_inqbuf,
	    (caddr_t)cd_scsibus_patterns,
	    sizeof(cd_scsibus_patterns) / sizeof(cd_scsibus_patterns[0]),
	    sizeof(cd_scsibus_patterns[0]), &priority);
	return (priority);
}

/*
 * The routine called by the low level scsi routine when it discovers
 * A device suitable for this driver
 */
void
cd_scsibus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct cd_softc *cd = (void *)self;
	struct scsipibus_attach_args *sa = aux;
	struct scsipi_periph *periph = sa->sa_periph;

	SC_DEBUG(periph, SCSIPI_DB2, ("cd_scsibus_attach: "));

	scsipi_strvis(cd->name, 16, sa->sa_inqbuf.product, 16);

	cdattach(parent, cd, periph, &cd_scsibus_ops);

	/*
	 * Note if this device is ancient.  This is used in cdminphys().
	 */
	if (periph->periph_version == 0)
		cd->flags |= CDF_ANCIENT;

	/* should I get the SCSI_CAP_PAGE here ? */
}

int
cd_scsibus_set_pa_immed(cd, flags)
	struct cd_softc *cd;
	int flags;
{
	struct scsi_cd_mode_data data;
	int error;

	if ((error = scsipi_mode_sense(cd->sc_periph, SMS_DBD, AUDIO_PAGE,
	    &data.header, AUDIOPAGESIZE, flags | XS_CTL_DATA_ONSTACK,
	    CDRETRIES, 20000)) != 0)
		return (error);
	data.page.audio.flags &= ~CD_PA_SOTC;
	data.page.audio.flags |= CD_PA_IMMED;
	data.header.data_length = 0;
	return (scsipi_mode_select(cd->sc_periph, SMS_PF,
	    &data.header, AUDIOPAGESIZE,
	    flags | XS_CTL_DATA_ONSTACK, CDRETRIES, 20000));
}

int
cd_scsibus_setchan(cd, p0, p1, p2, p3, flags)
	struct cd_softc *cd;
	int p0, p1, p2, p3;
	int flags;
{
	struct scsi_cd_mode_data data;
	int error;

	if ((error = scsipi_mode_sense(cd->sc_periph, SMS_DBD, AUDIO_PAGE,
	    &data.header, AUDIOPAGESIZE, flags | XS_CTL_DATA_ONSTACK,
	    CDRETRIES, 20000)) != 0)
		return (error);
	data.page.audio.port[LEFT_PORT].channels = p0;
	data.page.audio.port[RIGHT_PORT].channels = p1;
	data.page.audio.port[2].channels = p2;
	data.page.audio.port[3].channels = p3;
	data.header.data_length = 0;
	return (scsipi_mode_select(cd->sc_periph, SMS_PF,
	    &data.header, AUDIOPAGESIZE,
	    flags | XS_CTL_DATA_ONSTACK, CDRETRIES, 20000));
}

int
cd_scsibus_getvol(cd, arg, flags)
	struct cd_softc *cd;
	struct ioc_vol *arg;
	int flags;
{

	struct scsi_cd_mode_data data;
	int error;

	if ((error = scsipi_mode_sense(cd->sc_periph, SMS_DBD, AUDIO_PAGE,
	    &data.header, AUDIOPAGESIZE,
	    flags | XS_CTL_DATA_ONSTACK, CDRETRIES, 20000)) != 0)
		return (error);
	arg->vol[LEFT_PORT] = data.page.audio.port[LEFT_PORT].volume;
	arg->vol[RIGHT_PORT] = data.page.audio.port[RIGHT_PORT].volume;
	arg->vol[2] = data.page.audio.port[2].volume;
	arg->vol[3] = data.page.audio.port[3].volume;
	return (0);
}

int
cd_scsibus_setvol(cd, arg, flags)
	struct cd_softc *cd;
	const struct ioc_vol *arg;
	int flags;
{
	struct scsi_cd_mode_data data;
	int error;

	if ((error = scsipi_mode_sense(cd->sc_periph, SMS_DBD, AUDIO_PAGE,
	    &data.header, AUDIOPAGESIZE,
	    flags | XS_CTL_DATA_ONSTACK, CDRETRIES, 20000)) != 0)
		return (error);
	data.page.audio.port[LEFT_PORT].channels = CHANNEL_0;
	data.page.audio.port[LEFT_PORT].volume = arg->vol[LEFT_PORT];
	data.page.audio.port[RIGHT_PORT].channels = CHANNEL_1;
	data.page.audio.port[RIGHT_PORT].volume = arg->vol[RIGHT_PORT];
	data.page.audio.port[2].volume = arg->vol[2];
	data.page.audio.port[3].volume = arg->vol[3];
	data.header.data_length = 0;
	return (scsipi_mode_select(cd->sc_periph, SMS_PF,
	    &data.header, AUDIOPAGESIZE,
	    flags | XS_CTL_DATA_ONSTACK, CDRETRIES, 20000));
}

int
cd_scsibus_load_unload(cd, options, slot)
	struct cd_softc *cd;
	int options, slot;
{
	/*
	 * Not supported on SCSI CDs that we know of (but we'll leave
	 * the hook here Just In Case).
	 */
	return (ENODEV);
}

int
cd_scsibus_setblksize(cd)
	struct cd_softc *cd;
{
	struct {
		struct scsipi_mode_header header;
		struct scsi_blk_desc blk_desc;
	} data;
	int error;

	if ((error = scsipi_mode_sense(cd->sc_periph, 0, 0, &data.header,
	    sizeof(data), XS_CTL_DATA_ONSTACK, CDRETRIES,20000)) != 0)
		return (error);

	_lto3b(2048, data.blk_desc.blklen);
	data.header.data_length = 0;

	return (scsipi_mode_select(cd->sc_periph, SMS_PF, &data.header,
	    sizeof(data), XS_CTL_DATA_ONSTACK, CDRETRIES, 20000));
}
