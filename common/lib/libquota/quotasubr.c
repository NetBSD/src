/* $NetBSD: quotasubr.c,v 1.1 2011/03/24 17:05:40 bouyer Exp $ */
/*-
  * Copyright (c) 2011 Manuel Bouyer
  * All rights reserved.
  * This software is distributed under the following condiions
  * compliant with the NetBSD foundation policy.
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

#include <sys/param.h>
#include <sys/time.h>
#include <sys/inttypes.h>
#include <sys/errno.h>

#include <quota/quota.h>

int
quota_check_limit(uint64_t cur, uint64_t change, uint64_t soft, uint64_t hard,
    time_t expire, time_t now)
{ 
	if (cur + change > hard) {
		if (cur <= soft)
			return (QL_F_CROSS | QL_S_DENY_HARD);
		return QL_S_DENY_HARD;
	} else if (cur + change > soft) {
		if (cur <= soft)
			return (QL_F_CROSS | QL_S_ALLOW_SOFT);
		if (now > expire) {
			return QL_S_DENY_GRACE;
		}
		return QL_S_ALLOW_SOFT;
	}
	return QL_S_ALLOW_OK;
} 
