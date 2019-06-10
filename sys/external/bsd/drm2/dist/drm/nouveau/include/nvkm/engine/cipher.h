/*	$NetBSD: cipher.h,v 1.2.6.2 2019/06/10 22:08:14 christos Exp $	*/

#ifndef __NVKM_CIPHER_H__
#define __NVKM_CIPHER_H__
#include <core/engine.h>
int g84_cipher_new(struct nvkm_device *, int, struct nvkm_engine **);
#endif
