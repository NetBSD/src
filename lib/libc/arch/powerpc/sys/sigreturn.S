/*	$NetBSD: sigreturn.S,v 1.4 1998/12/02 01:01:04 thorpej Exp $	*/

#include "SYS.h"

WARN_REFERENCES(sigreturn, \
    "warning: reference to compatibility sigreturn(); include <signal.h> for correct reference")

PSEUDO(sigreturn,compat_13_sigreturn13)
