/*	$NetBSD: compat_shmctl.S,v 1.1 2023/05/19 06:50:07 skrll Exp $	*/

/*
 * This file placed in the public domain.
 * Nick Hudson, 19th May, 2023.
 */

#include "SYS.h"

WARN_REFERENCES(shmctl, \
    "warning: reference to compatibility shmctl(); include <sys/shm.h> for correct reference")

PSEUDO(shmctl,compat_14_shmctl)
