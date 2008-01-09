/*	$NetBSD: adt7467var.h,v 1.2.48.2 2008/01/09 01:52:40 matt Exp $	*/

/*-
 * Copyright (C) 2005 Michael Lorenz
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * a driver fot the ADT7467 environmental controller found in the iBook G4 
 * and probably other Apple machines 
 */

#ifndef ADT7456VAR_H
#define ADT7456VAR_H

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: adt7467var.h,v 1.2.48.2 2008/01/09 01:52:40 matt Exp $");

#include <dev/i2c/i2cvar.h>

#include <dev/sysmon/sysmonvar.h>
#include "sysmon_envsys.h"

#define ADT7467_MAXSENSORS	5

struct adt7467c_softc {
	struct device sc_dev;
	struct device *parent;
	int sc_node, address;
	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sensor[ADT7467_MAXSENSORS];
	struct i2c_controller *sc_i2c;
	uint8_t regs[32];
};

void adt7467c_setup(struct adt7467c_softc *);

#endif
