/*  $NetBSD: perfused.h,v 1.5.2.1 2012/04/17 00:09:51 yamt Exp $ */

/*-
 *  Copyright (c) 2010 Emmanuel Dreyfus. All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */ 

#ifndef _PERFUSED_H_
#define _PERFUSED_H_

#include <puffs.h>
#include "../../lib/libperfuse/perfuse_if.h"
#include "fuse.h"

#define PERFUSE_MSG_T struct puffs_framebuf

#define _PATH_VAR_RUN_PERFUSE_TRACE "/var/run/perfused%s.trace"

__BEGIN_DECLS

#ifdef PERFUSE_DEBUG
void perfused_hexdump(const char *, size_t);
const char *perfused_opname(int);
extern int perfused_diagflags;
#endif /* PERFUSE_DEBUG */

int perfused_open_sock(void);
void *perfused_recv_early(int, struct sockcred *, size_t); 
int perfused_readframe(struct puffs_usermount *, 
     struct puffs_framebuf *, int, int *);
int perfused_writeframe(struct puffs_usermount *, 
     struct puffs_framebuf *, int, int *);
int perfused_cmpframe(struct puffs_usermount *, 
     struct puffs_framebuf *, struct puffs_framebuf *, int *);
void perfused_gotframe(struct puffs_usermount *, struct puffs_framebuf *);
void perfused_fdnotify(struct puffs_usermount *, int, int) __dead;

struct fuse_out_header *perfused_get_outhdr(perfuse_msg_t *);
struct fuse_in_header *perfused_get_inhdr(perfuse_msg_t *);
char *perfused_get_inpayload(perfuse_msg_t *);
char *perfused_get_outpayload(perfuse_msg_t *);
void perfused_umount(struct puffs_usermount *);

perfuse_msg_t *perfused_new_pb(struct puffs_usermount *, 
    puffs_cookie_t, int, size_t, const struct puffs_cred *);
int perfused_xchg_pb(struct puffs_usermount *, perfuse_msg_t *, size_t, 
    enum perfuse_xchg_pb_reply);

void perfused_panic(void) __dead;

__END_DECLS

#endif /* _PERFUSED_H_ */
