/*	$NetBSD: sigaction.S,v 1.2 1998/12/02 01:01:04 thorpej Exp $	*/

#include "SYS.h"

WARN_REFERENCES(sigaction, \
    "warning: reference to compatibility sigaction(); include <signal.h> for correct reference")

PSEUDO(sigaction,compat_13_sigaction13)
