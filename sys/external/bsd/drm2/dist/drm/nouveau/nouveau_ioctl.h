/*	$NetBSD: nouveau_ioctl.h,v 1.1.1.1.32.1 2019/06/10 22:08:06 christos Exp $	*/

#ifndef __NOUVEAU_IOCTL_H__
#define __NOUVEAU_IOCTL_H__

long nouveau_compat_ioctl(struct file *, unsigned int cmd, unsigned long arg);
long nouveau_drm_ioctl(struct file *, unsigned int cmd, unsigned long arg);

#endif
