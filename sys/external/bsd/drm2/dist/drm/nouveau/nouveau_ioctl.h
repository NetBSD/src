/*	$NetBSD: nouveau_ioctl.h,v 1.2 2018/08/27 04:58:24 riastradh Exp $	*/

#ifndef __NOUVEAU_IOCTL_H__
#define __NOUVEAU_IOCTL_H__

long nouveau_compat_ioctl(struct file *, unsigned int cmd, unsigned long arg);
long nouveau_drm_ioctl(struct file *, unsigned int cmd, unsigned long arg);

#endif
