/*
 * Written by Julian Elischer (julian@tfs.com)
 * for TRW Financial Systems for use under the MACH(2.5) operating system.
 * Hacked by Theo de Raadt <deraadt@fsa.ca>
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
 *	$Id: scsiconf.c,v 1.2 1994/06/30 01:12:49 phil Exp $
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/dkbad.h>
#include <sys/disklabel.h>

#include "../../scsi/scsi_all.h"
#include "../../scsi/scsiconf.h"

#include "st.h"
#include "sd.h"
#include "ch.h"
#include "cd.h"
#define	NBLL 0
#define	NCALS 0
#define	NKIL 0

#if NSD > 0
extern int sdattach();
#endif NSD
#if NST > 0
extern int stattach();
#endif NST
#if NCH > 0
extern int chattach();
#endif NCH
#if NCD > 0
extern int cdattach();
#endif NCD
#if NBLL > 0
extern int bllattach();
#endif NBLL
#if NCALS > 0
extern int calsattach();
#endif NCALS
#if NKIL > 0
extern int kil_attach();
#endif NKIL

struct scsidevs knowndevs[] = {
#if NSD > 0
	{
		SC_TSD, T_DIRECT, T_FIXED, "standard", "any" ,"any",
		sdattach, "sd" ,SC_ONE_LU
	}, {
		SC_TSD, T_DIRECT, T_FIXED, "MAXTOR  ", "XT-4170S	", "B5A ",
		sdattach, "mx1", SC_ONE_LU
	},
#endif NSD
#if NST > 0
	{
		SC_TST, T_SEQUENTIAL, T_REMOV, "standard", "any", "any",
		stattach, "st" ,SC_ONE_LU
	},
#endif NST
#if NCD > 0
	{
		SC_TCD, T_READONLY, T_REMOV, "SONY    ", "CD-ROM CDU-8012 ", "3.1a",
		cdattach, "cd", SC_ONE_LU
	}, {
		SC_TCD, T_READONLY, T_REMOV, "PIONEER ", "CD-ROM DRM-600  ", "any",
		cdattach, "cd", SC_MORE_LUS
	},
#endif NCD
#if NCALS > 0
	{
		-1, T_PROCESSOR, T_FIXED, "standard" , "any" ,"any",
		calsattach, "cals", SC_MORE_LUS
	}
#endif NCALS
#if NCH > 0
	{
		-1, T_CHANGER, T_REMOV, "standard", "any", "any",
		chattach, "ch", SC_ONE_LU
	},
#endif NCH
#if NBLL > 0
	{
		-1, T_PROCESSOR, T_FIXED, "AEG     ", "READER	  ", "V1.0",
		bllattach, "bll", SC_MORE_LUS
	},
#endif NBLL
#if NKIL > 0
	{
		-1, T_SCANNER, T_FIXED, "KODAK   ", "IL Scanner 900  ", "any",
		kil_attach, "kil", SC_ONE_LU
	},
#endif NKIL
};

/* controls debug level within the scsi subsystem: see scsiconf.h */
int scsi_debug	= 0;

struct scsidevs *
scsi_probe(int masunit, struct scsi_switch *sw, int physid, int type, int want)
{
	static struct scsi_inquiry_data inqbuf;
	struct scsidevs *ret = (struct scsidevs *)0;
	int targ = physid >> 3;
	int lun = physid & 7;
	char *qtype=NULL, *dtype=NULL, *desc;
	char manu[9], model[17], revision[5];
	int len;

	bzero(&inqbuf, sizeof inqbuf);

	/*printf("probe: %s%d targ %d lun %d\n",
		sw->name, masunit, targ, lun);*/

	if( scsi_ready(masunit, targ, lun, sw,
	    SCSI_NOSLEEP | SCSI_NOMASK) != COMPLETE)
		return (struct scsidevs *)-1;

	if( scsi_inquire(masunit, targ, lun, sw, (u_char *)&inqbuf,
	    SCSI_NOSLEEP | SCSI_NOMASK) != COMPLETE)
		return (struct scsidevs *)0;

	if( inqbuf.device_qualifier==3 && inqbuf.device_type==T_NODEVICE)
		return (struct scsidevs *)0;

	switch(inqbuf.device_qualifier) {
	case 0:
		qtype = "";
		break;
	case 1:
		qtype = "Unit not Connected!";
		break;
	case 2:
		qtype =", Reserved Peripheral Qualifier!";
		break;
	case 3:
		qtype = ", The Target can't support this Unit!";
		break;
	default:
		dtype = "vendor specific";
		qtype = "";
		break;
	}

	if (dtype == NULL) {
		switch(inqbuf.device_type) {
		case T_DIRECT:
			dtype = "direct";
			break;
		case T_SEQUENTIAL:
			dtype = "seq";
			break;
		case T_PRINTER:
			dtype = "pr";
			break;
		case T_PROCESSOR:
			dtype = "cpu";
			break;
		case T_READONLY:
			dtype = "ro";
			break;
		case T_WORM:
			dtype = "worm";
			break;
		case T_SCANNER:
			dtype = "scan";
			break;
		case T_OPTICAL:
			dtype = "optic";
			break;
		case T_CHANGER:
			dtype = "changer";
			break;
		case T_COMM:
			dtype = "comm";
			break;
		default:
			dtype = "???";
			break;
		}
	}

	if(inqbuf.ansii_version > 0) {
		len = inqbuf.additional_length +
			((char *)inqbuf.unused - (char *)&inqbuf);
		if( len > sizeof(struct scsi_inquiry_data) - 1)
			len = sizeof(struct scsi_inquiry_data) - 1;
		desc = inqbuf.vendor;
		desc[len-(desc-(char *)&inqbuf)] = 0;
		strncpy(manu, inqbuf.vendor, sizeof inqbuf.vendor);
		manu[sizeof inqbuf.vendor] = '\0';
		strncpy(model, inqbuf.product, sizeof inqbuf.product);
		model[sizeof inqbuf.product] = '\0';
		strncpy(revision, inqbuf.revision, sizeof inqbuf.revision);
		revision[sizeof inqbuf.revision] = '\0';
	} else {
		desc = "early protocol device";
		strcpy(manu, "????");
		strcpy(model, "");
		strcpy(revision, "");
	}

	if(want)
		goto print;

	ret = selectdev(masunit, targ, lun, sw, inqbuf.device_qualifier,
		inqbuf.device_type, inqbuf.removable, manu, model, revision, type);
	if(sw->printed[targ] & (1<<lun))
		return ret;

print:
	printf("%s%d targ %d lun %d: type %d(%s) %s <%s%s%s> SCSI%d\n",
		sw->name, masunit, targ, lun,
		inqbuf.device_type, dtype,
		inqbuf.removable ? "removable" : "fixed",
		manu, model, revision, inqbuf.ansii_version);
	if(qtype[0])
		printf("%s%d targ %d lun %d: qualifier %d(%s)\n",
			sw->name, masunit, targ, lun,
			inqbuf.device_qualifier, qtype);
	sw->printed[targ] |= (1<<lun);
	return ret;
}

void
scsi_warn(int masunit, int mytarg, struct scsi_switch *sw)
{
	struct scsidevs *match = (struct scsidevs *)0;
	int physid;
	int targ, lun;

	for(targ=0; targ<8; targ++) {
		if(targ==mytarg)
			continue;
		for(lun=0; lun<8; lun++) {
			/* check if device already used, or empty */
			if( sw->empty[targ] & (1<<lun) )
				continue;
			if( sw->used[targ] & (1<<lun) )
				continue;

			physid = targ*8 + lun;
			match = scsi_probe(masunit, sw, physid, 0, 0);

			if(match == (struct scsidevs *)-1) {
				if(lun==0)
					sw->empty[targ] = 0xff;
				else
					sw->empty[targ] = 0xff;
				continue;
			}
			if(match) {
				targ = physid >> 3;
				lun = physid & 7;
				if(match->flags & SC_MORE_LUS)
					sw->empty[targ] |= (1<<lun);
				else
					sw->empty[targ] = 0xff;
			}
		}
	}
}

/*
 * not quite perfect. If we have two "drive ?" entries, this will
 * probe through all the devices twice. It should have realized that
 * any device that is not found the first time won't exist later on.
 */
int
scsi_attach(int masunit, int mytarg, struct scsi_switch *sw,
	int *physid, int *unit, int type)
{
	struct scsidevs *match = (struct scsidevs *)0;
	int targ, lun;
	int ret=0;

	/*printf("%s%d probing at targ %d lun %d..\n",
		sw->name, masunit, *physid >> 3, *physid & 7);*/

	if( *physid!=-1 ) {
		targ = *physid >> 3;
		lun = *physid & 7;

		if( (sw->empty[targ] & (1<<lun)) || (sw->used[targ] & (1<<lun)) )
			return 0;

		match = scsi_probe(masunit, sw, *physid, type, 0);
		if(match == (struct scsidevs *)-1) {
			match = (struct scsidevs *)0;
			if(lun==0)
				sw->empty[targ] = 0xff;
			else
				sw->empty[targ] |= (1<<lun);
			return 0;
		}

		if(!match)
			return 0;

		ret = (*(match->attach_rtn))(masunit, sw, *physid, unit);
		goto success;
	}

	for(targ=0; targ<8; targ++) {
		if(targ==mytarg)
			continue;
		for(lun=0; lun<8; lun++) {
			if( (sw->empty[targ] & (1<<lun)) || (sw->used[targ] & (1<<lun)) )
				continue;

			*physid = targ*8 + lun;
			match = scsi_probe(masunit, sw, *physid, type, 0);
			if( match==(struct scsidevs *)-1) {
				if(lun==0)
					sw->empty[targ] = 0xff;
				else
					sw->empty[targ] |= (1<<lun);
				match = (struct scsidevs *)0;
				continue;
			}
			if(!match)
				break;
			ret = (*(match->attach_rtn))(masunit, sw, *physid, unit);
			if(ret)
				goto success;
			return 0;
		}
	}
	*physid = -1;	/* failed... */
	return 0;

success:
	targ = *physid >> 3;
	lun = *physid & 7;
	if(match->flags & SC_MORE_LUS)
		sw->used[targ] |= (1<<lun);
	else
		sw->used[targ] = 0xff;
	return ret;
}

/*
 * Try make as good a match as possible with
 * available sub drivers
 */
struct scsidevs *
selectdev(int unit, int target, int lu, struct scsi_switch *sw, int qual,
	int dtype, int remov, char *manu, char *model, char *rev, int type)
{
	struct scsidevs *sdbest = (struct scsidevs *)0;
	struct scsidevs *sdent = knowndevs;
	int numents = sizeof(knowndevs)/sizeof(struct scsidevs);
	int count = 0, sdbestes = 0;

	dtype |= (qual << 5);

	sdent--;
	while( count++ < numents) {
		sdent++;
		if(dtype != sdent->dtype)
			continue;
		if(type != sdent->type)
			continue;
		if(sdbestes < 1) {
			sdbestes = 1;
			sdbest = sdent;
		}
		if(remov != sdent->removable)
			continue;
		if(sdbestes < 2) {
			sdbestes = 2;
			sdbest = sdent;
		}
		if(sdent->flags & SC_SHOWME)
			printf("\n%s-\n%s-", sdent->manufacturer, manu);
		if(strcmp(sdent->manufacturer, manu))
			continue;
		if(sdbestes < 3) {
			sdbestes = 3;
			sdbest = sdent;
		}
		if(sdent->flags & SC_SHOWME)
			printf("\n%s-\n%s-",sdent->model, model);
		if(strcmp(sdent->model, model))
			continue;
		if(sdbestes < 4) {
			sdbestes = 4;
			sdbest = sdent;
		}
		if(sdent->flags & SC_SHOWME)
			printf("\n%s-\n%s-",sdent->version, rev);
		if(strcmp(sdent->version, rev))
			continue;
		if(sdbestes < 5) {
			sdbestes = 5;
			sdbest = sdent;
			break;
		}
	}
	return sdbest;
}

/*
 * Do a scsi operation asking a device if it is
 * ready. Use the scsi_cmd routine in the switch
 * table.
 */
int
scsi_ready(int unit, int target, int lu,
	struct scsi_switch *sw, int flags)
{
	struct scsi_test_unit_ready scsi_cmd;
	struct scsi_xfer scsi_xfer;
	volatile int rval;
	int key;

	bzero(&scsi_cmd, sizeof(scsi_cmd));
	bzero(&scsi_xfer, sizeof(scsi_xfer));
	scsi_cmd.op_code = TEST_UNIT_READY;

	scsi_xfer.flags = flags | INUSE;
	scsi_xfer.adapter = unit;
	scsi_xfer.targ = target;
	scsi_xfer.lu = lu;
	scsi_xfer.cmd = (struct scsi_generic *)&scsi_cmd;
	scsi_xfer.retries = 8;
	scsi_xfer.timeout = 10000;
	scsi_xfer.cmdlen = sizeof(scsi_cmd);
	scsi_xfer.data = 0;
	scsi_xfer.datalen = 0;
	scsi_xfer.resid = 0;
	scsi_xfer.when_done = 0;
	scsi_xfer.done_arg = 0;
retry:	scsi_xfer.error = 0;

	/* don't use interrupts! */

	rval = (*(sw->scsi_cmd))(&scsi_xfer);
	if (rval != COMPLETE) {
		if(scsi_debug) {
			printf("scsi error, rval = 0x%x\n", rval);
			printf("code from driver: 0x%x\n", scsi_xfer.error);
		}
		switch(scsi_xfer.error) {
		case XS_SENSE:
			/*
			 * Any sense value is illegal except UNIT ATTENTION
			 * In which case we need to check again to get the
			 * correct response. (especially exabytes)
			 */
			if(scsi_xfer.sense.error_class == 7 ) {
				key = scsi_xfer.sense.ext.extended.sense_key ;
				switch(key) { 
				case 2:	/* not ready BUT PRESENT! */
					return(COMPLETE);
				case 6:
					spinwait(1000);
					if(scsi_xfer.retries--) {
						scsi_xfer.flags &= ~ITSDONE;
						goto retry;
					}
					return(COMPLETE);
				default:
					if(scsi_debug)
						printf("%d:%d,key=%x.", target,
							lu, key);
				}
			}
			return(HAD_ERROR);
		case XS_BUSY:
			spinwait(1000);
			if(scsi_xfer.retries--) {
				scsi_xfer.flags &= ~ITSDONE;
				goto retry;
			}
			return COMPLETE;	/* it's busy so it's there */
		case XS_TIMEOUT:
		default:
			return HAD_ERROR;
		}
	}
	return COMPLETE;
}

/*
 * Do a scsi operation asking a device what it is
 * Use the scsi_cmd routine in the switch table.
 */
int
scsi_inquire(int unit, int target, int lu, struct scsi_switch *sw,
	u_char *inqbuf, int flags)
{
	struct scsi_inquiry scsi_cmd;
	struct scsi_xfer scsi_xfer;

	bzero(&scsi_cmd, sizeof(scsi_cmd));
	bzero(&scsi_xfer, sizeof(scsi_xfer));
	scsi_cmd.op_code = INQUIRY;
	scsi_cmd.length = sizeof(struct scsi_inquiry_data);

	scsi_xfer.flags = flags | SCSI_DATA_IN | INUSE;
	scsi_xfer.adapter = unit;
	scsi_xfer.targ = target;
	scsi_xfer.lu = lu;
	scsi_xfer.retries = 8;
	scsi_xfer.timeout = 10000;
	scsi_xfer.cmd = (struct scsi_generic *)&scsi_cmd;
	scsi_xfer.cmdlen =  sizeof(struct scsi_inquiry);
	scsi_xfer.data = inqbuf;
	scsi_xfer.datalen = sizeof(struct scsi_inquiry_data);
	scsi_xfer.resid = sizeof(struct scsi_inquiry_data);
	scsi_xfer.when_done = 0;
	scsi_xfer.done_arg = 0;

retry:
	scsi_xfer.error=0;
	/* don't use interrupts! */

	if ((*(sw->scsi_cmd))(&scsi_xfer) != COMPLETE) {
		if(scsi_debug)
			printf("inquiry had error(0x%x) ",scsi_xfer.error);
		switch(scsi_xfer.error) {
		case XS_NOERROR:
			break;
		case XS_SENSE:
			/*
			 * Any sense value is illegal except UNIT ATTENTION
			 * In which case we need to check again to get the
			 * correct response. (especially exabytes)
			 */
			if( scsi_xfer.sense.error_class==7 &&
			    scsi_xfer.sense.ext.extended.sense_key==6) {
				/* it's changed so it's there */
				spinwait(1000);
				if(scsi_xfer.retries--) {
					scsi_xfer.flags &= ~ITSDONE;
					goto retry;
				}
				return COMPLETE;
			}
			return HAD_ERROR;
		case XS_BUSY:
			spinwait(1000);
			if(scsi_xfer.retries--) {
				scsi_xfer.flags &= ~ITSDONE;
				goto retry;
			}
		case XS_TIMEOUT:
		default:
			return(HAD_ERROR);
		}
	}
	return COMPLETE;
}

/*
 * convert a physical address to 3 bytes,
 * MSB at the lowest address,
 * LSB at the highest.
 */
void
lto3b(u_long val, u_char *bytes)
{
	*bytes++ = (val&0xff0000)>>16;
	*bytes++ = (val&0xff00)>>8;
	*bytes = val&0xff;
}

/*
 * The reverse of lto3b
 */
u_long
_3btol(u_char *bytes)
{
	u_long rc;

	rc = (*bytes++ << 16);
	rc += (*bytes++ << 8);
	rc += *bytes;
	return rc;
}

