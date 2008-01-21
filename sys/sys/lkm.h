/*	$NetBSD: lkm.h,v 1.37.6.5 2008/01/21 09:47:51 yamt Exp $	*/

/*
 * Header file used by loadable kernel modules and loadable kernel module
 * utilities.
 *
 * 23 Jan 93	Terry Lambert		Original
 *
 * Copyright (c) 1992 Terrence R. Lambert.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Terrence R. Lambert.
 * 4. The name Terrence R. Lambert may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TERRENCE R. LAMBERT ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE TERRENCE R. LAMBERT BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SYS_LKM_H_
#define _SYS_LKM_H_

#include <sys/queue.h>

/*
 * Supported module types
 */
typedef enum loadmod {
	LM_SYSCALL,
	LM_VFS,
	LM_DEV,
	LM_STRMOD,
	LM_EXEC,
	LM_COMPAT,
	LM_MISC,
	LM_DRV
} MODTYPE;

#define MODTYPE_NAMES \
	"SYSCALL", \
	"VFS", \
	"DEV", \
	"STRMOD", \
	"EXEC", \
	"COMPAT", \
	"MISC", \
	"DRV"

/*
 * Version of module interface. Bump if kernel structures or API affecting
 * LKM modules change, unless the kernel version is bumped at the
 * same time too.
 */
#define	LKM_VERSION	2

#define	MAXLKMNAME	32

/****************************************************************************/

#ifdef _KERNEL

/*
 * Any module (to get type and name info without knowing type)
 */
struct lkm_any {
	MODTYPE	lkm_type;
	const char *lkm_name;
	u_long	lkm_offset;
	u_int	lkm_modver;
	u_int	lkm_sysver;
	const char *lkm_envver;
};


/*
 * Loadable system call
 */
struct lkm_syscall {
	struct lkm_any mod;
	struct sysent	*lkm_sysent;
	struct sysent	lkm_oldent;	/* save area for unload */
};

/*
 * Loadable file system
 */
struct lkm_vfs {
	struct lkm_any mod;
	struct vfsops	*lkm_vfsops;
};

/*
 * Loadable device driver
 */
struct lkm_dev {
	struct lkm_any mod;
	const char *lkm_devname;
	const struct bdevsw	*lkm_bdev;
	int lkm_bdevmaj;
	const struct cdevsw	*lkm_cdev;
	int lkm_cdevmaj;
};

#ifdef STREAMS
/*
 * Loadable streams module
 */
struct lkm_strmod {
	struct lkm_any mod;
	/*
	 * Removed: future release
	 */
};
#endif

/*
 * Exec loader
 */
struct lkm_exec {
	struct lkm_any mod;
	struct execsw	*lkm_execsw;
	const char *lkm_emul;
};

/*
 * Compat (emulation) loader
 */
struct lkm_compat {
	struct lkm_any mod;
	const struct emul	*lkm_compat;
};

/*
 * Miscellaneous module (complex load/unload, potentially complex stat)
 */
struct lkm_misc {
	struct lkm_any mod;
};

/*
 * Driver module
 */
struct lkm_drv {
	struct lkm_any mod;
	struct cfdriver **lkm_cd;
	const struct cfattachlkminit *lkm_cai;
	struct cfdata *lkm_cf;
};

/*
 * Generic reference ala XEvent to allow single entry point in the xxxinit()
 * routine.
 */
union lkm_generic {
	struct lkm_any		*lkm_any;
	struct lkm_syscall	*lkm_syscall;
	struct lkm_vfs		*lkm_vfs;
	struct lkm_dev		*lkm_dev;
#ifdef STREAMS
	struct lkm_strmod	*lkm_strmod;
#endif
	struct lkm_exec		*lkm_exec;
	struct lkm_compat	*lkm_compat;
	struct lkm_misc		*lkm_misc;
	struct lkm_drv		*lkm_drv;
};

/*
 * Per module information structure
 */
struct lkm_table {
	char	refcnt;		/* Reference count */
	char	forced;		/* Forced load, skipping compatibility check */

	int	(*entry)(struct lkm_table *, int, int);/* entry function */
	union lkm_generic	private;	/* module private data */

	u_long	size;
	u_long	offset;
	u_long	area;

				/* ddb support */
        u_long  syms;		/* start of symbol table */
	u_long	sym_size;	/* size of symbol table (syms+strings) */
	u_long	sym_offset;	/* offset of next symbol chunk */
	u_long	sym_symsize;	/* size of symbol part only */

	int	id;		/* Identifier */
	TAILQ_ENTRY(lkm_table) link;
};


#define	LKM_E_LOAD	1
#define	LKM_E_UNLOAD	2
#define	LKM_E_STAT	3


#define	MOD_SYSCALL(name,callslot,sysentp)	\
	static struct lkm_syscall _module = {	\
		{ LM_SYSCALL, name, callslot,	\
		  LKM_VERSION, __NetBSD_Version__, _LKM_ENV_VERSION },	\
		sysentp				\
	};

#define	MOD_VFS(name,vfsslot,vfsopsp)		\
	static struct lkm_vfs _module = {	\
		{ LM_VFS, name, (u_long)vfsslot,	\
		  LKM_VERSION, __NetBSD_Version__, _LKM_ENV_VERSION },	\
		vfsopsp				\
	};

#define	MOD_DEV(name,devname,bdevp,bdevm,cdevp,cdevm)	\
	static struct lkm_dev _module = {	\
		{ LM_DEV, name, (u_long)-1,		\
		  LKM_VERSION, __NetBSD_Version__, _LKM_ENV_VERSION },	\
		devname,			\
		bdevp,				\
		bdevm,				\
		cdevp,				\
		cdevm,				\
	};

#define	MOD_COMPAT(name,compatslot,emulp)	\
	static struct lkm_compat _module = {	\
		{ LM_COMPAT, name, (u_long)compatslot,	\
		  LKM_VERSION, __NetBSD_Version__, _LKM_ENV_VERSION },	\
		emulp				\
	};

#define	MOD_EXEC(name,execslot,execsw,emul)	\
	static struct lkm_exec _module = {	\
		{ LM_EXEC, name, (u_long)execslot,	\
		  LKM_VERSION, __NetBSD_Version__, _LKM_ENV_VERSION },	\
		execsw,				\
		emul				\
	};

#define	MOD_MISC(name)				\
	static struct lkm_misc _module = {	\
		{ LM_MISC, name, (u_long)-1,		\
		  LKM_VERSION, __NetBSD_Version__, _LKM_ENV_VERSION },	\
	};

#define	MOD_DRV(name,drvs,atts,cfdata)	\
	static struct lkm_drv _module = {	\
		{ LM_DRV, name, (u_long)-1,		\
		  LKM_VERSION, __NetBSD_Version__, _LKM_ENV_VERSION },	\
		drvs, atts, cfdata		\
	};

/*
 * Environment encoding, for LKM<->kernel compatibility check.
 */
#ifdef DEBUG
#define _LKM_E_DEBUG		",DEBUG"
#else
#define _LKM_E_DEBUG		""
#endif

#ifdef MALLOCLOG
#define _LKM_E_MALLOCLOG	",MALLOCLOG"
#else
#define _LKM_E_MALLOCLOG	""
#endif

#define	_LKM_ENV_VERSION	_LKM_E_DEBUG _LKM_E_MALLOCLOG

int lkm_nofunc(struct lkm_table *, int);
int lkmexists(struct lkm_table *);
int lkmdispatch(struct lkm_table *, int);

/*
 * LKM_DISPATCH -- body function for use in module entry point function;
 * generally, the function body will consist entirely of a single
 * LKM_DISPATCH line.
 *
 * If load/unload/stat are called on each corresponding entry instance.
 * If no function is desired for load/stat/unload, lkm_nofunc() should
 * be specified.  "cmd" is passed to each function so that a single
 * function can be used if desired.
 */
#define	LKM_DISPATCH(lkmtp, cmd, envdep, load, unload, stat)		\
	switch (cmd) {							\
	int	error;							\
	case LKM_E_LOAD:						\
		lkmtp->private.lkm_any = (void *)&_module;		\
		if ((error = lkmdispatch(lkmtp, cmd)) != 0)		\
			return error;					\
		if ((error = load(lkmtp, cmd)) != 0)			\
			(void)lkmdispatch(lkmtp, LKM_E_UNLOAD);		\
		return error;						\
		break;							\
	case LKM_E_UNLOAD:						\
		if ((error = unload(lkmtp, cmd)) != 0)			\
			return error;					\
		return lkmdispatch(lkmtp, cmd);				\
		break;							\
	case LKM_E_STAT:						\
		if ((error = stat(lkmtp, cmd)) != 0)			\
			return error;					\
		return lkmdispatch(lkmtp, cmd);				\
		break;							\
	}								\
	return (0);

/* remap the old macro for backward source compatibility */
#define	DISPATCH(lkmtp, cmd, ver, att, det, stat)	\
	LKM_DISPATCH(lkmtp, cmd, NULL, att, det, stat)

extern struct vm_map *lkm_map;

#endif /* _KERNEL */

/****************************************************************************/

/*
 * IOCTL's recognized by /dev/lkm
 */
#define	LMLOADBUF	_IOW('K', 1, struct lmc_loadbuf)
#define	LMUNRESRV	_IO('K', 2)
#define	LMREADY		_IOW('K', 3, u_long)
#define	LMRESERV	_IOWR('K', 4, struct lmc_resrv)

#define	LMLOAD		_IOW('K', 9, struct lmc_load)
#define	LMUNLOAD	_IOWR('K', 10, struct lmc_unload)
#define	LMSTAT		_IOWR('K', 11, struct lmc_stat)
#define	LMLOADSYMS	_IOW('K', 12, struct lmc_loadbuf)
#define	LMFORCE		_IOW('K', 13, u_long)

#define	MODIOBUF	512		/* # of bytes at a time to loadbuf */

/*
 * IOCTL arguments
 */


/*
 * Reserve a page-aligned block of kernel memory for the module
 */
struct lmc_resrv {
	u_long	size;		/* IN: size of module to reserve */
	char	*name;		/* IN: name (must be provided */
	int	slot;		/* OUT: allocated slot (module ID) */
	u_long	addr;		/* OUT: Link-to address */
				/* ddb support */
	u_long  xxx_unused1;	/* unused */
	u_long	sym_size;	/* IN: total size of symbol table */
	u_long	xxx_unused2;	/* unused */
	u_long	sym_symsize;	/* IN: size of symbol portion of symtable */
	u_long	sym_addr;	/* OUT: address of symbol table */
};

/*
 * Copy a buffer at a time into the allocated area in the kernel; writes
 * are assumed to occur contiguously.
 */
struct lmc_loadbuf {
	int	cnt;		/* IN: # of chars pointed to by data */
	char	*data;		/* IN: pointer to data buffer */
};


/*
 * Load a module (assumes it's been mmapped to address before call)
 */
struct lmc_load {
	void *	address;	/* IN: user space mmap address */
	int	status;		/* OUT: status of operation */
	int	id;		/* OUT: module ID if loaded */
};

/*
 * Unload a module (by name/id)
 */
struct lmc_unload {
	int	id;		/* IN: module ID to unload */
	char	*name;		/* IN: module name to unload if id -1 */
	int	status;		/* OUT: status of operation */
};


/*
 * Get module information for a given id (or name if id == -1).
 */
struct lmc_stat {
	int	id;			/* IN: module ID to unload */
	char	name[MAXLKMNAME];	/* IN/OUT: name of module */
	u_long	offset;			/* OUT: target table offset */
	MODTYPE	type;			/* OUT: type of module */
	u_long	area;			/* OUT: kernel load addr */
	u_long	size;			/* OUT: module size (pages) */
	u_long	private;		/* OUT: module private data */
	int	ver;			/* OUT: lkm compile version */
};

#define	LKM_MAKEMAJOR(b, c)	((((b) & 0xffff) << 16) | ((c) & 0xffff))
#define	LKM_BLOCK_MAJOR(v)	(int)((int16_t)(((uint32_t)(v) >> 16) & 0xffff))
#define	LKM_CHAR_MAJOR(v)	(int)((int16_t)((v) & 0xffff))

#endif	/* !_SYS_LKM_H_ */
