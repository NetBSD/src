/*	$NetBSD: compat_sigprocmask.S,v 1.1 2023/05/19 06:50:07 skrll Exp $	*/

/*
 * This file placed in the public domain.
 * Nick Hudson, 19th May, 2023.
 */

#include "SYS.h"

WARN_REFERENCES(sigprocmask, \
    "warning: reference to compatibility sigprocmask(); include <signal.h> for correct reference")

PSEUDO(sigprocmask,compat_13_sigprocmask13)
