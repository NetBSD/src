#	$NetBSD: bsd.kernobj.mk,v 1.11 2003/07/18 04:04:03 lukem Exp $

# KERNSRCDIR	Is the location of the top of the kernel src.
# 		It defaults to `${NETBSDSRCDIR}/sys'.
#
# KERNARCHDIR	Is the location of the machine dependent kernel sources.
#		It defaults to `arch/${MACHINE}', but may be overridden
#		in case ${MACHINE} is not correct.
#
# KERNCONFDIR	Is where the configuration files for kernels are found.
#		It defaults to `${KERNSRCDIR}/${KERNARCHDIR}/conf'.
#
# KERNOBJDIR	Is the kernel build directory.  The kernel GENERIC for
# 		instance will be compiled in ${KERNOBJDIR}/GENERIC.
# 		The default is the .OBJDIR of
#		`${KERNSRCDIR}/${KERNARCHDIR}/compile'.
#

.include <bsd.own.mk>

KERNSRCDIR?=	${NETBSDSRCDIR}/sys
KERNARCHDIR?=	arch/${MACHINE}
KERNCONFDIR?=	${KERNSRCDIR}/${KERNARCHDIR}/conf
.if !defined(KERNOBJDIR)
KERNOBJDIR!=	cd ${KERNSRCDIR}/${KERNARCHDIR}/compile && ${PRINTOBJDIR}
.endif
