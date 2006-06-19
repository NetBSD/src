/* $NetBSD: kern_auth.c,v 1.2.2.1 2006/06/19 04:07:15 chap Exp $ */

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
 * Todo:
 *   - Garbage collection to pool_put() unused scopes/listeners.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/ucred.h>
#include <sys/pool.h>
#include <sys/kauth.h>
#include <sys/acct.h>
#include <sys/sysctl.h>

/* 
 * Credentials.
 */
struct kauth_cred {
	struct simplelock cr_lock;	/* lock */
	uint32_t cr_refcnt;		/* reference count */
	uid_t cr_uid;			/* user id */
	uid_t cr_euid;			/* effective user id */
	uid_t cr_svuid;			/* saved effective user id */
	gid_t cr_gid;			/* group id */
	gid_t cr_egid;			/* effective group id */
	gid_t cr_svgid;			/* saved effective group id */
	uint16_t cr_ngroups;		/* number of groups */
	gid_t cr_groups[NGROUPS];	/* group memberships */
};

/*
 * Listener.
 */
struct kauth_listener {
	kauth_scope_callback_t		func;		/* callback */
	kauth_scope_t			scope;		/* scope backpointer */
	uint32_t			refcnt;		/* reference count */
	SIMPLEQ_ENTRY(kauth_listener)	listener_next;	/* listener list */
};

/*
 * Scope.
 */
struct kauth_scope {
	const char		       *id;		/* scope name */
	void			       *cookie;		/* user cookie */
	uint32_t			nlisteners;	/* # of listeners */
	SIMPLEQ_HEAD(, kauth_listener)	listenq;	/* listener list */
	SIMPLEQ_ENTRY(kauth_scope)	next_scope;	/* scope list */
};

static POOL_INIT(kauth_scope_pool, sizeof(struct kauth_scope), 0, 0, 0,
	  "kauth_scopepl", &pool_allocator_nointr);
static POOL_INIT(kauth_listener_pool, sizeof(struct kauth_listener), 0, 0, 0,
	  "kauth_listenerpl", &pool_allocator_nointr);
static POOL_INIT(kauth_cred_pool, sizeof(struct kauth_cred), 0, 0, 0,
	  "kauth_credpl", &pool_allocator_nointr);

/* List of scopes and its lock. */
static SIMPLEQ_HEAD(, kauth_scope) scope_list;
static struct simplelock scopes_lock;

/* Built-in scopes: generic, process. */
static kauth_scope_t kauth_builtin_scope_generic;
static kauth_scope_t kauth_builtin_scope_process;

/* Allocate new, empty kauth credentials. */
kauth_cred_t
kauth_cred_alloc(void)
{
	kauth_cred_t cred;

	cred = pool_get(&kauth_cred_pool, PR_WAITOK);
	memset(cred, 0, sizeof(*cred));
	simple_lock_init(&cred->cr_lock);
	cred->cr_refcnt = 1;

	return (cred);
}

/* Increment reference count to cred. */
void
kauth_cred_hold(kauth_cred_t cred)
{
	KASSERT(cred != NULL);

        simple_lock(&cred->cr_lock);
        cred->cr_refcnt++;
        simple_unlock(&cred->cr_lock);
}

/* Decrease reference count to cred. If reached zero, free it. */
void
kauth_cred_free(kauth_cred_t cred)
{
	KASSERT(cred != NULL);

	simple_lock(&cred->cr_lock);
	cred->cr_refcnt--;
	simple_unlock(&cred->cr_lock);

	if (cred->cr_refcnt == 0)
		pool_put(&kauth_cred_pool, cred);
}

void
kauth_cred_clone(kauth_cred_t from, kauth_cred_t to)
{
	KASSERT(from != NULL);
	KASSERT(to != NULL);

	to->cr_uid = from->cr_uid;
	to->cr_euid = from->cr_euid;
	to->cr_svuid = from->cr_svuid;
	to->cr_gid = from->cr_gid;
	to->cr_egid = from->cr_egid;
	to->cr_svgid = from->cr_svgid;
	to->cr_ngroups = from->cr_ngroups;
	memcpy(to->cr_groups, from->cr_groups,
	       sizeof(to->cr_groups));
}

/*
 * Duplicate cred and return a new kauth_cred_t.
 */
kauth_cred_t
kauth_cred_dup(kauth_cred_t cred)
{
	kauth_cred_t new_cred;

	KASSERT(cred != NULL);

	new_cred = kauth_cred_alloc();

	kauth_cred_clone(cred, new_cred);

	return (new_cred);
}

/*
 * Similar to crcopy(), only on a kauth_cred_t.
 * XXX: Is this even needed? [kauth_cred_copy]
 */
kauth_cred_t
kauth_cred_copy(kauth_cred_t cred)
{
	kauth_cred_t new_cred;

	KASSERT(cred != NULL);

	/* If the provided credentials already have one reference, use them. */
	if (cred->cr_refcnt == 1)
		return (cred);

	new_cred = kauth_cred_alloc();

	kauth_cred_clone(cred, new_cred);

	kauth_cred_free(cred);

	return (new_cred);
}

uid_t
kauth_cred_getuid(kauth_cred_t cred)
{
	KASSERT(cred != NULL);

	return (cred->cr_uid);
}

uid_t
kauth_cred_geteuid(kauth_cred_t cred)
{
	KASSERT(cred != NULL);

	return (cred->cr_euid);
}

uid_t
kauth_cred_getsvuid(kauth_cred_t cred)
{
	KASSERT(cred != NULL);

	return (cred->cr_svuid);
}

gid_t
kauth_cred_getgid(kauth_cred_t cred)
{
	KASSERT(cred != NULL);

	return (cred->cr_gid);
}

gid_t
kauth_cred_getegid(kauth_cred_t cred)
{
	KASSERT(cred != NULL);

	return (cred->cr_egid);
}

gid_t
kauth_cred_getsvgid(kauth_cred_t cred)
{
	KASSERT(cred != NULL);

	return (cred->cr_svgid);
}

void
kauth_cred_setuid(kauth_cred_t cred, uid_t uid)
{
	KASSERT(cred != NULL);

	cred->cr_uid = uid;
}

void
kauth_cred_seteuid(kauth_cred_t cred, uid_t uid)
{
	KASSERT(cred != NULL);

	cred->cr_euid = uid;
}

void
kauth_cred_setsvuid(kauth_cred_t cred, uid_t uid)
{
	KASSERT(cred != NULL);

	cred->cr_svuid = uid;
}

void
kauth_cred_setgid(kauth_cred_t cred, gid_t gid)
{
	KASSERT(cred != NULL);

	cred->cr_gid = gid;
}

void
kauth_cred_setegid(kauth_cred_t cred, gid_t gid)
{
	KASSERT(cred != NULL);

	cred->cr_egid = gid;
}

void
kauth_cred_setsvgid(kauth_cred_t cred, gid_t gid)
{
	KASSERT(cred != NULL);

	cred->cr_svgid = gid;
}

/* Checks if gid is a member of the groups in cred. */
int
kauth_cred_ismember_gid(kauth_cred_t cred, gid_t gid, int *resultp)
{
	int i;

	KASSERT(cred != NULL);
	KASSERT(resultp != NULL);

	*resultp = 0;

	for (i = 0; i < cred->cr_ngroups; i++)
		if (cred->cr_groups[i] == gid) {
			*resultp = 1;
			break;
		}

	return (0);
}

uint16_t
kauth_cred_ngroups(kauth_cred_t cred)
{
	KASSERT(cred != NULL);

	return (cred->cr_ngroups);
}

/*
 * Return the group at index idx from the groups in cred.
 */
gid_t
kauth_cred_group(kauth_cred_t cred, uint16_t idx)
{
	KASSERT(cred != NULL);
	KASSERT(idx < cred->cr_ngroups);

	return (cred->cr_groups[idx]);
}

/* XXX elad: gmuid is unused for now. */
int
kauth_cred_setgroups(kauth_cred_t cred, gid_t *grbuf, size_t len, uid_t gmuid)
{
	KASSERT(cred != NULL);
	KASSERT(len < sizeof(cred->cr_groups) / sizeof(cred->cr_groups[0]));

	simple_lock(&cred->cr_lock);

	if (len)
		memcpy(cred->cr_groups, grbuf, len * sizeof(cred->cr_groups[0]));
	memset(cred->cr_groups + len, 0xff,
	    sizeof(cred->cr_groups) - (len * sizeof(cred->cr_groups[0])));

	cred->cr_ngroups = len;

	simple_unlock(&cred->cr_lock);

	return (0);
}

int
kauth_cred_getgroups(kauth_cred_t cred, gid_t *grbuf, size_t len)
{
	KASSERT(cred != NULL);
	KASSERT(len <= cred->cr_ngroups);

	memset(grbuf, 0xff, sizeof(*grbuf) * len);
	simple_lock(&cred->cr_lock);
	memcpy(grbuf, cred->cr_groups, sizeof(*grbuf) * len);
	simple_unlock(&cred->cr_lock);

	return (0);
}

/*
 * Match uids in two credentials. Checks if cred1 can access stuff owned by
 * cred2.
 * XXX: root bypasses this!
 */
static int
kauth_cred_uidmatch(kauth_cred_t cred1, kauth_cred_t cred2)
{
	KASSERT(cred1 != NULL);
	KASSERT(cred2 != NULL);

	/* Are we root? */
	if (cred1->cr_euid == 0)
		return (1);

	if (cred1->cr_uid == cred2->cr_uid ||
	    cred1->cr_euid == cred2->cr_uid ||
	    cred1->cr_uid == cred2->cr_euid ||
	    cred1->cr_euid == cred2->cr_euid)
		return (1);

	return (0);
}

uint32_t
kauth_cred_getrefcnt(kauth_cred_t cred)
{
	KASSERT(cred != NULL);

	return (cred->cr_refcnt);
}

/*
 * Convert userland credentials (struct uucred) to kauth_cred_t.
 * XXX: For NFS code.
 */
void
kauth_cred_uucvt(kauth_cred_t cred, const struct uucred *uuc)
{
	KASSERT(cred != NULL);
	KASSERT(uuc != NULL);

	cred->cr_refcnt = 1;
	cred->cr_uid = uuc->cr_uid;
	cred->cr_euid = uuc->cr_uid;
	cred->cr_svuid = uuc->cr_uid;
	cred->cr_gid = uuc->cr_gid;
	cred->cr_egid = uuc->cr_gid;
	cred->cr_svgid = uuc->cr_gid;
	cred->cr_ngroups = min(uuc->cr_ngroups, NGROUPS);
	kauth_cred_setgroups(cred, __UNCONST(uuc->cr_groups),
	    cred->cr_ngroups, -1);
}

/*
 * Compare kauth_cred_t and uucred credentials.
 * XXX: Modelled after crcmp() for NFS.
 */
int
kauth_cred_uucmp(kauth_cred_t cred, const struct uucred *uuc)
{
	KASSERT(cred != NULL);
	KASSERT(uuc != NULL);

	if (cred->cr_euid == uuc->cr_uid &&
	    cred->cr_egid == uuc->cr_gid &&
	    cred->cr_ngroups == uuc->cr_ngroups) {
		int i;

		/* Check if all groups from uuc appear in cred. */
		for (i = 0; i < uuc->cr_ngroups; i++) {
			int ismember;

			ismember = 0;
			if (kauth_cred_ismember_gid(cred, uuc->cr_groups[i],
			    &ismember) != 0 || !ismember)
				return (1);
		}

		return (0);
	}

	return (1);
}

/*
 * Make a struct ucred out of a kauth_cred_t.
 * XXX: For sysctl.
 */
void
kauth_cred_toucred(kauth_cred_t cred, struct ucred *uc)
{
	KASSERT(cred != NULL);
	KASSERT(uc != NULL);

	uc->cr_uid = cred->cr_euid;
	uc->cr_gid = cred->cr_egid;
	uc->cr_ngroups = min(cred->cr_ngroups,
			     sizeof(uc->cr_groups) / sizeof(uc->cr_groups[0]));
	memcpy(uc->cr_groups, cred->cr_groups,
	       uc->cr_ngroups * sizeof(uc->cr_groups[0]));
}

/*
 * Make a struct pcred out of a kauth_cred_t.
 * XXX: For sysctl.
 */
void
kauth_cred_topcred(kauth_cred_t cred, struct pcred *pc)
{
	KASSERT(cred != NULL);
	KASSERT(pc != NULL);

	pc->pc_ucred = (struct ucred *)cred; /* XXX this is just wrong */
	pc->p_ruid = cred->cr_uid;
	pc->p_svuid = cred->cr_svuid;
	pc->p_rgid = cred->cr_gid;
	pc->p_svgid = cred->cr_svgid;
	pc->p_refcnt = cred->cr_refcnt;
}

/*
 * Return kauth_cred_t for the current process.
 */
kauth_cred_t
kauth_cred_get(void)
{
	return (curproc->p_cred);
}

/*
 * Returns a scope matching the provided id.
 * Requires the scope list lock to be held by the caller.
 */
static kauth_scope_t
kauth_ifindscope(const char *id)
{
	kauth_scope_t scope;

	/* XXX: assert lock on scope list? */

	scope = NULL;
	SIMPLEQ_FOREACH(scope, &scope_list, next_scope) {
		if (strcmp(scope->id, id) == 0)
			break;
	}

	return (scope);
}

/*
 * Register a new scope.
 *
 * id - identifier for the scope
 * callback - the scope's default listener
 * cookie - cookie to be passed to the listener(s)
 */
kauth_scope_t
kauth_register_scope(const char *id, kauth_scope_callback_t callback,
		     void *cookie)
{
	kauth_scope_t scope;
	kauth_listener_t listener;

	/* Sanitize input */
	if (id == NULL || callback == NULL)
		return (NULL);

	/* Allocate space for a new scope and listener. */
	scope = pool_get(&kauth_scope_pool, PR_WAITOK);
	listener = pool_get(&kauth_listener_pool, PR_WAITOK);

	/* Acquire scope list lock. */
	simple_lock(&scopes_lock);

	/* Check we don't already have a scope with the same id */
	if (kauth_ifindscope(id) != NULL) {
		simple_unlock(&scopes_lock);

		pool_put(&kauth_scope_pool, scope);
		pool_put(&kauth_listener_pool, listener);

		return (NULL);
	}

	/* Initialize new scope with parameters */
	scope->id = id;
	scope->cookie = cookie;
	scope->nlisteners = 1;

	/* Add default listener */
	listener->func = callback;
	listener->scope = scope;
	listener->refcnt = 0;
	SIMPLEQ_INIT(&scope->listenq);
	SIMPLEQ_INSERT_HEAD(&scope->listenq, listener, listener_next);

	/* Insert scope to scopes list */
	if (SIMPLEQ_EMPTY(&scope_list))
		SIMPLEQ_INSERT_HEAD(&scope_list, scope, next_scope);
	else
		SIMPLEQ_INSERT_TAIL(&scope_list, scope, next_scope);

	simple_unlock(&scopes_lock);

	return (scope);
}

/*
 * Initialize the kernel authorization subsystem.
 *
 * Initialize the scopes list lock.
 * Register built-in scopes: generic, process.
 */
void
kauth_init(void)
{
	simple_lock_init(&scopes_lock);

	/* Register generic scope. */
	kauth_builtin_scope_generic = kauth_register_scope(KAUTH_SCOPE_GENERIC,
	    kauth_authorize_cb_generic, NULL);

	/* Register process scope. */
	kauth_builtin_scope_process = kauth_register_scope(KAUTH_SCOPE_PROCESS,
	    kauth_authorize_cb_process, NULL);
}

/*
 * Deregister a scope.
 * Requires scope list lock to be held by the caller.
 *
 * scope - the scope to deregister
 */
void
kauth_deregister_scope(kauth_scope_t scope)
{
	if (scope != NULL) {
		/* Remove scope from list */
		SIMPLEQ_REMOVE(&scope_list, scope, kauth_scope, next_scope);
	}
}

/*
 * Register a listener.
 *
 * id - scope identifier.
 * callback - the callback routine for the listener.
 * cookie - cookie to pass unmoidfied to the callback.
 */
kauth_listener_t
kauth_listen_scope(const char *id, kauth_scope_callback_t callback,
		   void *cookie)
{
	kauth_scope_t scope;
	kauth_listener_t listener;

	/* Find scope struct */
	simple_lock(&scopes_lock);
	scope = kauth_ifindscope(id);
	simple_unlock(&scopes_lock);
	if (scope == NULL)
		return (NULL);

	/* Allocate listener */
	listener = pool_get(&kauth_listener_pool, PR_WAITOK);

	/* Initialize listener with parameters */
	listener->func = callback;
	listener->refcnt = 0;

	/* Add listener to scope */
	SIMPLEQ_INSERT_TAIL(&scope->listenq, listener, listener_next);

	/* Raise number of listeners on scope. */
	scope->nlisteners++;
	listener->scope = scope;

	return (listener);
}

/*
 * Deregister a listener.
 *
 * listener - listener reference as returned from kauth_listen_scope().
 */
void
kauth_unlisten_scope(kauth_listener_t listener)
{
	if (listener != NULL) {
		SIMPLEQ_REMOVE(&listener->scope->listenq, listener,
		    kauth_listener, listener_next);
		listener->scope->nlisteners--;
	}
}

/*
 * Authorize a request.
 *
 * scope - the scope of the request as defined by KAUTH_SCOPE_* or as
 *	   returned from kauth_register_scope().
 * credential - credentials of the user ("actor") making the request.
 * action - request identifier.
 * arg[0-3] - passed unmodified to listener(s).
 */
int
kauth_authorize_action(kauth_scope_t scope, kauth_cred_t cred,
		       kauth_action_t action, void *arg0, void *arg1,
		       void *arg2, void *arg3)
{
	kauth_listener_t listener;
	int error, allow, fail;

	/* Sanitize input */
	if (scope == NULL || cred == NULL)
		return (EFAULT);
	if (!action)
		return (EINVAL);

	/*
	 * Each scope is associated with at least one listener. We need to
	 * traverse that list of listeners, as long as they return either
	 * KAUTH_REQUEST_DEFER or KAUTH_REQUEST_ALLOW.
	 */
	fail = 0;
	allow = 0;
	SIMPLEQ_FOREACH(listener, &scope->listenq, listener_next) {
		error = listener->func(cred, action, scope->cookie, arg0,
				       arg1, arg2, arg3);

		if (error == KAUTH_RESULT_ALLOW)
			allow = 1;
		else if (error == KAUTH_RESULT_DENY)
			fail = 1;
	}

	return ((allow && !fail) ? 0 : EPERM);
};

/*
 * Generic scope default callback.
 */
int
kauth_authorize_cb_generic(kauth_cred_t cred, kauth_action_t action,
			   void *cookie, void *arg0, void *arg1, void *arg2,
			   void *arg3)
{
	int error;

	error = KAUTH_RESULT_DEFER;

	switch (action) {
	case KAUTH_GENERIC_ISSUSER:
		/* Check if credential belongs to superuser. */
		if (cred->cr_euid == 0) {
			u_short *acflag = (u_short *)arg0;

			if (acflag != NULL)
				*acflag |= ASU;

			error = KAUTH_RESULT_ALLOW;
		} else
			error = KAUTH_RESULT_DENY;
		break;
	}

	return (error);
}

/*
 * Generic scope authorization wrapper.
 */
int
kauth_authorize_generic(kauth_cred_t cred, kauth_action_t action, void *arg0)
{
	return (kauth_authorize_action(kauth_builtin_scope_generic, cred, 
	    action, arg0, NULL, NULL, NULL));
}

/*
 * Process scope default callback.
 */
int
kauth_authorize_cb_process(kauth_cred_t cred, kauth_action_t action,
			   void *cookie, void *arg0, void *arg1, void *arg2,
			   void *arg3)
{
	struct proc *p;
	int error;

	error = KAUTH_RESULT_DEFER;

	p = arg0;

	switch (action) {
	case KAUTH_PROCESS_CANSIGNAL: {
		int signum;

		signum = (int)(unsigned long)arg1;

		if (kauth_cred_uidmatch(cred, p->p_cred) ||
		    (signum == SIGCONT && (curproc->p_session == p->p_session)))
			error = KAUTH_RESULT_ALLOW;
		else
			error = KAUTH_RESULT_DEFER;

		break;
		}

	case KAUTH_PROCESS_CANPTRACE:
		if (kauth_cred_uidmatch(cred, p->p_cred))
			error = KAUTH_RESULT_ALLOW;
		else
			error = KAUTH_RESULT_DENY;
		break;

	case KAUTH_PROCESS_CANSEE:
		if (!security_curtain) {
			error = KAUTH_RESULT_ALLOW;
		} else {
			if (kauth_cred_uidmatch(cred, p->p_cred))
				error = KAUTH_RESULT_ALLOW;
			else
				error = KAUTH_RESULT_DENY;
				/* arg2 - type of information [XXX NOTIMPL] */
		}
		break;
	}

	return (error);
}

/*
 * Process scope authorization wrapper.
 */
int
kauth_authorize_process(kauth_cred_t cred, kauth_action_t action,
    struct proc *p, void *arg1, void *arg2, void *arg3)
{
	return (kauth_authorize_action(kauth_builtin_scope_process, cred,
	    action, p, arg1, arg2, arg3));
}
