/*
 * lkminit_bsdcomp.c
 *
 * written by Iain Hibbert <plunky@rya-online.net>
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/exec.h>
#include <sys/lkm.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/mbuf.h>

#define PACKETPTR struct mbuf *
#include <net/ppp_defs.h>
#include <net/ppp-comp.h>

extern struct compressor *ppp_compressors[];
extern struct compressor ppp_bsd_compress;

static int bsdcomp_handle(struct lkm_table *lkmtp, int cmd);
int bsdcomp_lkmentry(struct lkm_table *lkmtp, int cmd, int ver);

MOD_MISC("bsdcomp")
static int
bsdcomp_handle(struct lkm_table *lkmtp, int cmd)
{
	int i = 0;

	switch (cmd) {
	case LKM_E_LOAD:
		/*
		 * Load the compressor into the master table in the first
		 * available slot, unless we already have a CI_BSD_COMPRESS
		 * type listed. Leave a space at the end, its a NULL
		 * terminated list.
		 */
		while (ppp_compressors[i]) {
			if (ppp_compressors[i]->compress_proto
			    == CI_BSD_COMPRESS)
				return EEXIST;	/* either me, or the
						 * in-kernel version */

			if (++i == PPP_COMPRESSORS_MAX - 1)
				return ENFILE;	/* no room */
		}

		ppp_compressors[i] = &ppp_bsd_compress;
		break;

	case LKM_E_UNLOAD:

		/*
		 * Find the first instance of CI_BSD_COMPRESS in the table,
		 * and * unload it. If this instance was not mine, Somebody has
		 * been Playing With Fire, and will likely Get Burnt.
		 */
		while (ppp_compressors[i]->compress_proto != CI_BSD_COMPRESS)
			i++;

		while (ppp_compressors[i] && i < (PPP_COMPRESSORS_MAX - 1)) {
			ppp_compressors[i] = ppp_compressors[i + 1];
			i++;
		}
		ppp_compressors[i] = NULL;
		break;

	case LKM_E_STAT:
		break;

	default:
		return EINVAL;
	}

	return 0;		/* success */
}

/*
 * the module entry point.
 */
int
bsdcomp_lkmentry(struct lkm_table *lkmtp, int cmd, int ver)
{
	DISPATCH(lkmtp, cmd, ver, bsdcomp_handle, bsdcomp_handle,
	    bsdcomp_handle)
}
