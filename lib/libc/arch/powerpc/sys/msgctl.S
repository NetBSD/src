/*	$NetBSD: msgctl.S,v 1.1 1999/08/25 05:09:31 thorpej Exp $	*/

#include "SYS.h"

WARN_REFERENCES(msgctl, \
    "warning: reference to compatibility msgctl(); include <sys/msg.h> for correct reference")

PSEUDO(msgctl,compat_14_msgctl)
