/*	$NetBSD: promdev.c,v 1.27.6.1 2017/08/28 17:51:52 skrll Exp $ */

/*
 * Copyright (c) 1993 Paul Kranenburg
 * Copyright (c) 1995 Gordon W. Ross
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
 *      This product includes software developed by Paul Kranenburg.
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

/*
 * Note: the `#ifndef BOOTXX' in here serve to queeze the code size
 * of the 1st-stage boot program.
 */
#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <machine/oldmon.h>
#include <machine/promlib.h>
#include <machine/ctlreg.h>
#include <sparc/sparc/asm.h>
#include <machine/pte.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>
#include <lib/libkern/libkern.h>
#include <sparc/stand/common/promdev.h>
#include <sparc/stand/common/isfloppy.h>

#ifndef BOOTXX
#include <sys/disklabel.h>
#include <dev/sun/disklabel.h>
#include <dev/raidframe/raidframevar.h>
#endif

/* OBP V0-3 PROM vector */
#define obpvec	((struct promvec *)romp)

int	obp_close(struct open_file *);
int	obp_strategy(void *, int, daddr_t, size_t, void *, size_t *);
int	obp_v0_strategy(void *, int, daddr_t, size_t, void *, size_t *);
ssize_t	obp_v0_xmit(struct promdata *, void *, size_t);
ssize_t	obp_v0_recv(struct promdata *, void *, size_t);
int	obp_v2_strategy(void *, int, daddr_t, size_t, void *, size_t *);
ssize_t	obp_v2_xmit(struct promdata *, void *, size_t);
ssize_t	obp_v2_recv(struct promdata *, void *, size_t);
int	oldmon_close(struct open_file *);
int	oldmon_strategy(void *, int, daddr_t, size_t, void *, size_t *);
void	oldmon_iclose(struct saioreq *);
int	oldmon_iopen(struct promdata *);
ssize_t	oldmon_xmit(struct promdata *, void *, size_t);
ssize_t	oldmon_recv(struct promdata *, void *, size_t);

static char	*oldmon_mapin(u_long, int, int);
#ifndef BOOTXX
static char	*mygetpropstring(int, char *);
static int	getdevtype(int, char *);
#endif

extern struct fs_ops file_system_nfs[];
extern struct fs_ops file_system_ufs[];

#define null_devopen	(void *)sparc_noop
#define null_devioctl	(void *)sparc_noop

#if 0
struct devsw devsw[];
int	ndevs = (sizeof(devsw)/sizeof(devsw[0]));
#endif

struct devsw oldmon_devsw =
	{ "oldmon", oldmon_strategy, null_devopen, oldmon_close, null_devioctl };
struct devsw obp_v0_devsw =
	{ "obp v0", obp_v0_strategy, null_devopen, obp_close, null_devioctl };
struct devsw obp_v2_devsw =
	{ "obp v2", obp_v2_strategy, null_devopen, obp_close, null_devioctl };


char	prom_bootdevice[MAX_PROM_PATH];
static int	saveecho;

#ifndef BOOTXX
static daddr_t doffset = 0;
#endif

void
putchar(int c)
{
 
	if (c == '\n')
		prom_putchar('\r');
	prom_putchar(c);
}

void
_rtt(void)
{

	prom_halt();
}

int
devopen(struct open_file *f, const char *fname, char **file)
{
	int	error = 0, fd = 0;
	struct	promdata *pd;
#ifndef BOOTXX
	char *partition;
	int part = 0;
	char rawpart[MAX_PROM_PATH];
	struct promdata *disk_pd;
	char buf[DEV_BSIZE];
	struct disklabel *dlp;
	size_t read;
#endif

	pd = (struct promdata *)alloc(sizeof *pd);
	f->f_devdata = (void *)pd;

	switch (prom_version()) {
	case PROM_OLDMON:
		error = oldmon_iopen(pd);
#ifndef BOOTXX
		pd->xmit = oldmon_xmit;
		pd->recv = oldmon_recv;
#endif
		f->f_dev = &oldmon_devsw;
		saveecho = *romVectorPtr->echo;
		*romVectorPtr->echo = 0;
		break;

	case PROM_OBP_V0:
	case PROM_OBP_V2:
	case PROM_OBP_V3:
	case PROM_OPENFIRM:
		if (*prom_bootdevice == '\0') {
			error = ENXIO;
			break;
		}
		fd = prom_open(prom_bootdevice);
		if (fd == 0) {
			error = ENXIO;
			break;
		}
		pd->fd = fd;
		switch (prom_version()) {
		case PROM_OBP_V0:
#ifndef BOOTXX
			pd->xmit = obp_v0_xmit;
			pd->recv = obp_v0_recv;
#endif
			f->f_dev = &obp_v0_devsw;
			break;
		case PROM_OBP_V2:
		case PROM_OBP_V3:
		case PROM_OPENFIRM:
#ifndef BOOTXX
			pd->xmit = obp_v2_xmit;
			pd->recv = obp_v2_recv;
#endif
			f->f_dev = &obp_v2_devsw;
		}
	}

	if (error) {
		printf("Can't open device `%s'\n", prom_bootdevice);
		return (error);
	}

#ifdef BOOTXX
	pd->devtype = DT_BLOCK;
#else /* BOOTXX */
	pd->devtype = getdevtype(fd, prom_bootdevice);
	/* Assume type BYTE is a raw device */
	if (pd->devtype != DT_BYTE)
		*file = (char *)fname;

	if (pd->devtype == DT_NET) {
		nfsys = 1;
		memcpy(file_system, file_system_nfs,
		    sizeof(struct fs_ops) * nfsys);
		if ((error = net_open(pd)) != 0) {
			printf("Can't open NFS network connection on `%s'\n",
				prom_bootdevice);
			return (error);
		}
	} else {
		memcpy(file_system, file_system_ufs,
		    sizeof(struct fs_ops) * nfsys);

#ifdef NOTDEF_DEBUG
	printf("devopen: Checking disklabel for RAID partition\n");
#endif

		/*
		 * Don't check disklabel on floppy boot since
		 * reopening it could cause Data Access Exception later.
		 */
		if (bootdev_isfloppy(prom_bootdevice))
			return 0;

		/*
		 * We need to read from the raw partition (i.e. the
		 * beginning of the disk in order to check the NetBSD
		 * disklabel to see if the boot partition is type RAID.
		 *
		 * For machines with prom_version() == PROM_OLDMON, we
		 * only handle boot from RAID for the first disk partition.
		 */
		disk_pd = (struct promdata *)alloc(sizeof *disk_pd);
		memcpy(disk_pd, pd, sizeof(struct promdata));
		if (prom_version() != PROM_OLDMON) {
			strcpy(rawpart, prom_bootdevice);
			if ((partition = strchr(rawpart, ':')) != '\0' &&
		    	    *++partition >= 'a' &&
			    *partition <= 'a' +  MAXPARTITIONS) {
				part = *partition - 'a';
				*partition = RAW_PART + 'a';
			} else
				strcat(rawpart, ":c");
			if ((disk_pd->fd = prom_open(rawpart)) == 0)
				return 0;
		}
		error = f->f_dev->dv_strategy(disk_pd, F_READ, LABELSECTOR,
		    DEV_BSIZE, &buf, &read);
		if (prom_version() != PROM_OLDMON)
			prom_close(disk_pd->fd);
		if (error || (read != DEV_BSIZE))
			return 0;
#ifdef NOTDEF_DEBUG
		{
			int x = 0;
			char *p = (char *) buf;

			printf("  Sector %d:\n", LABELSECTOR);
			printf("00000000  ");
			while (x < DEV_BSIZE) {
				if (*p >= 0x00 && *p < 0x10)
					printf("0%x ", *p & 0xff);
				else
					printf("%x ", *p & 0xff);
				x++;
				if (x && !(x % 8))
					printf(" ");
				if (x && !(x % 16)) {
					if(x < 0x100)
						printf("\n000000%x  ", x);
					else
						printf("\n00000%x  ", x);
				}
				p++;
			}
			printf("\n");
		}
#endif
		/* Check for NetBSD disk label. */
		dlp = (struct disklabel *) (buf + LABELOFFSET);
		if (dlp->d_magic == DISKMAGIC && !dkcksum(dlp) &&
		    dlp->d_partitions[part].p_fstype == FS_RAID) {
#ifdef NOTDEF_DEBUG
			printf("devopen: found RAID partition, "
			    "adjusting offset to %d\n", RF_PROTECTED_SECTORS);
#endif
			doffset = RF_PROTECTED_SECTORS;
		}
	}
#endif /* BOOTXX */
	return (0);
}


int
obp_v0_strategy(void *devdata, int flag, daddr_t dblk, size_t size,
		void *buf, size_t *rsize)
{
	int	n, error = 0;
	struct	promdata *pd = (struct promdata *)devdata;
	int	fd = pd->fd;

#ifndef BOOTXX
	dblk += doffset;
#endif
#ifdef DEBUG_PROM
	printf("promstrategy: size=%zd dblk=%d\n", size, (int)dblk);
#endif

#define prom_bread(fd, nblk, dblk, buf) \
		(*obpvec->pv_v0devops.v0_rbdev)(fd, nblk, dblk, buf)
#define prom_bwrite(fd, nblk, dblk, buf) \
		(*obpvec->pv_v0devops.v0_wbdev)(fd, nblk, dblk, buf)

#ifndef BOOTXX	/* We know it's a block device, so save some space */
	if (pd->devtype != DT_BLOCK) {
		printf("promstrategy: non-block device not supported\n");
		error = EINVAL;
	}
#endif

	n = (flag == F_READ)
		? prom_bread(fd, btodb(size), dblk, buf)
		: prom_bwrite(fd, btodb(size), dblk, buf);

	*rsize = dbtob(n);

#ifdef DEBUG_PROM
	printf("rsize = %zx\n", *rsize);
#endif
	return (error);
}

int
obp_v2_strategy(void *devdata, int flag, daddr_t dblk, size_t size,
		void *buf, size_t *rsize)
{
	int	error = 0;
	struct	promdata *pd = (struct promdata *)devdata;
	int	fd = pd->fd;

#ifndef BOOTXX
	dblk += doffset;
#endif
#ifdef DEBUG_PROM
	printf("promstrategy: size=%zd dblk=%d\n", size, (int)dblk);
#endif

#ifndef BOOTXX	/* We know it's a block device, so save some space */
	if (pd->devtype == DT_BLOCK)
#endif
		prom_seek(fd, dbtob(dblk));

	*rsize = (flag == F_READ)
		? prom_read(fd, buf, size)
		: prom_write(fd, buf, size);

#ifdef DEBUG_PROM
	printf("rsize = %zx\n", *rsize);
#endif
	return (error);
}

/*
 * On old-monitor machines, things work differently.
 */
int
oldmon_strategy(void *devdata, int flag, daddr_t dblk, size_t size,
		void *buf, size_t *rsize)
{
	struct promdata	*pd = devdata;
	struct saioreq	*si;
	struct om_boottable *ops;
	char	*dmabuf;
	int	si_flag;
	size_t	xcnt;

	si = pd->si;
	ops = si->si_boottab;

#ifndef BOOTXX
	dblk += doffset;
#endif
#ifdef DEBUG_PROM
	printf("prom_strategy: size=%zd dblk=%d\n", size, (int)dblk);
#endif

	dmabuf = dvma_mapin(buf, size);
	
	si->si_bn = dblk;
	si->si_ma = dmabuf;
	si->si_cc = size;

	si_flag = (flag == F_READ) ? SAIO_F_READ : SAIO_F_WRITE;
	xcnt = (*ops->b_strategy)(si, si_flag);
	dvma_mapout(dmabuf, size);

#ifdef DEBUG_PROM
	printf("disk_strategy: xcnt = %zx\n", xcnt);
#endif

	if (xcnt <= 0)
		return (EIO);

	*rsize = xcnt;
	return (0);
}

int
obp_close(struct open_file *f)
{
	struct promdata *pd = f->f_devdata;
	register int fd = pd->fd;

#ifndef BOOTXX
	if (pd->devtype == DT_NET)
		net_close(pd);
#endif
	prom_close(fd);
	return 0;
}

int
oldmon_close(struct open_file *f)
{
	struct promdata *pd = f->f_devdata;

#ifndef BOOTXX
	if (pd->devtype == DT_NET)
		net_close(pd);
#endif
	oldmon_iclose(pd->si);
	pd->si = NULL;
	*romVectorPtr->echo = saveecho; /* Hmm, probably must go somewhere else */
	return 0;
}

#ifndef BOOTXX
ssize_t
obp_v0_xmit(struct promdata *pd, void *buf, size_t len)
{

	return ((*obpvec->pv_v0devops.v0_wnet)(pd->fd, len, buf));
}

ssize_t
obp_v2_xmit(struct promdata *pd, void *buf, size_t len)
{

	return (prom_write(pd->fd, buf, len));
}

ssize_t
obp_v0_recv(struct promdata *pd, void *buf, size_t len)
{

	return ((*obpvec->pv_v0devops.v0_rnet)(pd->fd, len, buf));
}

ssize_t
obp_v2_recv(struct promdata *pd, void *buf, size_t len)
{
	int	n;

	n = prom_read(pd->fd, buf, len);

	/* OBP V2 & V3 may return -2 */
	return (n == -2 ? 0 : n);
}

ssize_t
oldmon_xmit(struct promdata *pd, void *buf, size_t len)
{
	struct saioreq	*si;
	struct saif	*sif;
	char		*dmabuf;
	int		rv;

	si = pd->si;
	sif = si->si_sif;
	if (sif == NULL) {
		printf("xmit: not a network device\n");
		return (-1);
	}
	dmabuf = dvma_mapin(buf, len);
	rv = sif->sif_xmit(si->si_devdata, dmabuf, len);
	dvma_mapout(dmabuf, len);

	return (ssize_t)(rv ? -1 : len);
}

ssize_t
oldmon_recv(struct promdata *pd, void *buf, size_t len)
{
	struct saioreq	*si;
	struct saif	*sif;
	char		*dmabuf;
	int		rv;

	si = pd->si;
	sif = si->si_sif;
	dmabuf = dvma_mapin(buf, len);
	rv = sif->sif_poll(si->si_devdata, dmabuf);
	dvma_mapout(dmabuf, len);

	return (ssize_t)rv;
}

int
getchar(void)
{

	return (prom_getchar());
}

satime_t
getsecs(void)
{

	(void)prom_peekchar();
	return (prom_ticks() / 1000);
}

/*
 * A number of well-known devices on sun4s.
 */
static struct dtab {
	char	*name;
	int	type;
} dtab[] = {
	{ "sd",	DT_BLOCK },
	{ "st",	DT_BLOCK },
	{ "xd",	DT_BLOCK },
	{ "xy",	DT_BLOCK },
	{ "fd",	DT_BLOCK },
	{ "le",	DT_NET },
	{ "ie",	DT_NET },
	{ NULL, 0 }
};

int
getdevtype(int fd, char *name)
{
	struct dtab *dp;
	int node;
	char *cp;

	switch (prom_version()) {
	case PROM_OLDMON:
	case PROM_OBP_V0:
		for (dp = dtab; dp->name; dp++) {
			if (name[0] == dp->name[0] &&
			    name[1] == dp->name[1])
				return (dp->type);
		}
		break;

	case PROM_OBP_V2:
	case PROM_OBP_V3:
	case PROM_OPENFIRM:
		node = prom_instance_to_package(fd);
		cp = mygetpropstring(node, "device_type");
		if (strcmp(cp, "block") == 0)
			return (DT_BLOCK);
		if (strcmp(cp, "scsi") == 0)
			return (DT_BLOCK);
		else if (strcmp(cp, "network") == 0)
			return (DT_NET);
		else if (strcmp(cp, "byte") == 0)
			return (DT_BYTE);
		break;
	}
	return (0);
}

/*
 * Return a string property.  There is a (small) limit on the length;
 * the string is fetched into a static buffer which is overwritten on
 * subsequent calls.
 */
char *
mygetpropstring(int node, char *name)
{
	int len;
static	char buf[64];

	len = prom_proplen(node, name);
	if (len > sizeof(buf))
		len = sizeof(buf)-1;
	if (len > 0)
		_prom_getprop(node, name, buf, len);
	else
		len = 0;

	buf[len] = '\0';	/* usually unnecessary */
	return (buf);
}
#endif /* BOOTXX */

/*
 * Old monitor routines
 */

struct saioreq prom_si;
static int promdev_inuse;

int
oldmon_iopen(struct promdata *pd)
{
	struct om_bootparam *bp;
	struct om_boottable *ops;
	struct devinfo *dip;
	struct saioreq *si;
	int	error;

	if (promdev_inuse)
		return (EMFILE);

	bp = *romVectorPtr->bootParam;
	ops = bp->bootTable;
	dip = ops->b_devinfo;

#ifdef DEBUG_PROM
	printf("Boot device type: %s\n", ops->b_desc);
	printf("d_devbytes=%d\n", dip->d_devbytes);
	printf("d_dmabytes=%d\n", dip->d_dmabytes);
	printf("d_localbytes=%d\n", dip->d_localbytes);
	printf("d_stdcount=%d\n", dip->d_stdcount);
	printf("d_stdaddrs[%d]=%lx\n", bp->ctlrNum, dip->d_stdaddrs[bp->ctlrNum]);
	printf("d_devtype=%d\n", dip->d_devtype);
	printf("d_maxiobytes=%d\n", dip->d_maxiobytes);
#endif

	dvma_init();

	si = &prom_si;
	memset(si, 0, sizeof(*si));
	si->si_boottab = ops;
	si->si_ctlr = bp->ctlrNum;
	si->si_unit = bp->unitNum;
	si->si_boff = bp->partNum;

	if (si->si_ctlr > dip->d_stdcount)
		return (ECTLR);

	if (dip->d_devbytes) {
		si->si_devaddr = oldmon_mapin(dip->d_stdaddrs[si->si_ctlr],
			dip->d_devbytes, dip->d_devtype);
#ifdef	DEBUG_PROM
		printf("prom_iopen: devaddr=%p pte=0x%x\n",
			si->si_devaddr,
			getpte4((u_long)si->si_devaddr & ~PGOFSET));
#endif
	}

	if (dip->d_dmabytes) {
		si->si_dmaaddr = dvma_alloc(dip->d_dmabytes);
#ifdef	DEBUG_PROM
		printf("prom_iopen: dmaaddr=%p\n", si->si_dmaaddr);
#endif
	}

	if (dip->d_localbytes) {
		si->si_devdata = alloc(dip->d_localbytes);
#ifdef	DEBUG_PROM
		printf("prom_iopen: devdata=%p\n", si->si_devdata);
#endif
	}

	/* OK, call the PROM device open routine. */
	error = (*ops->b_open)(si);
	if (error != 0) {
		printf("prom_iopen: \"%s\" error=%d\n", ops->b_desc, error);
		return (ENXIO);
	}
#ifdef	DEBUG_PROM
	printf("prom_iopen: succeeded, error=%d\n", error);
#endif

	pd->si = si;
	promdev_inuse++;
	return (0);
}

void
oldmon_iclose(struct saioreq *si)
{
	struct om_boottable *ops;
	struct devinfo *dip;

	if (promdev_inuse == 0)
		return;

	ops = si->si_boottab;
	dip = ops->b_devinfo;

	(*ops->b_close)(si);

	if (si->si_dmaaddr) {
		dvma_free(si->si_dmaaddr, dip->d_dmabytes);
		si->si_dmaaddr = NULL;
	}

	promdev_inuse = 0;
}

static struct mapinfo {
	int maptype;
	int pgtype;
	int base;
} oldmon_mapinfo[] = {
#define PG_COMMON	(PG_V|PG_W|PG_S|PG_NC)
	{ MAP_MAINMEM,   PG_OBMEM | PG_COMMON, 0 },
	{ MAP_OBIO,      PG_OBIO  | PG_COMMON, 0 },
	{ MAP_MBMEM,     PG_VME16 | PG_COMMON, 0xFF000000 },
	{ MAP_MBIO,      PG_VME16 | PG_COMMON, 0xFFFF0000 }, 
	{ MAP_VME16A16D, PG_VME16 | PG_COMMON, 0xFFFF0000 },
	{ MAP_VME16A32D, PG_VME32 | PG_COMMON, 0xFFFF0000 },
	{ MAP_VME24A16D, PG_VME16 | PG_COMMON, 0xFF000000 },
	{ MAP_VME24A32D, PG_VME32 | PG_COMMON, 0xFF000000 },
	{ MAP_VME32A16D, PG_VME16 | PG_COMMON, 0 },
	{ MAP_VME32A32D, PG_VME32 | PG_COMMON, 0 },
};
static int oldmon_mapinfo_cnt =
	sizeof(oldmon_mapinfo) / sizeof(oldmon_mapinfo[0]);

/* The virtual address we will use for PROM device mappings. */
static u_long prom_devmap = MONSHORTSEG;

static char *
oldmon_mapin(u_long physaddr, int length, int maptype)
{
	int i, pa, pte, va;

	if (length > (4*NBPG))
		panic("oldmon_mapin: length=%d", length);

	for (i = 0; i < oldmon_mapinfo_cnt; i++)
		if (oldmon_mapinfo[i].maptype == maptype)
			goto found;
	panic("oldmon_mapin: invalid maptype %d", maptype);

found:
	pte = oldmon_mapinfo[i].pgtype;
	pa = oldmon_mapinfo[i].base;
	pa += physaddr;
	pte |= ((pa >> SUN4_PGSHIFT) & PG_PFNUM);

	va = prom_devmap;
	do {
		setpte4(va, pte);
		va += NBPG;
		pte += 1;
		length -= NBPG;
	} while (length > 0);
	return ((char*)(prom_devmap | (pa & PGOFSET)));
}
