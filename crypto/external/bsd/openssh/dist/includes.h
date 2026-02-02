/*	$NetBSD: includes.h,v 1.10.2.1 2026/02/02 18:07:59 martin Exp $	*/
#include <sys/cdefs.h>
#ifndef __OpenBSD__
#define __bounded__(a, b, c)
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

#include "namespace.h"

#ifdef USE_PAM
#include <security/pam_appl.h>
#endif

void freezero(void *, size_t);
#define explicit_bzero(a, b) explicit_memset((a), 0, (b))
#define timingsafe_bcmp(a, b, c) (!consttime_memequal((a), (b), (c)))
void	*recallocarray(void *, size_t, size_t, size_t);
#endif

#ifndef WITH_OPENSSL
#define SHA256Init SHA256_Init
#define SHA256Update SHA256_Update
#define SHA256Final SHA256_Final

#define SHA384Init SHA384_Init
#define SHA384Update SHA384_Update
#define SHA384Final SHA384_Final

#define SHA512Init SHA512_Init
#define SHA512Update SHA512_Update
#define SHA512Final SHA512_Final
#endif
