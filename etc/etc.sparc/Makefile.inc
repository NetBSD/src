#	$NetBSD: Makefile.inc,v 1.10 1998/06/24 08:15:47 fair Exp $
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
	cd ${KERNOBJDIR}/${kernel} && make clean && make depend && make
.endfor
	cp ${KERNOBJDIR}/GENERIC/netbsd \
		${DESTDIR}/snapshot/netbsd
	cp ${KERNOBJDIR}/GENERIC_SCSI3/netbsd \
		${DESTDIR}/snapshot/netbsd.scsi3

.endif	# DESTDIR check
