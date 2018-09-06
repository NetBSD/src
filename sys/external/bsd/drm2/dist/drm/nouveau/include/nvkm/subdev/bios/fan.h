/*	$NetBSD: fan.h,v 1.2.2.2 2018/09/06 06:56:24 pgoyette Exp $	*/

#ifndef __NVBIOS_FAN_H__
#define __NVBIOS_FAN_H__
#include <subdev/bios/therm.h>

u16 nvbios_fan_parse(struct nvkm_bios *bios, struct nvbios_therm_fan *fan);
#endif
