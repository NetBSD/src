/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time and initializes the vba 
 * device tables and the memory controller monitoring.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/disklabel.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/dkstat.h>
#include <sys/conf.h>
#include <sys/dmap.h>
#include <sys/reboot.h>
#include <sys/systm.h>

void	setroot __P((void));
static	int getstr __P((char *, int));
static	int findblkmajor __P((struct dkdevice *));
static	struct dkdevice *getdisk __P((char *, int, int, dev_t *));
static	struct dkdevice *parsedisk __P((char *, int, int, dev_t *));
void	findbootdev __P((void));

/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */
int	cold;		/* if 1, still working on cold start */

void
configure()
{
	if (!config_rootfound("isa", NULL))
		panic("configure: no root");
	splnone();
	cold = 0;
}

/*
 * Configure swap space and related parameters.
 */
swapconf()
{
	register struct swdevt *swp;
	register int nblks;

	for (swp = swdevt; swp->sw_dev != NODEV; swp++)
		if (bdevsw[major(swp->sw_dev)].d_psize) {
			nblks =
			  (*bdevsw[major(swp->sw_dev)].d_psize)(swp->sw_dev);
			if (nblks != -1 &&
			    (swp->sw_nblks == 0 || swp->sw_nblks > nblks))
				swp->sw_nblks = nblks;
		}
}

#define	DOSWAP			/* change swdevt and dumpdev */
u_long	bootdev;		/* should be dev_t, but not until 32 bits */
struct	device *bootdv;
struct	dkdevice *bootdk;

#define	PARTITIONMASK	0x7
#define	UNITSHIFT	3

void
findbootdev()
{
	register struct dkdevice *dk;
	register void (*strat)(struct buf *);
	register int unit;
	int major;

	major = B_TYPE(bootdev);
	if (major < 0 || major >= nblkdev)
		return;
	strat = bdevsw[major].d_strategy;
	unit = B_UNIT(bootdev);

	for (dk = dkhead; dk; dk = dk->dk_next) {
		if (dk->dk_driver->d_strategy == strat &&
		    dk->dk_device->dv_unit == unit) {
			bootdk = dk;
			bootdv = dk->dk_device;
			return;
		}
	}
}

static int
findblkmajor(dk)
	register struct dkdevice *dk;
{
	register int i;

	for (i = 0; i < nblkdev; ++i)
		if ((void (*)(struct buf *))bdevsw[i].d_strategy ==
		    dk->dk_driver->d_strategy)
			return i;
	return -1;
}

static struct dkdevice *
getdisk(str, len, defpart, devp)
	char *str;
	int len, defpart;
	dev_t *devp;
{
	register struct dkdevice *dk;

	dk = parsedisk(str, len, defpart, devp);
	if (!dk) {
		printf("use one of:");
		for (dk = dkhead; dk; dk = dk->dk_next)
			printf(" %s[a-h]", dk->dk_device->dv_xname);
		printf("\n");
	}
	return dk;
}

static struct dkdevice *
parsedisk(str, len, defpart, devp)
	char *str;
	int len, defpart;
	dev_t *devp;
{
	register struct dkdevice *dk;
	register struct device *dv;
	register char *cp;
	int majdev, mindev, part;

	if (len == 0)
		return (NULL);
	cp = str + len - 1;
	if (*cp >= 'a' && *cp <= 'h') {
		part = *cp - 'a';
		*cp-- = '\0';
	} else
		part = defpart;

	for (dk = dkhead; dk; dk = dk->dk_next) {
		dv = dk->dk_device;
		if (strcmp(str, dv->dv_xname) == 0) {
			majdev = findblkmajor(dk);
			if (majdev < 0)
				panic("parsedisk");
			mindev = (dv->dv_unit << UNITSHIFT) + part;
			*devp = makedev(majdev, mindev);
			return (dk);
		}
	}

	return (NULL);
}

/*
 * Attempt to find the device from which we were booted.
 * If we can do so, and not instructed not to do so,
 * change rootdev to correspond to the load device.
 */
void
setroot()
{
	register struct swdevt *swp;
	register struct device *dv;
	register struct dkdevice *dk;
	register int len, majdev, mindev, part;
	dev_t nrootdev, nswapdev;
	char buf[128];
#ifdef DOSWAP
	dev_t temp;
#endif
#ifdef NFSCLIENT
	extern int (*mountroot)(), nfs_mountroot();
#endif

	findbootdev();
	if (boothowto & RB_ASKNAME || !bootdv) {
		for (;;) {
			printf("root device");
			if (bootdv)
				printf(" (default %s%c)", bootdv->dv_xname,
					B_PARTITION(bootdev) + 'a');
			printf("? ");
			len = getstr(buf, sizeof(buf));
			if (bootdv && len == 0) {
				nrootdev = makedev(B_TYPE(bootdev),
						   B_UNIT(bootdev) << UNITSHIFT
						   + B_PARTITION(bootdev));
				break;
			}
#ifdef GENERIC
			if (len > 0 && buf[len - 1] == '*') {
				buf[--len] = '\0';
				dk = getdisk(buf, len, 1, &nrootdev);
				if (dk) {
					bootdk = dk;
					bootdv = dk->dk_device;
					nswapdev = nrootdev;
					goto gotswap;
				}
			}
#endif
			dk = getdisk(buf, len, 0, &nrootdev);
			if (dk) {
				bootdk = dk;
				bootdv = dk->dk_device;
				break;
			}
		}
		for (;;) {
			printf("swap device (default %sb)? ", bootdv->dv_xname);
			len = getstr(buf, sizeof(buf));
			if (len == 0) {
				nswapdev = makedev(major(nrootdev),
				    (minor(nrootdev) & ~PARTITIONMASK) | 1);
				break;
			}
			if (getdisk(buf, len, 1, &nswapdev))
				break;
		}
#ifdef GENERIC
	    gotswap:
#endif
		rootdev = nrootdev;
		swapdev = nswapdev;
		dumpdev = nswapdev;		/* ??? */
		swdevt[0].sw_dev = nswapdev;
		swdevt[1].sw_dev = NODEV;
		return;
	}

	/* XXX currently there's no way to set RB_DFLTROOT... */
	if (boothowto & RB_DFLTROOT || !bootdv)
		return;

	switch (bootdv->dv_class) {

#ifdef NFSCLIENT
	    case DV_IFNET:
		mountroot = nfs_mountroot;
		return;
#endif

#if defined(FFS) || defined(LFS)
	    case DV_DISK:
		majdev = findblkmajor(bootdk);
		if (majdev < 0)
			return;
		part = 0;
		mindev = (bootdv->dv_unit << UNITSHIFT) + part;
		break;
#endif

	    default:
		printf("can't figure root, hope your kernel is right\n");
		return;
	}

	/*
	 * Form a new rootdev
	 */
	nrootdev = makedev(majdev, mindev);
	/*
	 * If the original rootdev is the same as the one
	 * just calculated, don't need to adjust the swap configuration.
	 */
	if (rootdev == nrootdev)
		return;

	rootdev = nrootdev;
	printf("Changing root device to %s%c\n", bootdv->dv_xname, part + 'a');

#ifdef DOSWAP
	mindev &= ~PARTITIONMASK;
	temp = NODEV;
	for (swp = swdevt; swp->sw_dev != NODEV; swp++) {
		if (majdev == major(swp->sw_dev) &&
		    mindev == (minor(swp->sw_dev) & ~PARTITIONMASK)) {
			temp = swdevt[0].sw_dev;
			swdevt[0].sw_dev = swp->sw_dev;
			swp->sw_dev = temp;
			break;
		}
	}
	if (swp->sw_dev == NODEV)
		return;

	/*
	 * If dumpdev was the same as the old primary swap device, move
	 * it to the new primary swap device.
	 */
	if (temp == dumpdev)
		dumpdev = swdevt[0].sw_dev;
#endif
}

static int
getstr(cp, size)
	register char *cp;
	register int size;
{
	register char *lp;
	register int c;
	register int len;

	lp = cp;
	len = 0;
	for (;;) {
		c = cngetc();
		switch (c) {
		    case '\n':
		    case '\r':
			printf("\n");
			*lp++ = '\0';
			return (len);
		    case '\b':
		    case '\177':
		    case '#':
			if (len) {
				--len;
				--lp;
				printf("\b \b");
			}
			continue;
		    case '@':
		    case 'u'&037:
			len = 0;
			lp = cp;
			printf("\n");
			continue;
		    default:
			if (len + 1 >= size || c < ' ') {
				printf("\007");
				continue;
			}
			printf("%c", c);
			++len;
			*lp++ = c;
		}
	}
}
