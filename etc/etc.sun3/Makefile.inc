#	$NetBSD: Makefile.inc,v 1.5 1998/06/25 17:54:05 fair Exp $
#
#	etc.sun3/Makefile.inc -- sun3-specific etc Makefile targets
#

BIN3+= etc.sun3/ttyaction

.ifdef DESTDIR

# the regular expression does a basename(1) on .CURDIR so that we don't
# have to keep looking up .. at compile time.

KERNSRCDIR?=	${.CURDIR:C/[^\/]+$//}sys
KERNOBJDIR?=	${KERNSRCDIR}/arch/${MACHINE}/compile
KERNCONFDIR?=	${KERNSRCDIR}/arch/${MACHINE}/conf

snap_md: netbsd
	cp ${KERNOBJDIR}/GENERIC/netbsd \
		${DESTDIR}/snapshot/netbsd.sun3
	cp ${KERNOBJDIR}/GENERIC3X/netbsd \
		${DESTDIR}/snapshot/netbsd.sun3x

netbsd:
.for kernel in GENERIC GENERIC3X
	cd ${KERNCONFDIR} && config \
		-b ${KERNOBJDIR}/${kernel} \
		-s ${KERNSRCDIR} \
		${kernel}
	cd ${KERNOBJDIR}/${kernel} && make clean && make depend && make
.endfor

.endif	# DESTDIR check
