#	$NetBSD: Makefile.inc,v 1.1.1.2 2015/09/23 03:03:38 mrg Exp $

GCC_MACHINE_ARCH=${MACHINE_ARCH:S/earmv5/earm/}

.ifndef _EXTERNAL_GPL3_GCC_LIB_MAKEFILE_INC_
_EXTERNAL_GPL3_GCC_LIB_MAKEFILE_INC_=1

.sinclude "../../Makefile.gcc_path"
.sinclude "../../../Makefile.gcc_path"
.sinclude "../../../../Makefile.gcc_path"

.endif
