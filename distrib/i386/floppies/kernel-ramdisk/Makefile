#	$NetBSD: Makefile,v 1.12 2002/02/27 13:42:35 lukem Exp $

TOP=	${.CURDIR}/..

.include <bsd.own.mk>

.include "${TOP}/Makefile.inc"
.include <bsd.kernobj.mk>

#	create ${RAMDISK_*} variables
#
RAMDISKS=	RAMDISK_B	ramdisk-big	\
		RAMDISK_S	ramdisk-small	\
		RAMDISK_T	ramdisk-tiny

.for V F in ${RAMDISKS}
${V}!=		cd ${TOP}/${F}/; \
	        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/${F}.fs\n" | \
		${MAKE} -s -f-
.endfor

#	create netbsd.*.gz targets
#
#	TARGETS is a list of:
#		KERNEL_name	fs-image	"nocluster" disables clustering
#
TARGETS=	INSTALL		${RAMDISK_B}	-		\
		INSTALL_LAPTOP	${RAMDISK_B}	-		\
		INSTALL_SMALL	${RAMDISK_S}	-		\
		INSTALL_TINY	${RAMDISK_T}	nocluster	\
		INSTALL_PS2	${RAMDISK_S}	nocluster

.for K R E in ${TARGETS}
_E:=${E}			# work around obscure issue in make(1)
KERNELS+=	netbsd.${K}.gz
KERNELSYMS+=	netbsd.${K}.symbols
netbsd.${K}.gz: .NOTMAIN ${KERNOBJDIR}/${K}/netbsd ${R}
	@echo "Populating ${K} with ${R}"
	cp ${KERNOBJDIR}/${K}/netbsd netbsd.tmp
.if ${_E} == "nocluster"
	gdb --write -batch -x ${.CURDIR}/nocluster.gdb netbsd.tmp </dev/null
.endif
	${MDSETIMAGE} -v netbsd.tmp ${R}
	${NM} netbsd.tmp > netbsd.${K}.symbols
	${STRIP} netbsd.tmp
	gzip -9 netbsd.tmp
	mv netbsd.tmp.gz ${.TARGET}
.endfor

CLEANFILES+=	netbsd.tmp ${KERNELS} ${KERNELSYMS}

#	do the work
#
realall: ${KERNELS}

release: check_RELEASEDIR
	-mkdir -p ${RELEASEDIR}/binary/kernel
	cp -p ${KERNELS} ${KERNELSYMS} ${RELEASEDIR}/binary/kernel

.include <bsd.prog.mk>
