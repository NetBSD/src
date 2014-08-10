/* $NetBSD: hytp14var.h,v 1.2 2014/08/10 16:44:35 tls Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank Kardel.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * IST-AG P14 calibrated Hygro-/Temperature sensor module
 * Devices: HYT-271, HYT-221 and HYT-939 
 *
 * see:
 * http://www.ist-ag.com/eh/ist-ag/resource.nsf/imgref/Download_AHHYTM_E2.1.pdf/
 *      $FILE/AHHYTM_E2.1.pdf
 */ 
#ifndef _DEV_I2C_HYTP14VAR_H_
#define _DEV_I2C_HYTP14VAR_H_

#define HYTP14_DEFAULT_ADDR	0x28

#define HYTP14_NUM_SENSORS	2

struct hytp14_sc {
	device_t	sc_dev;
	i2c_tag_t	sc_tag;
	i2c_addr_t	sc_addr;

	int		sc_refresh; /* last refresh from hardclock_ticks */
        int             sc_valid;   /* ENVSYS validity state for this sensor */
        uint8_t         sc_data[4]; /* last data read */

	int		sc_numsensors;

	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sensors[HYTP14_NUM_SENSORS];
};

struct hytp14_sensor {
	const char	  *desc;
	enum envsys_units  type;
	void		 (*refresh)(struct hytp14_sc *, envsys_data_t *);
};

#endif
/*
 * $Log: hytp14var.h,v $
 * Revision 1.2  2014/08/10 16:44:35  tls
 * Merge tls-earlyentropy branch into HEAD.
 *
 * Revision 1.1.4.2  2014/08/10 06:54:51  tls
 * Rebase.
 *
 * Revision 1.1  2014/05/18 11:46:23  kardel
 * add HYT-221/271/939 humidity/temperature I2C sensor
 * extend envsys(4) framework by %rH (relative humidity)
 *
 */
