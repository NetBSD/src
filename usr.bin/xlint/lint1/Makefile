#	$NetBSD: Makefile,v 1.18 2001/10/24 01:12:45 tv Exp $

.include <bsd.own.mk>

PROG=	lint1
SRCS=	cgram.y scan.l mem1.c mem.c err.c main1.c decl.c tree.c func.c \
	init.c emit.c emit1.c
MAN=	lint.7
LDADD+=	-ll
DPADD+=	${LIBL}
YHEADER=1

CPPFLAGS+=-I${.CURDIR} -I.
.if ${OBJECT_FMT} == "a.out"
CPPFLAGS+=-DTARGET_OBJFMT_AOUT
.elif ${OBJECT_FMT} == "COFF"
CPPFLAGS+=-DTARGET_OBJFMT_COFF
.elif ${OBJECT_FMT} == "ELF"
CPPFLAGS+=-DTARGET_OBJFMT_ELF
.else
.BEGIN:
	@echo "Unrecognized OBJECT_FMT ${OBJECT_FMT}"
	@false
.endif

LINTFLAGS=-aehpz
WFORMAT=1		#hopeless
BINDIR=	/usr/libexec

scan.c: cgram.h

lint.7:	${PROG}
	sh ${.CURDIR}/makeman ${.OBJDIR}/${PROG} -m > ${.TARGET}

CLEANFILES+=${MAN}

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
