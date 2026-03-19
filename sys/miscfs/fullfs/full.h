/*	$NetBSD$	*/

#ifndef _MISCFS_FULLFS_H_
#define _MISCFS_FULLFS_H_

#include <miscfs/genfs/layer.h>

struct full_args {
	struct	layer_args	la;	/* generic layerfs args */
};
#define	fulla_target	la.target
#define	fulla_export	la.export

#ifdef _KERNEL
#include <sys/mutex.h>

struct full_mount {
	struct	layer_mount	lm;	/* generic layerfs mount stuff */

	kmutex_t	fullm_lock;
	int		fullm_mode;
	int		fullm_opmask;
	int		fullm_error;
	unsigned int	fullm_rate;
	unsigned int	fullm_doom;
};
#define	fullm_rootvp		lm.layerm_rootvp
#define	fullm_export		lm.layerm_export
#define	fullm_flags		lm.layerm_flags
#define	fullm_size		lm.layerm_size
#define	fullm_tag		lm.layerm_tag
#define	fullm_bypass		lm.layerm_bypass
#define	fullm_alloc		lm.layerm_alloc
#define	fullm_vnodeop_p		lm.layerm_vnodeop_p
#define	fullm_node_hashtbl	lm.layerm_node_hashtbl
#define	fullm_node_hash		lm.layerm_node_hash
#define	fullm_hashlock		lm.layerm_hashlock

/*
 * A cache of vnode references
 */
struct full_node {
	struct	layer_node	ln;
};
#define	full_hash	ln.layer_hash
#define	full_lowervp	ln.layer_lowervp
#define	full_vnode	ln.layer_vnode
#define	full_flags	ln.layer_flags

#define	MOUNTTOFULLMOUNT(mp) ((struct full_mount *)((mp)->mnt_data))

extern int (**full_vnodeop_p)(void *);
extern struct vfsops fullfs_vfsops;

#endif /* _KERNEL */
#endif /* _MISCFS_FULLFS_H_ */
