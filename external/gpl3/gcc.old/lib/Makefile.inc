#	$NetBSD: Makefile.inc,v 1.10.2.1 2026/11/04 23:00:03 perseant Exp $

GCC_MACHINE_ARCH?=${MACHINE_ARCH:S/earmv5/earm/}

.ifndef _EXTERNAL_GPL3_GCC_LIB_MAKEFILE_INC_
_EXTERNAL_GPL3_GCC_LIB_MAKEFILE_INC_=1

.sinclude "../../Makefile.gcc_path"
.sinclude "../../../Makefile.gcc_path"
.sinclude "../../../../Makefile.gcc_path"

WARNS=1

.endif
