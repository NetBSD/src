#	$NetBSD: Makefile,v 1.1 1998/08/27 20:47:21 tv Exp $

PROG=		ld

SRCS=		ldctor.c ldemul.c ldexp.c ldfile.c ldlang.c ldmain.c ldmisc.c \
		ldver.c ldwrite.c lexsup.c mri.c ldcref.c ldgram.y ldlex.l

CPPFLAGS+=	-I. -I${.CURDIR} -I${BFDOBJ} -I${DIST}/ld \
		-I${DIST}/bfd -I${DIST}/include \
		-DDEFAULT_EMULATION='"${DEFAULT_EMUL}"' \
		-DSCRIPTDIR='"/usr/libdata"' \
		-DTARGET='"${MACHINE_GNU_ARCH}--netbsd"' \
		-DEMULATION_LIST='${EMULS:S/^/\&ld_/:S/$/_emulation,/} 0'
YHEADER=1
DPADD+=		${BFDOBJ}/libbfd_pic.a
LDADD+=		-L${BFDOBJ} -lbfd

DIST=		${.CURDIR}/../../dist
BFDOBJ!=	cd ${.CURDIR}/../../lib/bfd; ${MAKE} print-objdir

.PATH:		${DIST}/ld

.include "${MACHINE_ARCH}.mk"

LIB_PATH=	${LIBDIR}	# passed to genscripts.sh

.for _EMUL_ in ${EMULS}
SRCS+=		e${_EMUL_}.c
CLEANFILES+=	e${_EMUL_}.c ${_EMUL_}.x*
e${_EMUL_}.c: ${DIST}/ld/genscripts.sh ${DIST}/ld/emulparams/${_EMUL_}.sh
	sh ${DIST}/ld/genscripts.sh ${DIST}/ld /usr/lib \
		${MACHINE_GNU_ARCH}-netbsd ${MACHINE_GNU_ARCH}-netbsd \
		${MACHINE_GNU_ARCH}-netbsd ${DEFAULT_EMUL} \
		"" ${_EMUL_}
.endfor

cleanprog: __cleanldscripts
__cleanldscripts:
	rm -rf ldscripts

.include <bsd.prog.mk>
