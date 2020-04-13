/* Written by Ricky Zhou <ricky@fedoraproject.org>
 * Fredrik Thulin <fredrik@yubico.com> implemented pam_modutil_drop_priv
 *
 * Copyright (c) 2011-2014 Yubico AB
 * Copyright (c) 2011 Ricky Zhou <ricky@fedoraproject.org>
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

#ifndef HAVE_PAM_MODUTIL_DROP_PRIV

#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "drop_privs.h"
#include "util.h"

#ifdef HAVE_SECURITY_PAM_APPL_H
#include <security/pam_appl.h>
#endif
#ifdef HAVE_SECURITY_PAM_MODULES_H
#include <security/pam_modules.h>
#endif

int pam_modutil_drop_priv(pam_handle_t *pamh, struct _ykpam_privs *privs,
                          struct passwd *pw) {
  privs->saved_euid = geteuid();
  privs->saved_egid = getegid();

  if ((privs->saved_euid == pw->pw_uid) && (privs->saved_egid == pw->pw_gid)) {
    D(privs->debug_file, "Privilges already dropped, pretend it is all right");
    return 0;
  }

  privs->saved_groups_length = getgroups(0, NULL);
  if (privs->saved_groups_length < 0) {
    D(privs->debug_file, "getgroups: %s", strerror(errno));
    return -1;
  }

  if (privs->saved_groups_length > SAVED_GROUPS_MAX_LEN) {
    D(privs->debug_file, "too many groups, limiting.");
    privs->saved_groups_length = SAVED_GROUPS_MAX_LEN;
  }

  if (privs->saved_groups_length > 0) {
    if (getgroups(privs->saved_groups_length, privs->saved_groups) < 0) {
      D(privs->debug_file, "getgroups: %s", strerror(errno));
      goto free_out;
    }
  }

  if (initgroups(pw->pw_name, pw->pw_gid) < 0) {
    D(privs->debug_file, "initgroups: %s", strerror(errno));
    goto free_out;
  }

  if (setegid(pw->pw_gid) < 0) {
    D(privs->debug_file, "setegid: %s", strerror(errno));
    goto free_out;
  }

  if (seteuid(pw->pw_uid) < 0) {
    D(privs->debug_file, "seteuid: %s", strerror(errno));
    goto free_out;
  }

  return 0;
free_out:
  return -1;
}

int pam_modutil_regain_priv(pam_handle_t *pamh, struct _ykpam_privs *privs) {
  if ((privs->saved_euid == geteuid()) && (privs->saved_egid == getegid())) {
    D(privs->debug_file,
      "Privilges already as requested, pretend it is all right");
    return 0;
  }

  if (seteuid(privs->saved_euid) < 0) {
    D(privs->debug_file, "seteuid: %s", strerror(errno));
    return -1;
  }

  if (setegid(privs->saved_egid) < 0) {
    D(privs->debug_file, "setegid: %s", strerror(errno));
    return -1;
  }

  if (setgroups(privs->saved_groups_length, privs->saved_groups) < 0) {
    D(privs->debug_file, "setgroups: %s", strerror(errno));
    return -1;
  }

  return 0;
}

#else

// drop_privs.c:124: warning: ISO C forbids an empty translation unit
// [-Wpedantic]
typedef int make_iso_compilers_happy;

#endif // HAVE_PAM_MODUTIL_DROP_PRIV
