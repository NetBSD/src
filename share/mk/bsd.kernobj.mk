#	$NetBSD: bsd.kernobj.mk,v 1.7 2001/11/27 05:39:03 jmc Exp $

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

#
# XXX It's ugly but it does what we need here. If making objects use the above
# rules for trying to figure out a KERNOBJDIR.
#
# When coming back through here in rules (such as building kernels for
# a release), check which vars we're using and which directory base has been
# made in the previous obj stage to figure out which one to expose.
#
# All cases will fall through to the ${KERNSRCDIR}/${KERNARCHDIR}/compile case
# if nothing ends up setting this.
.if make(obj) || \
    (defined(MAKEOBJDIRPREFIX) && exists(${MAKEOBJDIRPREFIX}${KERNSRCDIR}/${KERNARCHDIR}/compile)) || \
    (defined(_SRC_TOP_OBJ_) && exists(${_SRC_TOP_OBJ_}/sys/${KERNARCHDIR}/compile))
.if defined (MAKEOBJDIRPREFIX)
KERNOBJDIR?=    ${MAKEOBJDIRPREFIX}${KERNSRCDIR}/${KERNARCHDIR}/compile
.else
.if defined(_SRC_TOP_OBJ_) && ${_SRC_TOP_OBJ_} != ""
KERNOBJDIR?=	${_SRC_TOP_OBJ_}/sys/${KERNARCHDIR}/compile
.endif
.endif
.endif

KERNOBJDIR?=	${KERNSRCDIR}/${KERNARCHDIR}/compile

KERNCONFDIR?=	${KERNSRCDIR}/${KERNARCHDIR}/conf
