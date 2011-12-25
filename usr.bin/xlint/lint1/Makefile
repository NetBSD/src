#	$NetBSD: Makefile,v 1.47 2011/12/25 01:05:44 christos Exp $

.include <bsd.own.mk>

PROG=		lint1
SRCS=		cgram.y scan.l mem1.c mem.c err.c main1.c decl.c tree.c func.c \
		init.c emit.c emit1.c inittyp.c tyname.c print.c
MAN=		lint.7
YHEADER=
#DBG=-g
#CPPFLAGS+=-DYYDEBUG=1
#YFLAGS+=-v

CPPFLAGS+=	-I${.CURDIR} -I. -DPASS=\"${PROG}.h\"

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

.include "Makefile.ops-h"
.include "Makefile.ops-c"

.include <bsd.prog.mk>
