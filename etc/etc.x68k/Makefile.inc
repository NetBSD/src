#	$NetBSD: Makefile.inc,v 1.3 1998/06/28 09:38:44 fair Exp $
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
.for kernel in GENERIC ALL
	cd ${KERNCONFDIR} && config \
		-b ${KERNOBJDIR}/${kernel} \
		-s ${KERNSRCDIR} \
		${kernel}
	cd ${KERNOBJDIR}/${kernel} && make clean && make depend && make && \
		gzip -c -9 netbsd > ${DESTDIR}/snapshot/netbsd.${kernel}.gz
.endfor

.endif	# DESTDIR check
