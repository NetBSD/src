#	$NetBSD: Makefile.inc,v 1.12 1998/10/09 10:41:34 itohy Exp $
#
#	etc.sparc/Makefile.inc -- sparc-specific etc Makefile targets
#

.ifdef DESTDIR

# the regular expression does a basename(1) on .CURDIR so that we don't
# have to keep looking up .. at compile time.

KERNSRCDIR?=	${.CURDIR:C/[^\/]+$//}sys
KERNOBJDIR?=	${KERNSRCDIR}/arch/${MACHINE}/compile
KERNCONFDIR?=	${KERNSRCDIR}/arch/${MACHINE}/conf

snap_md:
.for kernel in GENERIC GENERIC_SCSI3
	cd ${KERNCONFDIR} && config \
		-b ${KERNOBJDIR}/${kernel} \
		-s ${KERNSRCDIR} \
		${kernel}
	cd ${KERNOBJDIR}/${kernel} &&	\
		${MAKE} clean && ${MAKE} depend && ${MAKE} &&	\
		cp netbsd ${DESTDIR}/snapshot/netbsd.${kernel}
.endfor

.endif	# DESTDIR check
