# $NetBSD: Makefile.inc,v 1.9 2021/04/09 23:15:42 mrg Exp $

.ifndef _EXTERNAL_GPL3_GCC_LIB_LIBSTDCXX_MAKEFILE_INC_
_EXTERNAL_GPL3_GCC_LIB_LIBSTDCXX_MAKEFILE_INC_=1

GCC_MACHINE_ARCH=${MACHINE_ARCH:S/earmv5/earm/}

.include "${.PARSEDIR}/../Makefile.inc"
.include "${.PARSEDIR}/../../Makefile.gcc_path"

.endif
