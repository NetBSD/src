# $NetBSD: Makefile.inc,v 1.5.2.1 2018/11/26 01:52:00 pgoyette Exp $

.ifndef _EXTERNAL_GPL3_GCC_LIB_LIBSTDCXX_INCLUDE_MAKEFILE_INC_
_EXTERNAL_GPL3_GCC_LIB_LIBSTDCXX_INCLUDE_MAKEFILE_INC_=1

.include "../Makefile.inc"
.include "../../../Makefile.gcc_path"

DIST=${GCCDIST}

.endif
