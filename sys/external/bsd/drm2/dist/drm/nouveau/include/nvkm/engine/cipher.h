/*	$NetBSD: cipher.h,v 1.1.1.1 2018/08/27 01:34:55 riastradh Exp $	*/

#ifndef __NVKM_CIPHER_H__
#define __NVKM_CIPHER_H__
#include <core/engine.h>
int g84_cipher_new(struct nvkm_device *, int, struct nvkm_engine **);
#endif
