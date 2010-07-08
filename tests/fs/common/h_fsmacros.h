/*	$NetBSD: h_fsmacros.h,v 1.3 2010/07/08 13:21:02 pooka Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nicolas Joly.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __H_FSMACROS_H_
#define __H_FSMACROS_H_

#include <atf-c.h>
#include <string.h>

#include "ext2fs.c"
#include "ffs.c"
#include "lfs.c"
#include "msdosfs.c"
#include "sysvbfs.c"
#include "tmpfs.c"

#define IMGNAME "image.fs"
#define IMGSIZE (10000 * 512)
#define MNTNAME "/mnt"

#define ATF_TC_FSADD(fs,type,func,desc) \
  ATF_TC(fs##_##func); \
  ATF_TC_HEAD(fs##_##func,tc) \
  { \
    atf_tc_set_md_var(tc, "descr", type " test for " desc); \
    atf_tc_set_md_var(tc, "use.fs", "true"); \
  } \
  ATF_TC_BODY(fs##_##func,tc) \
  { \
    void *tmp; \
    atf_check_fstype(tc, type); \
    if (fs##_newfs(&tmp, IMGNAME, IMGSIZE) != 0) \
      atf_tc_fail("newfs failed"); \
    if (fs##_mount(tmp, MNTNAME, 0) != 0) \
      atf_tc_fail("mount failed"); \
    func(MNTNAME); \
    if (fs##_unmount(MNTNAME, 0) != 0) \
      atf_tc_fail("unmount failed"); \
    if (fs##_delfs(tmp) != 0) \
      atf_tc_fail("delfs failed"); \
  }

#define ATF_TP_FSADD(fs,func) \
  ATF_TP_ADD_TC(tp,fs##_##func)

#define ATF_TC_FSAPPLY(func,desc) \
  ATF_TC_FSADD(ext2fs,"ext2fs",func,desc) \
  ATF_TC_FSADD(ffs,"ffs",func,desc) \
  ATF_TC_FSADD(lfs,"lfs",func,desc) \
  ATF_TC_FSADD(msdosfs,"msdosfs",func,desc) \
  ATF_TC_FSADD(sysvbfs,"sysvbfs",func,desc) \
  ATF_TC_FSADD(tmpfs,"tmpfs",func,desc)

#define ATF_TP_FSAPPLY(func) \
  ATF_TP_FSADD(ext2fs,func); \
  ATF_TP_FSADD(ffs,func); \
  ATF_TP_FSADD(lfs,func); \
  ATF_TP_FSADD(msdosfs,func); \
  ATF_TP_FSADD(sysvbfs,func); \
  ATF_TP_FSADD(tmpfs,func);

#define ATF_FSAPPLY(func,desc) \
  ATF_TC_FSAPPLY(func,desc); \
  ATF_TP_ADD_TCS(tp) \
  { \
    ATF_TP_FSAPPLY(func); \
    return atf_no_error(); \
  }

static void
atf_check_fstype(const atf_tc_t *tc, const char *fs)
{
  const char *fstype;

  if (!atf_tc_has_config_var(tc, "fstype"))
    return;
  fstype = atf_tc_get_config_var(tc, "fstype");
  if (strcmp(fstype, fs) == 0)
    return;
  atf_tc_skip("filesystem not selected");
}

#endif /* __H_FSMACROS_H_ */
