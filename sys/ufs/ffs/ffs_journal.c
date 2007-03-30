/*
 * Temporary journal file
 * Matt Fleming, 2007
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/queue.h>
#include <sys/kauth.h>
#include <sys/mount.h>
#include <sys/mutex.h>

#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufs_trans.h>
#include <ufs/ufs/ufsmount.h>

#include <ufs/ffs/ffs_extern.h>

/*
 * Commit a transaction to disk, currently we just bawrite
 * the buf to disk. We _MUST_ clear the B_LOCKED flag on every
 * buf that we write.
 */
int 
ffs_commit(struct ufs_trans *trans, struct gen_journal *j)
{
	struct ufs_trans_chunk *tc;

	mutex_enter(&j->gj_mutex);
	TAILQ_FOREACH(tc, &trans->t_head, tc_entries) {
		bwrite(tc->tc_data);
		CLR(tc->tc_data->b_flags, B_LOCKED);
	}
	mutex_exit(&j->gj_mutex);
	return 0;
}
