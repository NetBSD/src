/*	$NetBSD: sigreturn.S,v 1.5 2004/03/26 22:15:03 drochner Exp $	*/

#include "SYS.h"

WARN_REFERENCES(sigreturn, \
    "warning: reference to compatibility sigreturn()")

PSEUDO(sigreturn,compat_13_sigreturn13)
