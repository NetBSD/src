/*	$NetBSD: tasklet.h,v 1.7 2022/07/17 14:11:07 riastradh Exp $	*/

/*-
 * Copyright (c) 2018, 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef	_LINUX_TASKLET_H_
#define	_LINUX_TASKLET_H_

/* namespace */
#define	__tasklet_disable_sync_once	linux___tasklet_disable_sync_once
#define	__tasklet_enable		linux___tasklet_enable
#define	__tasklet_enable_sync_once	linux___tasklet_enable_sync_once
#define	__tasklet_is_enabled		linux___tasklet_is_enabled
#define	__tasklet_is_scheduled		linux___tasklet_is_scheduled
#define	tasklet_disable			linux_tasklet_disable
#define	tasklet_disable_nosync		linux_tasklet_disable_nosync
#define	tasklet_enable			linux_tasklet_enable
#define	tasklet_hi_schedule		linux_tasklet_hi_schedule
#define	tasklet_init			linux_tasklet_init
#define	tasklet_is_locked		linux_tasklet_is_locked
#define	tasklet_kill			linux_tasklet_kill
#define	tasklet_schedule		linux_tasklet_schedule
#define	tasklet_struct			linux_tasklet_struct
#define	tasklet_trylock			linux_tasklet_trylock
#define	tasklet_unlock			linux_tasklet_unlock
#define	tasklet_unlock_wait		linux_tasklet_unlock_wait

struct tasklet_struct {
	SIMPLEQ_ENTRY(tasklet_struct)	tl_entry;
	volatile unsigned		tl_state;
	volatile unsigned		tl_disablecount;
	/* begin Linux API */
	void				(*func)(unsigned long);
	unsigned long			data;
	/* end Linux API */
};

#define	DEFINE_TASKLET(name, func, data)				      \
	struct tasklet_struct name = {					      \
	    .tl_state = 0,						      \
	    .tl_disablecount = 0,					      \
	    .func = (func),						      \
	    .data = (data),						      \
	}

#define	DEFINE_TASKLET_DISABLED(name, func, data)			      \
	struct tasklet_struct name = {					      \
	    .tl_state = 0,						      \
	    .tl_disablecount = 1,					      \
	    .func = (func),						      \
	    .data = (data),						      \
	}

int	linux_tasklets_init(void);
void	linux_tasklets_fini(void);

void	tasklet_init(struct tasklet_struct *, void (*)(unsigned long),
	    unsigned long);
void	tasklet_disable_nosync(struct tasklet_struct *);
void	tasklet_disable(struct tasklet_struct *);
void	tasklet_enable(struct tasklet_struct *);
void	tasklet_schedule(struct tasklet_struct *);
void	tasklet_hi_schedule(struct tasklet_struct *);
void	tasklet_kill(struct tasklet_struct *);

bool	tasklet_is_locked(const struct tasklet_struct *);
bool	tasklet_trylock(struct tasklet_struct *);
void	tasklet_unlock(struct tasklet_struct *);
void	tasklet_unlock_wait(const struct tasklet_struct *);

/* i915 hacks */
void	__tasklet_disable_sync_once(struct tasklet_struct *);
void	__tasklet_enable_sync_once(struct tasklet_struct *);
bool	__tasklet_is_enabled(const struct tasklet_struct *);
bool	__tasklet_is_scheduled(const struct tasklet_struct *);
bool	__tasklet_enable(struct tasklet_struct *);

#endif	/* _LINUX_TASKLET_H_ */
