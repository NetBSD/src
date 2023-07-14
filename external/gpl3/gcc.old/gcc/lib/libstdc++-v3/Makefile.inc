# $NetBSD: Makefile.inc,v 1.1.1.1 2023/07/14 06:34:26 mrg Exp $

.ifndef _EXTERNAL_GPL3_GCC_LIB_LIBSTDCXX_MAKEFILE_INC_
_EXTERNAL_GPL3_GCC_LIB_LIBSTDCXX_MAKEFILE_INC_=1

GCC_MACHINE_ARCH=${MACHINE_ARCH:S/earmv5/earm/}

.include "${.PARSEDIR}/../Makefile.inc"
.include "${.PARSEDIR}/../../Makefile.gcc_path"

.endif
