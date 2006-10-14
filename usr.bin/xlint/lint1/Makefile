#	$NetBSD: Makefile,v 1.37 2006/10/14 21:08:50 christos Exp $

.include <bsd.own.mk>

PROG=		lint1
SRCS=		cgram.y scan.l mem1.c mem.c err.c main1.c decl.c tree.c func.c \
		init.c emit.c emit1.c inittyp.c tyname.c print.c
MAN=		lint.7
YHEADER=

CPPFLAGS+=	-I${.CURDIR} -I. -DPASS=\"${PROG}.h\"

.if ${OBJECT_FMT} == "a.out"
CPPFLAGS+=	-DTARGET_OBJFMT_AOUT
.elif ${OBJECT_FMT} == "COFF"
CPPFLAGS+=	-DTARGET_OBJFMT_COFF
.elif ${OBJECT_FMT} == "ELF"
CPPFLAGS+=	-DTARGET_OBJFMT_ELF
.else
.BEGIN:
	@echo "Unrecognized OBJECT_FMT ${OBJECT_FMT}"
	@false
.endif

LINTFLAGS=	-aehpz
BINDIR=		/usr/libexec

CLEANFILES+=	${MAN}

.if ${USETOOLS} == "yes"
LINT1=		${TOOLDIR}/libexec/${MACHINE_GNU_PLATFORM}-lint1
.endif
LINT1?=		./${PROG}

${MAN}:		makeman ${LINT1:C/^\.\///} Makefile
	${_MKTARGET_CREATE}
	${HOST_SH} ${.ALLSRC:M*makeman} ${LINT1} -m >${.TARGET}

LDADD+=		-lm
.ifndef HOSTPROG
DPADD+=		${LIBM}

LDADD+=		-ll
DPADD+=		${LIBL}
.endif

.include <bsd.prog.mk>
