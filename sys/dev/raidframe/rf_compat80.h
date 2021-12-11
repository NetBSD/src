/*	$NetBSD: rf_compat80.h,v 1.4 2021/12/11 19:24:21 mrg Exp $	*/

/*
 * Copyright (c) 2017 Matthew R. Green
 * All rights reserved.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _RF_COMPAT80_H_
#define _RF_COMPAT80_H_

#include <sys/ioccom.h>

/*
 * These ioctls were versioned after NetBSD 8.x.
 *
 * RAIDFRAME_FAIL_DISK had unused pointers removed from the structure.
 * RAIDFRAME_CHECK_*EXT and RAIDFRAME_GET_COMPONENT_LABEL were converted
 * from pointers to full structures so that ioctl framework does the copyout.
 * RAIDFRAME_GET_INFO80 had the containing structure reorganised so that
 * the layout became bitsize neutral.
 *
 * These 6 changes enable COMPAT_NETBSD32 support to work sanely with
 * these ioctls, leaving only RAIDFRAME_CONFIGURE needing extra help.
 */

#define RAIDFRAME_FAIL_DISK80				_IOW ('r', 5,  struct rf_recon_req80)
#define RAIDFRAME_GET_COMPONENT_LABEL80			_IOWR('r', 19, RF_ComponentLabel_t *)
#define RAIDFRAME_CHECK_RECON_STATUS_EXT80		_IOWR('r', 32, RF_ProgressInfo_t *)
#define RAIDFRAME_CHECK_PARITYREWRITE_STATUS_EXT80	_IOWR('r', 33, RF_ProgressInfo_t *)
#define RAIDFRAME_CHECK_COPYBACK_STATUS_EXT80		_IOWR('r', 34, RF_ProgressInfo_t *)
#define RAIDFRAME_GET_INFO80				_IOWR('r', 36, RF_DeviceConfig_t80 *)
#define RAIDFRAME_CONFIGURE80				_IOW ('r', 35, void *)

#endif /* _RF_COMPAT80_H_ */
