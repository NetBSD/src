/*	$NetBSD: h_fsmacros.h,v 1.13 2010/07/20 17:44:01 njoly Exp $	*/

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

#include <sys/mount.h>

#include <atf-c.h>
#include <string.h>

#define FSPROTOS(_fs_)							\
int _fs_##_fstest_newfs(const atf_tc_t *, void **, const char *, off_t);\
int _fs_##_fstest_delfs(const atf_tc_t *, void *);			\
int _fs_##_fstest_mount(const atf_tc_t *, void *, const char *, int);	\
int _fs_##_fstest_unmount(const atf_tc_t *, const char *, int);

FSPROTOS(ext2fs);
FSPROTOS(ffs);
FSPROTOS(lfs);
FSPROTOS(msdosfs);
FSPROTOS(puffs);
FSPROTOS(sysvbfs);
FSPROTOS(tmpfs);

#define FSTEST_IMGNAME "image.fs"
#define FSTEST_IMGSIZE (10000 * 512)
#define FSTEST_MNTNAME "/mnt"

#define ATF_TC_FSADD(fs,type,func,desc) \
  ATF_TC_WITH_CLEANUP(fs##_##func); \
  ATF_TC_HEAD(fs##_##func,tc) \
  { \
    atf_tc_set_md_var(tc, "descr", type " test for " desc); \
    atf_tc_set_md_var(tc, "use.fs", "true"); \
    atf_tc_set_md_var(tc, "X-fs.type", type); \
  } \
  void *fs##func##tmp; \
  ATF_TC_BODY(fs##_##func,tc) \
  { \
    if (!atf_check_fstype(tc, type)) \
      atf_tc_skip("filesystem not selected"); \
    if (fs##_fstest_newfs(tc, &fs##func##tmp, FSTEST_IMGNAME, FSTEST_IMGSIZE) != 0) \
      atf_tc_fail("newfs failed"); \
    if (fs##_fstest_mount(tc, fs##func##tmp, FSTEST_MNTNAME, 0) != 0) \
      atf_tc_fail("mount failed"); \
    func(tc,FSTEST_MNTNAME); \
    if (fs##_fstest_unmount(tc, FSTEST_MNTNAME, 0) != 0) \
      atf_tc_fail("unmount failed"); \
  } \
  ATF_TC_CLEANUP(fs##_##func,tc) \
  { \
    if (!atf_check_fstype(tc, type)) \
      return; \
    if (fs##_fstest_delfs(tc, fs##func##tmp) != 0) \
      atf_tc_fail("delfs failed"); \
  }

#define ATF_TP_FSADD(fs,func) \
  ATF_TP_ADD_TC(tp,fs##_##func)

#define ATF_TC_FSAPPLY(func,desc) \
  ATF_TC_FSADD(ext2fs,MOUNT_EXT2FS,func,desc) \
  ATF_TC_FSADD(ffs,MOUNT_FFS,func,desc) \
  ATF_TC_FSADD(lfs,MOUNT_LFS,func,desc) \
  ATF_TC_FSADD(msdosfs,MOUNT_MSDOS,func,desc) \
  ATF_TC_FSADD(puffs,MOUNT_PUFFS,func,desc) \
  ATF_TC_FSADD(sysvbfs,MOUNT_SYSVBFS,func,desc) \
  ATF_TC_FSADD(tmpfs,MOUNT_TMPFS,func,desc)

#define ATF_TP_FSAPPLY(func) \
  ATF_TP_FSADD(ext2fs,func); \
  ATF_TP_FSADD(ffs,func); \
  ATF_TP_FSADD(lfs,func); \
  ATF_TP_FSADD(msdosfs,func); \
  ATF_TP_FSADD(puffs,func); \
  ATF_TP_FSADD(sysvbfs,func); \
  ATF_TP_FSADD(tmpfs,func);

#define ATF_FSAPPLY(func,desc) \
  ATF_TC_FSAPPLY(func,desc); \
  ATF_TP_ADD_TCS(tp) \
  { \
    ATF_TP_FSAPPLY(func); \
    return atf_no_error(); \
  }

static __inline bool
atf_check_fstype(const atf_tc_t *tc, const char *fs)
{
  const char *fstype;

  if (!atf_tc_has_config_var(tc, "fstype"))
    return true;
  fstype = atf_tc_get_config_var(tc, "fstype");
  if (strcmp(fstype, fs) == 0)
    return true;
  return false;
}

#define FSTYPE_EXT2FS(tc)\
    (strcmp(atf_tc_get_md_var(tc, "X-fs.type"), MOUNT_EXT2FS) == 0)
#define FSTYPE_FFS(tc)\
    (strcmp(atf_tc_get_md_var(tc, "X-fs.type"), MOUNT_FFS) == 0)
#define FSTYPE_LFS(tc)\
    (strcmp(atf_tc_get_md_var(tc, "X-fs.type"), MOUNT_LFS) == 0)
#define FSTYPE_MSDOS(tc)\
    (strcmp(atf_tc_get_md_var(tc, "X-fs.type"), MOUNT_MSDOS) == 0)
#define FSTYPE_PUFFS(tc)\
    (strcmp(atf_tc_get_md_var(tc, "X-fs.type"), MOUNT_PUFFS) == 0)
#define FSTYPE_SYSVBFS(tc)\
    (strcmp(atf_tc_get_md_var(tc, "X-fs.type"), MOUNT_SYSVBFS) == 0)
#define FSTYPE_TMPFS(tc)\
    (strcmp(atf_tc_get_md_var(tc, "X-fs.type"), MOUNT_TMPFS) == 0)

#endif /* __H_FSMACROS_H_ */
