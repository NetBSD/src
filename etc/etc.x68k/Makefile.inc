#	$NetBSD: Makefile.inc,v 1.5 1999/01/03 14:49:39 minoura Exp $
#
#	etc.x68k/Makefile.inc -- x68k-specific etc Makefile targets
#

.ifdef DESTDIR
LOCALTIME=	Japan

# the regular expression does a basename(1) on .CURDIR so that we don't
# have to keep looking up .. at compile time.

KERNSRCDIR?=	${.CURDIR:C/[^\/]+$//}sys
KERNOBJDIR?=	${KERNSRCDIR}/arch/${MACHINE}/compile
KERNCONFDIR?=	${KERNSRCDIR}/arch/${MACHINE}/conf

snap_md:
.for kernel in GENERIC SMALL
	cd ${KERNCONFDIR} && config \
		-b ${KERNOBJDIR}/${kernel} \
		-s ${KERNSRCDIR} \
		${kernel}
	cd ${KERNOBJDIR}/${kernel} &&	\
		${MAKE} clean && ${MAKE} depend && ${MAKE} &&	\
		gzip -c -9 netbsd > ${DESTDIR}/snapshot/netbsd.${kernel}.gz
.endfor

.endif	# DESTDIR check
