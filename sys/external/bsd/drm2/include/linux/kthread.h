/*	$NetBSD: kthread.h,v 1.3 2021/12/19 12:23:07 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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

#ifndef _LINUX_KTHREAD_H_
#define _LINUX_KTHREAD_H_

struct task_struct;

#define	__kthread_should_park	linux___kthread_should_park
#define	kthread_park		linux_kthread_park
#define	kthread_parkme		linux_kthread_parkme
#define	kthread_run		linux_kthread_run
#define	kthread_should_park	linux_kthread_should_park
#define	kthread_should_stop	linux_kthread_should_stop
#define	kthread_stop		linux_kthread_stop
#define	kthread_unpark		linux_kthread_unpark

struct task_struct *kthread_run(int (*)(void *), void *, const char *);

int kthread_stop(struct task_struct *);
int kthread_should_stop(void);

void kthread_park(struct task_struct *);
void kthread_unpark(struct task_struct *);
int __kthread_should_park(struct task_struct *);
int kthread_should_park(void);
void kthread_parkme(void);

int linux_kthread_init(void);
void linux_kthread_fini(void);

#endif  /* _LINUX_KTHREAD_H_ */
