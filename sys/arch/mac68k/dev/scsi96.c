/*
 * Copyright (C) 1994	Allen K. Briggs
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
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: scsi96.c,v 1.1 1994/06/26 13:00:34 briggs Exp $
 *
 */

#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/device.h>
#include "../scsi/scsi_all.h"
#include "../scsi/scsi_debug.h"
#include "../scsi/scsiconf.h"

#include <machine/scsi96reg.h>

/* Support for the NCR 53C96 SCSI processor--primarily for '040 Macs. */

#ifdef DDB
int Debugger();
#else
#define Debugger() panic("Should call Debugger here (mac/dev/scsi96.c).")
#endif

#define NNCR53C96	1

struct ncr53c96_data {
	struct device		sc_dev;

	void			*reg_base;
	int			adapter_target;
	struct scsi_link	sc_link;
} *ncr53c96data[NNCR53C96];

static unsigned int	ncr53c96_adapter_info(struct ncr53c96_data *ncr53c96);
static void		ncr53c96_minphys(struct buf *bp);
static int		ncr53c96_scsi_cmd(struct scsi_xfer *xs);

static int		ncr53c96_show_scsi_cmd(struct scsi_xfer *xs);
static int		ncr53c96_reset_target(int adapter, int target);
static int		ncr53c96_poll(int adapter, int timeout);
static int		ncr53c96_send_cmd(struct scsi_xfer *xs);

static int	scsi_gen(int adapter, int id, int lun,
			 struct scsi_generic *cmd, int cmdlen,
			 void *databuf, int datalen);
static int	scsi_group0(int adapter, int id, int lun,
			    int opcode, int addr, int len,
			    int flags, caddr_t databuf, int datalen);

static char scsi_name[] = "ncr96scsi";

struct scsi_adapter	ncr53c96_switch = {
	ncr53c96_scsi_cmd,		/* scsi_cmd()		*/
	ncr53c96_minphys,		/* scsi_minphys()	*/
	0,				/* open_target_lu()	*/
	0,				/* close_target_lu()	*/
	ncr53c96_adapter_info,		/* adapter_info()	*/
	scsi_name,			/* name			*/
	{0, 0}				/* spare[3]		*/
};

/* This is copied from julian's bt driver */
/* "so we have a default dev struct for our link struct." */
struct scsi_device ncr53c96_dev = {
	NULL,		/* Use default error handler.	    */
	NULL,		/* have a queue, served by this (?) */
	NULL,		/* have no async handler.	    */
	NULL,		/* Use default "done" routine.	    */
	"ncr53c96",
	0,
	0, 0
};

extern int	matchbyname();
static int	ncr96probe();
static void	ncr96attach();

struct cfdriver ncr96scsicd =
      {	NULL, "ncr96scsi", ncr96probe, ncr96attach,
	DV_DULL, sizeof(struct ncr53c96_data), NULL, 0 };

static int
ncr96_print(aux, name)
	void *aux;
	char *name;
{
	/* printf("%s: (sc_link = 0x%x)", name, (int) aux); 
	return UNCONF;*/
}

static int
ncr96probe(parent, cf, aux)
	struct device	*parent;
	struct cfdata	*cf;
	void		*aux;
{
extern	int			has53c96scsi;
static	int			probed = 0;
	int			unit = cf->cf_unit;
	struct ncr53c96_data	*ncr53c96;

	if (!has53c96scsi) {
		return 0;
	}

	if (strcmp(*((char **) aux), ncr96scsicd.cd_name)) {
		return 0;
	}

 	if (unit >= NNCR53C96) {
		printf("ncr53c96attach: unit %d more than %d configured.\n",
			unit+1, NNCR53C96);
		return 0;
	}
	ncr53c96 = malloc(sizeof(struct ncr53c96_data), M_TEMP, M_NOWAIT);
	if (!ncr53c96) {
		printf("ncr53c96attach: Can't malloc.\n");
		return 0;
	}

	bzero(ncr53c96, sizeof(*ncr53c96));
	ncr53c96data[unit] = ncr53c96;

	if (!probed) {
	}

	return 1;
}

static void
ncr96attach(parent, dev, aux)
	struct device	*parent, *dev;
	void		*aux;
{
	int				unit = dev->dv_unit;
	struct ncr53c96_data		*ncr53c96 = ncr53c96data[unit];
	int				r;

	bcopy((char *) ncr53c96 + sizeof(struct device),
	      (char *) dev + sizeof(struct device),
	      sizeof(struct ncr53c96_data) - sizeof(struct device));
	free(ncr53c96, M_TEMP);

	ncr53c96data[unit] = ncr53c96 = (struct ncr53c96_data *) dev;

	ncr53c96->sc_link.scsibus = unit;
	ncr53c96->sc_link.adapter_targ = 7;
	ncr53c96->sc_link.adapter = &ncr53c96_switch;
	ncr53c96->sc_link.device = &ncr53c96_dev;

	printf("\n");

	config_found(dev, &(ncr53c96->sc_link), ncr96_print);
}

static unsigned int
ncr53c96_adapter_info(struct ncr53c96_data *ncr53c96)
{
	return 1;
}

#define MIN_PHYS	65536	/*BARF!!!!*/
static void
ncr53c96_minphys(struct buf *bp)
{
	if (bp->b_bcount > MIN_PHYS) {
		printf("Uh-oh...  ncr53c96_minphys setting bp->b_bcount "
			"= %x.\n", MIN_PHYS);
		bp->b_bcount = MIN_PHYS;
	}
}
#undef MIN_PHYS

static int
ncr53c96_scsi_cmd(struct scsi_xfer *xs)
{
	int flags, s, r;

	flags = xs->flags;
	if (xs->bp) flags |= (SCSI_NOSLEEP);
	if ( flags & ITSDONE ) {
		printf("Already done?");
		xs->flags &= ~ITSDONE;
	}
	if ( ! ( flags & INUSE ) ) {
		printf("Not in use?");
		xs->flags |= INUSE;
	}

	if ( flags & SCSI_RESET ) {
		printf("flags & SCSIRESET.\n");
		if ( ! ( flags & SCSI_NOSLEEP ) ) {
			s = splbio();
			ncr53c96_reset_target(xs->sc_link->scsibus,
						xs->sc_link->target);
			splx(s);
			return(SUCCESSFULLY_QUEUED);
		} else {
			ncr53c96_reset_target(xs->sc_link->scsibus,
						xs->sc_link->target);
			if (ncr53c96_poll(xs->sc_link->scsibus, xs->timeout)) {
				return (HAD_ERROR);
			}
			return (COMPLETE);
		}
	}
	/*
	 * OK.  Now that that's over with, let's pack up that
	 * SCSI puppy and send it off.  If we can, we'll just
	 * queue and go; otherwise, we'll wait for the command
	 * to finish.
	if ( ! ( flags & SCSI_NOSLEEP ) ) {
		s = splbio();
		ncr53c96_send_cmd(xs);
		splx(s);
		return(SUCCESSFULLY_QUEUED);
	}
	 */

	r = ncr53c96_send_cmd(xs);
	xs->flags |= ITSDONE;
	scsi_done(xs);
	switch(r) {
		case COMPLETE: case SUCCESSFULLY_QUEUED:
			r = SUCCESSFULLY_QUEUED;
			if (xs->flags&SCSI_NOMASK)
				r = COMPLETE;
			break;
		default:
			break;
	}
	return r;
/*
	do {
		if (ncr53c96_poll(xs->sc_link->scsibus, xs->timeout)) {
			if ( ! ( xs->flags & SCSI_SILENT ) )
				printf("cmd fail.\n");
			cmd_cleanup
			xs->error = XS_DRIVER_STUFFUP;
			splx(s);
		}
	} while ( ! ( xs->flags & ITSDONE ) );
*/
}

static int
ncr53c96_show_scsi_cmd(struct scsi_xfer *xs)
{
	u_char	*b = (u_char *) xs->cmd;
	int	i  = 0;

	if ( ! ( xs->flags & SCSI_RESET ) ) {
		printf("ncr53c96(%d:%d:%d)-",
			xs->sc_link->scsibus, xs->sc_link->target,
						xs->sc_link->lun);
		while (i < xs->cmdlen) {
			if (i) printf(",");
			printf("%x",b[i++]);
		}
		printf("-\n");
	} else {
		printf("ncr53c96(%d:%d:%d)-RESET-\n",
			xs->sc_link->scsibus, xs->sc_link->target,
						xs->sc_link->lun);
	}
}

/*
 * Actual chip control.
 */

static void
delay(int timeo)
{
	int	len;
	for (len=0;len<timeo*2;len++);
}

extern void
ncr53c96_intr(int adapter)
{
}

extern int
ncr53c96_irq_intr(void)
{
	return 1;
}

extern int
ncr53c96_drq_intr(void)
{
	return 1;
}

static int
ncr53c96_reset_target(int adapter, int target)
{
}

static int
ncr53c96_poll(int adapter, int timeout)
{
}

static int
ncr53c96_send_cmd(struct scsi_xfer *xs)
{
}
