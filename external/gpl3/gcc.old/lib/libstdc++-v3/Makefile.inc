# $NetBSD: Makefile.inc,v 1.3 2015/09/23 03:39:30 mrg Exp $

.ifndef _EXTERNAL_GPL3_GCC_LIB_LIBSTDCXX_MAKEFILE_INC_
_EXTERNAL_GPL3_GCC_LIB_LIBSTDCXX_MAKEFILE_INC_=1

GCC_MACHINE_ARCH=${MACHINE_ARCH:S/earmv5/earm/}

.include "${.PARSEDIR}/../Makefile.inc"
.include "${.PARSEDIR}/../../Makefile.gcc_path"

.endif
