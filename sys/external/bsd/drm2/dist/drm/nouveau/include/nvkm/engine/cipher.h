/*	$NetBSD: cipher.h,v 1.2.2.2 2018/09/06 06:56:24 pgoyette Exp $	*/

#ifndef __NVKM_CIPHER_H__
#define __NVKM_CIPHER_H__
#include <core/engine.h>
int g84_cipher_new(struct nvkm_device *, int, struct nvkm_engine **);
#endif
