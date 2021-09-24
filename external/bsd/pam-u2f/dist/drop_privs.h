/* Copyright (c) 2011-2014 Yubico AB
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __PAM_U2F_DROP_PRIVS_H_INCLUDED__
#define __PAM_U2F_DROP_PRIVS_H_INCLUDED__

#ifdef HAVE_PAM_MODUTIL_DROP_PRIV
#include <security/pam_modutil.h>
#elif HAVE_OPENPAM_BORROW_CRED
#include <sys/types.h>
#include <security/pam_appl.h>
#include <security/openpam.h>

#define PAM_MODUTIL_DEF_PRIVS(n) /* noop */
#define pam_modutil_drop_priv(pamh, privs, pwd)                                \
  ((openpam_borrow_cred((pamh), (pwd)) == PAM_SUCCESS) ? 0 : -1)
#define pam_modutil_regain_priv(pamh, privs)                                   \
  ((openpam_restore_cred((pamh)) == PAM_SUCCESS) ? 0 : -1)

#else

#error "Please provide an implementation for pam_modutil_{drop,regain}_priv"

#endif /* HAVE_PAM_MODUTIL_DROP_PRIV */
#endif /* __PAM_U2F_DROP_PRIVS_H_INCLUDED__ */
