/*
 *	Load and boot NetBSD kernel on Human68k
 *
 *	written by Yasha (ITOH Yasufumi)
 *	public domain
 *
 *	loadbsd [-hvV] [-abDs] [-r root_device] netbsd
 *
 *	loadbsd options:
 *		-h	help
 *		-v	verbose
 *		-V	print version and exit
 *
 *	kernel options:
 *		-a	auto boot, opposite of -s
 *		-s	single user boot (default)
 *		-D	enter kernel debugger
 *		-b	ask root device
 *		-d	use compiled-in rootdev
 *		-r	specify root device
 *
 *	$NetBSD: loadbsd.c,v 1.4 1999/09/23 15:14:59 minoura Exp $
 */

#include <sys/cdefs.h>

__RCSID("$NetBSD: loadbsd.c,v 1.4 1999/09/23 15:14:59 minoura Exp $");
#define VERSION	"$Revision: 1.4 $ $Date: 1999/09/23 15:14:59 $"

#include <sys/types.h>		/* ntohl */
#include <sys/reboot.h>
#include <sys/param.h>		/* ALIGN, ALIGNBYTES */
#include <a.out.h>
#include <string.h>
#include <machine/bootinfo.h>

#include <dos.h>
#include <iocs.h>
#include "../common/xprintf.h"
#include "trampoline.h"

#define DEFAULT_ROOTDEVNAME	"sd@0,0:a"

#define ISDIGIT(c)	((c) >= '0' && (c) <= '9')

#define GETDECIMAL(var, str)	\
	do {	var *= 10; var += *str++ - '0'; } while (ISDIGIT(*str))

static const char *lookupif __P((const char *name,
				 unsigned *pif, unsigned *punit));
static void get_current_scsi_interface __P((unsigned *pif, unsigned *punit));
static int bootdev __P((const char *devstr));
static struct tramparg *read_kernel __P((const char *fn));
static int chkmpu __P((void));
static __dead void usage __P((int status, const char *msg))
					__attribute__((noreturn));

int main __P((int argc, char *argv[]));

int opt_v;
int opt_N;

const struct hatbl {
	char name[4];
	unsigned short id;
} hatable[] = {
	X68K_BOOT_SCSIIF_LIST
};

/*
 * parse interface name
 * return the next position
 */
static const char *
lookupif(name, pif, punit)
	const char *name;
	unsigned *pif, *punit;
{
	unsigned u, unit;
	const char *p;

	for (u = 0; u < sizeof hatable / sizeof hatable[0]; u++) {
		const char *n;

		for (n = hatable[u].name, p = name; *n && *n == *p; n++, p++)
			;
		if (!*n)
			goto found;
	}
	/* not found */
	return (char *) 0;

found:
	if (*p == '@')
		p++;

	/* get unit # */
	if (!ISDIGIT(*p))
		return (char *) 0;

	unit = 0;
	GETDECIMAL(unit, p);

	*pif = hatable[u].id;
	*punit = unit;

	return p;
}

/*
 * if the SCSI interface is not specified, use the current one
 */
static void
get_current_scsi_interface(pif, punit)
	unsigned *pif, *punit;
{
	unsigned binf;
	char *bootrom;
	int bus_err_buf;

	binf = (unsigned) IOCS_BOOTINF();
	if (binf < 0x00fc0000)
		return;			/* not booted from SCSI */

	bootrom = (char *) (binf & 0x00ffffe0);
	if (IOCS_B_LPEEK(bootrom + 0x24) == 0x53435349 &&	/* 'SCSI' */
	    IOCS_B_WPEEK(bootrom + 0x28) == 0x494E) {		/* 'IN' */
		/* spc0 */
		*pif = X68K_BOOT_SCSIIF_SPC;
		*punit = 0;
	} else if (DOS_BUS_ERR(&bus_err_buf, (void *)EXSPC_BDID, 1)) {
		/* mha0 */
		*pif = X68K_BOOT_SCSIIF_MHA;
		*punit = 0;
	} else {
		/* spc1 */
		*pif = X68K_BOOT_SCSIIF_SPC;
		*punit = 1;
	}
}

/*
 * parse device name
 *
 * [/<controller>@<unit>/]<device>@<unit>[,<lun>][:<partition>]
 *
 * <unit> must be target SCSI ID if <device> is a SCSI device
 *
 *	full form:
 *		/spc@0/sd@1,2:e
 *
 *	partial form:
 *		/mha@0/sd@1	= /mha@0/sd@1,0:a
 *		sd@1:e		= /current_device/sd@1,0e
 *		sd@1,2:e	= /current_device/sd@1,2:e
 */

const struct devtbl {
	char name[3];
	u_char major;
} devtable[] = {
	X68K_BOOT_DEV_LIST,
	X68K_BOOT_NETIF_LIST
};

static int
bootdev(devstr)
	const char *devstr;
{
	unsigned u;
	unsigned major, unit, lun, partition;
	int dev;
	const char *s = devstr;
	unsigned interface = 0, unit_if = 0;

	if (*s == '/') {
		/*
		 * /<interface>/<device>"
		 * "/spc@1/sd@2,3:e"
		 */
		while (*++s == '/')	/* skip slashes */
			;
		if (!strchr(s, '/'))
			xerrx(1, "%s: bad format", devstr);

		if (!(s = lookupif(s, &interface, &unit_if)))
			xerrx(1, "%s: unknown interface", devstr);

		while (*s == '/')	/* skip slashes */
			s++;
	} else {
		/* make lint happy */
		interface = 0;
		unit_if = 0;
	}

	/* allow r at the top */
	if (*s == 'r')
		s++;

	for (u = 0; u < sizeof devtable / sizeof devtable[0]; u++)
		if (s[0] == devtable[u].name[0] && s[1] == devtable[u].name[1])
			goto found;

	/* not found */
	xerrx(1, "%s: unknown device", devstr);

found:	major = devtable[u].major;

	/*
	 * <type>@unit[,lun][:part]
	 * "sd@1,3:a"
	 */

	/* get device unit # */
	s += 2;
	if (*s == '@')
		s++;
	if (!*s)
		xerrx(1, "%s: missing unit number", devstr);
	if (!ISDIGIT(*s))
		xerrx(1, "%s: wrong device", devstr);

	unit = 0;
	GETDECIMAL(unit, s);

	lun = 0;
	if (*s == ',') {
		s++;
		if (!ISDIGIT(*s))
			xerrx(1, "%s: wrong device", devstr);
		GETDECIMAL(lun, s);
	}

	/* get device partition */
	if (*s == ':')
		s++;
	if (!*s)
		partition = 0;	/* no partition letter -- assuming 'a' */
	else if (!s[1])
		partition = *s - 'a';
	else
		xerrx(1, "%s: wrong partition letter", devstr);

	/*
	 * sanity check
	 */
	if (unit_if >= 16)
		xerrx(1, "%s: interface unit # too large", devstr);
	if (unit >= 16)
		xerrx(1, "%s: device unit # too large", devstr);
	if (lun >= 8)
		xerrx(1, "%s: SCSI LUN >= 8 is not supported yet", devstr);
	if (partition >= 16)
		xerrx(1, "%s: unsupported partition", devstr);

	/*
	 * encode device to be passed to kernel
	 */
	if (X68K_BOOT_DEV_IS_SCSI(major)) {
		/*
		 * encode SCSI device
		 */
		if (interface == 0)
			get_current_scsi_interface(&interface, &unit_if);

		dev = X68K_MAKESCSIBOOTDEV(major, interface, unit_if,
						unit, lun, partition);
	} else {
		/* encode non-SCSI device */
		dev = X68K_MAKEBOOTDEV(major, unit, partition);
	}

	if (opt_v)
		xwarnx("%s: major %u, if %u, un_if %u, unit %u, lun %u, partition %u; bootdev 0x%x",
			devstr, major, interface, unit_if, unit, lun, partition, dev);

	return dev;
}

/*
 * read kernel and create trampoline
 *
 *	|----------------------| <- allocated buf addr
 *	| kernel image         |
 *	~ (header is excluded) ~
 *	|                      |
 *	|----------------------| <- return value (entry addr of trampoline)
 *	| struct tramparg      |
 *	| (trampoline args)    |
 *	|----------------------|
 *	| trampoline code      |
 *	| (in assembly)        |
 *	|----------------------|
 */
static struct tramparg *
read_kernel(fn)
	const char *fn;
{
	int fd;
	union dos_fcb *fcb;
	size_t filesize, nread;
	void *buf;
	struct exec hdr;
	int i;
	struct tramparg *arg;
	size_t size_tramp = end_trampoline - trampoline;

	if ((fd = DOS_OPEN(fn, 0x20)) < 0)	/* RO, share READ */
		xerr(1, "%s: open", fn);

	if ((int)(fcb = DOS_GET_FCB_ADR(fd)) < 0)
		xerr(1, "%s: get_fcb_adr", fn);

	/*
	 * XXX FCB is in supervisor area
	 */
	/*if (fcb->blk.mode != 0)*/
	if (IOCS_B_BPEEK((char *)fcb + 1) & 0x80)
		xerrx(1, "%s: Not a regular file", fn);

	/*filesize = fcb->blk.size;*/
	filesize = IOCS_B_LPEEK(&fcb->blk.size);

	/*
	 * read a.out header
	 */
	if ((nread = DOS_READ(fd, (void *) &hdr, sizeof hdr)) != sizeof hdr) {
		if ((int)nread < 0)
			xerr(1, "%s: read header", fn);
		else
			xerrx(1, "%s: Not an a.out", fn);
	}
	/*
	 * check header
	 */
	if (N_GETMAGIC(hdr) != NMAGIC)
		xerrx(1, "%s: Bad magic number", fn);
	if ((i = N_GETMID(hdr)) != MID_M68K)
		xerrx(1, "%s: Wrong architecture (mid %d)", fn, i);

	if (opt_v)
		xwarnx("%s: %u bytes; text %u, data %u, bss %u, sym %u",
			fn, filesize, hdr.a_text, hdr.a_data,
			hdr.a_bss, hdr.a_syms);

	/*
	 * then, read entire body
	 */
	if ((int)(buf = DOS_MALLOC(filesize + ALIGNBYTES - sizeof hdr
				   + sizeof(struct tramparg)
				   + size_tramp + SIZE_TMPSTACK)) < 0)
		xerr(1, "read_kernel");

	if ((nread = DOS_READ(fd, buf, filesize - sizeof hdr))
					!= filesize - sizeof hdr) {
		if ((int)nread < 0)
			xerr(1, "%s: read", fn);
		else
			xerrx(1, "%s: short read", fn);
	}

	if (DOS_CLOSE(fd) < 0)
		xerr(1, "%s: close", fn);

	/*
	 * create argument for trampoline code
	 */
	arg = (struct tramparg *) ALIGN(buf + nread);

	if (opt_v)
		xwarnx("trampoline arg at %p", arg);

	arg->bsr_inst = TRAMP_BSR + sizeof(struct tramparg) - 2;
	arg->tmp_stack = (char *) arg + sizeof(struct tramparg)
				+ size_tramp + SIZE_TMPSTACK;
	arg->mpu_type = IOCS_MPU_STAT() & 0xff;
	arg->xk.image_top = buf;
	arg->xk.load_addr = 0x00000000;	/* XXX should not be a fixed addr */
	arg->xk.text_size = hdr.a_text;
	arg->xk.data_size = hdr.a_data;
	arg->xk.bss_size = hdr.a_bss;
	arg->xk.symbol_size = hdr.a_syms;
	arg->xk.d5 = IOCS_BOOTINF();	/* unused for now */
#if 0
	/* filled afterwards */
	arg->xk.rootdev = 
	arg->xk.boothowto = 
#endif
	arg->xk.entry_addr = hdr.a_entry;

	if (opt_v)
		xwarnx("args: mpu %d, image %p, load 0x%x, entry 0x%x",
			arg->mpu_type, arg->xk.image_top, arg->xk.load_addr,
			arg->xk.entry_addr);

	/*
	 * copy trampoline code
	 */
	if (opt_v)
		xwarnx("trampoline code at %p (%u bytes)",
			(char *) arg + sizeof(struct tramparg), size_tramp);

	memcpy((char *) arg + sizeof(struct tramparg), trampoline, size_tramp);

	return arg;
}

/*
 * MC68000/010		-> return zero
 * MC68020 and later	-> return nonzero
 */
static int
chkmpu()
{
	register int ret asm("d0");

	asm("| %0 <- this must be d0\n\
	moveq	#1,d0\n\
	.long	0x103B02FF	| foo: moveb pc@((foo+1)-foo-2:B,d0:W:2),d0\n\
	|	      ^ ^\n\
	| d0.b	= 0x02	(68000/010)\n\
	|	= 0xff	(68020 and later)\n\
	bmis	1f\n\
	moveq	#0,d0		| 68000/010\n\
1:"	: "=d" (ret));

	return ret;
}

static __dead void
usage(status, msg)
	int status;
	const char *msg;
{
	extern const char *const __progname;

	if (msg)
		xwarnx("%s", msg);

	xerrprintf("\
%s [-hvV] [-abDs] [-r root_device] netbsd\n\
\n\
loadbsd options:\n\
\t-h	help\n\
\t-v	verbose\n\
\t-V	print version and exit\n\
\n\
kernel options:\n\
\t-a	auto boot, opposite of -s\n\
\t-s	single user boot (default)\n\
\t-D	enter kernel debugger\n\
\t-b	ask root device\n\
\t-d	use compiled-in rootdev\n\
\t-r	specify root device (default %s)\n\
\t	format:  [/interface/]device@unit[,lun][:partition]\n\
\t	    interface: one of  spc@0, spc@1, mha@0\n\
\t		       (current boot interface if omitted)\n\
\t	    device:    one of  fd, sd, cd, md, ne\n\
\t	    unit:      device unit number (SCSI ID for SCSI device)\n\
\t	    lun:       SCSI LUN # (0 if omitted)\n\
\t	    partition: partition letter ('a' if omitted)\n\
", __progname, DEFAULT_ROOTDEVNAME);

	DOS_EXIT2(status);
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	char *rootdevname = 0;
	int rootdev;
	u_long boothowto = RB_SINGLE;
	const char *kernel;
	char *p, **flg, **arg;
	struct tramparg *tramp;
	struct dos_dregs regs;	/* unused... */
	int i;

	/* parse options */
	for (arg = flg = argv + 1; (p = *flg) && *p == '-'; ) {
		int c;

		while ((c = *++p))
			switch (c) {
			case 'h':
				usage(0, (char *) 0);
				/* NOTREACHED */
				break;
			case 'N':	/* don't actually execute kernel */
				opt_N = 1;
				break;
			case 'v':
				opt_v = 1;
				break;
			case 'V':
				xprintf("loadbsd %s\n", VERSION);
				return 0;

			/*
			 * kernel boot flags
			 */
			case 'r':
				if (rootdevname)
					usage(1, "multiple -r flags");
				else if (!*++arg)
					usage(1, "-r requires device name");
				else
					rootdevname = *arg;
				break;
			case 'd':
				boothowto |= RB_DFLTROOT;
				break;
			case 'b':
				boothowto |= RB_ASKNAME;
				break;
			case 'a':
				boothowto &= ~RB_SINGLE;
				break;
			case 's':
				boothowto |= RB_SINGLE;
				break;
			case 'D':
				boothowto |= RB_KDB;
				break;

			default:
				usage(1, (char *) 0);
				/* NOTREACHED */
				break;
			}
		flg = ++arg;
	}

	/* check MPU */
	if (chkmpu() == 0)
		xerrx(1, "Can't boot NetBSD on 68000/010");

	argc -= arg - argv;
	argv = arg;

	if (argc != 1)
		usage(1, (char *) 0);

	kernel = *argv;

	rootdev = bootdev(rootdevname ? rootdevname : DEFAULT_ROOTDEVNAME);

	if (opt_v)
		xwarnx("boothowto 0x%x", boothowto);

	tramp = read_kernel(kernel);

	tramp->xk.rootdev = rootdev;
	tramp->xk.boothowto = boothowto;

	/*
	 * we never return, and make sure the disk cache
	 * be flushed (if write-back cache is enabled)
	 */
	if (opt_v)
		xwarnx("flush disk cache...");

	i = DOS_FFLUSH_SET(1);		/* enable fflush */
	DOS_FFLUSH();			/* then, issue fflush */
	(void) DOS_FFLUSH_SET(i);	/* restore old mode just in case */

	/*
	 * the program assumes the MPU caches off
	 */
	if (opt_v)
		xwarnx("flush and disable MPU caches...");

	IOCS_CACHE_MD(-1);		/* flush */
	if (!opt_N)
		IOCS_CACHE_MD(0);	/* disable both caches */

	if (opt_v)
		xwarnx("Jumping to the kernel.  Good Luck!");

	if (opt_N)
		xerrx(0, "But don't actually do it.");

	DOS_SUPER_JSR((void (*) __P((void))) tramp, &regs, &regs);

	/* NOTREACHED */

	xwarnx("??? return from kernel");

	return 1;
}
