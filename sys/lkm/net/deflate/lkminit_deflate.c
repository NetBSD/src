/*
 * lkminit_deflate.c
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
extern struct compressor ppp_deflate;
extern struct compressor ppp_deflate_draft;

static int deflate_handle(struct lkm_table * lkmtp, int cmd);
int deflate_lkmentry(struct lkm_table * lkmtp, int cmd, int ver);

MOD_MISC("deflate")

static int
deflate_handle(struct lkm_table *lkmtp, int cmd)
{
	int i = 0;
	static int qty;	/* how many compressors we loaded */

	switch (cmd) {
	case LKM_E_LOAD:
		/*
		 * Load the compressor into the first available slot in the
		 * kernel compressors table, unless there is a CI_DEFLATE
		 * compressor already listed. The table is a NULL terminated
		 * list, so leave a space at the end.
		 */
		while (ppp_compressors[i]) {
			if (ppp_compressors[i]->compress_proto == CI_DEFLATE)
				return EEXIST;	/* either me, or the
						 * in-kernel version */

			if (++i == PPP_COMPRESSORS_MAX - 1)
				return ENFILE;	/* no room */
		}
		ppp_compressors[i] = &ppp_deflate;
		qty = 1;

		/*
		 * and add deflate_draft if we have any room left
		 */
		if (++i < PPP_COMPRESSORS_MAX - 1) {
			ppp_compressors[i] = &ppp_deflate_draft;
			qty = 2;
		}
		break;

	case LKM_E_UNLOAD:

		/*
		 * Find the first instance of CI_DEFLATE in the table, and
		 * unload it. If this instance was not mine, Somebody has been
		 * Playing With Fire, and will likely Get Burnt.
		 */
		while (ppp_compressors[i]->compress_proto != CI_DEFLATE)
			i++;

		while (ppp_compressors[i] && i < (PPP_COMPRESSORS_MAX - qty)) {
			ppp_compressors[i] = ppp_compressors[i + qty];
			i++;
		}
		ppp_compressors[i] = NULL;
		break;

	case LKM_E_STAT:
		break;

	default:
		return EINVAL;
	}

	return 0; /* success */
}

/*
 * the module entry point.
 */
int
deflate_lkmentry(struct lkm_table *lkmtp, int cmd, int ver)
{
	DISPATCH(lkmtp, cmd, ver, deflate_handle, deflate_handle,
	    deflate_handle)
}
