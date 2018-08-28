/*	$NetBSD: fan.h,v 1.2 2018/08/27 04:58:30 riastradh Exp $	*/

#ifndef __NVBIOS_FAN_H__
#define __NVBIOS_FAN_H__
#include <subdev/bios/therm.h>

u16 nvbios_fan_parse(struct nvkm_bios *bios, struct nvbios_therm_fan *fan);
#endif
