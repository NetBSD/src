/*	$NetBSD: intro.c,v 1.4.4.2 2008/01/09 01:38:58 matt Exp $	*/

/*
 * El extra-simplo example of the userspace driver framework.
 *
 * Eventually there will be a library a la libpuffs (perhaps,
 * gasp, even the same lib), but for now it's all manual until
 * I get it figured out.
 *
 * So how to run this?
 * 0) sh MAKEDEV putter (if you don't have a freshly created /dev)
 * 1) run this program with the argument "/dev/pud"
 * 2) mknod a char device with the major 377 (see sources below)
 * 3) echo ascii art and jokes into device created in previous step
 *    or read the device
 */

#include <sys/types.h>

#include <dev/pud/pud_msgif.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"

#define DEFALLOC 1024*1024
#define ECHOSTR1 "Would you like some sauce diable with that?\n"
#define ECHOSTR2 "Nej tack, you fool, I'm happy with my tournedos Rossini\n"
#define NSTR 2

const char *curstr = ECHOSTR1;

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

int
main(int argc, char *argv[])
{
	struct pud_req *pdr = malloc(DEFALLOC);
	struct pud_conf_reg pcr;
	int fd;
	ssize_t n;
	
	if (argc != 2)
		errx(1, "args");

	/*
	 * open pud device
	 */
	fd = open(argv[1], O_RDWR);
	if (fd == -1)
		err(1, "open");

	/*
	 * register our major number
	 */
	memset(&pcr, 0, sizeof(pcr));
	pcr.pm_pdr.pdr_pth.pth_framelen = sizeof(struct pud_conf_reg);
	pcr.pm_version = PUD_DEVELVERSION | PUD_VERSION;
	pcr.pm_pdr.pdr_reqclass = PUD_REQ_CONF;
	pcr.pm_pdr.pdr_reqtype = PUD_CONF_REG;

	pcr.pm_regdev = makedev(377, 0);
	pcr.pm_flags = PUD_CONFFLAG_BDEV;
	strlcpy(pcr.pm_devname, "testdev", sizeof(pcr.pm_devname));

	n = write(fd, &pcr, pcr.pm_pdr.pdr_pth.pth_framelen);
	if (n == -1)
		err(1, "configure write");

	/*
	 * process requests
	 */
	for (;;) {
		n = read(fd, pdr, DEFALLOC);
		printf("read %d %d\n", n, errno);

		switch (pdr->pdr_reqtype) {
		case PUD_CDEV_OPEN:
		case PUD_CDEV_CLOSE:
			printf("got openclose %d\n", pdr->pdr_reqtype);
			pdr->pdr_rv = 0;
			break;

		case PUD_CDEV_READ:
		/* uh oh case PUD_BDEV_STRATREAD: */
		{
			struct pud_creq_read *pc_read;
			size_t clen;

			pc_read = (void *)pdr;
			printf("read from offset %llu, resid %zu\n",
			    (unsigned long long)pc_read->pm_offset,
			    pc_read->pm_resid);

			clen = MIN(strlen(curstr), pc_read->pm_resid);
			strncpy(pc_read->pm_data, curstr, clen);
			if (pdr->pdr_reqclass == PUD_REQ_BDEV) {
				clen = pc_read->pm_resid;
				pc_read->pm_resid = 0;
			} else {
				pc_read->pm_resid -= clen;
			}
			pdr->pdr_pth.pth_framelen =
			    sizeof(struct pud_creq_read) + clen;
		}
			break;

		case PUD_CDEV_WRITE:
		/* uh uh oh case PUD_BDEV_STRATWRITE: */
		{
			struct pud_creq_write *pc_write;

			pc_write = (void *)pdr;
			printf("write to offset %llu, resid %zu\n",
			    (unsigned long long)pc_write->pm_offset,
			    pc_write->pm_resid);

			pc_write->pm_data[pc_write->pm_resid] = '\0';
			printf("got via write: %s", pc_write->pm_data);
			pdr->pdr_pth.pth_framelen =
			    sizeof(struct pud_creq_write);
			pc_write->pm_resid = 0;
		}
			break;

		case PUD_CDEV_IOCTL:
		{
			struct pud_req_ioctl *pc_ioctl;
			int *iocval;

			pc_ioctl = (void *)pdr;
			switch (pc_ioctl->pm_iocmd) {
			case INTROTOGGLE:
			case INTROTOGGLE_R:
				iocval = (int *)pc_ioctl->pm_data;
				if (*iocval < 0 || *iocval > 2) {
					pdr->pdr_rv = ERANGE;
					break;
				}

				if (*iocval == 1)
					curstr = ECHOSTR1;
				else
					curstr = ECHOSTR2;

				*iocval = 0;
				break;
			default:
				abort();
			}
		}
			break;

		default:
			abort();
		}

		n = write(fd, pdr, pdr->pdr_pth.pth_framelen);
		printf("wrote %d %d\n", n, errno);
	}
}
