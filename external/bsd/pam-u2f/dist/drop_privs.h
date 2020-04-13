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
#else

#include <pwd.h>
#include <stdio.h>

#ifdef HAVE_SECURITY_PAM_APPL_H
#include <security/pam_appl.h>
#endif
#ifdef HAVE_SECURITY_PAM_MODULES_H
#include <security/pam_modules.h>
#endif

#define SAVED_GROUPS_MAX_LEN 64 /* as pam_modutil.. */

struct _ykpam_privs {
  uid_t saved_euid;
  gid_t saved_egid;
  gid_t *saved_groups;
  int saved_groups_length;
  FILE *debug_file;
};

#define PAM_MODUTIL_DEF_PRIVS(n)                                               \
  gid_t n##_saved_groups[SAVED_GROUPS_MAX_LEN];                                \
  struct _ykpam_privs n = { \
      (uid_t)-1, \
      (gid_t)-1, \
      n##_saved_groups, \
      SAVED_GROUPS_MAX_LEN,     \
      cfg->debug_file, \
  }

int pam_modutil_drop_priv(pam_handle_t *, struct _ykpam_privs *,
                          struct passwd *);
int pam_modutil_regain_priv(pam_handle_t *, struct _ykpam_privs *);

#endif
#endif
