/*	$NetBSD: sysmonvar.h,v 1.1 2000/06/24 00:37:20 thorpej Exp $	*/

/*-
 * Copyright (c) 2000 Zembu Labs, Inc.
 * All rights reserved.
 *
 * Author: Jason R. Thorpe <thorpej@zembu.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Zembu Labs, Inc.
 * 4. Neither the name of Zembu Labs nor the names of its employees may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ZEMBU LABS, INC. ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WAR-
 * RANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DIS-
 * CLAIMED.  IN NO EVENT SHALL ZEMBU LABS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DEV_SYSMON_SYSMONVAR_H_
#define	_DEV_SYSMON_SYSMONVAR_H_

#include <sys/envsys.h>
#include <sys/queue.h>

struct sysmon_envsys {
	int32_t sme_envsys_version;	/* ENVSYS API version */

	LIST_ENTRY(sysmon_envsys) sme_list;

	const struct envsys_range *sme_ranges;
	struct envsys_basic_info *sme_sensor_info;
	struct envsys_tre_data *sme_sensor_data;
	void *sme_cookie;		/* for ENVSYS back-end */

	/* Callbacks */
	int (*sme_gtredata)(struct sysmon_envsys *, struct envsys_tre_data *);
	int (*sme_streinfo)(struct sysmon_envsys *, struct envsys_basic_info *);

	u_int sme_fsensor;		/* sensor index base, from sysmon */
	u_int sme_nsensors;		/* sensor count, from driver */
};

#define	SME_SENSOR_IDX(sme, idx)	((idx) - (sme)->sme_fsensor)

int	sysmon_envsys_register(struct sysmon_envsys *);
void	sysmon_envsys_unregister(struct sysmon_envsys *);

#endif /* _DEV_SYSMON_SYSMONVAR_H_ */
