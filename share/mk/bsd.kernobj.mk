#	$NetBSD: bsd.kernobj.mk,v 1.5 2001/11/27 05:11:41 jmc Exp $

# KERNSRCDIR	Is the location of the top of the kernel src.
# 		It defaults to ${BSDSRCDIR}/sys, but the top-level
# 		Makefile.inc sets it to ${ABSTOP}/sys (ABSTOP is the
# 		absolute path to the directory where the top-level
# 		Makefile.inc was found.
# 
# KERNARCHDIR	Is the location of the machine dependent kernel
# 		sources.  It defaults to arch/${MACHINE}
# 		
# KERNCONFDIR	Is where the configuration files for kernels are
# 		found; default is ${KERNSRCDIR}/${KERNARCHDIR}/conf.
# 
# KERNOBJDIR	Is the kernel build directory.  The kernel GENERIC for
# 		instance will be compiled in ${KERNOBJDIR}/GENERIC.
# 		The default value is
# 		${KERNSRCDIR}/${KERNARCHDIR}/compile
#
#		If MAKEOBJDIRPREFIX or _SRC_TOP_OBJ is set than the value will
#		be either 
#
#		${MAKEOBJDIRPREFIX}${KERNSRCDIR}/${KERNARCHDIR}/compile
#
#		or
#
#		${_SRC_TOP_OBJ_}/sys/${KERNARCHDIR}/compile
#
#		with MAKEOBJDIRPREFIX taking priority over _SRC_TOP_OBJ_ 
# 

.include <bsd.own.mk>

KERNSRCDIR?=	${BSDSRCDIR}/sys
# just incase ${MACHINE} is not always correct
KERNARCHDIR?=	arch/${MACHINE}

.if defined(MAKEOBJDIRPREFIX)
KERNOBJDIR?=    ${MAKEOBJDIRPREFIX}${KERNSRCDIR}/${KERNARCHDIR}/compile
.else
.if defined(_SRC_TOP_OBJ_) && ${_SRC_TOP_OBJ_} != ""
KERNOBJDIR?=	${_SRC_TOP_OBJ_}/sys/${KERNARCHDIR}/compile
.else
KERNOBJDIR?=	${KERNSRCDIR}/${KERNARCHDIR}/compile
.endif
.endif

KERNCONFDIR?=	${KERNSRCDIR}/${KERNARCHDIR}/conf
