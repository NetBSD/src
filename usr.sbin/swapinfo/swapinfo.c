/*
 * swapinfo - based on a program of the same name by Kevin Lahey
 *	      (written by Chris Torek, converted to NetBSD by cgd)
 */

#ifndef lint
static char rcsid[] = "$Id: swapinfo.c,v 1.5 1993/11/18 03:00:41 cgd Exp $";
#endif /* not lint */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/map.h>
#include <sys/stat.h>
#include <sys/termios.h>
#include <sys/tty.h>
#include <sys/uio.h>

#include <errno.h>
#include <err.h>
#include <fcntl.h>
#include <nlist.h>
#include <kvm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern char *devname __P((int, int));
void showspace __P((long blocksize));

int showmap;			/* show the contents of the swap map */
int nosum;			/* don't show totals */

struct nlist syms[] = {
	{ "_swapmap" },	/* list of free swap areas */
#define VM_SWAPMAP	0
	{ "_nswapmap" },/* size of the swap map */
#define VM_NSWAPMAP	1
	{ "_swdevt" },	/* list of swap devices and sizes */
#define VM_SWDEVT	2
	{ "_nswap" },	/* size of largest swap device */
#define VM_NSWAP	3
	{ "_nswdev" },	/* number of swap devices */
#define VM_NSWDEV	4
	{ "_dmmax" },	/* maximum size of a swap block */
#define VM_DMMAX	5
	0
};

main(argc, argv)
	int argc;
	char **argv;
{
	char *memf, *nlistf;
	int ch, i;
	long blocksize = 512;
	char errbuf[256];

	memf = nlistf = NULL;
	while ((ch = getopt(argc, argv, "kmnM:N:")) != EOF) {
		switch (ch) {

		case 'k':
			blocksize = 1024;
			break;

		case 'm':
			showmap = 1;
			break;

		case 'n':
			nosum = 1;
			break;

		case 'M':
			memf = optarg;
			break;

		case 'N':
			nlistf = optarg;
			break;

		case '?':
		default:
			errx(1, "usage: swapinfo [-M memfile] [-N kernel]\n");
		}
	}
	argc -= optind;
	argv += optind;
	if (nlistf != NULL || memf != NULL)
		setgid(getgid());
	if (kvm_openfiles(nlistf, memf, NULL) == -1)
		err(1, "can't kvm_openfiles");
	if (kvm_nlist((struct nlist *)syms)) {
		(void)fprintf(stderr, "swapinfo: cannot find ");
		for (i = 0; syms[i].n_name != NULL; i++)
			if (syms[i].n_value == 0)
				(void)fprintf(stderr, " %s", syms[i].n_name);
		(void)fprintf(stderr, "\n");
		exit(1);
	}
	showspace(blocksize);
	exit(0);
}

#define	SVAR(var) __STRING(var)	/* to force expansion */
#define	KGET(idx, var) \
	KGET1(idx, &var, sizeof(var), SVAR(var))
#define	KGET1(idx, p, s, msg) \
	KGET2(syms[idx].n_value, p, s, msg)
#define	KGET2(addr, p, s, msg) \
	if (kvm_read((void *)addr, p, s) != s) \
		errx(1, "cannot read %s: %s", msg, kvm_geterr())

void
showspace(blocksize)
	long blocksize;
{
	int nswap, nswdev, dmmax, nswapmap;
	int s, e, div, i, ind, avail, nfree, npfree, used;
	struct swdevt *sw;
	long *perdev;
	struct map *swapmap, *kswapmap;
	struct mapent *mp;

	div = blocksize / 512;		/* printing routines use this */

	KGET(VM_NSWAP, nswap);
	KGET(VM_NSWDEV, nswdev);
	KGET(VM_DMMAX, dmmax);
	KGET(VM_NSWAPMAP, nswapmap);
	KGET(VM_SWAPMAP, kswapmap);	/* kernel `swapmap' is a pointer */
	if ((sw = malloc(nswdev * sizeof(*sw))) == NULL ||
	    (perdev = malloc(nswdev * sizeof(*perdev))) == NULL ||
	    (mp = malloc(nswapmap * sizeof(*mp))) == NULL)
		err(1, "malloc");
	KGET1(VM_SWDEVT, sw, nswdev * sizeof(*sw), "swdevt");
	KGET2((long)kswapmap, mp, nswapmap * sizeof(*mp), "swapmap");

	/* first entry in map is `struct map'; rest are mapent's */
	swapmap = (struct map *)mp;
	if (nswapmap != swapmap->m_limit - (struct mapent *)kswapmap)
		errx(1, "panic: nswapmap goof");

	if (showmap) {
		printf("Total number of map entries: %d\n", nswapmap);
		printf("All offsets and sizes in %d-byte blocks\n",
		    blocksize);
		printf("\n");
		printf("%-8s %10s %10s %10s\n", "Entry", "Size", "Start",
			"End");
	}

	/*
	 * Count up swap space.
	 */
	nfree = 0;
	bzero(perdev, nswdev * sizeof(*perdev));
	for (mp++, ind=0; mp->m_addr != 0; mp++, ind++) {
		s = mp->m_addr;			/* start of swap region */
		e = mp->m_addr + mp->m_size;	/* end of region */
		nfree += mp->m_size;

		/*
		 * Swap space is split up among the configured disks.
		 * The first dmmax blocks of swap space some from the
		 * first disk, the next dmmax blocks from the next, 
		 * and so on.  The list of free space joins adjacent
		 * free blocks, ignoring device boundries.  If we want
		 * to keep track of this information per device, we'll
		 * just have to extract it ourselves.
		 */

		/* calculate first device on which this falls */
		i = (s / dmmax) % nswdev;
		while (s < e) {		/* XXX this is inefficient */
			int bound = roundup(s+1, dmmax);

			if (bound > e)
				bound = e;
			perdev[i] += bound - s;
			if (++i >= nswdev)
				i = 0;
			s = bound;
		}

		if (showmap)
			printf("%-8d %10d %10d %10d\n", ind, mp->m_size / div,
			    mp->m_addr / div, (mp->m_addr + mp->m_size) / div);
	}

	if (nosum)	/* XXX could break later, but rather not indent now */
		return;

	if (showmap)
		printf("\n");

	(void)printf("%-10s %4d-blocks %10s %10s %10s\n",
	    "Device", blocksize, "Used", "Available", "Capacity");
	avail = npfree = 0;
	for (i = 0; i < nswdev; i++) {
		int xsize, xfree;

		(void)printf("/dev/%-5s %11d ",
		    devname(sw[i].sw_dev, S_IFBLK),
		    sw[i].sw_nblks / div);

		/*
		 * Don't report statistics for partitions which have not
		 * yet been activated via swapon(8).
		 */
		if (!sw[i].sw_freed) {
			(void)printf(" *** not available for swapping ***\n");
			continue;
		}
		xsize = sw[i].sw_nblks;
		xfree = perdev[i];
		used = xsize - xfree;
		(void)printf("%10d %10d %7.0f%%\n", 
		    used / div, xfree / div,
		    (double)used / (double)xsize * 100.0);
		npfree++;
		avail += xsize;
	}

	/* 
	 * If only one partition has been set up via swapon(8), we don't
	 * need to bother with totals.
	 */

	if (npfree > 1) {
		used = avail - nfree;
		(void)printf("%-10s %11d %10d %10d %7.0f%%\n",
		    "Total", avail / div, used / div, nfree / div,
		    (double)used / (double)avail * 100.0);
	}
}

