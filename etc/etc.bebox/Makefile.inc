#	$NetBSD: Makefile.inc,v 1.1 1998/11/18 08:49:33 sakamoto Exp $
#
#	etc.bebox/Makefile.inc -- bebox-specific etc Makefile targets
#

.ifdef DESTDIR

# the regular expression does a basename(1) on .CURDIR so that we don't
# have to keep looking up .. at compile time.

KERNSRCDIR?=	${.CURDIR:C/[^\/]+$//}sys
KERNOBJDIR?=	${KERNSRCDIR}/arch/${MACHINE}/compile
KERNCONFDIR?=	${KERNSRCDIR}/arch/${MACHINE}/conf

snap_md:
.for kernel in GENERIC
	cd ${KERNCONFDIR} && config \
		-b ${KERNOBJDIR}/${kernel} \
		-s ${KERNSRCDIR} \
		${kernel}
	cd ${KERNOBJDIR}/${kernel} &&	\
		${MAKE} clean && ${MAKE} depend && ${MAKE} &&	\
		cp netbsd ${DESTDIR}/snapshot/netbsd.${kernel}
.endfor

.endif	# DESTDIR check
