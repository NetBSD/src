/*	$NetBSD: nouveau_vga.h,v 1.1.1.2 2018/08/27 01:34:55 riastradh Exp $	*/

#ifndef __NOUVEAU_VGA_H__
#define __NOUVEAU_VGA_H__

void nouveau_vga_init(struct nouveau_drm *);
void nouveau_vga_fini(struct nouveau_drm *);
void nouveau_vga_lastclose(struct drm_device *dev);

#endif
