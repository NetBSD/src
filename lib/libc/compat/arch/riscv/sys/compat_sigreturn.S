/*	$NetBSD: compat_sigreturn.S,v 1.1 2023/05/19 06:50:07 skrll Exp $	*/

/*
 * This file placed in the public domain.
 * Nick Hudson, 19th May, 2023.
 */

#include "SYS.h"

WARN_REFERENCES(sigreturn, \
    "warning: reference to compatibility sigreturn()")

PSEUDO(sigreturn,compat_13_sigreturn13)
