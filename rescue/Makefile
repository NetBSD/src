#	$NetBSD: Makefile,v 1.25 2010/02/05 09:44:23 roy Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

WARNS=		1
# XXX
.if ${MACHINE_ARCH} != "m68000"
DBG=		-Os
.endif

CRUNCHGEN_FLAGS=-d "${DBG}"

RESCUEDIR=	/rescue
CRUNCHBIN=	rescue
CRUNCHENV=	RESCUEDIR=${RESCUEDIR}
SMALLPROG=	0
LISTS=		${.CURDIR}/list
TARGETDIR=	${DESTDIR}/rescue
PARSELISTENV+=  TARGETDIR=${TARGETDIR:Q}

.for f in edlabel ldconfig pdisk
PROG_${f}!=	cd ${NETBSDSRCDIR}/sbin/${f} && ${MAKE} -V PROG
.if (${PROG_${f}} != "")
LISTS+=		${.CURDIR}/list.${f}
.endif
.endfor

.if ${USE_INET6} != "no"
LISTS+=		${.CURDIR}/list.inet6
.endif

.if ${MKCRYPTO} != "no"
LISTS+=		${.CURDIR}/list.crypto
CRUNCHENV+=	MKKERBEROS=no		# for ssh
.endif

LISTS+=		${.CURDIR}/list.ldd
LDD_ELF32DIR!=	cd ${NETBSDSRCDIR}/usr.bin/ldd/elf32 && ${PRINTOBJDIR}
LDD_ELF64DIR!=	cd ${NETBSDSRCDIR}/usr.bin/ldd/elf64 && ${PRINTOBJDIR}
PARSELISTENV+=	LDD_ELF32DIR=${LDD_ELF32DIR} LDD_ELF64DIR=${LDD_ELF64DIR}

#	Specially built objects to override the behaviour of
#	various library functions
#

OVERRIDEOBJS=	rcmd.o
.PATH:	${NETBSDSRCDIR}/lib/libc/net
CPPFLAGS.rcmd.c+=-I${NETBSDSRCDIR}/lib/libc/include -DRESCUEDIR=\"${RESCUEDIR}\"

LIBOVERRIDE=	liboverride.o
${LIBOVERRIDE}: ${OVERRIDEOBJS}
	${_MKTARGET_LINK}
	${LD} -r -o $@ ${OVERRIDEOBJS}

CLEANFILES+=	${OVERRIDEOBJS} ${LIBOVERRIDE} terminfo.db

${CRUNCHBIN}:	${LIBOVERRIDE}


#	The primary target ...
#

CLEANFILES+=	rescue.unstripped

.include "${DISTRIBDIR}/common/Makefile.crunch"

realall: ${CRUNCHBIN} terminfo.db

terminfo.db: terminfo
	${_MKTARGET_CREATE}
	${TOOL_TIC} -x -o "${.OBJDIR}/terminfo" "${.CURDIR}/terminfo" 

install: ${CRUNCHBIN}
# XXX this MKMSG doesn't line up
	${_MKMSG} "populate ${TARGETDIR}"
	${PARSELIST} -v mode=install ${LISTS} | ${MAKE} -f - install

.include <bsd.prog.mk>
