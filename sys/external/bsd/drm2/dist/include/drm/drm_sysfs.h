/*	$NetBSD: drm_sysfs.h,v 1.1.1.1.30.1 2018/09/06 06:56:35 pgoyette Exp $	*/

#ifndef _DRM_SYSFS_H_
#define _DRM_SYSFS_H_

/**
 * This minimalistic include file is intended for users (read TTM) that
 * don't want to include the full drmP.h file.
 */

extern int drm_class_device_register(struct device *dev);
extern void drm_class_device_unregister(struct device *dev);

#endif
