/*	$NetBSD: imxi2cvar.h,v 1.1.8.1 2015/04/06 15:17:52 skrll Exp $	*/

/*
* Copyright (c) 2012, 2015 Genetec Corporation.  All rights reserved.
* Written by Hashimoto Kenichi for Genetec Corporation.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _IMXI2CVAR_H_
#define _IMXI2CVAR_H_

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/motoi2cvar.h>

struct imxi2c_softc {
	device_t sc_dev;
	struct motoi2c_softc sc_motoi2c;
	struct motoi2c_settings sc_motoi2c_settings;
};

int imxi2c_attach_common(device_t, device_t,
    bus_space_tag_t, paddr_t, size_t, int, int);
int imxi2c_set_freq(device_t, long, int);

/*
* defined in imx51_i2c.c and imx31_i2c.c
*/
int imxi2c_match(device_t, cfdata_t, void *);
void imxi2c_attach(device_t, device_t, void *);

#endif /* _IMXI2CVAR_H_ */

