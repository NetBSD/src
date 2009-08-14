/* $NetBSD: keylock.h,v 1.1 2009/08/14 21:17:22 mbalmer Exp $ */
/*-
 * Copyright (c) 2009 Marc Balmer <marc@msys.ch>
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

#ifndef _SECMODEL_KEYLOCK_KEYLOCK_H_
#define	_SECMODEL_KEYLOCK_KEYLOCK_H_

int secmodel_keylock_sysctl(SYSCTLFN_PROTO);

void secmodel_keylock_init(void);
void secmodel_keylock_start(void);
void secmodel_keylock_stop(void);

#if defined(_LKM)
SYSCTL_SETUP_PROTO(sysctl_security_keylock_setup);
#endif /* _LKM */

int secmodel_keylock_system_cb(kauth_cred_t, kauth_action_t, void *,
    void *, void *, void *, void *);
int secmodel_keylock_process_cb(kauth_cred_t, kauth_action_t, void *,
    void *, void *, void *, void *);
int secmodel_keylock_network_cb(kauth_cred_t, kauth_action_t, void *,
    void *, void *, void *, void *);
int secmodel_keylock_machdep_cb(kauth_cred_t, kauth_action_t, void *,
    void *, void *, void *, void *);
int secmodel_keylock_device_cb(kauth_cred_t, kauth_action_t, void *,
    void *, void *, void *, void *);

#endif /* !_SECMODEL_KEYLOCK_KEYLOCK_H_ */
