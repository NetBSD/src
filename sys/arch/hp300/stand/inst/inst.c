/*	$NetBSD: inst.c,v 1.20 2014/08/10 07:40:49 isaki Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * Portions of this program are inspired by (and have borrowed code from)
 * the `editlabel' program that accompanies NetBSD/vax, which carries
 * the following notice:
 *
 * Copyright (c) 1995 Ludd, University of Lule}, Sweden.
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
 *	This product includes software developed at Ludd, University of
 *	Lule}, Sweden and its contributors.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define DKTYPENAMES

#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/disklabel.h>

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <hp300/stand/common/samachdep.h>

char line[100];

extern	u_int opendev;
extern	char *lowram;
extern	int noconsole;
extern	int netio_ask;

char	*kernel_name = "/netbsd";

void	main(void);
void	dsklabel(void);
void	miniroot(void);
void	bootmini(void);
void	resetsys(void);
void	gethelp(void);
int	opendisk(char *, char *, int, char, int *);
void	disklabel_edit(struct disklabel *);
void	disklabel_show(struct disklabel *);
int	disklabel_write(char *, int, struct open_file *);
void	get_fstype(struct disklabel *lp, int);
int	a2int(char *);

struct	inst_command {
	char	*ic_cmd;		/* command name */
	char	*ic_desc;		/* command description */
	void	(*ic_func)(void);	/* handling function */
} inst_commands[] = {
	{ "disklabel",	"place partition map on disk",	dsklabel },
	{ "miniroot",	"place miniroot on disk",	miniroot },
	{ "boot",	"boot from miniroot",		bootmini },
	{ "reset",	"reset the system",		resetsys },
	{ "help",	"display command list",		gethelp },
};
#define NCMDS	(sizeof(inst_commands) / sizeof(inst_commands[0]))

void
main(void)
{
	int i;

	/*
	 * We want netopen() to ask for IP address, etc, rather
	 * that using bootparams.
	 */
	netio_ask = 1;

	printf("\n");
	printf(">> %s, Revision %s (from NetBSD %s)\n",
	    bootprog_name, bootprog_rev, bootprog_kernrev);
	printf(">> HP 9000/%s SPU\n", getmachineid());
	gethelp();

	for (;;) {
		printf("sys_inst> ");
		memset(line, 0, sizeof(line));
		gets(line);
		if (line[0] == '\n' || line[0] == '\0')
			continue;

		for (i = 0; i < NCMDS; ++i)
			if (strcmp(line, inst_commands[i].ic_cmd) == 0) {
				(*inst_commands[i].ic_func)();
				break;
			}


		if (i == NCMDS)
			printf("unknown command: %s\n", line);
	}
}

void
gethelp(void)
{
	int i;

	printf(">> Available commands:\n");
	for (i = 0; i < NCMDS; ++i)
		printf(">>     %s - %s\n", inst_commands[i].ic_cmd,
		    inst_commands[i].ic_desc);
}

/*
 * Do all the steps necessary to place a disklabel on a disk.
 * Note, this assumes 512 byte sectors.
 */
void
dsklabel(void)
{
	struct disklabel *lp;
	struct open_file *disk_ofp;
	int dfd, error;
	size_t xfersize;
	char block[DEV_BSIZE], diskname[64];
	extern struct open_file files[];

	printf(
"You will be asked several questions about your disk, most of which\n"
"require prior knowledge of the disk's geometry.  There is no easy way\n"
"for the system to provide this information for you.  If you do not have\n"
"this information, please consult your disk's manual or another\n"
"informative source.\n\n");

	/* Error message printed by opendisk() */
	if (opendisk("Disk to label?", diskname, sizeof(diskname),
	    ('a' + RAW_PART), &dfd))
		return;

	disk_ofp = &files[dfd];

	memset(block, 0, sizeof(block));
	if ((error = (*disk_ofp->f_dev->dv_strategy)(disk_ofp->f_devdata,
	    F_READ, LABELSECTOR, sizeof(block), block, &xfersize)) != 0) {
		printf("cannot read disk %s, errno = %d\n", diskname, error);
		return;
	}

	printf("Successfully read %d bytes from %s\n", xfersize, diskname);

	lp = (struct disklabel *)((void *)(&block[LABELOFFSET]));

 disklabel_loop:
	memset(line, 0, sizeof(line));
	printf("(z)ap, (e)dit, (s)how, (w)rite, (d)one > ");
	gets(line);
	if (line[0] == '\n' || line[0] == '\0')
		goto disklabel_loop;

	switch (line[0]) {
	case 'z':
	case 'Z': {
		char zap[DEV_BSIZE];
		memset(zap, 0, sizeof(zap));
		(void)(*disk_ofp->f_dev->dv_strategy)(disk_ofp->f_devdata,
		    F_WRITE, LABELSECTOR, sizeof(zap), zap, &xfersize);
		}
		goto out;
		/* NOTREACHED */

	case 'e':
	case 'E':
		disklabel_edit(lp);
		break;

	case 's':
	case 'S':
		disklabel_show(lp);
		break;

	case 'w':
	case 'W':
		/*
		 * Error message will be displayed by disklabel_write()
		 */
		if (disklabel_write(block, sizeof(block), disk_ofp))
			goto out;
		else
			printf("Successfully wrote label to %s\n", diskname);
		break;

	case 'd':
	case 'D':
		goto out;
		/* NOTREACHED */

	default:
		printf("unknown command: %s\n", line);
	}

	goto disklabel_loop;
	/* NOTREACHED */

 out:
	/*
	 * Close disk.  Marks disk `not alive' so that partition
	 * information will be reloaded upon next open.
	 */
	(void)close(dfd);
}

#define GETNUM(out, num)						\
	printf((out), (num));						\
	memset(line, 0, sizeof(line));					\
	gets(line);							\
	if (line[0])							\
		(num) = atoi(line);

#define GETNUM2(out, num1, num2)					\
	printf((out), (num1), (num2));					\
	memset(line, 0, sizeof(line));					\
	gets(line);							\
	if (line[0])							\
		(num2) = atoi(line);

#define GETSTR(out, str)						\
	printf((out), (str));						\
	memset(line, 0, sizeof(line));					\
	gets(line);							\
	if (line[0])							\
		strcpy((str), line);

#define FLAGS(out, flag)						\
	printf((out), lp->d_flags & (flag) ? 'y' : 'n');		\
	memset(line, 0, sizeof(line));					\
	gets(line);							\
	if (line[0] == 'y' || line[0] == 'Y')				\
		lp->d_flags |= (flag);					\
	else								\
		lp->d_flags &= ~(flag);

struct fsname_to_type {
	const char *name;
	uint8_t type;
} n_to_t[] = {
	{ "unused",	FS_UNUSED },
	{ "ffs",	FS_BSDFFS },
	{ "swap",	FS_SWAP },
	{ "boot",	FS_BOOT },
	{ NULL,		0 },
};

void
get_fstype(struct disklabel *lp, int partno)
{
	static int blocksize = 8192;	/* XXX */
	struct partition *pp = &lp->d_partitions[partno];
	struct fsname_to_type *np;
	int fragsize;
	char line[80], str[80];

	if (pp->p_size == 0) {
		/*
		 * No need to bother asking for a zero-sized partition.
		 */
		pp->p_fstype = FS_UNUSED;
		return;
	}

	/*
	 * Select a default.
	 * XXX Should we check what might be in the label already?
	 */
	if (partno == 1)
		strcpy(str, "swap");
	else if (partno == RAW_PART)
		strcpy(str, "boot");
	else
		strcpy(str, "ffs");

 again:
	GETSTR("             fstype? [%s] ", str);

	for (np = n_to_t; np->name != NULL; np++)
		if (strcmp(str, np->name) == 0)
			break;

	if (np->name == NULL) {
		printf("Please use one of: ");
		for (np = n_to_t; np->name != NULL; np++)
			printf(" %s", np->name);
		printf(".\n");
		goto again;
	}

	pp->p_fstype = np->type;

	if (pp->p_fstype != FS_BSDFFS)
		return;

	/*
	 * Get additional information needed for FFS.
	 */
 ffs_again:
	GETNUM("             FFS block size? [%d] ", blocksize);
	if (blocksize < NBPG || (blocksize % NBPG) != 0) {
		printf("FFS block size must be a multiple of %d.\n", NBPG);
		goto ffs_again;
	}

	fragsize = blocksize / 8;	/* XXX */
	fragsize = max(fragsize, lp->d_secsize);
	GETNUM("             FFS fragment size? [%d] ", fragsize);
	if (fragsize < lp->d_secsize || (fragsize % lp->d_secsize) != 0) {
		printf("FFS fragment size must be a multiple of sector size"
		    " (%d).\n", lp->d_secsize);
		goto ffs_again;
	}
	if ((blocksize % fragsize) != 0) {
		printf("FFS fragment size must be an even divisor of FFS"
		    " block size (%d).\n", blocksize);
		goto ffs_again;
	}

	/*
	 * XXX Better sanity checking?
	 */

	pp->p_frag = blocksize / fragsize;
	pp->p_fsize = fragsize;
}

void
disklabel_edit(struct disklabel *lp)
{
	int i;

	printf("Select disk type.  Valid types:\n");
	for (i = 0; i < DKMAXTYPES; i++)
		printf("%d     %s\n", i, dktypenames[i]);
	printf("\n");

	GETNUM("Disk type (number)? [%d] ", lp->d_type);
	GETSTR("Disk model name? [%s] ", lp->d_typename);
	GETSTR("Disk pack name? [%s] ", lp->d_packname);
	FLAGS("Bad sectoring? [%c] ", D_BADSECT);
	FLAGS("Ecc? [%c] ", D_ECC);
	FLAGS("Removable? [%c] ", D_REMOVABLE);

	printf("\n");

	GETNUM("Interleave? [%d] ", lp->d_interleave);
	GETNUM("Rpm? [%d] ", lp->d_rpm);
	GETNUM("Trackskew? [%d] ", lp->d_trackskew);
	GETNUM("Cylinderskew? [%d] ", lp->d_cylskew);
	GETNUM("Headswitch? [%d] ", lp->d_headswitch);
	GETNUM("Track-to-track? [%d] ", lp->d_trkseek);
	GETNUM("Drivedata 0? [%d] ", lp->d_drivedata[0]);
	GETNUM("Drivedata 1? [%d] ", lp->d_drivedata[1]);
	GETNUM("Drivedata 2? [%d] ", lp->d_drivedata[2]);
	GETNUM("Drivedata 3? [%d] ", lp->d_drivedata[3]);
	GETNUM("Drivedata 4? [%d] ", lp->d_drivedata[4]);

	printf("\n");

	GETNUM("Bytes/sector? [%d] ", lp->d_secsize);
	GETNUM("Sectors/track? [%d] ", lp->d_nsectors);
	GETNUM("Tracks/cylinder? [%d] ", lp->d_ntracks);
	if (lp->d_secpercyl == 0)
		lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;
	GETNUM("Sectors/cylinder? [%d] ", lp->d_secpercyl);
	GETNUM("Cylinders? [%d] ", lp->d_ncylinders);
	if (lp->d_secperunit == 0)
		lp->d_secperunit = lp->d_ncylinders * lp->d_secpercyl;
	GETNUM("Total sectors? [%d] ", lp->d_secperunit);

	printf(
"Enter partition table.  Note, sizes and offsets are in sectors.\n\n");

	lp->d_npartitions = MAXPARTITIONS;
	for (i = 0; i < lp->d_npartitions; ++i) {
		GETNUM2("%c partition: offset? [%d] ", ('a' + i),
		    lp->d_partitions[i].p_offset);
		GETNUM("             size? [%d] ", lp->d_partitions[i].p_size);
		get_fstype(lp, i);
	}

	/* Perform magic. */
	lp->d_magic = lp->d_magic2 = DISKMAGIC;

	/* Calculate disklabel checksum. */
	lp->d_checksum = 0;
	lp->d_checksum = dkcksum(lp);
}

void
disklabel_show(struct disklabel *lp)
{
	int i;
	struct partition *pp;

	/*
	 * Check for valid disklabel.
	 */
	if (lp->d_magic != DISKMAGIC || lp->d_magic2 != DISKMAGIC) {
		printf("No disklabel to show.\n");
		return;
	}

	if (lp->d_npartitions > MAXPARTITIONS || dkcksum(lp) != 0) {
		printf("Corrupted disklabel.\n");
		return;
	}

	printf("\ndisk type %d (%s), %s: %s%s%s\n", lp->d_type,
	    lp->d_type < DKMAXTYPES ? dktypenames[lp->d_type] :
	    dktypenames[0], lp->d_typename,
	    (lp->d_flags & D_REMOVABLE) ? " removable" : "",
	    (lp->d_flags & D_ECC) ? " ecc" : "",
	    (lp->d_flags & D_BADSECT) ? " badsect" : "");

	printf("interleave %d, rpm %d, trackskew %d, cylinderskew %d\n",
	    lp->d_interleave, lp->d_rpm, lp->d_trackskew, lp->d_cylskew);

	printf("headswitch %d, track-to-track %d, drivedata: %d %d %d %d %d\n",
	    lp->d_headswitch, lp->d_trkseek, lp->d_drivedata[0],
	    lp->d_drivedata[1], lp->d_drivedata[2], lp->d_drivedata[3],
	    lp->d_drivedata[4]);

	printf("\nbytes/sector: %d\n", lp->d_secsize);
	printf("sectors/track: %d\n", lp->d_nsectors);
	printf("tracks/cylinder: %d\n", lp->d_ntracks);
	printf("sectors/cylinder: %d\n", lp->d_secpercyl);
	printf("cylinders: %d\n", lp->d_ncylinders);
	printf("total sectors: %d\n", lp->d_secperunit);

	printf("\n%d partitions:\n", lp->d_npartitions);
	printf("     size   offset\n");
	pp = lp->d_partitions;
	for (i = 0; i < lp->d_npartitions; i++) {
		printf("%c:   %d,    %d\n", 'a' + i, pp[i].p_size,
		    pp[i].p_offset);
	}
	printf("\n");
}

int
disklabel_write(char *block, int len, struct open_file *ofp)
{
	int error = 0;
	size_t xfersize;

	if ((error = (*ofp->f_dev->dv_strategy)(ofp->f_devdata, F_WRITE,
	    LABELSECTOR, len, block, &xfersize)) != 0)
		printf("cannot write disklabel, errno = %d\n", error);

	return (error);
}

int
opendisk(char *question, char *diskname, int len, char partition, int *fdp)
{
	char fulldiskname[64];
	int i;

 getdiskname:
	printf("%s ", question);
	memset(diskname, 0, len);
	memset(fulldiskname, 0, sizeof(fulldiskname));
	gets(diskname);
	if (diskname[0] == '\n' || diskname[0] == '\0')
		goto getdiskname;

	/*
	 * devopen() is picky.  Make sure it gets the sort of string it
	 * wants.
	 */
	memcpy(fulldiskname, diskname,
	    len < sizeof(fulldiskname) ? len : sizeof(fulldiskname));
	for (i = 0; fulldiskname[i + 1] != '\0'; ++i)
		/* Nothing. */ ;
	if (fulldiskname[i] < '0' || fulldiskname[i] > '9') {
		printf("invalid disk name %s\n", diskname);
		goto getdiskname;
	}
	fulldiskname[++i] = partition; fulldiskname[++i] = ':';

	/*
	 * We always open for writing.
	 */
	if ((*fdp = open(fulldiskname, 1)) < 0) {
		printf("cannot open %s\n", diskname);
		return 1;
	}

	return 0;
}

/*
 * Copy a miniroot image from an NFS server or tape to the `b' partition
 * of the specified disk.  Note, this assumes 512 byte sectors.
 */
void
miniroot(void)
{
	int sfd, dfd, i, nblks;
	char diskname[64], minirootname[128];
	char block[DEV_BSIZE];
	char tapename[64];
	int fileno, ignoreshread, eof, len;
	struct stat st;
	size_t xfersize;
	struct open_file *disk_ofp;
	extern struct open_file files[];

	/* Error message printed by opendisk() */
	if (opendisk("Disk for miniroot?", diskname, sizeof(diskname),
	    'b', &dfd))
		return;

	disk_ofp = &files[dfd];

 getsource:
	printf("Source? (N)FS, (t)ape, (d)one > ");
	memset(line, 0, sizeof(line));
	gets(line);
	if (line[0] == '\0')
		goto getsource;

	switch (line[0]) {
	case 'n':
	case 'N':
 name_of_nfs_miniroot:
		printf("Name of miniroot file? ");
		memset(line, 0, sizeof(line));
		memset(minirootname, 0, sizeof(minirootname));
		gets(line);
		if (line[0] == '\0')
			goto name_of_nfs_miniroot;
		(void)strcat(minirootname, "le0a:");
		(void)strcat(minirootname, line);
		if ((sfd = open(minirootname, 0)) < 0) {
			printf("can't open %s\n", line);
			return;
		}

		/*
		 * Find out how big the miniroot is... we can't
		 * check for size because it may be compressed.
		 */
		ignoreshread = 1;
		if (fstat(sfd, &st) < 0) {
			printf("can't stat %s\n", line);
			goto done;
		}
		nblks = (int)(st.st_size / sizeof(block));

		printf("Copying miniroot from %s to %s...", line,
		    diskname);
		break;

	case 't':
	case 'T':
 name_of_tape_miniroot:
		printf("Which tape device? ");
		memset(line, 0, sizeof(line));
		memset(minirootname, 0, sizeof(minirootname));
		memset(tapename, 0, sizeof(tapename));
		gets(line);
		if (line[0] == '\0')
			goto name_of_tape_miniroot;
		strcat(minirootname, line);
		strcat(tapename, line);

		printf("File number (first == 1)? ");
		memset(line, 0, sizeof(line));
		gets(line);
		fileno = a2int(line);
		if (fileno < 1 || fileno > 8) {
			printf("Invalid file number: %s\n", line);
			goto getsource;
		}
		for (i = 0; i < sizeof(minirootname); ++i) {
			if (minirootname[i] == '\0')
				break;
		}
		if (i == sizeof(minirootname) ||
		    (sizeof(minirootname) - i) < 8) {
			printf("Invalid device name: %s\n", tapename);
			goto getsource;
		}
		minirootname[i++] = 'a' + (fileno - 1);
		minirootname[i++] = ':';
		strcat(minirootname, "XXX");	/* lameness in open() */

		ignoreshread = 0;
		printf("Copy how many %d byte blocks? ", DEV_BSIZE);
		memset(line, 0, sizeof(line));
		gets(line);
		nblks = a2int(line);
		if (nblks < 0) {
			printf("Invalid block count: %s\n", line);
			goto getsource;
		} else if (nblks == 0) {
			printf("Zero blocks?  Ok, aborting.\n");
			return;
		}

		if ((sfd = open(minirootname, 0)) < 0) {
			printf("can't open %s file %c\n", tapename, fileno);
			return;
		}

		printf("Copying %s file %d to %s...", tapename, fileno,
		    diskname);
		break;

	case 'd':
	case 'D':
		return;

	default:
		printf("Unknown source: %s\n", line);
		goto getsource;
	}

	/*
	 * Copy loop...
	 * This is fairly slow... if someone wants to speed it
	 * up, they'll get no complaints from me.
	 */
	for (i = 0, eof = 0; i < nblks || ignoreshread == 0; i++) {
		if ((len = read(sfd, block, sizeof(block))) < 0) {
			printf("Read error, errno = %d\n", errno);
			goto out;
		}

		/*
		 * Check for end-of-file.
		 */
		if (len == 0)
			goto done;
		else if (len < sizeof(block))
			eof = 1;

		if ((*disk_ofp->f_dev->dv_strategy)(disk_ofp->f_devdata,
		    F_WRITE, i, len, block, &xfersize) || xfersize != len) {
			printf("Bad write at block %d, errno = %d\n",
			    i, errno);
			goto out;
		}

		if (eof)
			goto done;
	}
 done:
	printf("done\n");

	printf("Successfully copied miniroot image.\n");

 out:
	close(sfd);
	close(dfd);
}

/*
 * Boot the kernel from the miniroot image into single-user.
 */
void
bootmini(void)
{
	char diskname[64], bootname[64];
	int i;

 getdiskname:
	printf("Disk to boot from? ");
	memset(diskname, 0, sizeof(diskname));
	memset(bootname, 0, sizeof(bootname));
	gets(diskname);
	if (diskname[0] == '\n' || diskname[0] == '\0')
		goto getdiskname;

	/*
	 * devopen() is picky.  Make sure it gets the sort of string it
	 * wants.
	 */
	(void)strcat(bootname, diskname);
	for (i = 0; bootname[i + 1] != '\0'; ++i)
		/* Nothing. */ ;
	if (bootname[i] < '0' || bootname[i] > '9') {
		printf("invalid disk name %s\n", diskname);
		goto getdiskname;
	}
	bootname[++i] = 'b'; bootname[++i] = ':';
	(void)strcat(bootname, kernel_name);

	howto = RB_SINGLE;	/* _Always_ */

	printf("booting: %s -s\n", bootname);
	exec_hp300(bootname, (u_long)lowram, howto);
	printf("boot: %s\n", strerror(errno));
}

/*
 * Reset the system.
 */
void
resetsys(void)
{

	call_req_reboot();
	printf("panic: can't reboot, halting\n");
	__asm("stop #0x2700");
}

int
a2int(char *cp)
{
	if (*cp == '\0')
		return -1;

	return atoi(cp);
}
