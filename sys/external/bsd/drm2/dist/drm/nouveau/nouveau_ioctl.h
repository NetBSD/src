/*	$NetBSD: nouveau_ioctl.h,v 1.3 2021/12/18 23:45:32 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NOUVEAU_IOCTL_H__
#define __NOUVEAU_IOCTL_H__

long nouveau_compat_ioctl(struct file *, unsigned int cmd, unsigned long arg);
long nouveau_drm_ioctl(struct file *, unsigned int cmd, unsigned long arg);

#endif
