#	$NetBSD: Makefile.inc,v 1.5.2.1 2018/11/26 01:51:44 pgoyette Exp $

GCC_MACHINE_ARCH=${MACHINE_ARCH:S/earmv5/earm/}

.ifndef _EXTERNAL_GPL3_GCC_LIB_MAKEFILE_INC_
_EXTERNAL_GPL3_GCC_LIB_MAKEFILE_INC_=1

.sinclude "../../Makefile.gcc_path"
.sinclude "../../../Makefile.gcc_path"
.sinclude "../../../../Makefile.gcc_path"

WARNS=1

.endif
