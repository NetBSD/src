/*	$NetBSD: atapicd.c,v 1.1.2.1 1997/07/01 16:52:06 bouyer Exp $	*/

/*
 * Copyright (c) 1997 Manuel Bouyer.  All rights reserved.
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
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/buf.h>
#include <sys/conf.h>

#include <sys/cdio.h>

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipi_cd.h>
#include <dev/scsipi/atapi_cd.h>
#include <dev/scsipi/atapiconf.h>
#include <dev/scsipi/cd_link.h>

#ifdef __BROKEN_INDIRECT_CONFIG
int	atapicdmatch __P((struct device *, void *, void *));
#else
int	atapicdmatch __P((struct device *, struct cfdata *, void *));
#endif
void	atapicdattach __P((struct device *, struct device *, void *));
int acd_set_mode __P(( struct cd_softc *, struct atapi_mode_data *, int));
int acd_get_mode __P(( struct cd_softc *, struct atapi_mode_data *,
										int, int, int));
struct cfattach atapicd_ca = {
	sizeof(struct cd_softc), atapicdmatch, atapicdattach
};

struct scsipi_inquiry_pattern atapicd_patterns[] = {
	{T_CDROM, T_REMOV,
	 "",         "",                 ""},
	{T_WORM, T_REMOV,
	 "",         "",                 ""},
	{T_DIRECT, T_REMOV,
	 "NEC                 CD-ROM DRIVE:260", "", "3.04"},
};

int
atapicdmatch(parent, match, aux)
	struct device *parent;
#ifdef __BROKEN_INDIRECT_CONFIG
	void *match;
#else
	struct cfdata *match;
#endif
	void *aux;
{
	struct scsipibus_attach_args *sa = aux;
	int priority;

	if (sa->sa_sc_link->type != BUS_ATAPI)
		return 0;

	(void)scsipi_inqmatch(&sa->sa_inqbuf,
	    (caddr_t)atapicd_patterns,
		sizeof(atapicd_patterns)/sizeof(atapicd_patterns[0]),
	    sizeof(atapicd_patterns[0]), &priority);
	return (priority);
}

/*
 * The routine called by the low level scsi routine when it discovers
 * A device suitable for this driver
 */
void
atapicdattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct cd_softc *cd = (void *)self;
	struct scsipibus_attach_args *sa = aux;
	struct scsipi_link *sc_link = sa->sa_sc_link;

	SC_DEBUG(sc_link, SDEV_DB2, ("cdattach: "));

	cdattach(parent, cd, sc_link);

	/* XXX should I get the ATAPI_CAP_PAGE here ? */

	printf("\n");
}

int
acd_get_mode(cd, data, page, len, flags)
	struct cd_softc *cd;
	struct atapi_mode_data *data;
	int page, len, flags;
{
	struct atapi_mode_sense scsipi_cmd;
	int error;

	bzero(&scsipi_cmd, sizeof(scsipi_cmd));
	scsipi_cmd.opcode = ATAPI_MODE_SENSE;
	scsipi_cmd.page = page;
	_lto2b(len, scsipi_cmd.length);
	error = cd->sc_link->scsipi_cmd(cd->sc_link,
		(struct scsipi_generic *)&scsipi_cmd,
		sizeof(scsipi_cmd), (u_char *)data, sizeof(*data), CDRETRIES, 20000,
		NULL, SCSI_DATA_IN);
	SC_DEBUG(cd->sc_link, SDEV_DB2, ("acd_get_mode: error=%d\n", error));
	return error;

}

int
acd_set_mode(cd, data, len)
	struct cd_softc *cd;
	struct atapi_mode_data *data;
	int len;
{
	struct atapi_mode_select scsipi_cmd;
	int error;

	_lto2l(0, data->header.length);
	bzero(&scsipi_cmd, sizeof(scsipi_cmd));
	scsipi_cmd.opcode = ATAPI_MODE_SELECT;
	scsipi_cmd.byte2 = AMS_PF;
	scsipi_cmd.page  = data->page.page_code;
	_lto2b(len, scsipi_cmd.length);
	error = cd->sc_link->scsipi_cmd(cd->sc_link,
		(struct scsipi_generic *)&scsipi_cmd,
		sizeof(scsipi_cmd), (u_char *)data, sizeof(*data), CDRETRIES, 20000,
		NULL, SCSI_DATA_OUT);
	SC_DEBUG(cd->sc_link, SDEV_DB2, ("acd_set_mode: error=%d\n", error));
	return error;
}

int
acd_setchan(cd, p0, p1, p2, p3)
struct cd_softc *cd;
int p0, p1, p2, p3;
{
	struct atapi_mode_data data;
	int error;

	if ((error = acd_get_mode(cd, &data, ATAPI_AUDIO_PAGE,
		AUDIOPAGESIZE, 0)) != 0)
		return error;
	data.page.audio.port[LEFT_PORT].channels = p0;
	data.page.audio.port[RIGHT_PORT].channels = p1;
	data.page.audio.port[2].channels = p2;
	data.page.audio.port[3].channels = p3;
	return acd_set_mode(cd, &data, AUDIOPAGESIZE);
}

int acd_getvol(cd, arg)
	struct cd_softc *cd;
	struct ioc_vol *arg;
{
	struct atapi_mode_data data;
	int error;

	if ((error = acd_get_mode(cd, &data,
					 ATAPI_AUDIO_PAGE, AUDIOPAGESIZE, 0)) != 0)
		return error;
	arg->vol[0] = data.page.audio.port[0].volume;
	arg->vol[1] = data.page.audio.port[1].volume;
	arg->vol[2] = data.page.audio.port[2].volume;
	arg->vol[3] = data.page.audio.port[3].volume;
	return 0;
}

int acd_setvol(cd, arg)
	struct cd_softc *cd;
	struct ioc_vol *arg;
{
	struct atapi_mode_data data, mask;
	int error;

	if ((error = acd_get_mode(cd, &data,
					 ATAPI_AUDIO_PAGE, AUDIOPAGESIZE, 0)) != 0)
		return error;
	if ((error = acd_get_mode(cd, &mask,
					ATAPI_AUDIO_PAGE_MASK, AUDIOPAGESIZE, 0))   != 0)
		return error;

	data.page.audio.port[0].volume = arg->vol[0] &
		mask.page.audio.port[0].volume;
	data.page.audio.port[1].volume = arg->vol[1] &
		mask.page.audio.port[1].volume;
	data.page.audio.port[2].volume = arg->vol[2] &
		mask.page.audio.port[2].volume;
	data.page.audio.port[3].volume = arg->vol[3] &
		mask.page.audio.port[3].volume;

	return acd_set_mode(cd, &data, AUDIOPAGESIZE);
}
