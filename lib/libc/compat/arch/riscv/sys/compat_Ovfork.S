/*	$NetBSD: compat_Ovfork.S,v 1.1 2023/05/19 06:50:07 skrll Exp $	*/

/*
 * This file placed in the public domain.
 * Nick Hudson, 19th May, 2023.
 */

#include "SYS.h"

WARN_REFERENCES(vfork, \
    "warning: reference to compatibility vfork(); include <unistd.h> for correct reference")

PSEUDO(vfork,__vfork14)

