#	$NetBSD: bsd.kernobj.mk,v 1.1 2000/05/02 03:43:15 sjg Exp $

#   KERNSRCDIR points to kernel source; it is set by default to ../sys,
#	but can be overridden.
#   KERNARCHDIR is the directory under kernel source that holds the md
#	stuff.  Default is arch/${MACHINE}.
#   KERNOBJDIR is the kernel build directory, it defaults to
#	${MAKEOBJDIRPREFIX}${KERNSRCDIR}/${KERNARCHDIR}/compile/KERNELNAME
#	if ${MAKEOBJDIRPREFIX}${KERNSRCDIR}/${KERNARCHDIR}/compile
#	exists otherwise it defaults to
#	${KERNSRCDIR}/${KERNARCHDIR}/compile/KERNELNAME, but can be
#	overridden.  Or
#   KERNCONFDIR is where the configuration files for kernels are found;
#	default is ${KERNSRCDIR}/${KERNARCHDIR}/conf but can be overridden.

# we need a top level Makefile.inc to set BSDSRCDIR
KERNSRCDIR?=	${BSDSRCDIR}/sys
# just incase ${MACHINE} is not always correct
KERNARCHDIR?=	arch/${MACHINE}

.if exists(${MAKEOBJDIRPREFIX}${KERNSRCDIR}/${KERNARCHDIR}/compile)
KERNOBJDIR?=	${MAKEOBJDIRPREFIX}${KERNSRCDIR}/${KERNARCHDIR}/compile
.else
KERNOBJDIR?=	${KERNSRCDIR}/${KERNARCHDIR}/compile
.endif
KERNCONFDIR?=	${KERNSRCDIR}/${KERNARCHDIR}/conf
