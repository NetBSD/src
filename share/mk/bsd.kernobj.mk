#	$NetBSD: bsd.kernobj.mk,v 1.4 2000/05/07 01:20:47 sjg Exp $

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
# 		${MAKEOBJDIRPREFIX}${KERNSRCDIR}/${KERNARCHDIR}/compile
# 		if it exists or the target 'obj' is being made.
# 		Otherwise the default is
# 		${KERNSRCDIR}/${KERNARCHDIR}/compile.
# 

KERNSRCDIR?=	${BSDSRCDIR}/sys
# just incase ${MACHINE} is not always correct
KERNARCHDIR?=	arch/${MACHINE}

.if make(obj) || exists(${MAKEOBJDIRPREFIX}${KERNSRCDIR}/${KERNARCHDIR}/compile)
KERNOBJDIR?=	${MAKEOBJDIRPREFIX}${KERNSRCDIR}/${KERNARCHDIR}/compile
.else
KERNOBJDIR?=	${KERNSRCDIR}/${KERNARCHDIR}/compile
.endif
KERNCONFDIR?=	${KERNSRCDIR}/${KERNARCHDIR}/conf
