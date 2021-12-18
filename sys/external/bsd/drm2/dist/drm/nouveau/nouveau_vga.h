/*	$NetBSD: nouveau_vga.h,v 1.4 2021/12/18 23:45:32 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NOUVEAU_VGA_H__
#define __NOUVEAU_VGA_H__

struct nouveau_drm;
struct drm_device;

void nouveau_vga_init(struct nouveau_drm *);
void nouveau_vga_fini(struct nouveau_drm *);
void nouveau_vga_lastclose(struct drm_device *dev);

#endif
