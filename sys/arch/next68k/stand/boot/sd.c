/*      $NetBSD: sd.c,v 1.2 1999/03/26 06:54:40 dbj Exp $        */
/*
 * Copyright (c) 1994 Rolf Grossmann
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Rolf Grossmann.
 * 4. The name of the author may not be used to endorse or promote products
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

#include <sys/param.h>
#include <sys/disklabel.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_disk.h>
#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h> /* for bzero() */

#ifdef SD_DEBUG
#define DPRINTF(x) printf x;
#else
#define DPRINTF(x)
#endif

struct sdminilabel {
    u_short npart;
    long offset[MAXPARTITIONS]; /* offset from absolute block 0! */
};

struct  sd_softc {
    int sc_unit;
    int sc_lun;
    int sc_part;
    short sc_blkshift;
    int sc_dev_bsize;
    struct sdminilabel sc_pinfo;
};

#define NSD 7
#define MAXRETRIES 5	/* must be at least one */

void	scsi_init(void);
int	scsiicmd(char, char, u_char *, int, char *, int);
int	sdstrategy(struct sd_softc *ss, int rw, daddr_t dblk, size_t size,
		   void *buf, size_t *rsize);
int sdprobe(char target, char lun);
int sdgetinfo(struct sd_softc *ss);
int sdopen(struct open_file *f, char count, char lun, char part);
int sdclose(struct open_file *f);

int
sdprobe(char target, char lun)
{
    struct scsipi_test_unit_ready cdb1;
    struct scsipi_inquiry cdb2;
    struct scsipi_inquiry_data inq;
    int error, retries;

    bzero(&cdb1, sizeof(cdb1));
    cdb1.opcode =  TEST_UNIT_READY;

    retries = 0;
    do {
	error = scsiicmd(target, lun, (u_char *)&cdb1, sizeof(cdb1), NULL, 0);
	if (error == -SCSI_BUSY) { 
		register int N = 10000000; while (--N > 0);
	}
    } while ((error == -SCSI_CHECK || error == -SCSI_BUSY)
	     && retries++ < MAXRETRIES);

    if (error)
	return error<0 ? ENODEV : error;

    bzero(&cdb2, sizeof(cdb2));
    cdb2.opcode = INQUIRY;
    cdb2.length = sizeof(inq);
    error = scsiicmd(target, lun, (u_char *)&cdb2, sizeof(cdb2),
		     (char *)&inq, sizeof(inq));
    if (error != 0)
      return error<0 ? EHER : error;

    if ((inq.device & SID_TYPE) != T_DIRECT 
	&& (inq.device & SID_TYPE) != T_CDROM)
      return EUNIT;	/* not a disk */

    DPRINTF(("booting disk %s.\n", inq.vendor));
    
    return 0;
}

int
sdgetinfo(struct sd_softc *ss)
{
    struct scsipi_read_capacity cdb;
    struct scsipi_read_cap_data cap;
    struct sdminilabel *pi = &ss->sc_pinfo;
    struct cpu_disklabel *label;
    int error, i, blklen;
    char io_buf[LABELSIZE+LABELOFFSET];

    bzero(&cdb, sizeof(cdb));
    cdb.opcode = READ_CAPACITY;
    error = scsiicmd(ss->sc_unit, ss->sc_lun, (u_char *)&cdb, sizeof(cdb),
		     (char *)&cap, sizeof(cap));
    if (error != 0)
	return error<0 ? EHER : error;

    blklen = (cap.length[0]<<24) + (cap.length[1]<<16)
	     + (cap.length[2]<<8) + cap.length[3];
    ss->sc_dev_bsize = blklen;
    ss->sc_blkshift = 0;
    ss->sc_pinfo.offset[ss->sc_part] = 0; /* read absolute sector */
    error = sdstrategy(ss, F_READ, LABELSECTOR,
		       LABELSIZE+LABELOFFSET, io_buf, &i);
    if (error != 0) {
	DPRINTF(("sdgetinfo: sdstrategy error %d\n", error));
        return(ERDLAB);
    }
    label = (struct cpu_disklabel *)(io_buf+LABELOFFSET);

    if (!IS_DISKLABEL(label) || (label->cd_flags & CD_UNINIT)!=0)
	return EUNLAB;		/* bad magic */

    /* XXX calculate checksum ... for now we rely on the magic number */
    DPRINTF(("Disk is %s (%s, %s).\n",
	     label->cd_label,label->cd_name, label->cd_type));

    while (label->cd_secsize > blklen)
    {
	blklen <<= 1;
	++ss->sc_blkshift;
    }
    if (label->cd_secsize < blklen)
    {
	printf("bad label sectorsize (%d) or device blocksize (%d).\n",
	       label->cd_secsize, blklen>>ss->sc_blkshift);
	return ENXIO;
    }
    pi->npart = MAXPARTITIONS;
    for(i=0; i<MAXPARTITIONS; i++) {
	if (label->cd_partitions[i].cp_size > 0)
	    pi->offset[i] = label->cd_partitions[i].cp_offset
			    + label->cd_front;
	else
	    pi->offset[i] = -1;
    }
    
    return 0;
}

int
sdopen(struct open_file *f, char count, char lun, char part)
{
    register struct sd_softc *ss;
    char unit, cnt;
    int error;
    
    DPRINTF(("open: sd(%d,%d,%d)\n", count, lun, part));
    
    if (lun >= NSD)
	return EUNIT;

    scsi_init();

    for(cnt=0, unit=0; unit < NSD; unit++)
    {
	DPRINTF(("trying target %d lun %d.\n", unit, lun));
	error = sdprobe(unit, lun);
	if (error == 0)
	{
	    if (cnt++ == count)
		break;
	}
	else if (error != EUNIT)
	    return error;
    }

    if (unit >= NSD)
	return EUNIT;
    
    ss = alloc(sizeof(struct sd_softc));
    ss->sc_unit = unit;
    ss->sc_lun = lun;
    ss->sc_part = part;

    if ((error = sdgetinfo(ss)) != 0)
	return error;

    if ((unsigned char)part >= ss->sc_pinfo.npart
	|| ss->sc_pinfo.offset[(int)part] == -1)
	return EPART;

    f->f_devdata = ss;
    return 0;
}

int
sdclose(struct open_file *f)
{
    register struct sd_softc *ss = f->f_devdata;

    free(ss, sizeof(struct sd_softc));
    return 0;
}

int
sdstrategy(struct sd_softc *ss, int rw, daddr_t dblk, size_t size,
	   void *buf, size_t *rsize)
{
    u_long blk = (dblk + ss->sc_pinfo.offset[ss->sc_part]) << ss->sc_blkshift;
    u_long nblks = howmany(size, ss->sc_dev_bsize);
    struct scsipi_rw_big cdb;
    int error;
    
    if (size == 0)
	return 0;
    
    if (rw != F_READ)
    {
	printf("sdstrategy: write not implemented.\n");
	return EOPNOTSUPP;
    }
    
    DPRINTF(("sdstrategy: read block %ld, %d bytes (%ld blks).\n",
	     blk, size, nblks));

    bzero(&cdb, sizeof(cdb));
    cdb.opcode = READ_BIG;
    cdb.addr[0] = (blk & 0xff000000) >> 24;
    cdb.addr[1] = (blk & 0xff0000) >> 16;
    cdb.addr[2] = (blk & 0xff00) >> 8;
    cdb.addr[3] = blk & 0xff;
    cdb.length[0] = (nblks & 0xff00) >> 8;
    cdb.length[1] = nblks & 0xff;

    error = scsiicmd(ss->sc_unit, ss->sc_lun,
		     (u_char *)&cdb, sizeof(cdb), buf, size);
    if (error != 0)
    {
	DPRINTF(("sdstrategy: scsiicmd failed: %d = %s.\n", error, strerror(error)));
	return error<0 ? EIO : error;
    }
    *rsize = size;
    return 0;
}
