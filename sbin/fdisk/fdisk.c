/*	$NetBSD: fdisk.c,v 1.33.2.4 2000/06/01 17:36:12 he Exp $ */

/*
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: fdisk.c,v 1.33.2.4 2000/06/01 17:36:12 he Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/disklabel.h>
#include <sys/disklabel_mbr.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#ifdef __i386__
#include <ctype.h>
#include <machine/cpu.h>
#include <sys/sysctl.h>
#endif

#define LBUF 100
static char lbuf[LBUF];

/*
 * 14-Dec-89  Robert Baron (rvb) at Carnegie-Mellon University
 *	Copyright (c) 1989	Robert. V. Baron
 *	Created.
 */

char *disk = "/dev/rwd0d";

struct disklabel disklabel;		/* disk parameters */

int cylinders, sectors, heads, cylindersectors, disksectors;

struct mboot {
	u_int8_t	padding[2]; /* force the longs to be long alligned */
	u_int8_t	bootinst[MBR_PARTOFF];
	struct mbr_partition parts[NMBRPART];
	u_int16_t	signature;
};
struct mboot mboot;

#ifdef	__i386__

struct mbr_bootsel {
	u_int8_t defkey;
	u_int8_t flags;
	u_int16_t timeo;
	char nametab[4][9];
	u_int16_t magic;
} __attribute__((packed));

#define BFL_SELACTIVE   0x01
#define BFL_EXTINT13	0x02

#define SCAN_ENTER      0x1c
#define SCAN_F1         0x3b

#define MBR_BOOTSELOFF	(MBR_PARTOFF - sizeof (struct mbr_bootsel))

#define	DEFAULT_BOOTCODE	"/usr/mdec/mbr"
#define DEFAULT_BOOTSELCODE	"/usr/mdec/mbr_bootsel"
#define OPTIONS			"0123BSafius:b:c:"
#else
#define OPTIONS			"0123Safius:b:c:"
#endif

#define ACTIVE 0x80

int dos_cylinders;
int dos_heads;
int dos_sectors;
int dos_cylindersectors;

#define DOSSECT(s,c)	(((s) & 0x3f) | (((c) >> 2) & 0xc0))
#define DOSCYL(c)	((c) & 0xff)

#define	MAXCYL	1024
int partition = -1;

int a_flag;		/* set active partition */
int i_flag;		/* init bootcode */
int u_flag;		/* update partition data */
int sh_flag;		/* Output data as shell defines */
int f_flag;		/* force --not interactive */
int s_flag;		/* set id,offset,size */
int b_flag;		/* Set cyl, heads, secs (as c/h/s) */
int B_flag;		/* Edit/install bootselect code */
int b_cyl, b_head, b_sec;  /* b_flag values. */
int bootsel_modified;

unsigned char bootcode[8192];	/* maximum size of bootcode */
unsigned char tempcode[8192];
int bootsize;		/* actual size of bootcode */


static char reserved[] = "reserved";

struct part_type {
	int type;
	char *name;
} part_types[] = {
	{0x00, "unused"},
	{0x01, "Primary DOS with 12 bit FAT"},
	{0x02, "XENIX / filesystem"},
	{0x03, "XENIX /usr filesystem"},
	{0x04, "Primary DOS with 16 bit FAT <32M"},
	{0x05, "Extended partition"},
	{0x06, "Primary 'big' DOS, 16-bit FAT (> 32MB)"},
	{0x07, "OS/2 HPFS or NTFS or QNX2 or Advanced UNIX"},
	{0x08, "AIX filesystem or OS/2 (thru v1.3) or DELL multiple drives"
	       "or Commodore DOS or SplitDrive"},
	{0x09, "AIX boot partition or Coherent"},
	{0x0A, "OS/2 Boot Manager or Coherent swap or OPUS"},
	{0x0b, "Primary DOS with 32 bit FAT"},
	{0x0c, "Primary DOS with 32 bit FAT - LBA"},
	{0x0d, "Type 7??? - LBA"},
	{0x0E, "DOS (16-bit FAT) - LBA"},
	{0x0F, "Ext. partition - LBA"},
	{0x10, "OPUS"},
	{0x11, "OS/2 BM: hidden DOS 12-bit FAT"},
	{0x12, "Compaq diagnostics"},
	{0x14, "OS/2 BM: hidden DOS 16-bit FAT <32M or Novell DOS 7.0 bug"},
	{0x16, "OS/2 BM: hidden DOS 16-bit FAT >=32M"},
	{0x17, "OS/2 BM: hidden IFS"},
	{0x18, "AST Windows swapfile"},
	{0x19, "Willowtech Photon coS"},
	{0x1e, "hidden FAT95"},
	{0x20, "Willowsoft OFS1"},
	{0x21, reserved},
	{0x23, reserved},
	{0x24, "NEC DOS"},
	{0x26, reserved},
	{0x31, reserved},
	{0x33, reserved},
	{0x34, reserved},
	{0x36, reserved},
	{0x38, "Theos"},
	{0x3C, "PartitionMagic recovery"},
	{0x40, "VENIX 286 or LynxOS"},
	{0x41, "Linux/MINIX (sharing disk with DRDOS) or Personal RISC boot"},
	{0x42, "SFS or Linux swap (sharing disk with DRDOS)"},
	{0x43, "Linux native (sharing disk with DRDOS)"},
	{0x50, "DM (disk manager)"},
	{0x51, "DM6 Aux1 (or Novell)"},
	{0x52, "CP/M or Microport SysV/AT"},
	{0x53, "DM6 Aux3"},
	{0x54, "DM6 DDO"},
	{0x55, "EZ-Drive (disk manager)"},
	{0x56, "Golden Bow (disk manager)"},
	{0x5C, "Priam Edisk (disk manager)"},
	{0x61, "SpeedStor"},
	{0x63, "GNU HURD or Mach or Sys V/386 (such as ISC UNIX) or MtXinu"},
	{0x64, "Novell Netware 2.xx or Speedstore"},
	{0x65, "Novell Netware 3.xx"},
	{0x66, "Novell 386 Netware"},
	{0x67, "Novell"},
	{0x68, "Novell"},
	{0x69, "Novell"},
	{0x70, "DiskSecure Multi-Boot"},
	{0x71, reserved},
	{0x73, reserved},
	{0x74, reserved},
	{0x75, "PC/IX"},
	{0x76, reserved},
	{0x77, "QNX4.x"},
	{0x78, "QNX4.x 2nd part"},
	{0x79, "QNX4.x 3rd part"},
	{0x80, "MINIX until 1.4a"},
	{0x81, "MINIX since 1.4b, early Linux, Mitac dmgr"},
	{0x82, "Linux swap or Prime or Solaris"},
	{0x83, "Linux native"},
	{0x84, "OS/2 hidden C: drive"},
	{0x85, "Linux extended"},
	{0x86, "NT FAT volume set"},
	{0x87, "NTFS volume set or HPFS mirrored"},
	{0x93, "Amoeba filesystem"},
	{0x94, "Amoeba bad block table"},
	{0x99, "Mylex EISA SCSI"},
	{0x9f, "BSDI?"},
	{0xA0, "IBM Thinkpad hibernation"},
	{0xa1, reserved},
	{0xa3, reserved},
	{0xa4, reserved},
	{0xA5, "FreeBSD or 386BSD or old NetBSD"},
	{0xA6, "OpenBSD"},
	{0xA7, "NeXTSTEP 486"},
	{0xa9, "NetBSD"},
	{0xb1, reserved},
	{0xb3, reserved},
	{0xb4, reserved},
	{0xb6, reserved},
	{0xB7, "BSDI BSD/386 filesystem"},
	{0xB8, "BSDI BSD/386 swap"},
	{0xc0, "CTOS"},
	{0xC1, "DRDOS/sec (FAT-12)"},
	{0xC4, "DRDOS/sec (FAT-16, < 32M)"},
	{0xC6, "DRDOS/sec (FAT-16, >= 32M)"},
	{0xC7, "Syrinx (Cyrnix?) or HPFS disabled"},
	{0xd8, "CP/M 86"},
	{0xDB, "CP/M or Concurrent CP/M or Concurrent DOS or CTOS"},
	{0xE1, "DOS access or SpeedStor 12-bit FAT extended partition"},
	{0xE3, "DOS R/O or SpeedStor or Storage Dimensions"},
	{0xE4, "SpeedStor 16-bit FAT extended partition < 1024 cyl."},
	{0xe5, reserved},
	{0xe6, reserved},
	{0xeb, "BeOS"},
	{0xF1, "SpeedStor or Storage Dimensions"},
	{0xF2, "DOS 3.3+ Secondary"},
	{0xf3, reserved},
	{0xF4, "SpeedStor large partition or Storage Dimensions"},
	{0xf6, reserved},
	{0xFE, "SpeedStor >1024 cyl. or LANstep or IBM PS/2 IML"},
	{0xFF, "Xenix Bad Block Table"},
};

void	usage __P((void));
void	print_s0 __P((int));
void	print_part __P((int));
int	read_boot __P((char *, char *, size_t));
void	init_sector0 __P((int, int));
void	intuit_translated_geometry __P((void));
void	get_geometry __P((void));
void	get_diskname __P((char *, char *, size_t));
int	try_heads __P((quad_t, quad_t, quad_t, quad_t, quad_t, quad_t, quad_t,
		       quad_t));
int	try_sectors __P((quad_t, quad_t, quad_t, quad_t, quad_t));
void	change_part __P((int, int, int, int));
void	print_params __P((void));
void	change_active __P((int));
void	get_params_to_use __P((void));
void	dos __P((int, unsigned char *, unsigned char *, unsigned char *));
int	open_disk __P((int));
int	read_disk __P((int, void *));
int	write_disk __P((int, void *));
int	get_params __P((void));
int	read_s0 __P((void));
int	write_s0 __P((void));
int	yesno __P((char *));
void	decimal __P((char *, int *));
int	type_match __P((const void *, const void *));
char	*get_type __P((int));
int	get_mapping __P((int, int *, int *, int *, long *));
#ifdef __i386__
void	configure_bootsel __P((void));
#endif

static inline unsigned short getshort __P((void *));
static inline void putshort __P((void *p, unsigned short));
static inline unsigned long getlong __P((void *));
static inline void putlong __P((void *,	unsigned long));


int	main __P((int, char **));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch;
	int part;

	int csysid, cstart, csize;	/* For the b_flag. */

	a_flag = i_flag = u_flag = sh_flag = f_flag = s_flag = b_flag = 0;
	csysid = cstart = csize = 0;
	while ((ch = getopt(argc, argv, OPTIONS)) != -1)
		switch (ch) {
		case '0':
			partition = 0;
			break;
		case '1':
			partition = 1;
			break;
		case '2':
			partition = 2;
			break;
		case '3':
			partition = 3;
			break;
#ifdef __i386__
		case 'B':
			B_flag = 1;
			break;
#endif
		case 'S':
			sh_flag = 1;
			break;
		case 'a':
			a_flag = 1;
			break;
		case 'f':
			f_flag = 1;
			break;
		case 'i':
			i_flag = 1;
			break;
		case 'u':
			u_flag = 1;
			break;
		case 's':
			s_flag = 1;
			if (sscanf (optarg, "%d/%d/%d",
				    &csysid, &cstart, &csize) != 3) {
				(void)fprintf (stderr, "%s: Bad argument "
					       "to the -s flag.\n",
					       argv[0]);
				exit (1);
			}
			break;
		case 'b':
			b_flag = 1;
			if (sscanf (optarg, "%d/%d/%d",
				    &b_cyl, &b_head, &b_sec) != 3) {
				(void)fprintf (stderr, "%s: Bad argument "
					       "to the -b flag.\n",
					       argv[0]);
				exit (1);
			}
			if (b_cyl > MAXCYL)
				b_cyl = MAXCYL;
			break;
		case 'c':
			bootsize = read_boot(optarg, bootcode, sizeof bootcode);
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (sh_flag && (a_flag || i_flag || u_flag || f_flag || s_flag))
		usage();

	if (B_flag && (a_flag || i_flag || u_flag || f_flag || s_flag))
		usage();

	if (partition == -1 && s_flag) {
		(void) fprintf (stderr,
				"-s flag requires a partition selected.\n");
		usage();
	}

	if (argc > 0)
		disk = argv[0];

	if (open_disk(B_flag || a_flag || i_flag || u_flag) < 0)
		exit(1);

	if (read_s0())
		init_sector0(sectors > 63 ? 63 : sectors, 1);

#ifdef __i386__
	get_geometry();
#else
	intuit_translated_geometry();
#endif


	if ((i_flag || u_flag) && (!f_flag || b_flag))
		get_params_to_use();

	if (i_flag)
		init_sector0(dos_sectors > 63 ? 63 : dos_sectors, 0);

	/* Do the update stuff! */
	if (u_flag) {
		if (!f_flag)
			printf("Partition table:\n");
		if (partition == -1)
			for (part = 0; part < NMBRPART; part++)
				change_part(part,-1, -1, -1);
		else
			change_part(partition, csysid, cstart, csize);
	} else
		if (!i_flag)
			print_s0(partition);

	if (a_flag)
		change_active(partition);

#ifdef __i386__
	if (B_flag) {
		configure_bootsel();
		if (B_flag && bootsel_modified)
			write_s0();
	}
#endif

	if (u_flag || a_flag || i_flag) {
		if (!f_flag) {
			printf("\nWe haven't written the MBR back to disk "
			       "yet.  This is your last chance.\n");
			print_s0(-1);
			if (yesno("Should we write new partition table?"))
				write_s0();
		} else
			write_s0();
	}

	exit(0);
}

void
usage()
{
	(void)fprintf(stderr, "usage: fdisk [-aiufSc] [-0|-1|-2|-3] "
		      "[device]\n");
	exit(1);
}

void
print_s0(which)
	int which;
{
	int part;

	print_params();
	if (!sh_flag)
		printf("Partition table:\n");
	if (which == -1) {
		for (part = 0; part < NMBRPART; part++) {
			if (!sh_flag)
				printf("%d: ", part);
			print_part(part);
		}
	} else
		print_part(which);
}

static inline unsigned short
getshort(p)
	void *p;
{
	unsigned char *cp = p;

	return cp[0] | (cp[1] << 8);
}

static inline void
putshort(p, l)
	void *p;
	unsigned short l;
{
	unsigned char *cp = p;

	*cp++ = l;
	*cp++ = l >> 8;
}

static inline unsigned long
getlong(p)
	void *p;
{
	unsigned char *cp = p;

	return cp[0] | (cp[1] << 8) | (cp[2] << 16) | (cp[3] << 24);
}

static inline void
putlong(p, l)
	void *p;
	unsigned long l;
{
	unsigned char *cp = p;

	*cp++ = l;
	*cp++ = l >> 8;
	*cp++ = l >> 16;
	*cp++ = l >> 24;
}

void
print_part(part)
	int part;
{
	struct mbr_partition *partp;
	int empty;

	partp = &mboot.parts[part];
	empty = (partp->mbrp_typ == 0);

	if (sh_flag) {
		if (empty) {
			printf("PART%dSIZE=0\n", part);
			return;
		}

		printf("PART%dID=%d\n", part, partp->mbrp_typ);
		printf("PART%dSIZE=%ld\n", part, getlong(&partp->mbrp_size));
		printf("PART%dSTART=%ld\n", part, getlong(&partp->mbrp_start));
		printf("PART%dFLAG=0x%x\n", part, partp->mbrp_flag);
		printf("PART%dBCYL=%d\n", part, MBR_PCYL(partp->mbrp_scyl,
						      partp->mbrp_ssect));
		printf("PART%dBHEAD=%d\n", part, partp->mbrp_shd);
		printf("PART%dBSEC=%d\n", part, MBR_PSECT(partp->mbrp_ssect));
		printf("PART%dECYL=%d\n", part, MBR_PCYL(partp->mbrp_ecyl,
						      partp->mbrp_esect));
		printf("PART%dEHEAD=%d\n", part, partp->mbrp_ehd);
		printf("PART%dESEC=%d\n", part, MBR_PSECT(partp->mbrp_esect));
		return;
	}

	/* Not sh_flag. */
	if (empty) {
		printf("<UNUSED>\n");
		return;
	}
	printf("sysid %d (%s)\n", partp->mbrp_typ, get_type(partp->mbrp_typ));
	printf("    start %ld, size %ld (%ld MB), flag 0x%x\n",
	    getlong(&partp->mbrp_start), getlong(&partp->mbrp_size),
	    getlong(&partp->mbrp_size) * 512 / (1024 * 1024), partp->mbrp_flag);
	printf("\tbeg: cylinder %4d, head %3d, sector %2d\n",
	    MBR_PCYL(partp->mbrp_scyl, partp->mbrp_ssect),
	    partp->mbrp_shd, MBR_PSECT(partp->mbrp_ssect));
	printf("\tend: cylinder %4d, head %3d, sector %2d\n",
	    MBR_PCYL(partp->mbrp_ecyl, partp->mbrp_esect),
	    partp->mbrp_ehd, MBR_PSECT(partp->mbrp_esect));
}

int
read_boot(name, buf, len)
	char *name;
	char *buf;
	size_t len;
{
	int bfd, ret;
	struct stat st;

	if ((bfd = open(name, O_RDONLY)) < 0)
		err(1, "%s", name);
	if (fstat(bfd, &st) == -1)
		err(1, "%s", name);
	if (st.st_size > len)
		errx(1, "%s: bootcode too large", name);
	ret = st.st_size;
	if (ret < 0x200)
		errx(1, "%s: bootcode too small", name);
	if (read(bfd, buf, len) != ret)
		err(1, "%s", name);
	close(bfd);

	/*
	 * Do some sanity checking here
	 */
	if (getshort(bootcode + MBR_MAGICOFF) != MBR_MAGIC)
		errx(1, "%s: invalid magic", name);
	ret = (ret + 0x1ff) / 0x200;
	ret *= 0x200;
	return ret;
}

void
init_sector0(start, dopart)
	int start, dopart;
{
	int i;

#ifdef	DEFAULT_BOOTCODE
	if (!bootsize)
		bootsize = read_boot(DEFAULT_BOOTCODE, bootcode,
		    sizeof bootcode);
#endif

	memcpy(mboot.bootinst, bootcode, sizeof(mboot.bootinst));
	putshort(&mboot.signature, MBR_MAGIC);
	
	if (dopart)
		for (i=0; i<4; i++) 
			memset(&mboot.parts[i], 0, sizeof(struct mbr_partition));

}

#ifdef __i386__

void	    
get_diskname(fullname, diskname, size)
	char *fullname; 
	char *diskname;
	size_t size;    
{	       
	char *p;
	char *p2;
	size_t len;

	p = strrchr(fullname, '/');
	if (p == NULL)
		p = fullname;
	else
		p++;

	if (*p == 0) {
		strncpy(diskname, fullname, size - 1);
		diskname[size - 1] = '\0';
		return;
	}

	if (*p == 'r')
		p++;

	for (p2 = p; *p2 != 0; p2++)
		if (isdigit(*p2))
			break;
	if (*p2 == 0) {
		/* XXX invalid diskname? */
		strncpy(diskname, fullname, size - 1);
		diskname[size - 1] = '\0';
		return;
	}
	while (isdigit(*p2))
		p2++; 

	len = p2 - p;
	if (len > size) {
		/* XXX */
		strncpy(diskname, fullname, size - 1);
		diskname[size - 1] = '\0';
		return;
	}
 
	strncpy(diskname, p, len);
	diskname[len] = 0;
}

void
get_geometry()
{
	int mib[2], i;
	size_t len;
	struct disklist *dl;
	struct biosdisk_info *bip;
	struct nativedisk_info *nip;
	char diskname[8];

	mib[0] = CTL_MACHDEP;
	mib[1] = CPU_DISKINFO;
	if (sysctl(mib, 2, NULL, &len, NULL, 0) < 0) {
		intuit_translated_geometry();
		return;
	}
	dl = (struct disklist *) malloc(len);
	sysctl(mib, 2, dl, &len, NULL, 0);

	get_diskname(disk, diskname, sizeof diskname);

	for (i = 0; i < dl->dl_nnativedisks; i++) {
		nip = &dl->dl_nativedisks[i];
		if (strcmp(diskname, nip->ni_devname))
			continue;
		/*
		 * XXX listing possible matches is better. This is ok
		 * for now because the user has a chance to change
		 * it later.
		 */
		if (nip->ni_nmatches != 0) {
			bip = &dl->dl_biosdisks[nip->ni_biosmatches[0]];
			dos_cylinders = bip->bi_cyl;
			dos_heads = bip->bi_head;
			dos_sectors = bip->bi_sec;
			dos_cylindersectors = bip->bi_head * bip->bi_sec;
			return;
		}
	}
	/* Allright, allright, make a stupid guess.. */
	intuit_translated_geometry();
}

void
configure_bootsel()
{
	struct mbr_bootsel *mbs =
	    (struct mbr_bootsel *)&mboot.bootinst[MBR_BOOTSELOFF];
	int i, nused, firstpart = -1, item;
	char desc[10], *p;
	int timo, entry_changed = 0;

	for (i = nused = 0; i < NMBRPART; i++) {
		if (mboot.parts[i].mbrp_typ != 0) {
			if (firstpart == -1)
				firstpart = i;
			nused++;
		}
	}

	if (nused == 0) {
		printf("No used partitions found. Partition the disk first.\n");
		return;
	}

	if (mbs->magic != MBR_MAGIC) {
		if (!yesno("Bootselector not yet installed. Install it now?")) {
			printf("Bootselector not installed.\n");
			return;
		}
		bootsize = read_boot(DEFAULT_BOOTSELCODE, bootcode,
		    sizeof bootcode);
		memcpy(mboot.bootinst, bootcode, sizeof(mboot.bootinst));
		bootsel_modified = 1;
		mbs->flags |= BFL_SELACTIVE;
	} else {
		if (mbs->flags & BFL_SELACTIVE) {
			printf("The bootselector is installed and active.\n");
			if (!yesno("Do you want to change its settings?")) {
				if (yesno("Do you want to deactivate it?")) {
					mbs->flags &= ~BFL_SELACTIVE;
					bootsel_modified = 1;
					goto done;
				}
				return;
			}
		} else {
			printf("The bootselector is installed but not active.\n");
			if (yesno("Do you want to activate it?")) {
				mbs->flags |= BFL_SELACTIVE;
				bootsel_modified = 1;
			}
			if (!yesno("Do you want to change its settings?"))
				goto done;
		}
	}

	printf("\n\nPartition table:\n");
	for (i = 0; i < NMBRPART; i++) {
		printf("%d: ", i);
		print_part(i);
	}

	printf("\n\nCurrent boot selection menu option names:\n");
	for (i = 0; i < NMBRPART; i++) {
		if (mbs->nametab[i][0] != 0)
			printf("%d: %s\n", i, &mbs->nametab[i][0]);
		else
			printf("%d: Unused\n", i);
	}
	printf("\n");

	item = firstpart;

editentries:
	while (1) {
		decimal("Change which entry (-1 quits)?", &item);
		if (item == -1)
			break;
		if (item < 0 || item >= NMBRPART) {
			printf("Invalid entry number\n");
			continue;
		}
		if (mboot.parts[item].mbrp_typ == 0) {
			printf("The matching partition entry is unused\n");
			continue;
		}

		printf("Enter descriptions (max. 8 characters): ");
		rewind(stdin);
		fgets(desc, 10, stdin);
		p = strchr(desc, '\n');
		if (p != NULL)
			*p = 0;
		else
			desc[9] = 0;
		strcpy(&mbs->nametab[item][0], desc);
		entry_changed = bootsel_modified = 1;
	}

	if (entry_changed)
		printf("Boot selection menu option names are now:\n");

	firstpart = -1;
	for (i = 0; i < NMBRPART; i++) {
		if (mbs->nametab[i][0] != 0) {
			firstpart = i;
			if (entry_changed)
				printf("%d: %s\n", i, &mbs->nametab[i][0]);
		} else {
			if (entry_changed)
				printf("%d: Unused\n", i);
		}
	}
	if (entry_changed)
		printf("\n");

	if (firstpart == -1) {
		printf("All menu entries are now inactive.\n");
		if (!yesno("Are you sure about this?"))
			goto editentries;
	} else {
		if (!(mbs->flags & BFL_SELACTIVE)) {
			printf("The bootselector is not yet active.\n");
			if (yesno("Activate it now?"))
				mbs->flags |= BFL_SELACTIVE;
		}
	}

	/* bootsel is dirty from here on out. */
	bootsel_modified = 1;

	/* The timeout value is in ticks, 18.2 Hz. Avoid using floats. */
	timo = ((1000 * mbs->timeo) / 18200);
	do {
		decimal("Timeout value", &timo);
	} while (timo < 0 || timo > 3600);
	mbs->timeo = (u_int16_t)((timo * 18200) / 1000);

	printf("Select the default boot option. Options are:\n\n");
	for (i = 0; i < NMBRPART; i++) {
		if (mbs->nametab[i][0] != 0)
			printf("%d: %s\n", i, &mbs->nametab[i][0]);
	}
	for (i = 4; i < 10; i++)
		printf("%d: Harddisk %d\n", i, i - 4);
	printf("10: The first active partition\n");

	if (mbs->defkey == SCAN_ENTER)
		item = 10;
	else
		item = mbs->defkey - SCAN_F1;

	if (item < 0 || item > 10 || mbs->nametab[item][0] == 0)
		item = 10;

	do {
		decimal("Default boot option", &item);
	} while (item < 0 || item > 10 ||
		    (item <= 3 && mbs->nametab[item][0] == 0));

	if (item == 10)
		mbs->defkey = SCAN_ENTER;
	else
		mbs->defkey = SCAN_F1 + item;

done:
	for (i = 0; i < NMBRPART; i++) {
		if (mboot.parts[i].mbrp_typ != 0 &&
		   mboot.parts[i].mbrp_start >=
		     (dos_cylinders * dos_heads * dos_sectors)) {
			mbs->flags |= BFL_EXTINT13;
			break;
		}
	}

	if (bootsel_modified != 0 && !yesno("Update the bootselector?"))
		bootsel_modified = 0;
}
#endif


/* Prerequisite: the disklabel parameters and master boot record must
 *		 have been read (i.e. dos_* and mboot are meaningful).
 * Specification: modifies dos_cylinders, dos_heads, dos_sectors, and
 *		  dos_cylindersectors to be consistent with what the
 *		  partition table is using, if we can find a geometry
 *		  which is consistent with all partition table entries.
 *		  We may get the number of cylinders slightly wrong (in
 *		  the conservative direction).  The idea is to be able
 *		  to create a NetBSD partition on a disk we don't know
 *		  the translated geometry of.
 * This whole routine should be replaced with a kernel interface to get
 * the BIOS geometry (which in turn requires modifications to the i386
 * boot loader to pass in the BIOS geometry for each disk). */
void
intuit_translated_geometry()
{

	int cylinders = -1, heads = -1, sectors = -1, i, j;
	int c1, h1, s1, c2, h2, s2;
	long a1, a2;
	quad_t num, denom;

	/* Try to deduce the number of heads from two different mappings. */
	for (i = 0; i < NMBRPART * 2; i++) {
		if (get_mapping(i, &c1, &h1, &s1, &a1) < 0)
			continue;
		for (j = 0; j < 8; j++) {
			if (get_mapping(j, &c2, &h2, &s2, &a2) < 0)
				continue;
			num = (quad_t)h1*(a2-s2) - (quad_t)h2*(a1-s1);
			denom = (quad_t)c2*(a1-s1) - (quad_t)c1*(a2-s2);
			if (denom != 0 && num % denom == 0) {
				heads = num / denom;
				break;
			}
		}
		if (heads != -1)	
			break;
	}

	if (heads == -1)
		return;

	/* Now figure out the number of sectors from a single mapping. */
	for (i = 0; i < NMBRPART * 2; i++) {
		if (get_mapping(i, &c1, &h1, &s1, &a1) < 0)
			continue;
		num = a1 - s1;
		denom = c1 * heads + h1;
		if (denom != 0 && num % denom == 0) {
			sectors = num / denom;
			break;
		}
	}

	if (sectors == -1)
		return;

	/* Estimate the number of cylinders. */
	cylinders = disklabel.d_secperunit / heads / sectors;

	/* Now verify consistency with each of the partition table entries.
	 * Be willing to shove cylinders up a little bit to make things work,
	 * but translation mismatches are fatal. */
	for (i = 0; i < NMBRPART * 2; i++) {
		if (get_mapping(i, &c1, &h1, &s1, &a1) < 0)
			continue;
		if (sectors * (c1 * heads + h1) + s1 != a1)
			return;
		if (c1 >= cylinders)
			cylinders = c1 + 1;
	}

	/* Everything checks out.  Reset the geometry to use for further
	 * calculations. */
	dos_cylinders = cylinders;
	dos_heads = heads;
	dos_sectors = sectors;
	dos_cylindersectors = heads * sectors;
}

/* For the purposes of intuit_translated_geometry(), treat the partition
 * table as a list of eight mapping between (cylinder, head, sector)
 * triplets and absolute sectors.  Get the relevant geometry triplet and
 * absolute sectors for a given entry, or return -1 if it isn't present.
 * Note: for simplicity, the returned sector is 0-based. */
int
get_mapping(i, cylinder, head, sector, absolute)
	int i, *cylinder, *head, *sector;
	long *absolute;
{
	struct mbr_partition *part = &mboot.parts[i / 2];

	if (part->mbrp_typ == 0)
		return -1;
	if (i % 2 == 0) {
		*cylinder = MBR_PCYL(part->mbrp_scyl, part->mbrp_ssect);
		*head = part->mbrp_shd;
		*sector = MBR_PSECT(part->mbrp_ssect) - 1;
		*absolute = getlong(&part->mbrp_start);
	} else {
		*cylinder = MBR_PCYL(part->mbrp_ecyl, part->mbrp_esect);
		*head = part->mbrp_ehd;
		*sector = MBR_PSECT(part->mbrp_esect) - 1;
		*absolute = getlong(&part->mbrp_start)
		    + getlong(&part->mbrp_size) - 1;
	}
	return 0;
}

void
change_part(part, csysid, cstart, csize)
	int part, csysid, cstart, csize;
{
	struct mbr_partition *partp;

	partp = &mboot.parts[part];

	if (s_flag) {
		if (csysid == 0 && cstart == 0 && csize == 0)
			memset(partp, 0, sizeof *partp);
		else {
			partp->mbrp_typ = csysid;
#if 0
			checkcyl(cstart / dos_cylindersectors);
#endif
			putlong(&partp->mbrp_start, cstart);
			putlong(&partp->mbrp_size, csize);
			dos(getlong(&partp->mbrp_start),
			    &partp->mbrp_scyl, &partp->mbrp_shd, &partp->mbrp_ssect);
			dos(getlong(&partp->mbrp_start)
			    + getlong(&partp->mbrp_size) - 1,
			    &partp->mbrp_ecyl, &partp->mbrp_ehd, &partp->mbrp_esect);
		}
		if (f_flag)
			return;
	}

	printf("The data for partition %d is:\n", part);
	print_part(part);
	if (!u_flag || !yesno("Do you want to change it?"))
		return;

	do {
		{
			int sysid, start, size;

			sysid = partp->mbrp_typ,
			start = getlong(&partp->mbrp_start),
			size = getlong(&partp->mbrp_size);
			decimal("sysid", &sysid);
			decimal("start", &start);
			decimal("size", &size);
			partp->mbrp_typ = sysid;
			putlong(&partp->mbrp_start, start);
			putlong(&partp->mbrp_size, size);
		}

		if (yesno("Explicitly specify beg/end address?")) {
			int tsector, tcylinder, thead;

			tcylinder = MBR_PCYL(partp->mbrp_scyl, partp->mbrp_ssect);
			thead = partp->mbrp_shd;
			tsector = MBR_PSECT(partp->mbrp_ssect);
			decimal("beginning cylinder", &tcylinder);
#if 0
			checkcyl(tcylinder);
#endif
			decimal("beginning head", &thead);
			decimal("beginning sector", &tsector);
			partp->mbrp_scyl = DOSCYL(tcylinder);
			partp->mbrp_shd = thead;
			partp->mbrp_ssect = DOSSECT(tsector, tcylinder);

			tcylinder = MBR_PCYL(partp->mbrp_ecyl, partp->mbrp_esect);
			thead = partp->mbrp_ehd;
			tsector = MBR_PSECT(partp->mbrp_esect);
			decimal("ending cylinder", &tcylinder);
			decimal("ending head", &thead);
			decimal("ending sector", &tsector);
			partp->mbrp_ecyl = DOSCYL(tcylinder);
			partp->mbrp_ehd = thead;
			partp->mbrp_esect = DOSSECT(tsector, tcylinder);
		} else {

			if (partp->mbrp_typ == 0
			    && getlong(&partp->mbrp_start) == 0
			    && getlong(&partp->mbrp_size) == 0)
				memset(partp, 0, sizeof *partp);
			else {
#if 0
				checkcyl(getlong(&partp->mbrp_start)
					 / dos_cylindersectors);
#endif
				dos(getlong(&partp->mbrp_start), &partp->mbrp_scyl,
				    &partp->mbrp_shd, &partp->mbrp_ssect);
				dos(getlong(&partp->mbrp_start)
				    + getlong(&partp->mbrp_size) - 1,
				    &partp->mbrp_ecyl, &partp->mbrp_ehd,
				    &partp->mbrp_esect);
			}
		}

		print_part(part);
	} while (!yesno("Is this entry okay?"));
}

void
print_params()
{

	if (sh_flag) {
		printf ("DLCYL=%d\nDLHEAD=%d\nDLSEC=%d\nDLSIZE=%d\n",
			cylinders, heads, sectors, disksectors);
		printf ("BCYL=%d\nBHEAD=%d\nBSEC=%d\n",
			dos_cylinders, dos_heads, dos_sectors);
		return;
	}

	/* Not sh_flag */
	printf("NetBSD disklabel disk geometry:\n");
	printf("cylinders: %d heads: %d sectors/track: %d (%d sectors/cylinder)\n\n",
	    cylinders, heads, sectors, cylindersectors);
	printf("BIOS disk geometry:\n");
	printf("cylinders: %d heads: %d sectors/track: %d (%d sectors/cylinder)\n\n",
	    dos_cylinders, dos_heads, dos_sectors, dos_cylindersectors);
}

void
change_active(which)
	int which;
{
	struct mbr_partition *partp;
	int part;
	int active = 4;

	partp = &mboot.parts[0];

	if (a_flag && which != -1)
		active = which;
	else {
		for (part = 0; part < NMBRPART; part++)
			if (partp[part].mbrp_flag & ACTIVE)
				active = part;
	}
	if (!f_flag) {
		if (yesno("Do you want to change the active partition?")) {
			printf ("Choosing 4 will make no partition active.\n");
			do {
				decimal("active partition", &active);
			} while (!yesno("Are you happy with this choice?"));
		} else
			return;
	} else
		if (active != 4)
			printf ("Making partition %d active.\n", active);

	for (part = 0; part < NMBRPART; part++)
		partp[part].mbrp_flag &= ~ACTIVE;
	if (active < 4)
		partp[active].mbrp_flag |= ACTIVE;
}

void
get_params_to_use()
{
	if (b_flag) {
		dos_cylinders = b_cyl;
		dos_heads = b_head;
		dos_sectors = b_sec;
		dos_cylindersectors = dos_heads * dos_sectors;
		return;
	}

	print_params();
	if (yesno("Do you want to change our idea of what BIOS thinks?")) {
		do {
			decimal("BIOS's idea of #cylinders", &dos_cylinders);
			decimal("BIOS's idea of #heads", &dos_heads);
			decimal("BIOS's idea of #sectors", &dos_sectors);
			dos_cylindersectors = dos_heads * dos_sectors;
			print_params();
		} while (!yesno("Are you happy with this choice?"));
	}
}

/***********************************************\
* Change real numbers into strange dos numbers	*
\***********************************************/
void
dos(sector, cylinderp, headp, sectorp)
	int sector;
	unsigned char *cylinderp, *headp, *sectorp;
{
	int cylinder, head;
	int biosmaxsec;

	biosmaxsec = dos_cylinders * dos_heads * dos_sectors - 1;
	if (sector > biosmaxsec)
		sector = biosmaxsec;

	cylinder = sector / dos_cylindersectors;

	sector -= cylinder * dos_cylindersectors;

	head = sector / dos_sectors;
	sector -= head * dos_sectors;

	*cylinderp = DOSCYL(cylinder);
	*headp = head;
	*sectorp = DOSSECT(sector + 1, cylinder);
}

#if 0
void
checkcyl(cyl)
	int cyl;
{
	if (cyl >= MAXCYL)
		warnx("partition start beyond BIOS limit");
}
#endif

int fd;

int
open_disk(u_flag)
	int u_flag;
{
	static char namebuf[MAXPATHLEN + 1];
	struct stat st;

	fd = opendisk(disk, u_flag ? O_RDWR : O_RDONLY, namebuf,
	    sizeof(namebuf), 0);
	if (fd < 0) {
		warn("%s", namebuf);
		return (-1);
	}
	disk = namebuf;
	if (fstat(fd, &st) == -1) {
		close(fd);
		warn("%s", disk);
		return (-1);
	}
	if (!S_ISCHR(st.st_mode) && !S_ISREG(st.st_mode)) {
		close(fd);
		warnx("%s is not a character device or regular file", disk);
		return (-1);
	}
	if (get_params() == -1) {
		close(fd);
		return (-1);
	}
	return (0);
}

int
read_disk(sector, buf)
	int sector;
	void *buf;
{

	if (lseek(fd, (off_t)(sector * 512), 0) == -1)
		return (-1);
	return (read(fd, buf, 512));
}

int
write_disk(sector, buf)
	int sector;
	void *buf;
{

	if (lseek(fd, (off_t)(sector * 512), 0) == -1)
		return (-1);
	return (write(fd, buf, 512));
}

int
get_params()
{

	if (ioctl(fd, DIOCGDINFO, &disklabel) == -1) {
		warn("DIOCGDINFO");
		return (-1);
	}

	dos_cylinders = cylinders = disklabel.d_ncylinders;
	dos_heads = heads = disklabel.d_ntracks;
	dos_sectors = sectors = disklabel.d_nsectors;
	dos_cylindersectors = cylindersectors = heads * sectors;
	disksectors = disklabel.d_secperunit;

	return (0);
}

int
read_s0()
{

	if (read_disk(0, mboot.bootinst) == -1) {
		warn("can't read fdisk partition table");
		return (-1);
	}
	if (getshort(&mboot.signature) != MBR_MAGIC) {
		warnx("invalid fdisk partition table found");
		return (-1);
	}
	return (0);
}

int
write_s0()
{
	int flag, i;

	/*
	 * write enable label sector before write (if necessary),
	 * disable after writing.
	 * needed if the disklabel protected area also protects
	 * sector 0. (e.g. empty disk)
	 */
	flag = 1;
	if (ioctl(fd, DIOCWLABEL, &flag) < 0)
		warn("DIOCWLABEL");
	if (write_disk(0, mboot.bootinst) == -1) {
		warn("can't write fdisk partition table");
		return -1;
	}
	for (i = bootsize; (i -= 0x200) > 0;)
		if (write_disk(i / 0x200, bootcode + i) == -1) {
			warn("can't write bootcode");
			return -1;
		}
	flag = 0;
	if (ioctl(fd, DIOCWLABEL, &flag) < 0)
		warn("DIOCWLABEL");
	return 0;
}

int
yesno(str)
	char *str;
{
	int ch, first;

	printf("%s [n] ", str);

	first = ch = getchar();
	while (ch != '\n' && ch != EOF)
		ch = getchar();
	return (first == 'y' || first == 'Y');
}

void
decimal(str, num)
	char *str;
	int *num;
{
	int acc = 0;
	char *cp;

	for (;; printf("%s is not a valid decimal number.\n", lbuf)) {
		printf("%s: [%d] ", str, *num);

		fgets(lbuf, LBUF, stdin);
		lbuf[strlen(lbuf)-1] = '\0';
		cp = lbuf;

		cp += strspn(cp, " \t");
		if (*cp == '\0')
			return;

		if (!isdigit(*cp) && *cp != '-')
			continue;
		acc = strtol(lbuf, &cp, 10);

		cp += strspn(cp, " \t");
		if (*cp != '\0')
			continue;

		*num = acc;
		return;
	}

}

int
type_match(key, item)
	const void *key, *item;
{
	const int *typep = key;
	const struct part_type *ptr = item;

	if (*typep < ptr->type)
		return (-1);
	if (*typep > ptr->type)
		return (1);
	return (0);
}

char *
get_type(type)
	int type;
{
	struct part_type *ptr;

	ptr = bsearch(&type, part_types,
	    sizeof(part_types) / sizeof(struct part_type),
	    sizeof(struct part_type), type_match);
	if (ptr == 0)
		return ("unknown");
	return (ptr->name);
}
