/*	$NetBSD: intel_guc_log.c,v 1.1 2021/12/19 11:49:12 riastradh Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intel_guc_log.c,v 1.1 2021/12/19 11:49:12 riastradh Exp $");

#include "gt/uc/intel_guc_log.h"

void
intel_guc_log_init_early(struct intel_guc_log *log)
{
}

int
intel_guc_log_create(struct intel_guc_log *log)
{

	return 0;
}

void
intel_guc_log_destroy(struct intel_guc_log *log)
{
}

int
intel_guc_log_set_level(struct intel_guc_log *log, u32 level)
{

	return -ENOSYS;
}

bool
intel_guc_log_relay_created(const struct intel_guc_log *log)
{

	return false;
}

int
intel_guc_log_relay_open(struct intel_guc_log *log)
{

	return -ENOSYS;
}

int
intel_guc_log_relay_start(struct intel_guc_log *log)
{

	return -ENOSYS;
}

void
intel_guc_log_relay_flush(struct intel_guc_log *log)
{
}

void
intel_guc_log_relay_close(struct intel_guc_log *log)
{
}

void
intel_guc_log_handle_flush_event(struct intel_guc_log *log)
{
}
