/*	$NetBSD: oimm.h,v 1.1.1.1 2021/12/18 20:15:37 riastradh Exp $	*/

#ifndef __NV50_KMS_OIMM_H__
#define __NV50_KMS_OIMM_H__
#include "wndw.h"

int oimm507b_init(struct nouveau_drm *, s32, struct nv50_wndw *);

int nv50_oimm_init(struct nouveau_drm *, struct nv50_wndw *);
#endif
