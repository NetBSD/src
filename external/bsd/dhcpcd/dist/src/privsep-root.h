/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Privilege Separation for dhcpcd
 * Copyright (c) 2006-2020 Roy Marples <roy@marples.name>
 * All rights reserved

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef PRIVSEP_ROOT_H
#define PRIVSEP_ROOT_H

#include "if.h"

pid_t ps_root_start(struct dhcpcd_ctx *ctx);
int ps_root_stop(struct dhcpcd_ctx *ctx);

ssize_t ps_root_readerror(struct dhcpcd_ctx *);
ssize_t ps_root_docopychroot(struct dhcpcd_ctx *, const char *);
ssize_t ps_root_copychroot(struct dhcpcd_ctx *, const char *);
ssize_t ps_root_ioctl(struct dhcpcd_ctx *, ioctl_request_t, void *, size_t);
ssize_t ps_root_unlink(struct dhcpcd_ctx *, const char *);
ssize_t ps_root_os(struct ps_msghdr *, struct msghdr *);
#if defined(BSD) || defined(__sun)
ssize_t ps_root_route(struct dhcpcd_ctx *, void *, size_t);
ssize_t ps_root_ioctllink(struct dhcpcd_ctx *, unsigned long, void *, size_t);
ssize_t ps_root_ioctl6(struct dhcpcd_ctx *, unsigned long, void *, size_t);
#endif
#ifdef __linux__
ssize_t ps_root_sendnetlink(struct dhcpcd_ctx *, int, struct msghdr *);
ssize_t ps_root_writepathuint(struct dhcpcd_ctx *, const char *, unsigned int);
#endif
ssize_t ps_root_script(const struct interface *, const void *, size_t);

#endif
