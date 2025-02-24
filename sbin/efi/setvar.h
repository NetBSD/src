/* $NetBSD: setvar.h,v 1.1 2025/02/24 13:47:57 christos Exp $ */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SETVAR_H_
#define _SETVAR_H_

#ifndef lint
__RCSID("$NetBSD: setvar.h,v 1.1 2025/02/24 13:47:57 christos Exp $");
#endif /* not lint */

int del_bootnext(int fd);
int del_bootorder(int fd, const char *);
int del_bootorder_dups(int, const char *);
int del_timeout(int);
int del_variable(int, const char *, uint16_t);
int prefix_bootorder(int, const char *, const char *, uint16_t);
int remove_bootorder(int, const char *, const char *, uint16_t);
int set_active(int, const char *, uint16_t, bool);
int set_bootnext(int, uint16_t);
int set_bootorder(int, const char *, const char *);
int set_timeout(int, uint16_t);

#endif /* _SETVAR_H_ */
