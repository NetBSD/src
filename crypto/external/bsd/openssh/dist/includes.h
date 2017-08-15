/*	$NetBSD: includes.h,v 1.2.10.1 2017/08/15 04:39:21 snj Exp $	*/
#include <sys/cdefs.h>
#ifndef __OpenBSD__
#define __bounded__(a, b, c)
#define explicit_bzero(a, b) explicit_memset((a), 0, (b))
#define timingsafe_bcmp(a, b, c) (!consttime_memequal((a), (b), (c)))
#endif

#include "namespace.h"
#include "netbsd_ssh_compat.h"
