/*	$NetBSD: adm1030var.h,v 1.5.2.1 2008/06/23 04:31:01 wrstuden Exp $	*/

/*-
 * Copyright (C) 2005 Michael Lorenz.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* 
 * a driver fot the ADM1030 environmental controller found in some iBook G3 
 * and probably other Apple machines 
 */

#ifndef ADM1030VAR_H
#define ADM1030VAR_H

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: adm1030var.h,v 1.5.2.1 2008/06/23 04:31:01 wrstuden Exp $");

#include <dev/i2c/i2cvar.h>

#include <dev/sysmon/sysmonvar.h>
#include "sysmon_envsys.h"

struct adm1030c_softc {
	device_t parent;
	struct sysmon_envsys *sc_sme;
	envsys_data_t *sc_sensor;
	struct i2c_controller *sc_i2c;
	int sc_node, address;
	uint8_t regs[3];
};

void adm1030c_setup(device_t);

#endif
