/*  $NetBSD: perfused.h,v 1.3 2010/09/15 01:51:44 manu Exp $ */

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

__BEGIN_DECLS

#ifdef PERFUSE_DEBUG
void perfuse_hexdump(char *, size_t);
const char *perfuse_opname(int);
extern int perfuse_diagflags;
#endif /* PERFUSE_DEBUG */

int perfuse_open_sock(void);
void *perfuse_recv_early(int, struct sockcred *, size_t); 
int perfuse_readframe(struct puffs_usermount *, 
     struct puffs_framebuf *, int, int *);
int perfuse_writeframe(struct puffs_usermount *, 
     struct puffs_framebuf *, int, int *);
int perfuse_cmpframe(struct puffs_usermount *, 
     struct puffs_framebuf *, struct puffs_framebuf *, int *);
void perfuse_gotframe(struct puffs_usermount *, struct puffs_framebuf *);
void perfuse_fdnotify(struct puffs_usermount *, int, int);

struct fuse_out_header *perfuse_get_outhdr(perfuse_msg_t *);
struct fuse_in_header *perfuse_get_inhdr(perfuse_msg_t *);
char *perfuse_get_inpayload(perfuse_msg_t *);
char *perfuse_get_outpayload(perfuse_msg_t *);

perfuse_msg_t *perfuse_new_pb(struct puffs_usermount *, 
    puffs_cookie_t, int, size_t, const struct puffs_cred *);
int perfuse_xchg_pb(struct puffs_usermount *, perfuse_msg_t *, size_t, 
    enum perfuse_xchg_pb_reply);

__END_DECLS

#endif /* _PERFUSED_H_ */
