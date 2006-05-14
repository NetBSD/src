/* $NetBSD: kauth.h,v 1.2 2006/05/14 21:12:38 elad Exp $ */

/*-
 * Copyright (c) 2005, 2006 Elad Efrat <elad@NetBSD.org>  
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
 *      This product includes software developed by Elad Efrat.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This is based on Apple TN2127, available online at
 * http://developer.apple.com/technotes/tn2005/tn2127.html
 */

#ifndef _SYS_KAUTH_H_
#define	_SYS_KAUTH_H_

struct uucred;
struct ucred;
struct pcred;
struct proc;

/* Types. */
typedef struct kauth_scope     *kauth_scope_t;
typedef struct kauth_listener  *kauth_listener_t;
typedef uint32_t		kauth_action_t;
typedef int (*kauth_scope_callback_t)(kauth_cred_t, kauth_action_t,
				      void *, void *, void *, void *, void *);

/*
 * Possible return values for a listener.
 */
#define	KAUTH_RESULT_ALLOW	0	/* allow access */
#define	KAUTH_RESULT_DENY	1	/* deny access */
#define	KAUTH_RESULT_DEFER	2	/* let others decide */

/*
 * Scopes.
 */
#define	KAUTH_SCOPE_GENERIC	"org.netbsd.kauth.generic"
#define	KAUTH_SCOPE_PROCESS	"org.netbsd.kauth.process"

/*
 * Process scope - actions.
 */
#define	KAUTH_PROCESS_CANPTRACE	1	/* check if can attach ptrace */
#define	KAUTH_PROCESS_CANSIGNAL	2	/* check if can post signal */
#define	KAUTH_PROCESS_CANSEE	3	/* check if can see proc info */

/*
 * Generic scope - actions.
 */
#define	KAUTH_GENERIC_ISSUSER	1	/* check for super-user */

#define NOCRED ((kauth_cred_t)-1)	/* no credential available */
#define FSCRED ((kauth_cred_t)-2)	/* filesystem credential */

/*
 * Prototypes.
 */
void kauth_init(void);
kauth_scope_t kauth_register_scope(const char *, kauth_scope_callback_t, void *);
void kauth_deregister_scope(kauth_scope_t);
kauth_listener_t kauth_listen_scope(const char *, kauth_scope_callback_t, void *);
void kauth_unlisten_scope(kauth_listener_t);
int kauth_authorize_action(kauth_scope_t, kauth_cred_t, kauth_action_t, void *,
			   void *, void *, void *);

/* Default callbacks for built-in scopes. */
int kauth_authorize_cb_generic(kauth_cred_t, kauth_action_t, void *,
			       void *, void *, void *, void *);
int kauth_authorize_cb_process(kauth_cred_t, kauth_action_t, void *,
			       void *, void *, void *, void *);

/* Authorization wrappers. */
int kauth_authorize_generic(kauth_cred_t, kauth_action_t, void *);
int kauth_authorize_process(kauth_cred_t, kauth_action_t, struct proc *,
    void *, void *, void *);

/* Kauth credentials management routines. */
kauth_cred_t kauth_cred_alloc(void);
void kauth_cred_free(kauth_cred_t);
void kauth_cred_destroy(kauth_cred_t);
void kauth_cred_clone(kauth_cred_t, kauth_cred_t);
kauth_cred_t kauth_cred_dup(kauth_cred_t);
kauth_cred_t kauth_cred_copy(kauth_cred_t);

uid_t kauth_cred_getuid(kauth_cred_t);
uid_t kauth_cred_geteuid(kauth_cred_t);
uid_t kauth_cred_getsvuid(kauth_cred_t);
gid_t kauth_cred_getgid(kauth_cred_t);
gid_t kauth_cred_getegid(kauth_cred_t);
gid_t kauth_cred_getsvgid(kauth_cred_t);
int kauth_cred_ismember_gid(kauth_cred_t, gid_t, int *);
uint16_t kauth_cred_ngroups(kauth_cred_t);
gid_t kauth_cred_group(kauth_cred_t, uint16_t);

void kauth_cred_setuid(kauth_cred_t, uid_t);
void kauth_cred_seteuid(kauth_cred_t, uid_t);
void kauth_cred_setsvuid(kauth_cred_t, uid_t);
void kauth_cred_setgid(kauth_cred_t, gid_t);
void kauth_cred_setegid(kauth_cred_t, gid_t);
void kauth_cred_setsvgid(kauth_cred_t, gid_t);

void kauth_cred_hold(kauth_cred_t);
uint32_t kauth_cred_getrefcnt(kauth_cred_t);

int kauth_cred_setgroups(kauth_cred_t, gid_t *, size_t, uid_t);
int kauth_cred_getgroups(kauth_cred_t, gid_t *, size_t);

void kauth_cred_uucvt(kauth_cred_t, const struct uucred *);
int kauth_cred_uucmp(kauth_cred_t, const struct uucred *);
void kauth_cred_toucred(kauth_cred_t, struct ucred *);
void kauth_cred_topcred(kauth_cred_t, struct pcred *);

kauth_cred_t kauth_cred_get(void);

#endif	/* !_SYS_KAUTH_H_ */
