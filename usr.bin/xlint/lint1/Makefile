#	$NetBSD: Makefile,v 1.48 2013/05/10 15:31:22 christos Exp $

.include <bsd.own.mk>

PROG=		lint1
SRCS=		cgram.y decl.c emit.c emit1.c err.c func.c init.c inittyp.c \
		main1.c mem.c mem1.c print.c scan.l tree.c tyname.c

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
