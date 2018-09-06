/*	$NetBSD: nouveau_ioctl.h,v 1.1.1.1.30.1 2018/09/06 06:56:18 pgoyette Exp $	*/

#ifndef __NOUVEAU_IOCTL_H__
#define __NOUVEAU_IOCTL_H__

long nouveau_compat_ioctl(struct file *, unsigned int cmd, unsigned long arg);
long nouveau_drm_ioctl(struct file *, unsigned int cmd, unsigned long arg);

#endif
