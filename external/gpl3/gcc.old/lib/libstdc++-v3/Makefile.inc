# $NetBSD: Makefile.inc,v 1.5.2.1 2018/11/26 01:51:56 pgoyette Exp $

.ifndef _EXTERNAL_GPL3_GCC_LIB_LIBSTDCXX_MAKEFILE_INC_
_EXTERNAL_GPL3_GCC_LIB_LIBSTDCXX_MAKEFILE_INC_=1

GCC_MACHINE_ARCH=${MACHINE_ARCH:S/earmv5/earm/}

.include "${.PARSEDIR}/../Makefile.inc"
.include "${.PARSEDIR}/../../Makefile.gcc_path"

.endif
