/*	$NetBSD: crashme.h,v 1.2 2022/09/21 10:50:11 riastradh Exp $	*/

/*
 * Copyright (c) 2018, 2019 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* header for kernel crashme node handling. */

#ifndef _SYS_CRASHME_H_
#define _SYS_CRASHME_H_

/*
 * don't mark this __dead.  crashme nodes failures should return to
 * the caller so it can be handled.
 *
 * return zero if successful, and should return to user (may be part
 * of the setup needed.)  return non-zero otherwise (will call plain
 * panic().)
 */
typedef int (*crashme_fn)(int);

typedef struct crashme_node {
	const char		*cn_name;
	const char		*cn_longname;
	crashme_fn		 cn_fn;
	int			 cn_sysctl;
	struct crashme_node	*cn_next;
} crashme_node;

int crashme_add(crashme_node *ncn);
int crashme_remove(crashme_node *ncn);

#endif /* _SYS_CRASHME_H_ */
