#	$NetBSD: Makefile.inc,v 1.11 2025/09/07 04:11:47 mrg Exp $

GCC_MACHINE_ARCH=${MACHINE_ARCH:S/earmv5/earm/}

.ifndef _EXTERNAL_GPL3_GCC_LIB_MAKEFILE_INC_
_EXTERNAL_GPL3_GCC_LIB_MAKEFILE_INC_=1

.sinclude "../../Makefile.gcc_path"
.sinclude "../../../Makefile.gcc_path"
.sinclude "../../../../Makefile.gcc_path"

WARNS=1

.endif
