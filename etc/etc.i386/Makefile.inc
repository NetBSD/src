#	$NetBSD: Makefile.inc,v 1.10 1998/06/28 09:38:42 fair Exp $
#
#	etc.i386/Makefile.inc -- i386-specific etc Makefile targets
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
	cd ${KERNOBJDIR}/${kernel} && make clean && make depend && make && \
		cp netbsd ${DESTDIR}/snapshot/netbsd.${kernel}
.endfor

.endif	# DESTDIR check
