/*	$NetBSD: irix_usema.h,v 1.4 2002/05/30 05:16:10 manu Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _IRIX_USEMA_H_
#define _IRIX_USEMA_H_

#include <sys/param.h>
#include <sys/device.h>
#include <sys/queue.h>
  
#include <compat/irix/irix_types.h>
#include <compat/irix/irix_exec.h>

extern const dev_t irix_usemaclonedev;

extern struct vfsops irix_usema_dummy_vfsops;
void irix_usema_dummy_vfs_init __P((void));
extern const struct vnodeopv_desc * const irix_usema_vnodeopv_descs[];
extern const struct vnodeopv_desc irix_usema_opv_desc;
extern int (**irix_usema_vnodeop_p) __P((void *));
extern const struct vnodeopv_entry_desc irix_usema_vnodeop_entries[];


void	irix_usemaattach __P((struct device *, struct device *, void *));
int	irix_usemaopen	__P((dev_t, int, int, struct proc *));
int	irix_usemaread	__P((dev_t, struct uio *, int));
int	irix_usemawrite	__P((dev_t, struct uio *, int)); 
int	irix_usemapoll	__P((dev_t, int, struct proc *));
int	irix_usemaioctl	__P((dev_t, u_long, caddr_t, int, struct proc *));
int	irix_usemaclose	__P((dev_t, int, int, struct proc *)); 

int	irix_usema_close	__P((void *));
int	irix_usema_access	__P((void *));
int	irix_usema_getattr	__P((void *));
int	irix_usema_setattr	__P((void *));
int	irix_usema_fcntl	__P((void *));
int	irix_usema_ioctl	__P((void *));
int	irix_usema_poll		__P((void *));
int	irix_usema_inactive	__P((void *));

#ifdef DEBUG_IRIX
void	irix_usema_debug	__P((void));
#endif

#define IRIX_USEMADEV_MINOR	1
#define IRIX_USEMACLNDEV_MINOR	0

/* Semaphore internal structure: undocumented in IRIX */
struct irix_semaphore {
	int is_val;	/* Sempahore value */	
	int is_uk1;	/* unknown, usually small integer < 3000  */
	int is_uk2;	/* metric, debug or history pointer ? */
	int is_uk3;	/* unknown, usually equal to 0 */
	int is_uk4;	/* unknown, usually equal to 0 */
	int is_shid;	/* unique ID for the shared arena ? */
	int is_oid;	/* owned id? usually equal to -1 */
	int is_uk7;	/* unknown, usually equal to -1 */
	int is_uk8;	/* unknown, usually equal to 0 */
	int is_uk9;	/* unknown, usually equal to 0 */
	int is_uk10;	/* semaphore page base address ? */
	int is_uk12;	/* unknown, usually equal to 0 */
	int is_uk13;	/* metric, debug or history pointer ? */
	int is_uk14;	/* padding? */
};

struct irix_usema_idaddr {
	int iui_uk1;	/* unknown, usually equal to 0 */
	int *iui_oidp;	/* pointer to is_oid field in struct irix_semaphore */
};

/* waigint processes list */
struct irix_waiting_proc_rec {
	TAILQ_ENTRY(irix_waiting_proc_rec) iwpr_list;
	struct proc *iwpr_p;
};

/* semaphore list, their vnode counterparts, and waiting processes lists */
struct irix_usema_rec {
	LIST_ENTRY(irix_usema_rec) iur_list;
	struct vnode *iur_vn;
	struct irix_semaphore *iur_sem;
	int iur_shid;
	struct proc *iur_p;
	int iur_waiting_count;
	TAILQ_HEAD(iur_waiting_p, irix_waiting_proc_rec) iur_waiting_p;
	TAILQ_HEAD(iur_released_p, irix_waiting_proc_rec) iur_released_p;
};

/* From IRIX's <sys/usioctl.h> */
#define IRIX_USEMADEV		"/dev/usema"
#define IRIX_USEMACLNDEV	"/dev/usemaclone"

#define IRIX_UIOC	('u' << 16 | 's' << 8)
#define IRIX_UIOC_MASK	0x00ffff00

#define IRIX_UIOCATTACHSEMA	(IRIX_UIOC|2)
#define IRIX_UIOCBLOCK		(IRIX_UIOC|3)
#define IRIX_UIOCABLOCK		(IRIX_UIOC|4)
#define IRIX_UIOCNOIBLOCK	(IRIX_UIOC|5)
#define IRIX_UIOCUNBLOCK	(IRIX_UIOC|6)
#define IRIX_UIOCAUNBLOCK	(IRIX_UIOC|7)
#define IRIX_UIOCINIT		(IRIX_UIOC|8)
#define IRIX_UIOCGETSEMASTATE 	(IRIX_UIOC|9)
#define IRIX_UIOCABLOCKPID	(IRIX_UIOC|10)
#define IRIX_UIOCADDPID		(IRIX_UIOC|11)
#define IRIX_UIOCABLOCKQ	(IRIX_UIOC|12)
#define IRIX_UIOCAUNBLOCKQ	(IRIX_UIOC|13)
#define IRIX_UIOCIDADDR		(IRIX_UIOC|14)
#define IRIX_UIOCSETSEMASTATE 	(IRIX_UIOC|15)
#define IRIX_UIOCGETCOUNT	(IRIX_UIOC|16) 

struct irix_usattach_s {
	irix_dev_t	us_dev;
	void		*us_handle;
};
typedef struct irix_usattach_s irix_usattach_t;

struct irix_irix5_usattach_s {
	__uint32_t	us_dev;
	__uint32_t	us_handle;
};
typedef struct irix_irix5_usattach_s irix_irix5_usattach_t;

struct irix_ussemastate_s {
	int	ntid;
	int	nprepost;
	int	nfilled;
	int	nthread;
	struct irix_ussematidstate_s {
		pid_t	pid;
		int	tid;
		int	count;
	} *tidinfo;
};
typedef struct irix_ussemastate_s irix_ussemastate_t;
typedef struct irix_ussematidstate_s irix_ussematidstate_t;


/* usync_fcntl() commands, undocumented in IRIX */
#define IRIX_USYNC_BLOCK		1
#define IRIX_USYNC_INTR_BLOCK		2
#define IRIX_USYNC_UNBLOCK_ALL		3
#define IRIX_USYNC_UNBLOCK		4
#define IRIX_USYNC_NOTIFY_REGISTER	5
#define IRIX_USYNC_NOTIFY		6
#define IRIX_USYNC_NOTIFY_DELETE	7
#define IRIX_USYNC_NOTIFY_CLEAR		8
#define IRIX_USYNC_GET_STATE		11

struct irix_usync_arg {
	int iua_uk0;	/* unknown, usually small integer around 1000 */
	int iua_uk1;	/* unknown, usually pointer to code in libc */
	int iua_uk2;	/* unknown, usually null */
	struct irix_semaphore *iua_sem;	/* semaphore address */
};

#endif /* _IRIX_USEMA_H_ */
