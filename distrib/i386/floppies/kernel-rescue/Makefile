#	$NetBSD: Makefile,v 1.5 2002/02/03 16:22:24 lukem Exp $

.include "../Makefile.inc"
.include <bsd.kernobj.mk>
.include <bsd.own.mk>

#	create ${RAMDISK_*} variables
#
RAMDISKS=	RAMDISK_S	ramdisk-rescuesmall	\
		RAMDISK_T	ramdisk-rescuetiny

.for V F in ${RAMDISKS}
${V}!=		cd ${.CURDIR}/../${F}/; \
	        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/${F}.fs\n" | \
		${MAKE} -s -f-
.endfor

#	create netbsd.*.gz targets
#
#	TARGETS is a list of:
#		KERNEL_name	fs-image
#
TARGETS=	INSTALL_SMALL	${RAMDISK_S}	\
		INSTALL_TINY	${RAMDISK_T}

.for K R in ${TARGETS}
KERNELS+=	netbsd.${K}.gz
KERNELSYMS+=	netbsd.${K}.symbols
netbsd.${K}.gz: .NOTMAIN ${KERNOBJDIR}/${K}/netbsd ${R}
	@echo "Populating ${K} with ${R}"
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

clean cleandir distclean:
	rm -f *.core netbsd.tmp ${KERNELS} ${KERNELSYMS}

.include <bsd.obj.mk>
.include <bsd.prog.mk>
