#	$NetBSD: Makefile,v 1.4 2002/03/12 06:39:18 lukem Exp $

.include "${.CURDIR}/../../Makefile.inc"
.include <bsd.own.mk>
.include <bsd.kernobj.mk>

CLEANFILES=	netbsd.tmp

RAMDISKS=	RAMDISK	ramdisk

.for V F in ${RAMDISKS}
${V}DIR!=	cd ${.CURDIR}/../${F} && ${PRINTOBJDIR}
${V}=		${${V}DIR}/${F}.fs
.endfor

#	create netbsd.*.gz targets
#
#	TARGETS is a list of:
#		KERNEL_name	fs-image
#
TARGETS=	RAMDISK		${RAMDISK}

.for K R in ${TARGETS}
KERNELS+=	netbsd.${K}.gz
KERNELSYMS+=	netbsd.${K}.symbols
CLEANFILES+=	netbsd.${K}.gz netbsd.${K}.symbols
netbsd.${K}.gz: .NOTMAIN ${KERNOBJDIR}/${K}/netbsd ${R}
	@echo "Populating ${.TARGET} with ${R}"
	cp ${KERNOBJDIR}/${K}/netbsd netbsd.tmp
	${MDSETIMAGE} -v netbsd.tmp ${R}
	${NM} netbsd.tmp > netbsd.${K}.symbols
	${STRIP} netbsd.tmp
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}
.endfor

#	do the work
#
all: ${KERNELS}

release:
	-mkdir -p ${RELEASEDIR}/binary/kernel
	cp -p ${KERNELS} ${KERNELSYMS} ${RELEASEDIR}/binary/kernel

.include <bsd.prog.mk>
