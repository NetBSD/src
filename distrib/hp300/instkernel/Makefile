#	$NetBSD: Makefile,v 1.2 2002/02/13 06:01:36 gmcgarry Exp $

TOP=	${.CURDIR}/..

.include <bsd.kernobj.mk>
.include <bsd.own.mk>

RAMDISKS=	RAMDISK	ramdisk

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
TARGETS=	RAMDISK		${RAMDISK}	-

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

#	do the work
#
all: ${KERNELS}

release:
	-mkdir -p ${RELEASEDIR}/binary/kernel
	cp -p ${KERNELS} ${KERNELSYMS} ${RELEASEDIR}/binary/kernel

clean cleandir distclean:
	rm -f *.core netbsd.tmp ${KERNELS} ${KERNELSYMS}

.include <bsd.obj.mk>
.include <bsd.prog.mk>
