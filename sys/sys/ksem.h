/*
 * Copyright (c) 2002 Alfred Perlstein <alfred@FreeBSD.org>
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
 *	$FreeBSD: src/sys/posix4/_semaphore.h,v 1.1 2002/09/19 00:43:32 alfred Exp $
 */
#ifndef _SYS_KSEM_H_
#define _SYS_KSEM_H_

__BEGIN_DECLS

#ifndef _KERNEL

int _ksem_close(semid_t id);
int _ksem_destroy(semid_t id);
int _ksem_getvalue(semid_t id, int *val);
int _ksem_init(unsigned int value, semid_t *idp);
int _ksem_open(const char *name, int oflag, mode_t mode,
    unsigned int value, semid_t *idp);
int _ksem_post(semid_t id);
int _ksem_trywait(semid_t id);
int _ksem_unlink(const char *name);
int _ksem_wait(semid_t id);

#else

void ksem_init(void);

#endif /* !_KERNEL */

__END_DECLS

#endif /* _SYS_KSEM_H_ */
