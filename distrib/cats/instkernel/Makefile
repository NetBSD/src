#	$NetBSD: Makefile,v 1.1 2002/04/11 16:33:59 skrll Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

.include <bsd.kernobj.mk>

CLEANFILES=	netbsd.tmp netbsd.tmp.gz

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
TARGETS=	INSTALL		${RAMDISK}

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
	OBJCOPY=${OBJCOPY:Q} SIZE=${SIZE:Q} \
	    ${_SRC_TOP_}/sys/arch/arm/conf/elf2aout.sh netbsd.tmp netbsd.tmp
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}
.endfor

#	do the work
#
realall: ${KERNELS}

release:
	-mkdir -p ${RELEASEDIR}/binary/kernel
	cp -p ${KERNELS} ${KERNELSYMS} ${RELEASEDIR}/binary/kernel

.include <bsd.prog.mk>
