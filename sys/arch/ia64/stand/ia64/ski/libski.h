/*-
 * Copyright (c) 2001 Doug Rabson
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$: src/sys/boot/ia64/libski/libski.h,v 1.4 2003/02/01 22:50:08 marcel Exp $
 */

/*
 * SKI fully-qualified device descriptor
 */
struct ski_devdesc {
	struct	devsw	*d_dev;
	int		d_type;
#define	DEVT_NONE	0
#define	DEVT_DISK	1
#define	DEVT_NET	2
	union {
		struct {
			int	unit;
			int	slice;
			int	partition;
		} skidisk;
		struct {
			int	unit;	/* XXX net layer lives over these? */
		} netif;
	} d_kind;
};

extern int	ski_getdev(void **vdev, const char *devspec, const char **path);
extern char	*ski_fmtdev(void *vdev);
extern int	ski_setcurrdev(struct env_var *ev, int flags, void *value);

#define	MAXDEV	31	/* maximum number of distinct devices */

typedef unsigned long physaddr_t;

/* exported devices XXX rename? */
extern struct devsw devsw[];
extern struct netif_driver ski_net;

/* Wrapper over SKI filesystems. */
extern struct fs_ops ski_fsops;

/* this is in startup code */
extern void		delay(int);
extern void		reboot(void);

extern ssize_t		ski_copyin(const void *src, vaddr_t dest, size_t len);
extern ssize_t		ski_copyout(const vaddr_t src, void *dest, size_t len);
extern ssize_t		ski_readin(int fd, vaddr_t dest, size_t len);

extern int		ski_boot(void);

struct bootinfo;
struct preloaded_file;
extern int		bi_load(struct bootinfo *, struct preloaded_file *, char *args);

#define SSC_CONSOLE_INIT		20
#define SSC_GETCHAR			21
#define SSC_PUTCHAR			31
#define SSC_OPEN			50
#define SSC_CLOSE			51
#define SSC_READ			52
#define SSC_WRITE			53
#define SSC_GET_COMPLETION		54
#define SSC_WAIT_COMPLETION		55
#define SSC_GET_RTC			65
#define SSC_EXIT			66
#define SSC_LOAD_SYMBOLS		69
#define	SSC_SAL_SET_VECTORS		120

u_int64_t ssc(u_int64_t in0, u_int64_t in1, u_int64_t in2, u_int64_t in3,
	      int which);
