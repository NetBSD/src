#
#	etc.amiga/Makefile.inc -- amiga-specific etc Makefile targets
#
#	$NetBSD: Makefile.inc,v 1.10 1998/10/09 10:40:59 itohy Exp $

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
		gzip -c -9 netbsd > ${DESTDIR}/snapshot/netbsd.${kernel}.gz
.endfor

.endif	# DESTDIR check
