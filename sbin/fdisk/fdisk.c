/*	$NetBSD: fdisk.c,v 1.77 2004/03/24 02:49:37 lukem Exp $ */

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

/*
 * 14-Dec-89  Robert Baron (rvb) at Carnegie-Mellon University
 *	Copyright (c) 1989	Robert. V. Baron
 *	Created.
 */

#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: fdisk.c,v 1.77 2004/03/24 02:49:37 lukem Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/disklabel.h>
#include <sys/bootblock.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/sysctl.h>

#include <ctype.h>
#include <disktab.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#define	DEFAULT_BOOTDIR		"/usr/mdec"

#if defined(__i386__) || defined(__x86_64__)
#include <machine/cpu.h>
#define BOOTSEL

#define	DEFAULT_BOOTCODE	"mbr"
#define	DEFAULT_BOOTSELCODE	"mbr_bootsel"
#define	DEFAULT_BOOTEXTCODE	"mbr_ext"

/* Scan values for the various keys we use, as returned by the BIOS */
#define	SCAN_ENTER	0x1c
#define	SCAN_F1		0x3b
#define	SCAN_1		0x2

#endif

#define LBUF 100
static char lbuf[LBUF];

#ifndef PRIdaddr
#define PRIdaddr PRId64
#endif

#ifndef _PATH_DEFDISK
#define _PATH_DEFDISK	"/dev/rwd0d"
#endif

const char *disk = _PATH_DEFDISK;

struct disklabel disklabel;		/* disk parameters */

uint cylinders, sectors, heads;
daddr_t disksectors;
#define cylindersectors (heads * sectors)

struct mbr_sector mboot;


struct {
	struct mbr_sector *ptn;		/* array of pbrs */
	daddr_t		base;		/* first sector of ext. ptn */
	daddr_t		limit;		/* last sector of ext. ptn */
	int		num_ptn;	/* number of contained partitions */
	int		ptn_id;		/* entry in mbr */
	int		is_corrupt;	/* 1 if extended chain illegal */
} ext;

char *boot_dir = DEFAULT_BOOTDIR;
char *boot_path = 0;			/* name of file we actually opened */

#ifdef BOOTSEL

#define DEFAULT_ACTIVE	(~(daddr_t)0)

#define OPTIONS			"0123BFSafiluvs:b:c:E:r:w:t:T:"
#else
#define change_part(e, p, id, st, sz, bm) change__part(e, p, id, st, sz)
#define OPTIONS			"0123FSafiluvs:b:c:E:r:w:"
#endif

uint dos_cylinders;
uint dos_heads;
uint dos_sectors;
daddr_t dos_disksectors;
#define dos_cylindersectors (dos_heads * dos_sectors)
#define dos_totalsectors (dos_heads * dos_sectors * dos_cylinders)

#define DOSSECT(s,c)	(((s) & 0x3f) | (((c) >> 2) & 0xc0))
#define DOSCYL(c)	((c) & 0xff)
#define SEC_IN_1M (1024 * 1024 / 512)
#define SEC_TO_MB(sec) ((uint)(((sec) + SEC_IN_1M / 2) / SEC_IN_1M))
#define SEC_TO_CYL(sec) (((sec) + dos_cylindersectors/2) / dos_cylindersectors)

#define MAXCYL		1024	/* Usual limit is 1023 */
#define	MAXHEAD		256	/* Usual limit is 255 */
#define	MAXSECTOR	63
int partition = -1;

int fd = -1, wfd = -1, *rfd = &fd;
char *disk_file;
char *disk_type = NULL;

int a_flag;		/* set active partition */
int i_flag;		/* init bootcode */
int u_flag;		/* update partition data */
int v_flag;		/* more verbose */
int sh_flag;		/* Output data as shell defines */
int f_flag;		/* force --not interactive */
int s_flag;		/* set id,offset,size */
int b_flag;		/* Set cyl, heads, secs (as c/h/s) */
int B_flag;		/* Edit/install bootselect code */
int E_flag;		/* extended partition number */
int b_cyl, b_head, b_sec;  /* b_flag values. */
int F_flag = 0;

struct mbr_sector bootcode[8192 / sizeof (struct mbr_sector)];
int bootsize;		/* actual size of bootcode */
int boot_installed;	/* 1 if we've copied code into the mbr */

#if defined(__i386__) || defined(__x86_64__)
struct disklist *dl;
#endif


static char reserved[] = "reserved";

struct part_type {
	int		 type;
	const char	*name;
} part_types[] = {
	{0x00, "<UNUSED>"},
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
	{0x4D, "QNX4.x"},
	{0x4E, "QNX4.x 2nd part"},
	{0x4F, "QNX4.x 3rd part"},
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
	{0xa8, "Apple UFS"},
	{0xa9, "NetBSD"},
	{0xab, "Apple Boot"},
	{0xaf, "Apple HFS"},
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

#define KNOWN_SYSIDS	(sizeof(part_types)/sizeof(part_types[0]))

void	usage(void);
void	print_s0(int);
void	print_part(struct mbr_sector *, int, daddr_t);
void	print_mbr_partition(struct mbr_sector *, int, daddr_t, daddr_t, int);
int	read_boot(const char *, void *, size_t, int);
void	init_sector0(int);
void	intuit_translated_geometry(void);
void	get_geometry(void);
void	get_extended_ptn(void);
void	get_diskname(const char *, char *, size_t);
int	change_part(int, int, int, daddr_t, daddr_t, char *);
void	print_params(void);
void	change_active(int);
void	get_params_to_use(void);
void	dos(int, unsigned char *, unsigned char *, unsigned char *);
int	open_disk(int);
int	read_disk(daddr_t, void *);
int	write_disk(daddr_t, void *);
int	get_params(void);
int	read_s0(daddr_t, struct mbr_sector *);
int	write_mbr(void);
int	yesno(const char *, ...);
int	decimal(const char *, int, int, int, int);
#define DEC_SEC		1		/* asking for a sector number */
#define	DEC_RND		2		/* round to end of first track */
#define	DEC_RND_0	4		/* round 0 to size of a track */
#define DEC_RND_DOWN	8		/* subtract 1 track */
#define DEC_RND_DOWN_2	16		/* subtract 2 tracks */
void	string(const char *, int, char *);
int	ptn_id(const char *, int *);
int	type_match(const void *, const void *);
const char *get_type(int);
int	get_mapping(int, uint *, uint *, uint *, unsigned long *);
#ifdef BOOTSEL
daddr_t	configure_bootsel(daddr_t);
void	install_bootsel(int);
daddr_t	get_default_boot(void);
void	set_default_boot(daddr_t);
#endif


int	main(int, char *[]);

int
main(int argc, char *argv[])
{
	struct stat sb;
	int ch, mib[2];
	size_t len;
	char *root_device;
	char *cp;
	int n;
#ifdef BOOTSEL
	daddr_t default_ptn;		/* start sector of default ptn */
	char *cbootmenu = 0;
#endif

	int csysid, cstart, csize;	/* For the b_flag. */

	mib[0] = CTL_KERN;
	mib[1] = KERN_ROOT_DEVICE;
	if (sysctl(mib, 2, NULL, &len, NULL, 0) != -1 &&
	    (root_device = malloc(len)) != NULL &&
	    sysctl(mib, 2, root_device, &len, NULL, 0) != -1)
		disk = root_device;

	a_flag = i_flag = u_flag = sh_flag = f_flag = s_flag = b_flag = 0;
	v_flag = 0;
	E_flag = 0;
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
		case 'E':	/* Extended partition number */
			E_flag = 1;
			partition = strtoul(optarg, &cp, 0);
			if (*cp || partition < 0)
				errx(1, "Bad partition number -E %s.", optarg);
			break;
#ifdef BOOTSEL
		case 'B':	/* Bootselect parameters */
			B_flag = 1;
			break;
#endif
		case 'F':	/* device argument is really a file */
			F_flag = 1;
			break;
		case 'S':	/* Output as shell variables */
			sh_flag = 1;
			break;
		case 'a':	/* Set active partition */
			a_flag = 1;
			break;
		case 'f':	/* Non interactive */
			f_flag = 1;
			break;
		case 'i':	/* Always update bootcode */
			i_flag = 1;
			break;
		case 'l':	/* List known partition types */
			for (len = 0; len < KNOWN_SYSIDS; len++)
				printf("%03d %s\n", part_types[len].type,
				    part_types[len].name);
			return 0;
		case 'u':	/* Update partition details */
			u_flag = 1;
			break;
		case 'v':	/* Be verbose */
			v_flag++;
			break;
		case 's':	/* Partition details */
			s_flag = 1;
			if (sscanf(optarg, "%d/%d/%d%n", &csysid, &cstart,
			    &csize, &n) == 3) {
				if (optarg[n] == 0)
					break;
#ifdef BOOTSEL
				if (optarg[n] == '/') {
					cbootmenu = optarg + n + 1;
					break;
				}
#endif
			}
			errx(1, "Bad argument to the -s flag.");
			break;
		case 'b':	/* BIOS geometry */
			b_flag = 1;
			if (sscanf(optarg, "%d/%d/%d%n", &b_cyl, &b_head,
			    &b_sec, &n) != 3 || optarg[n] != 0)
				errx(1, "Bad argument to the -b flag.");
			if (b_cyl > MAXCYL)
				b_cyl = MAXCYL;
			break;
		case 'c':	/* file/directory containing boot code */
			if (strchr(optarg, '/') != NULL &&
			    stat(optarg, &sb) == 0 &&
			    (sb.st_mode & S_IFMT) == S_IFDIR) {
				boot_dir = optarg;
				break;
			}
			bootsize = read_boot(optarg, bootcode,
						sizeof bootcode, 1);
			i_flag = 1;
			break;
		case 'r':	/* read data from disk_file (not raw disk) */
			rfd = &wfd;
			/* FALLTHROUGH */
		case 'w':	/* write data to disk_file */
			disk_file = optarg;
			break;
		case 't':
			if (setdisktab(optarg) == -1)
				errx(EXIT_FAILURE, "bad disktab");
			break;
		case 'T':
			disk_type = optarg;
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (disk_type != NULL && getdiskbyname(disk_type) == NULL)
		errx(EXIT_FAILURE, "bad disktype");

	if (sh_flag && (a_flag || i_flag || u_flag || f_flag || s_flag))
		usage();

	if (B_flag && f_flag) {
		warnx("Bootselector may only be configured interactively");
		usage();
	}

	if (f_flag && u_flag && !s_flag) {
		warnx("Partition data not specified");
		usage();
	}

	if (s_flag && partition == -1) {
		warnx("-s flag requires a partition selected.");
		usage();
	}

	if (argc > 0)
		disk = argv[0];

	if (open_disk(B_flag || a_flag || i_flag || u_flag) < 0)
		exit(1);

	if (read_s0(0, &mboot))
		/* must have been a blank disk */
		init_sector0(1);

#if defined(__i386__) || defined(__x86_64__)
	get_geometry();
#else
	intuit_translated_geometry();
#endif
	get_extended_ptn();

#ifdef BOOTSEL
	default_ptn = get_default_boot();
#endif

	if (E_flag && !u_flag && partition >= ext.num_ptn)
		errx(1, "Extended partition %d is not defined.", partition);

	if (u_flag && (!f_flag || b_flag))
		get_params_to_use();

	/* Do the update stuff! */
	if (u_flag) {
		if (s_flag)
			change_part(E_flag, partition, csysid, cstart, csize,
				cbootmenu);
		else {
			int part = partition, chg_ext = E_flag, prompt = 1;
			do {
				if (prompt) {
					printf("\n");
					print_s0(partition);
				}
				if (partition == -1)
					part = ptn_id(
				    "Which partition do you want to change?",
							&chg_ext);
				if (part < 0)
					break;
				prompt = change_part(chg_ext, part, 0, 0, 0, 0);
			} while (partition == -1);
		}
	} else
		if (!i_flag && !B_flag) {
			print_params();
			print_s0(partition);
		}

	if (a_flag && !E_flag)
		change_active(partition);

#ifdef BOOTSEL
	if (B_flag || u_flag || i_flag)
		/* Ensure the mbr code supports this configuration */
		install_bootsel(0);
	if (B_flag)
		default_ptn = configure_bootsel(default_ptn);
	set_default_boot(default_ptn);
#else
	if (i_flag)
		init_sector0(0);
#endif

	if (u_flag || a_flag || i_flag || B_flag) {
		if (!f_flag) {
			printf("\nWe haven't written the MBR back to disk "
			       "yet.  This is your last chance.\n");
			if (u_flag)
				print_s0(-1);
			if (yesno("Should we write new partition table?"))
				write_mbr();
		} else
			write_mbr();
	}

	exit(0);
}

void
usage(void)
{
	int indent = 7 + (int)strlen(getprogname()) + 1;

	(void)fprintf(stderr, "usage: %s [-afiluvBS] "
		"[-b cylinders/heads/sectors] \\\n"
		"%*s[-0123 | -E num "
		"[-s id/start/size[/bootmenu]]] \\\n"
		"%*s[-t disktab] [-T disktype] \\\n"
		"%*s[-c bootcode] [-r|-w file] [device]\n"
		"\t-a change active partition\n"
		"\t-f force - not interactive\n"
		"\t-i initialise MBR code\n"
		"\t-l list partition types\n"
		"\t-u update partition data\n"
		"\t-v verbose output, -v -v more verbose still\n"
		"\t-B update bootselect options\n"
		"\t-F treat device as a regular file\n"
		"\t-S output as shell defines\n",
		getprogname(), indent, "", indent, "", indent, "");
	exit(1);
}

static daddr_t
ext_offset(int part)
{
	daddr_t offset = ext.base;

	if (part != 0)
		offset += le32toh(ext.ptn[part - 1].mbr_parts[1].mbrp_start);
	return offset;
}

void
print_s0(int which)
{
	int part;

	if (which == -1) {
		if (!sh_flag)
			printf("Partition table:\n");
		for (part = 0; part < MBR_PART_COUNT; part++) {
			if (!sh_flag)
				printf("%d: ", part);
			print_part(&mboot, part, 0);
		}
		if (!sh_flag) {
			if (ext.is_corrupt)
				printf("Extended partition table is currupt\n");
			else
				if (ext.num_ptn != 0)
					printf("Extended partition table:\n");
		}
		for (part = 0; part < ext.num_ptn; part++) {
			if (!sh_flag)
				printf("E%d: ", part);
			print_part(&ext.ptn[part], 0, ext_offset(part));
			if (!sh_flag && v_flag >= 2) {
				printf("link: ");
				print_mbr_partition(&ext.ptn[part], 1,
						ext_offset(part), ext.base, 0);
			}
		}
#ifdef BOOTSEL
		if (!sh_flag &&
		    le16toh(mboot.mbr_bootsel_magic) == MBR_BS_MAGIC) {
			int tmo;

			printf("Bootselector ");
			if (mboot.mbr_bootsel.mbrbs_flags & MBR_BS_ACTIVE) {
				printf("enabled");
				tmo = le16toh(mboot.mbr_bootsel.mbrbs_timeo);
				if (tmo == 0xffff)
					printf(", infinite timeout");
				else
					printf(", timeout %d seconds",
						    (10 * tmo + 9) / 182);
			} else
				printf("disabled");
			printf(".\n");
		}
#endif
		return;
	}

	if (E_flag) {
		if (!sh_flag)
			printf("Extended partition E%d:\n", which);
		if (which > ext.num_ptn)
			printf("Undefined\n");
		else
			print_part(&ext.ptn[which], 0, ext_offset(which));
	} else {
		if (!sh_flag)
			printf("Partition %d:\n", which);
		print_part(&mboot, which, 0);
	}
}

void
print_part(struct mbr_sector *boot, int part, daddr_t offset)
{
	struct mbr_partition *partp;
	char *e;

	if (!sh_flag) {
		print_mbr_partition(boot, part, offset, 0, 0);
		return;
	}

	partp = &boot->mbr_parts[part];
	if (boot != &mboot) {
		part = boot - ext.ptn;
		e = "E";
	} else
		e = "";

	if (partp->mbrp_type == 0) {
		printf("PART%s%dSIZE=0\n", e, part);
		return;
	}

	printf("PART%s%dID=%d\n", e, part, partp->mbrp_type);
	printf("PART%s%dSIZE=%u\n", e, part, le32toh(partp->mbrp_size));
	printf("PART%s%dSTART=%"PRIdaddr"\n", e, part,
	    offset + le32toh(partp->mbrp_start));
	printf("PART%s%dFLAG=0x%x\n", e, part, partp->mbrp_flag);
	printf("PART%s%dBCYL=%d\n", e, part,
	    MBR_PCYL(partp->mbrp_scyl, partp->mbrp_ssect));
	printf("PART%s%dBHEAD=%d\n", e, part, partp->mbrp_shd);
	printf("PART%s%dBSEC=%d\n", e, part, MBR_PSECT(partp->mbrp_ssect));
	printf("PART%s%dECYL=%d\n", e, part,
	    MBR_PCYL(partp->mbrp_ecyl, partp->mbrp_esect));
	printf("PART%s%dEHEAD=%d\n", e, part, partp->mbrp_ehd);
	printf("PART%s%dESEC=%d\n", e, part, MBR_PSECT(partp->mbrp_esect));
}

static void
pr_cyls(daddr_t sector)
{
	ulong cyl, head, sect;
	cyl = sector / dos_cylindersectors;
	sect = sector - cyl * dos_cylindersectors;
	head = sect / dos_sectors;
	sect -= head * dos_sectors;

	printf("%lu", cyl);
	if (head == 0 && sect == 0)
		return;
	printf("/%lu/%lu", head, sect + 1);
}

void
print_mbr_partition(struct mbr_sector *boot, int part,
    daddr_t offset, daddr_t exoffset, int indent)
{
	daddr_t	start;
	daddr_t	size;
	struct mbr_partition *partp = &boot->mbr_parts[part];
	struct mbr_sector eboot;
	int p;
	static int dumped = 0;

	if (partp->mbrp_type == 0 && v_flag < 2) {
		printf("<UNUSED>\n");
		return;
	}

	start = le32toh(partp->mbrp_start);
	size = le32toh(partp->mbrp_size);
	if (MBR_IS_EXTENDED(partp->mbrp_type))
		start += exoffset;
	else
		start += offset;

	printf("%s (sysid %d)\n", get_type(partp->mbrp_type), partp->mbrp_type);
#ifdef BOOTSEL
	if (le16toh(boot->mbr_bootsel_magic) == MBR_BS_MAGIC &&
	    boot->mbr_bootsel.mbrbs_nametab[part][0])
		printf("%*s    bootmenu: %s\n", indent, "",
		    boot->mbr_bootsel.mbrbs_nametab[part]);
#endif

	printf("%*s    start %"PRIdaddr", size %"PRIdaddr,
	    indent, "", start, size);
	if (size != 0) {
		printf(" (%u MB, Cyls ", SEC_TO_MB(size));
		if (v_flag == 0 && le32toh(partp->mbrp_start) == dos_sectors)
			pr_cyls(start - dos_sectors);
		else
			pr_cyls(start);
		printf("-");
		pr_cyls(start + size);
		printf(")");
	}

	switch (partp->mbrp_flag) {
	case 0:
		break;
	case MBR_PFLAG_ACTIVE:
		printf(", Active");
		break;
	default:
		printf(", flag 0x%x", partp->mbrp_flag);
		break;
	}
	printf("\n");

	if (v_flag) {
		printf("%*s        beg: cylinder %4d, head %3d, sector %2d\n",
		    indent, "",
		    MBR_PCYL(partp->mbrp_scyl, partp->mbrp_ssect),
		    partp->mbrp_shd, MBR_PSECT(partp->mbrp_ssect));
		printf("%*s        end: cylinder %4d, head %3d, sector %2d\n",
		    indent, "",
		    MBR_PCYL(partp->mbrp_ecyl, partp->mbrp_esect),
		    partp->mbrp_ehd, MBR_PSECT(partp->mbrp_esect));
	}

	if (!MBR_IS_EXTENDED(partp->mbrp_type) ||
	    (v_flag <= 2 && !ext.is_corrupt))
		return;

	/*
	 * Recursive dump extended table,
	 * This is read from the disk - so is wrong during editing.
	 * Just ensure we only show it once.
	 */
	if (dumped)
		return;

	printf("%*s    Extended partition table:\n", indent, "");
	indent += 4;
	if (read_s0(start, &eboot) == -1)
		return;
	for (p = 0; p < MBR_PART_COUNT; p++) {
		printf("%*s%d: ", indent, "", p);
		print_mbr_partition(&eboot, p, start,
				    exoffset ? exoffset : start, indent);
	}

	if (exoffset == 0)
		dumped = 1;
}

int
read_boot(const char *name, void *buf, size_t len, int err_exit)
{
	int bfd, ret;
	struct stat st;

	if (boot_path != NULL)
		free(boot_path);
	if (strchr(name, '/') == 0)
		asprintf(&boot_path, "%s/%s", boot_dir, name);
	else
		boot_path = strdup(name);
	if (boot_path == NULL)
		err(1, "Malloc failed");

	if ((bfd = open(boot_path, O_RDONLY)) < 0 || fstat(bfd, &st) == -1) {
		warn("%s", boot_path);
		goto fail;
	}

	if (st.st_size > (off_t)len) {
		warnx("%s: bootcode too large", boot_path);
		goto fail;
	}
	ret = st.st_size;
	if (ret < 0x200) {
		warnx("%s: bootcode too small", boot_path);
		goto fail;
	}
	if (read(bfd, buf, len) != ret) {
		warn("%s", boot_path);
		goto fail;
	}

	/*
	 * Do some sanity checking here
	 */
	if (le16toh(((struct mbr_sector *)buf)->mbr_magic) != MBR_MAGIC) {
		warnx("%s: invalid magic", boot_path);
		goto fail;
	}

	close(bfd);
	ret = (ret + 0x1ff) & ~0x1ff;
	return ret;

    fail:
	close(bfd);
	if (err_exit)
		exit(1);
	return 0;
}

void
init_sector0(int zappart)
{
	int i;
	int copy_size =  MBR_PART_OFFSET;

#ifdef DEFAULT_BOOTCODE
	if (bootsize == 0)
		bootsize = read_boot(DEFAULT_BOOTCODE, bootcode,
			sizeof bootcode, 1);
#endif
#ifdef BOOTSEL
	if (le16toh(mboot.mbr_bootsel_magic) == MBR_BS_MAGIC 
	    && le16toh(bootcode[0].mbr_bootsel_magic) == MBR_BS_MAGIC)
		copy_size = MBR_BS_OFFSET;
#endif

	if (bootsize != 0) {
		boot_installed = 1;
		memcpy(&mboot, bootcode, copy_size);
	}
	mboot.mbr_magic = htole16(MBR_MAGIC);
	
	if (!zappart)
		return;
	for (i = 0; i < MBR_PART_COUNT; i++)
		memset(&mboot.mbr_parts[i], 0, sizeof(mboot.mbr_parts[i]));
}

void
get_extended_ptn(void)
{
	struct mbr_partition *mp;
	struct mbr_sector *boot;
	daddr_t offset;
	struct mbr_sector *nptn;

	/* find first (there should only be one) extended partition */
	for (mp = mboot.mbr_parts; !MBR_IS_EXTENDED(mp->mbrp_type); mp++)
		if (mp >= &mboot.mbr_parts[MBR_PART_COUNT])
			return;

	/*
	 * The extended partition should be structured as a linked list
	 * (even though it appears, at first glance, to be a tree).
	 */
	ext.base = le32toh(mp->mbrp_start);
	ext.limit = ext.base + le32toh(mp->mbrp_size);
	ext.ptn_id = mp - mboot.mbr_parts;
	for (offset = 0;; offset = le32toh(boot->mbr_parts[1].mbrp_start)) {
		nptn = realloc(ext.ptn, (ext.num_ptn + 1) * sizeof *ext.ptn);
		if (nptn == NULL)
			err(1, "Malloc failed");
		ext.ptn = nptn;
		boot = ext.ptn + ext.num_ptn;
		if (read_s0(offset + ext.base, boot) == -1)
			break;
		/* expect p0 to be valid and p1 to be another extended ptn */
		if (MBR_IS_EXTENDED(boot->mbr_parts[0].mbrp_type))
			break;
		if (boot->mbr_parts[1].mbrp_type != 0 &&
		    !MBR_IS_EXTENDED(boot->mbr_parts[1].mbrp_type))
			break;
		/* p2 and p3 should be unallocated */
		if (boot->mbr_parts[2].mbrp_type != 0 ||
		    boot->mbr_parts[3].mbrp_type != 0)
			break;
		/* data ptn inside extended one */
		if (boot->mbr_parts[0].mbrp_type != 0 &&
		    offset + le32toh(boot->mbr_parts[0].mbrp_start)
		    + le32toh(boot->mbr_parts[0].mbrp_size) > ext.limit)
			break;

		ext.num_ptn++;

		if (boot->mbr_parts[1].mbrp_type == 0)
			/* end of extended partition chain */
			return;
		/* must be in sector order */
		if (offset >= le32toh(boot->mbr_parts[1].mbrp_start))
			break;
	}

	warnx("Extended partition table is corrupt\n");
	ext.is_corrupt = 1;
	ext.num_ptn = 0;
}

#if defined(__i386__) || defined(__x86_64__)

void	    
get_diskname(const char *fullname, char *diskname, size_t size)
{	       
	const char *p, *p2;
	size_t len;

	p = strrchr(fullname, '/');
	if (p == NULL)
		p = fullname;
	else
		p++;

	if (*p == 0) {
		strlcpy(diskname, fullname, size);
		return;
	}

	if (*p == 'r')
		p++;

	for (p2 = p; *p2 != 0; p2++)
		if (isdigit(*p2))
			break;
	if (*p2 == 0) {
		/* XXX invalid diskname? */
		strlcpy(diskname, fullname, size);
		return;
	}
	while (isdigit(*p2))
		p2++; 

	len = p2 - p;
	if (len > size) {
		/* XXX */
		strlcpy(diskname, fullname, size);
		return;
	}
 
	memcpy(diskname, p, len);
	diskname[len] = 0;
}

void
get_geometry(void)
{
	int mib[2], i;
	size_t len;
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
		 * XXX listing possible matches is better. This is ok for
		 * now because the user has a chance to change it later.
		 * Also, if all the disks have the same parameters then we can
		 * just use them, we don't need to know which disk is which.
		 */
		if (nip->ni_nmatches != 0) {
			bip = &dl->dl_biosdisks[nip->ni_biosmatches[0]];
			dos_cylinders = bip->bi_cyl;
			dos_heads = bip->bi_head;
			dos_sectors = bip->bi_sec;
			if (bip->bi_lbasecs)
				dos_disksectors = bip->bi_lbasecs;
			return;
		}
	}
	/* Allright, allright, make a stupid guess.. */
	intuit_translated_geometry();
}
#endif

#ifdef BOOTSEL
daddr_t
get_default_boot(void)
{
	uint id;
	int p;

	if (le16toh(mboot.mbr_bootsel_magic) != MBR_BS_MAGIC)
		/* default to first active partition */
		return DEFAULT_ACTIVE;

	if (mboot.mbr_bootsel.mbrbs_defkey == SCAN_ENTER)
		return DEFAULT_ACTIVE;
		
	id = mboot.mbr_bootsel.mbrbs_defkey;

	/* 1+ => allocated partition id, F1+ => disk 0+ */
	if (id >= SCAN_F1)
		return id - SCAN_F1;
	id -= SCAN_1;

	for (p = 0; p < MBR_PART_COUNT; p++) {
		if (mboot.mbr_parts[p].mbrp_type == 0)
			continue;
		if (mboot.mbr_bootsel.mbrbs_nametab[p][0] == 0)
			continue;
		if (id-- == 0)
			return le32toh(mboot.mbr_parts[p].mbrp_start);
	}

	for (p = 0; p < ext.num_ptn; p++) {
		if (ext.ptn[p].mbr_parts[0].mbrp_type == 0)
			continue;
		if (ext.ptn[p].mbr_bootsel.mbrbs_nametab[0][0] == 0)
			continue;
		if (id-- == 0)
			return ext_offset(p)
				+ le32toh(ext.ptn[p].mbr_parts[0].mbrp_start);
	}

	return DEFAULT_ACTIVE;
}

void
set_default_boot(daddr_t default_ptn)
{
	int p;
	int key = SCAN_1;

	if (le16toh(mboot.mbr_bootsel_magic) != MBR_BS_MAGIC)
		/* sanity */
		return;

	if (default_ptn == DEFAULT_ACTIVE) {
		mboot.mbr_bootsel.mbrbs_defkey = SCAN_ENTER;
		return;
	}

	for (p = 0; p < MBR_PART_COUNT; p++) {
		if (mboot.mbr_parts[p].mbrp_type == 0)
			continue;
		if (mboot.mbr_bootsel.mbrbs_nametab[p][0] == 0)
			continue;
		if (le32toh(mboot.mbr_parts[p].mbrp_start) == default_ptn) {
			mboot.mbr_bootsel.mbrbs_defkey = key;
			return;
		}
		key++;
	}

	if (mboot.mbr_bootsel.mbrbs_flags & MBR_BS_EXTLBA) {
		for (p = 0; p < ext.num_ptn; p++) {
			if (ext.ptn[p].mbr_parts[0].mbrp_type == 0)
				continue;
			if (ext.ptn[p].mbr_bootsel.mbrbs_nametab[0][0] == 0)
				continue;
			if (le32toh(ext.ptn[p].mbr_parts[0].mbrp_start) +
			    ext_offset(p) == default_ptn) {
				mboot.mbr_bootsel.mbrbs_defkey = key;
				return;
			}
			key++;
		}
	}

	if (default_ptn < 8) {
		key = SCAN_F1;
		mboot.mbr_bootsel.mbrbs_defkey = key + default_ptn;
		return;
	}

	/* Default to first active partition */
	mboot.mbr_bootsel.mbrbs_defkey = SCAN_ENTER;
}

void
install_bootsel(int needed)
{
	struct mbr_bootsel *mbs = &mboot.mbr_bootsel;
	int p;
	int ext13 = 0;
	char *code;

	needed |= MBR_BS_NEWMBR;	/* need new bootsel code */

	/* Work out which boot code we need for this configuration */
	for (p = 0; p < MBR_PART_COUNT; p++) {
		if (mboot.mbr_parts[p].mbrp_type == 0)
			continue;
		if (le16toh(mboot.mbr_bootsel_magic) != MBR_BS_MAGIC)
			break;
		if (mbs->mbrbs_nametab[p][0] == 0)
			continue;
		needed |= MBR_BS_ACTIVE;
		if (le32toh(mboot.mbr_parts[p].mbrp_start) >= dos_totalsectors)
			ext13 = MBR_BS_EXTINT13;
	}

	for (p = 0; p < ext.num_ptn; p++) {
		if (le16toh(ext.ptn[p].mbr_bootsel_magic) != MBR_BS_MAGIC)
			continue;
		if (ext.ptn[p].mbr_parts[0].mbrp_type == 0)
			continue;
		if (ext.ptn[p].mbr_bootsel.mbrbs_nametab[p][0] == 0)
			continue;
		needed |= MBR_BS_EXTLBA | MBR_BS_ACTIVE;
	}

	if (B_flag)
		needed |= MBR_BS_ACTIVE;

	/* Is the installed code good enough ? */
	if (!i_flag && (needed == 0 ||
	    (le16toh(mboot.mbr_bootsel_magic) == MBR_BS_MAGIC
	    && (mbs->mbrbs_flags & needed) == needed))) {
		/* yes - just set flags */
		mbs->mbrbs_flags |= ext13;
		return;
	}

	/* ok - we need to replace the bootcode */

	if (f_flag && !(i_flag || B_flag)) {
		warnx("Installed bootfile doesn't support required options.");
		return;
	}

	if (!f_flag && bootsize == 0)
		/* Output an explanation for the 'update bootcode' prompt. */
		printf("\n%s\n",
		    "Installed bootfile doesn't support required options.");

	/* Were we told a specific file ? (which we have already read) */
	/* If so check that it supports what we need. */
	if (bootsize != 0 && needed != 0
	    && (le16toh(bootcode[0].mbr_bootsel_magic) != MBR_BS_MAGIC
	    || ((bootcode[0].mbr_bootsel.mbrbs_flags & needed) != needed))) {
		/* No it doesn't... */
		if (f_flag)
			warnx("Bootfile %s doesn't support "
				    "required bootsel options", boot_path );
			/* But install it anyway */
		else
			if (yesno("Bootfile %s doesn't support the required "
			    "options,\ninstall default bootfile instead?",
			    boot_path))
				bootsize = 0;
	}

	if (bootsize == 0) {
		/* Get name of bootfile that supports the required facilities */
		code = DEFAULT_BOOTCODE;
		if (needed & MBR_BS_ACTIVE)
			code = DEFAULT_BOOTSELCODE;
#ifdef DEFAULT_BOOTEXTCODE
		if (needed & MBR_BS_EXTLBA)
			code = DEFAULT_BOOTEXTCODE;
#endif

		bootsize = read_boot(code, bootcode, sizeof bootcode, 0);
		if (bootsize == 0)
			/* The old bootcode is better than no bootcode at all */
			return;
		if ((bootcode[0].mbr_bootsel.mbrbs_flags & needed) != needed)
			warnx("Default bootfile %s doesn't support required "
				"options.  Got flags 0x%x, wanted 0x%x\n",
				boot_path, bootcode[0].mbr_bootsel.mbrbs_flags,
				needed);
	}

	if (!f_flag && !yesno("Update the bootcode from %s?", boot_path))
		return;

	init_sector0(0);

	if (le16toh(mboot.mbr_bootsel_magic) == MBR_BS_MAGIC)
		mbs->mbrbs_flags = bootcode[0].mbr_bootsel.mbrbs_flags | ext13;
}

daddr_t
configure_bootsel(daddr_t default_ptn)
{
	struct mbr_bootsel *mbs = &mboot.mbr_bootsel;
	int i, item, opt;
	int tmo;
	daddr_t *off;
	int num_bios_disks;

	if (dl != NULL) {
		num_bios_disks = dl->dl_nbiosdisks;
		if (num_bios_disks > 8)
			num_bios_disks = 8;
	} else
		num_bios_disks = 8;

	printf("\nBoot selector configuration:\n");

	/* The timeout value is in ticks, ~18.2 Hz. Avoid using floats.
	 * Ticks are nearly 64k/3600 - so our long timers are sligtly out!
	 * Newer bootcode always waits for 1 tick, so treats 0xffff
	 * as wait forever.
	 */
	tmo = le16toh(mbs->mbrbs_timeo);
	tmo = tmo == 0xffff ? -1 : (10 * tmo + 9) / 182;
	tmo = decimal("Timeout value (0 to 3600 seconds, -1 => never)",
			tmo, 0, -1, 3600);
	mbs->mbrbs_timeo = htole16(tmo == -1 ? 0xffff : (tmo * 182) / 10);

	off = calloc(1 + MBR_PART_COUNT + ext.num_ptn + num_bios_disks, sizeof *off);
	if (off == NULL)
		err(1, "Malloc failed");

	printf("Select the default boot option. Options are:\n\n");
	item = 0;
	opt = 0;
	off[opt] = DEFAULT_ACTIVE;
	printf("%d: The first active partition\n", opt);
	for (i = 0; i < MBR_PART_COUNT; i++) {
		if (mboot.mbr_parts[i].mbrp_type == 0)
			continue;
		if (mbs->mbrbs_nametab[i][0] == 0)
			continue;
		printf("%d: %s\n", ++opt, &mbs->mbrbs_nametab[i][0]);
		off[opt] = le32toh(mboot.mbr_parts[i].mbrp_start);
		if (off[opt] == default_ptn)
			item = opt;
	}
	if (mbs->mbrbs_flags & MBR_BS_EXTLBA) {
		for (i = 0; i < ext.num_ptn; i++) {
			if (ext.ptn[i].mbr_parts[0].mbrp_type == 0)
				continue;
			if (ext.ptn[i].mbr_bootsel.mbrbs_nametab[0][0] == 0)
				continue;
			printf("%d: %s\n",
			    ++opt, ext.ptn[i].mbr_bootsel.mbrbs_nametab[0]);
			off[opt] = ext_offset(i) +
			    le32toh(ext.ptn[i].mbr_parts[0].mbrp_start);
			if (off[opt] == default_ptn)
				item = opt;
		}
	}
	for (i = 0; i < num_bios_disks; i++) {
		printf("%d: Harddisk %d\n", ++opt, i);
		off[opt] = i;
		if (i == default_ptn)
			item = opt;
	}

	item = decimal("Default boot option", item, 0, 0, opt);

	default_ptn = off[item];
	free(off);
	return default_ptn;
}
#endif /* BOOTSEL */


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
 * This routine is only used for non-x86 systems or when we fail to
 * get the BIOS geometry from the kernel.
 */
void
intuit_translated_geometry(void)
{
	int xcylinders = -1, xheads = -1, xsectors = -1, i, j;
	uint c1, h1, s1, c2, h2, s2;
	ulong a1, a2;
	uint64_t num, denom;

	/*
	 * The physical parameters may be invalid as bios geometry.
	 * If we cannot determine the actual bios geometry, we are
	 * better off picking a likely 'faked' geometry than leaving
	 * the invalid physical one.
	 */

	if (dos_cylinders > MAXCYL || dos_heads > MAXHEAD ||
	    dos_sectors > MAXSECTOR) {
		h1 = MAXHEAD - 1;
		c1 = MAXCYL - 1;
#if defined(__i386__) || defined(__x86_64__)
		if (dl != NULL) {
			/* BIOS may use 256 heads or 1024 cylinders */
			for (i = 0; i < dl->dl_nbiosdisks; i++) {
				if (h1 < dl->dl_biosdisks[i].bi_head)
					h1 = dl->dl_biosdisks[i].bi_head;
				if (c1 < dl->dl_biosdisks[i].bi_cyl)
					c1 = dl->dl_biosdisks[i].bi_cyl;
			}
		}
#endif
		dos_sectors = MAXSECTOR;
		dos_heads = h1;
		dos_cylinders = disklabel.d_secperunit / (MAXSECTOR * h1);
		if (dos_cylinders > c1)
			dos_cylinders = c1;
	}

	/* Try to deduce the number of heads from two different mappings. */
	for (i = 0; i < MBR_PART_COUNT * 2 - 1; i++) {
		if (get_mapping(i, &c1, &h1, &s1, &a1) < 0)
			continue;
		for (j = i + 1; j < MBR_PART_COUNT * 2; j++) {
			if (get_mapping(j, &c2, &h2, &s2, &a2) < 0)
				continue;
			a1 -= s1;
			a2 -= s2;
			num = (uint64_t)h1 * a2 - (uint64_t)h2 * a1;
			denom = (uint64_t)c2 * a1 - (uint64_t)c1 * a2;
			if (denom != 0 && num % denom == 0) {
				xheads = num / denom;
				xsectors = a1 / (c1 * xheads + h1);
				break;
			}
		}
		if (xheads != -1)	
			break;
	}

	if (xheads == -1)
		return;

	/* Estimate the number of cylinders. */
	xcylinders = disklabel.d_secperunit / xheads / xsectors;
	if (disklabel.d_secperunit > xcylinders * xheads * xsectors)
		xcylinders++;

	/*
	 * Now verify consistency with each of the partition table entries.
	 * Be willing to shove cylinders up a little bit to make things work,
	 * but translation mismatches are fatal.
	 */
	for (i = 0; i < MBR_PART_COUNT * 2; i++) {
		if (get_mapping(i, &c1, &h1, &s1, &a1) < 0)
			continue;
		if (c1 >= MAXCYL - 2)
			continue;
		if (xsectors * (c1 * xheads + h1) + s1 != a1)
			return;
	}


	/* Everything checks out.
	 * Reset the geometry to use for further calculations.
	 * But cylinders cannot be > 1024.
	 */
	if (xcylinders > MAXCYL)
		dos_cylinders = MAXCYL;
	else
		dos_cylinders = xcylinders;
	dos_heads = xheads;
	dos_sectors = xsectors;
}

/*
 * For the purposes of intuit_translated_geometry(), treat the partition
 * table as a list of eight mapping between (cylinder, head, sector)
 * triplets and absolute sectors.  Get the relevant geometry triplet and
 * absolute sectors for a given entry, or return -1 if it isn't present.
 * Note: for simplicity, the returned sector is 0-based.
 */
int
get_mapping(int i, uint *cylinder, uint *head, uint *sector,
    unsigned long *absolute)
{
	struct mbr_partition *part = &mboot.mbr_parts[i / 2];

	if (part->mbrp_type == 0)
		return -1;
	if (i % 2 == 0) {
		*cylinder = MBR_PCYL(part->mbrp_scyl, part->mbrp_ssect);
		*head = part->mbrp_shd;
		*sector = MBR_PSECT(part->mbrp_ssect) - 1;
		*absolute = le32toh(part->mbrp_start);
	} else {
		*cylinder = MBR_PCYL(part->mbrp_ecyl, part->mbrp_esect);
		*head = part->mbrp_ehd;
		*sector = MBR_PSECT(part->mbrp_esect) - 1;
		*absolute = le32toh(part->mbrp_start)
		    + le32toh(part->mbrp_size) - 1;
	}
	/* Sanity check the data against max values */
	if ((((*cylinder * MAXHEAD) + *head) * MAXSECTOR + *sector) < *absolute)
		/* cannot be a CHS mapping */
		return -1;
	return 0;
}

static void
delete_ptn(int part)
{
	if (part == ext.ptn_id) {
		/* forget all about the extended partition */
		free(ext.ptn);
		memset(&ext, 0, sizeof ext);
	}

	mboot.mbr_parts[part].mbrp_type = 0;
}

static void
delete_ext_ptn(int part)
{

	if (part == 0) {
		ext.ptn[0].mbr_parts[0].mbrp_type = 0;
		return;
	}
	ext.ptn[part - 1].mbr_parts[1] = ext.ptn[part].mbr_parts[1];
	memmove(&ext.ptn[part], &ext.ptn[part + 1],
		(ext.num_ptn - part - 1) * sizeof ext.ptn[0]);
	ext.num_ptn--;
}

static int
add_ext_ptn(daddr_t start, daddr_t size)
{
	int part;
	struct mbr_partition *partp;
	struct mbr_sector *nptn;

	nptn = realloc(ext.ptn, (ext.num_ptn + 1) * sizeof *ext.ptn);
	if (!nptn)
		err(1, "realloc");
	ext.ptn = nptn;
	for (part = 0; part < ext.num_ptn; part++)
		if (ext_offset(part) > start)
			break;
	/* insert before 'part' - make space... */
	memmove(&ext.ptn[part + 1], &ext.ptn[part],
		(ext.num_ptn - part) * sizeof ext.ptn[0]);
	memset(&ext.ptn[part], 0, sizeof ext.ptn[0]);
	ext.ptn[part].mbr_magic = htole16(MBR_MAGIC);
	/* we will be 'part' */
	if (part == 0) {
		/* link us to 'next' */
		partp = &ext.ptn[0].mbr_parts[1];
		/* offset will be fixed by caller */
		partp->mbrp_size = htole32(
		    le32toh(ext.ptn[1].mbr_parts[0].mbrp_start) +
		    le32toh(ext.ptn[1].mbr_parts[0].mbrp_size));
	} else {
		/* link us to prev's next */
		partp = &ext.ptn[part - 1].mbr_parts[1];
		ext.ptn[part].mbr_parts[1] = *partp;
		/* and prev onto us */
		partp->mbrp_start = htole32(start - dos_sectors - ext.base);
		partp->mbrp_size = htole32(size + dos_sectors);
	}
	partp->mbrp_type = 5;	/* as used by win98 */
	partp->mbrp_flag = 0;
	/* wallop in some CHS values - win98 doesn't saturate them */
	dos(le32toh(partp->mbrp_start),
	    &partp->mbrp_scyl, &partp->mbrp_shd, &partp->mbrp_ssect);
	dos(le32toh(partp->mbrp_start) + le32toh(partp->mbrp_size) - 1,
	    &partp->mbrp_ecyl, &partp->mbrp_ehd, &partp->mbrp_esect);
	ext.num_ptn++;

	return part;
}

static const char *
check_overlap(int part, int sysid, daddr_t start, daddr_t size, int fix)
{
	int p;
	uint p_s, p_e;

	if (sysid != 0) {
		if (start < dos_sectors)
			return "Track zero is reserved for the BIOS";
		if (start + size > dos_disksectors) 
			return "Partition exceeds size of disk";
		for (p = 0; p < MBR_PART_COUNT; p++) {
			if (p == part || mboot.mbr_parts[p].mbrp_type == 0)
				continue;
			p_s = le32toh(mboot.mbr_parts[p].mbrp_start);
			p_e = p_s + le32toh(mboot.mbr_parts[p].mbrp_size);
			if (start + size <= p_s || start >= p_e)
				continue;
			if (f_flag) {
				if (fix)
					delete_ptn(p);
				return 0;
			}
			return "Overlaps another partition";
		}
	}

	/* Are we trying to create an extended partition */
	if (!MBR_IS_EXTENDED(mboot.mbr_parts[part].mbrp_type)) {
		/* this wasn't the extended partition */
		if (!MBR_IS_EXTENDED(sysid))
			return 0;
		/* making an extended partition */
		if (ext.base != 0) {
			if (!f_flag)
				return "There cannot be 2 extended partitions";
			if (fix)
				delete_ptn(ext.ptn_id);
		}
		if (fix) {
			/* allocate a new extended partition */
			ext.ptn = calloc(1, sizeof ext.ptn[0]);
			if (ext.ptn == NULL)
				err(1, "Malloc failed");
			ext.ptn[0].mbr_magic = htole16(MBR_MAGIC);
			ext.ptn_id = part;
			ext.base = start;
			ext.limit = start + size;
			ext.num_ptn = 1;
		}
		return 0;
	}

	/* Check we haven't cut space allocated to an extended ptn */
	
	if (!MBR_IS_EXTENDED(sysid)) {
		/* no longer an extended partition */
		if (fix) {
			/* Kill all memory of the extended partitions */
			delete_ptn(part);
			return 0;
		}
		if (ext.num_ptn == 0 ||
		    (ext.num_ptn == 1 && ext.ptn[0].mbr_parts[0].mbrp_type == 0))
			/* nothing in extended partition */
			return 0;
		if (f_flag)
			return 0;
		if (yesno("Do you really want to delete all the extended partitions?"))
			return 0;
		return "Extended partition busy";
	}

	if (le32toh(mboot.mbr_parts[part].mbrp_start) != ext.base)
		/* maybe impossible, but an extra sanity check */
		return 0;

	for (p = ext.num_ptn; --p >= 0;) {
		if (ext.ptn[p].mbr_parts[0].mbrp_type == 0)
			continue;
		p_s = ext_offset(p);
		p_e = p_s + le32toh(ext.ptn[p].mbr_parts[0].mbrp_start)
			  + le32toh(ext.ptn[p].mbr_parts[0].mbrp_size);
		if (p_s >= start && p_e <= start + size)
			continue;
		if (!f_flag)
			return "Extended partition outside main partition";
		if (fix)
			delete_ext_ptn(p);
	}

	if (fix && start != ext.base) {
		/* The internal offsets need to be fixed up */
		for (p = 0; p < ext.num_ptn - 1; p++)
			ext.ptn[p].mbr_parts[1].mbrp_start = htole32(
			    le32toh(ext.ptn[p].mbr_parts[1].mbrp_start)
				    + ext.base - start);
		/* and maybe an empty partition at the start */
		if (ext.ptn[0].mbr_parts[0].mbrp_type == 0) {
			if (le32toh(ext.ptn[0].mbr_parts[1].mbrp_start) == 0) {
				/* don't need the empty slot */
				memmove(&ext.ptn[0], &ext.ptn[1],
					(ext.num_ptn - 1) * sizeof ext.ptn[0]);
				ext.num_ptn--;
			}
		} else {
			/* must create an empty slot */
			add_ext_ptn(start, dos_sectors);
			ext.ptn[0].mbr_parts[1].mbrp_start = htole32(ext.base
								- start);
		}
	}
	if (fix) {
		ext.base = start;
		ext.limit = start + size;
	}
	return 0;
}

static const char *
check_ext_overlap(int part, int sysid, daddr_t start, daddr_t size, int fix)
{
	int p;
	uint p_s, p_e;

	if (sysid == 0)
		return 0;

	if (MBR_IS_EXTENDED(sysid))
		return "Nested extended partitions are not allowed";

	/* allow one track at start for extended partition header */
	start -= dos_sectors;
	size += dos_sectors;
	if (start < ext.base || start + size > ext.limit)
		return "Outside bounds of extended partition";

	if (f_flag && !fix)
		return 0;

	for (p = ext.num_ptn; --p >= 0;) {
		if (p == part || ext.ptn[p].mbr_parts[0].mbrp_type == 0)
			continue;
		p_s = ext_offset(p);
		p_e = p_s + le32toh(ext.ptn[p].mbr_parts[0].mbrp_start)
			+ le32toh(ext.ptn[p].mbr_parts[0].mbrp_size);
		if (p == 0)
			p_s += le32toh(ext.ptn[p].mbr_parts[0].mbrp_start)
							- dos_sectors;
		if (start < p_e && start + size > p_s) {
			if (!f_flag)
				return "Overlaps another extended partition";
			if (fix) {
				if (part == -1)
					delete_ext_ptn(p);
				else
					/* must not change numbering yet */
					ext.ptn[p].mbr_parts[0].mbrp_type = 0;
			}
		}
	}
	return 0;
}

int
change_part(int extended, int part, int sysid, daddr_t start, daddr_t size,
	char *bootmenu)
{
	struct mbr_partition *partp;
	struct mbr_sector *boot;
	daddr_t offset;
	char *e;
	int upart = part;
	int p;
	int fl;
	daddr_t n_s, n_e;
	const char *errtext;
#ifdef BOOTSEL
	char tmp_bootmenu[MBR_PART_COUNT * (MBR_BS_PARTNAMESIZE + 1)];
	int bootmenu_len = (extended ? MBR_PART_COUNT : 1) * (MBR_BS_PARTNAMESIZE + 1);
#endif

	if (extended) {
		if (part != -1 && part < ext.num_ptn) {
			boot = &ext.ptn[part];
			partp = &boot->mbr_parts[0];
			offset = ext_offset(part);
		} else {
			part = -1;
			boot = 0;
			partp = 0;
			offset = 0;
		}
		upart = 0;
		e = "E";
	} else {
		boot = &mboot;
		partp = &boot->mbr_parts[part];
		offset = 0;
		e = "";
	}

	if (!f_flag && part != -1) {
		printf("The data for partition %s%d is:\n", e, part);
		print_part(boot, upart, offset);
	}

#ifdef BOOTSEL
	if (bootmenu != NULL)
		strlcpy(tmp_bootmenu, bootmenu, bootmenu_len);
	else
		if (boot != NULL &&
		    le16toh(boot->mbr_bootsel_magic) == MBR_BS_MAGIC)
			strlcpy(tmp_bootmenu,
				boot->mbr_bootsel.mbrbs_nametab[upart],
				bootmenu_len);
		else
			tmp_bootmenu[0] = 0;
#endif

	if (!s_flag && partp != NULL) {
		/* values not specified, default to current ones */
		sysid = partp->mbrp_type;
		start = offset + le32toh(partp->mbrp_start);
		size = le32toh(partp->mbrp_size);
	}

	/* creating a new partition, default to free space */
	if (!s_flag && sysid == 0 && extended) {
		/* non-extended partition */
		start = ext.base;
		for (p = 0; p < ext.num_ptn; p++) {
			if (ext.ptn[p].mbr_parts[0].mbrp_type == 0)
				continue;
			n_s = ext_offset(p);
			if (n_s > start + dos_sectors)
				break;
			start = ext_offset(p)
				+ le32toh(ext.ptn[p].mbr_parts[0].mbrp_start)
				+ le32toh(ext.ptn[p].mbr_parts[0].mbrp_size);
		}
		if (ext.limit - start <= dos_sectors) {
			printf("No space in extended partition\n");
			return 0;
		}
		start += dos_sectors;
	}

	if (!s_flag && sysid == 0 && !extended) {
		/* same for non-extended partition */
		/* first see if old start is free */
		if (start < dos_sectors)
			start = 0;
		for (p = 0; start != 0 && p < MBR_PART_COUNT; p++) {
			if (mboot.mbr_parts[p].mbrp_type == 0)
				continue;
			n_s = le32toh(mboot.mbr_parts[p].mbrp_start);
			if (start >= n_s &&
			    start < n_s + le32toh(mboot.mbr_parts[p].mbrp_size))
				start = 0;
		}
		if (start == 0) {
			/* Look for first gap */
			start = dos_sectors;
			for (p = 0; p < MBR_PART_COUNT; p++) {
				if (mboot.mbr_parts[p].mbrp_type == 0)
					continue;
				n_s = le32toh(mboot.mbr_parts[p].mbrp_start);
				n_e = n_s + le32toh(mboot.mbr_parts[p].mbrp_size);
				if (start >= n_s && start < n_e) {
					start = n_e;
					p = -1;
				}
			}
			if (start >= dos_disksectors) {
				printf("No free space\n");
				return 0;
			}
		}
	}

	if (!f_flag) {
		/* request new values from user */
		if (sysid == 0)
			sysid = 169;
		sysid = decimal("sysid", sysid, 0, 0, 255);
		if (sysid == 0 && !v_flag) {
			start = 0;
			size = 0;
#ifdef BOOTSEL
			tmp_bootmenu[0] = 0;
#endif
		} else {
			daddr_t old = start;
			daddr_t lim = extended ? ext.limit : dos_disksectors;
			start = decimal("start", start,
				DEC_SEC | DEC_RND_0 | (extended ? DEC_RND : 0),
				extended ? ext.base : 0, lim);
			/* Adjust 'size' so that end doesn't move when 'start'
			 * is only changed slightly.
			 */
			if (size > start - old)
				size -= start - old;
			else
				size = 0;
			/* Find end of available space from this start point */
			if (extended) {
				for (p = 0; p < ext.num_ptn; p++) {
					if (p == part)
						continue;
					if (ext.ptn[p].mbr_parts[0].mbrp_type == 0)
						continue;
					n_s = ext_offset(p);
					if (n_s > start && n_s < lim)
						lim = n_s;
					if (start >= n_s && start < n_s
				+ le32toh(ext.ptn[p].mbr_parts[0].mbrp_start)
				+ le32toh(ext.ptn[p].mbr_parts[0].mbrp_size)) {
						lim = start;
						break;
					}
				}
			} else {
				for (p = 0; p < MBR_PART_COUNT; p++) {
					if (p == part)
						continue;
					if (mboot.mbr_parts[p].mbrp_type == 0)
						continue;
					n_s = le32toh(mboot.mbr_parts[p].mbrp_start);
					if (n_s > start && n_s < lim)
						lim = n_s;
					if (start >= n_s && start < n_s
				    + le32toh(mboot.mbr_parts[p].mbrp_size)) {
						lim = start;
						break;
					}
				}
			}
			lim -= start;
			if (lim == 0) {
				printf("Start sector already allocated\n");
				return 0;
			}
			if (size == 0 || size > lim)
				size = lim;
			fl = DEC_SEC;
			if (start % dos_cylindersectors == dos_sectors)
				fl |= DEC_RND_DOWN;
			if (start == 2 * dos_sectors)
				fl |= DEC_RND_DOWN | DEC_RND_DOWN_2;
			size = decimal("size", size, fl, 0, lim);
#ifdef BOOTSEL
#ifndef DEFAULT_BOOTEXTCODE
			if (!extended)
#endif
				string("bootmenu", bootmenu_len, tmp_bootmenu);
#endif
		}
	}

	/*
	 * Before we write these away, we must verify that nothing
	 * untoward has been requested.
	 */

	if (extended)
		errtext = check_ext_overlap(part, sysid, start, size, 0);
	else
		errtext = check_overlap(part, sysid, start, size, 0);
	if (errtext != NULL) {
		if (f_flag)
			errx(2, "%s\n", errtext);
		printf("%s\n", errtext);
		return 0;
	}

	/*
	 * Before proceeding, delete any overlapped partitions.
	 * This can only happen if '-f' was supplied on the command line.
	 * Just hope the caller knows what they are doing.
	 * This also fixes the base of each extended partition if the
	 * partition itself has moved.
	 */

	if (extended)
		errtext = check_ext_overlap(part, sysid, start, size, 1);
	else
		errtext = check_overlap(part, sysid, start, size, 1);

	if (errtext)
		errx(1, "%s\n", errtext);

	if (sysid == 0) {
		/* delete this partition - save info though */
		if (partp == NULL)
			/* must have been trying to create an extended ptn */
			return 0;
		if (start == 0 && size == 0)
			memset(partp, 0, sizeof *partp);
#ifdef BOOTSEL
		if (le16toh(boot->mbr_bootsel_magic) == MBR_BS_MAGIC)
			memset(boot->mbr_bootsel.mbrbs_nametab[upart], 0,
				sizeof boot->mbr_bootsel.mbrbs_nametab[0]);
#endif
		if (extended)
			delete_ext_ptn(part);
		else
			delete_ptn(part);
		return 1;
	}


	if (extended) {
		if (part != -1)
			delete_ext_ptn(part);
		if (start == ext.base + dos_sectors)
			/* First one must have been free */
			part = 0;
		else
			part = add_ext_ptn(start, size);

		/* These must be re-calculated because of the realloc */
		boot = &ext.ptn[part];
		partp = &boot->mbr_parts[0];
		offset = ext_offset(part);
	}

	partp->mbrp_type = sysid;
	partp->mbrp_start = htole32( start - offset);
	partp->mbrp_size = htole32( size);
	dos(start, &partp->mbrp_scyl, &partp->mbrp_shd, &partp->mbrp_ssect);
	dos(start + size - 1,
		    &partp->mbrp_ecyl, &partp->mbrp_ehd, &partp->mbrp_esect);
#ifdef BOOTSEL
	if (extended) {
		boot->mbr_bootsel_magic = htole16(MBR_BS_MAGIC);
		strncpy(boot->mbr_bootsel.mbrbs_nametab[upart], tmp_bootmenu,
			bootmenu_len);
	} else {
		/* We need to bootselect code installed in order to have
		 * somewhere to safely write the menu tag.
		 */
		if (le16toh(boot->mbr_bootsel_magic) != MBR_BS_MAGIC) {
			if (yesno("The bootselect code is not installed, "
					    "do you want to install it now?"))
				install_bootsel(MBR_BS_ACTIVE);
		}
		if (le16toh(boot->mbr_bootsel_magic) == MBR_BS_MAGIC) {
			strncpy(boot->mbr_bootsel.mbrbs_nametab[upart],
				tmp_bootmenu, bootmenu_len);
		}
	}
#endif

	if (v_flag && !f_flag && yesno("Explicitly specify beg/end address?")) {
		/* this really isn't a good idea.... */
		int tsector, tcylinder, thead;

		tcylinder = MBR_PCYL(partp->mbrp_scyl, partp->mbrp_ssect);
		thead = partp->mbrp_shd;
		tsector = MBR_PSECT(partp->mbrp_ssect);
		tcylinder = decimal("beginning cylinder",
				tcylinder, 0, 0, dos_cylinders - 1);
		thead = decimal("beginning head",
				thead, 0, 0, dos_heads - 1);
		tsector = decimal("beginning sector",
				tsector, 0, 1, dos_sectors);
		partp->mbrp_scyl = DOSCYL(tcylinder);
		partp->mbrp_shd = thead;
		partp->mbrp_ssect = DOSSECT(tsector, tcylinder);

		tcylinder = MBR_PCYL(partp->mbrp_ecyl, partp->mbrp_esect);
		thead = partp->mbrp_ehd;
		tsector = MBR_PSECT(partp->mbrp_esect);
		tcylinder = decimal("ending cylinder",
				tcylinder, 0, 0, dos_cylinders - 1);
		thead = decimal("ending head",
				thead, 0, 0, dos_heads - 1);
		tsector = decimal("ending sector",
				tsector, 0, 1, dos_sectors);
		partp->mbrp_ecyl = DOSCYL(tcylinder);
		partp->mbrp_ehd = thead;
		partp->mbrp_esect = DOSSECT(tsector, tcylinder);
	}

	/* If we had to mark an extended partition as deleted because
	 * another request would have overlapped it, now is the time
	 * to do the actual delete.
	 */
	if (extended && f_flag) {
		for (p = ext.num_ptn; --p >= 0;)
			if (ext.ptn[p].mbr_parts[0].mbrp_type == 0)
				delete_ext_ptn(p);
	}
	return 1;
}

void
print_params(void)
{

	if (sh_flag) {
		printf("DISK=%s\n", disk);
		printf("DLCYL=%d\nDLHEAD=%d\nDLSEC=%d\nDLSIZE=%"PRIdaddr"\n",
			cylinders, heads, sectors, disksectors);
		printf("BCYL=%d\nBHEAD=%d\nBSEC=%d\nBDLSIZE=%"PRIdaddr"\n",
			dos_cylinders, dos_heads, dos_sectors, dos_disksectors);
		printf("NUMEXTPTN=%d\n", ext.num_ptn);
		return;
	}

	/* Not sh_flag */
	printf("Disk: %s\n", disk);
	printf("NetBSD disklabel disk geometry:\n");
	printf("cylinders: %d, heads: %d, sectors/track: %d "
	    "(%d sectors/cylinder)\ntotal sectors: %"PRIdaddr"\n\n",
	    cylinders, heads, sectors, cylindersectors, disksectors);
	printf("BIOS disk geometry:\n");
	printf("cylinders: %d, heads: %d, sectors/track: %d "
	    "(%d sectors/cylinder)\ntotal sectors: %"PRIdaddr"\n\n",
	    dos_cylinders, dos_heads, dos_sectors, dos_cylindersectors,
	    dos_disksectors);
}

void
change_active(int which)
{
	struct mbr_partition *partp;
	int part;
	int active = MBR_PART_COUNT;

	partp = &mboot.mbr_parts[0];

	if (a_flag && which != -1)
		active = which;
	else {
		for (part = 0; part < MBR_PART_COUNT; part++)
			if (partp[part].mbrp_flag & MBR_PFLAG_ACTIVE)
				active = part;
	}
	if (!f_flag) {
		if (yesno("Do you want to change the active partition?")) {
			printf ("Choosing %d will make no partition active.\n",
			    MBR_PART_COUNT);
			do {
				active = decimal("active partition",
						active, 0, 0, MBR_PART_COUNT);
			} while (!yesno("Are you happy with this choice?"));
		} else
			return;
	} else
		if (active != MBR_PART_COUNT)
			printf ("Making partition %d active.\n", active);

	for (part = 0; part < MBR_PART_COUNT; part++)
		partp[part].mbrp_flag &= ~MBR_PFLAG_ACTIVE;
	if (active < MBR_PART_COUNT)
		partp[active].mbrp_flag |= MBR_PFLAG_ACTIVE;
}

void
get_params_to_use(void)
{
#if defined(__i386__) || defined(__x86_64__)
	struct biosdisk_info *bip;
	int i;
#endif

	if (b_flag) {
		dos_cylinders = b_cyl;
		dos_heads = b_head;
		dos_sectors = b_sec;
		return;
	}

	print_params();
	if (!yesno("Do you want to change our idea of what BIOS thinks?"))
		return;

#if defined(__i386__) || defined(__x86_64__)
	if (dl != NULL) {
		for (i = 0; i < dl->dl_nbiosdisks; i++) {
			if (i == 0)
				printf("\nGeometries of known disks:\n");
			bip = &dl->dl_biosdisks[i];
			printf("Disk %d: cylinders %u, heads %u, sectors %u"
				" (%"PRIdaddr" sectors, %dMB)\n",
			    i, bip->bi_cyl, bip->bi_head, bip->bi_sec,
			    bip->bi_lbasecs, SEC_TO_MB(bip->bi_lbasecs));
				
		}
		printf("\n");
	}
#endif
	do {
		dos_cylinders = decimal("BIOS's idea of #cylinders",
					dos_cylinders, 0, 0, MAXCYL);
		dos_heads = decimal("BIOS's idea of #heads",
					dos_heads, 0, 0, MAXHEAD);
		dos_sectors = decimal("BIOS's idea of #sectors",
					dos_sectors, 0, 1, MAXSECTOR);
		print_params();
	} while (!yesno("Are you happy with this choice?"));
}


/***********************************************\
* Change real numbers into strange dos numbers	*
\***********************************************/
void
dos(int sector, unsigned char *cylinderp, unsigned char *headp,
    unsigned char *sectorp)
{
	int cylinder, head;

	cylinder = sector / dos_cylindersectors;
	sector -= cylinder * dos_cylindersectors;

	head = sector / dos_sectors;
	sector -= head * dos_sectors;
	if (cylinder > 1023)
		cylinder = 1023;

	*cylinderp = DOSCYL(cylinder);
	*headp = head;
	*sectorp = DOSSECT(sector + 1, cylinder);
}

int
open_disk(int update)
{
	static char namebuf[MAXPATHLEN + 1];

	fd = opendisk(disk, update && disk_file == NULL ? O_RDWR : O_RDONLY,
	    namebuf, sizeof(namebuf), 0);
	if (fd < 0) {
		if (errno == ENODEV)
			warnx("%s is not a character device", namebuf);
		else
			warn("%s", namebuf);
		return (-1);
	}
	disk = namebuf;
	if (get_params() == -1) {
		close(fd);
		fd = -1;
		return (-1);
	}
	if (disk_file != NULL) {
		/* for testing: read/write data from a disk file */
		wfd = open(disk_file, update ? O_RDWR|O_CREAT : O_RDONLY, 0777);
		if (wfd == -1) {
			warn("%s", disk_file);
			close(fd);
			fd = -1;
			return -1;
		}
	} else
		wfd = fd;
	return (0);
}

int
read_disk(daddr_t sector, void *buf)
{

	if (*rfd == -1)
		errx(1, "read_disk(); fd == -1");
	if (lseek(*rfd, sector * (off_t)512, 0) == -1)
		return (-1);
	return (read(*rfd, buf, 512));
}

int
write_disk(daddr_t sector, void *buf)
{

	if (wfd == -1)
		errx(1, "write_disk(); wfd == -1");
	if (lseek(wfd, sector * (off_t)512, 0) == -1)
		return (-1);
	return (write(wfd, buf, 512));
}

static void
guess_geometry(daddr_t _sectors)
{
	dos_sectors = MAXSECTOR;
	dos_heads = MAXHEAD - 1;	/* some BIOS might use 256 */
	dos_cylinders = _sectors / (MAXSECTOR * (MAXHEAD - 1));
	if (dos_cylinders < 1)
		dos_cylinders = 1;
	else if (dos_cylinders > MAXCYL - 1)
		dos_cylinders = MAXCYL - 1;
}

int
get_params(void)
{
	if (disk_type != NULL) {
		struct disklabel *tmplabel;

		if ((tmplabel = getdiskbyname(disk_type)) == NULL) {
			warn("bad disktype");
			return (-1);
		}
		disklabel = *tmplabel;
	} else if (F_flag) {
		struct stat st;
		if (fstat(fd, &st) == -1) {
			warn("fstat");
			return (-1);
		}
		if (st.st_size % 512 != 0) {
			warnx("%s size (%lld) is not divisible "
			    "by sector size (%d)", disk, (long long)st.st_size,
			    512);
		}
		disklabel.d_secperunit = st.st_size / 512;
		guess_geometry(disklabel.d_secperunit);
		disklabel.d_ncylinders = dos_cylinders;
		disklabel.d_ntracks = dos_heads;
		disklabel.d_nsectors = dos_sectors;
	} else if (ioctl(fd, DIOCGDEFLABEL, &disklabel) == -1) {
		warn("DIOCGDEFLABEL");
		if (ioctl(fd, DIOCGDINFO, &disklabel) == -1) {
			warn("DIOCGDINFO");
			return (-1);
		}
	}
	disksectors = disklabel.d_secperunit;
	cylinders = disklabel.d_ncylinders;
	heads = disklabel.d_ntracks;
	sectors = disklabel.d_nsectors;

	/* pick up some defaults for the BIOS sizes */
	if (sectors <= MAXSECTOR) {
		dos_cylinders = cylinders;
		dos_heads = heads;
		dos_sectors = sectors;
	} else {
		/* guess - has to better than the above */
		guess_geometry(disksectors);
	}
	dos_disksectors = disksectors;

	return (0);
}

int
read_s0(daddr_t offset, struct mbr_sector *boot)
{
	const char *tabletype = offset ? "extended" : "primary";

	if (read_disk(offset, boot) == -1) {
		warn("Can't read %s partition table", tabletype);
		return -1;
	}
	if (le16toh(boot->mbr_magic) != MBR_MAGIC) {
		warnx("%spartition table invalid, no magic in sector %"PRIdaddr,
		    tabletype, offset);
		return -1;
	}
#ifdef BOOTSEL
	if (le16toh(boot->mbr_bootsel_magic) == MBR_MAGIC) {
				/* mbr_bootsel in old location */
		warnx("%s partition table: using old-style bootsel information",
		    tabletype);
		memmove((u_int8_t *)boot + MBR_BS_OFFSET,
			(u_int8_t *)boot + MBR_BS_OFFSET + 4,
			sizeof(struct mbr_bootsel));
		if ( ! (boot->mbr_bootsel.mbrbs_flags & MBR_BS_NEWMBR)) {
				/* old style default key */
			int id;
				/* F1..F4 => ptn 0..3, F5+ => disk 0+ */
			id = boot->mbr_bootsel.mbrbs_defkey;
			id -= SCAN_F1;
			if (id >= MBR_PART_COUNT)
				id -= MBR_PART_COUNT; /* Use number of disk */
			else if (mboot.mbr_parts[id].mbrp_type != 0)
				id = le32toh(boot->mbr_parts[id].mbrp_start);
			else
				id = DEFAULT_ACTIVE;
			boot->mbr_bootsel.mbrbs_defkey = id;
		}
		boot->mbr_bootsel_magic = htole16(MBR_BS_MAGIC);
			/* highlight that new bootsel code is necessar */
	    	boot->mbr_bootsel.mbrbs_flags &= ~ MBR_BS_NEWMBR;
	}
#endif /* BOOTSEL */
	return (0);
}

int
write_mbr(void)
{
	int flag, i;
	daddr_t offset;
	int rval = -1;

	/*
	 * write enable label sector before write (if necessary),
	 * disable after writing.
	 * needed if the disklabel protected area also protects
	 * sector 0. (e.g. empty disk)
	 */
	flag = 1;
	if (wfd == fd && F_flag == 0 && ioctl(wfd, DIOCWLABEL, &flag) < 0)
		warn("DIOCWLABEL");
	if (write_disk(0, &mboot) == -1) {
		warn("Can't write fdisk partition table");
		goto protect_label;
	}
	if (boot_installed)
		for (i = bootsize; (i -= 0x200) > 0;)
			if (write_disk(i / 0x200, &bootcode[i / 0x200]) == -1) {
				warn("Can't write bootcode");
				goto protect_label;
			}
	for (offset = 0, i = 0; i < ext.num_ptn; i++) {
		if (write_disk(ext.base + offset, ext.ptn + i) == -1) {
			warn("Can't write %dth extended partition", i);
			goto protect_label;
		}
		offset = le32toh(ext.ptn[i].mbr_parts[1].mbrp_start);
	}
	rval = 0;
    protect_label:
	flag = 0;
	if (wfd == fd && F_flag == 0 && ioctl(wfd, DIOCWLABEL, &flag) < 0)
		warn("DIOCWLABEL");
	return rval;
}

int
yesno(const char *str, ...)
{
	int ch, first;
	va_list ap;

	va_start(ap, str);

	vprintf(str, ap);
	printf(" [n] ");

	first = ch = getchar();
	while (ch != '\n' && ch != EOF)
		ch = getchar();
	if (ch == EOF)
		errx(1, "EOF");
	return (first == 'y' || first == 'Y');
}

int
decimal(const char *prompt, int dflt, int flags, int minval, int maxval)
{
	int acc = 0;
	char *cp;

	for (;;) {
		if (flags & DEC_SEC) {
			printf("%s: [%d..%dcyl default: %d, %dcyl, %uMB] ",
			    prompt, SEC_TO_CYL(minval), SEC_TO_CYL(maxval),
			    dflt, SEC_TO_CYL(dflt), SEC_TO_MB(dflt));
		} else
			printf("%s: [%d..%d default: %d] ",
			    prompt, minval, maxval, dflt);

		if (!fgets(lbuf, LBUF, stdin))
			errx(1, "EOF");
		lbuf[strlen(lbuf)-1] = '\0';
		cp = lbuf;

		cp += strspn(cp, " \t");
		if (*cp == '\0')
			return dflt;

		if (cp[0] == '$' && cp[1] == 0)
			return maxval;

		if (isdigit(*cp) || *cp == '-') {
			acc = strtol(lbuf, &cp, 10);
			if (flags & DEC_SEC) {
				if (*cp == 'm' || *cp == 'M') {
					acc *= SEC_IN_1M;
					/* round to whole number of cylinders */
					acc += dos_cylindersectors / 2;
					acc /= dos_cylindersectors;
					cp = "c";
				}
				if (*cp == 'c' || *cp == 'C') {
					cp = "";
					acc *= dos_cylindersectors;
					/* adjustments for cylinder boundary */
					if (acc == 0 && flags & DEC_RND_0)
						acc += dos_sectors;
					if (flags & DEC_RND)
						acc += dos_sectors;
					if (flags & DEC_RND_DOWN)
						acc -= dos_sectors;
					if (flags & DEC_RND_DOWN_2)
						acc -= dos_sectors;
				}
			}
		}

		cp += strspn(cp, " \t");
		if (*cp != '\0') {
			printf("%s is not a valid %s number.\n", lbuf,
			    flags & DEC_SEC ? "sector" : "decimal");
			continue;
		}

		if (acc >= minval && acc <= maxval)
			return acc;
		printf("%d is not between %d and %d.\n", acc, minval, maxval);
	}
}

int
ptn_id(const char *prompt, int *extended)
{
	uint acc = 0;
	char *cp;

	for (;; printf("%s is not a valid partition number.\n", lbuf)) {
		printf("%s: [none] ", prompt);

		if (!fgets(lbuf, LBUF, stdin))
			errx(1, "EOF");
		lbuf[strlen(lbuf)-1] = '\0';
		cp = lbuf;

		cp += strspn(cp, " \t");
		*extended = 0;
		if (*cp == 0)
			return -1;

		if (*cp == 'E' || *cp == 'e') {
			cp++;
			*extended = 1;
		}

		acc = strtoul(cp, &cp, 10);

		cp += strspn(cp, " \t");
		if (*cp != '\0')
			continue;

		if (*extended || acc < MBR_PART_COUNT)
			return acc;
	}
}

#ifdef BOOTSEL
void
string(const char *prompt, int length, char *buf)
{
	int len;

	for (;;) {
		printf("%s: [%.*s] ", prompt, length, buf);

		if (!fgets(lbuf, LBUF, stdin))
			errx(1, "EOF");
		len = strlen(lbuf);
		if (len <= 1)
			/* unchanged if just <enter> */
			return;
		/* now strip trailing spaces, <space><enter> deletes string */
		do
			lbuf[--len] = 0;
		while (len != 0 && lbuf[len - 1] == ' ');
		if (len < length)
			break;
		printf("'%s' is longer than %d characters.\n",
		    lbuf, length - 1);
	}
	strncpy(buf, lbuf, length);
}
#endif

int
type_match(const void *key, const void *item)
{
	const int *typep = key;
	const struct part_type *ptr = item;

	if (*typep < ptr->type)
		return (-1);
	if (*typep > ptr->type)
		return (1);
	return (0);
}

const char *
get_type(int type)
{
	struct part_type *ptr;

	ptr = bsearch(&type, part_types,
	    sizeof(part_types) / sizeof(struct part_type),
	    sizeof(struct part_type), type_match);
	if (ptr == 0)
		return ("unknown");
	return (ptr->name);
}
