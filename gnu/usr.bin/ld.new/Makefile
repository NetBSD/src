#	$NetBSD: Makefile,v 1.5 1999/01/30 12:38:32 tv Exp $

# XXX set MACHINE_GNU_ARCH for _<arch>.mk
.include <bsd.own.mk>

PROG=		ld

SRCS=		ldctor.c ldemul.c ldexp.c ldfile.c ldlang.c ldmain.c ldmisc.c \
		ldver.c ldwrite.c lexsup.c mri.c ldcref.c ldgram.y ldlex.l
MAN=		ld.1
YHEADER=1
SCRIPTDIR=	/usr/libdata

CPPFLAGS+=	-I. -I${.CURDIR} -I${BFDOBJ} -I${DIST}/ld \
		-I${DIST}/bfd -I${DIST}/include \
		-DDEFAULT_EMULATION='"${DEFAULT_EMUL}"' \
		-DSCRIPTDIR=\"${SCRIPTDIR}\" \
		-DTARGET='"${MACHINE_GNU_ARCH}--netbsd"' \
		-DEMULATION_LIST='${EMULS:S/^/\&ld_/:S/$/_emulation,/} 0'

# gets these from BFDOBJ since ld uses bfd internal headers
DPADD+=		${BFDOBJ}/libbfd_pic.a
LDADD+=		-L${BFDOBJ} -lbfd

DIST=		${.CURDIR}/../../dist
BFDOBJ!=	cd ${.CURDIR}/../../lib/bfd; ${MAKE} print-objdir

.PATH:		${DIST}/ld

.include "_${MACHINE_ARCH}.mk"

LIB_PATH=	${LIBDIR}	# passed to genscripts.sh

.for _EMUL_ in ${EMULS}
SRCS+=		e${_EMUL_}.c
CLEANFILES+=	e${_EMUL_}.c
e${_EMUL_}.c: ${DIST}/ld/genscripts.sh ${DIST}/ld/emulparams/${_EMUL_}.sh
	sh ${DIST}/ld/genscripts.sh ${DIST}/ld ${LIBDIR} \
		${MACHINE_GNU_ARCH}-netbsd ${MACHINE_GNU_ARCH}-netbsd \
		${MACHINE_GNU_ARCH}-netbsd ${DEFAULT_EMUL} \
		"" ${_EMUL_}
.endfor

afterinstall:
	pax -rw ldscripts ${DESTDIR}${SCRIPTDIR}

cleanprog: __cleanldscripts
__cleanldscripts:
	rm -rf ldscripts

# Make sure no generated files exist in the src tree before depend or build.
# On a system where `.depend' is correct, this command does nothing.
beforedepend:
	@rm -f ${DIST}/ld/ldgram.c ${DIST}/ld/ldgram.h ${DIST}/ld/ldlex.c

.include <bsd.prog.mk>
