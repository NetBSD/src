#	$NetBSD: Makefile,v 1.5 2001/03/29 09:59:09 tron Exp $

# XXX Note we aren't building ../conf/main.cf.default
# The shipped makefiles construct it using postconf -d after building
# postconf. It isn't entirely clear how to deal with that in a cross
# build environment, and the .default file isn't that useful to
# the user anyway.

PROG=	postconf

DIST=	${.CURDIR}/../../../dist/postfix/src/${PROG}
.PATH:	${DIST}

BINDIR= ${PFIX_SBINDIR}           

PSRCS=	postconf.c
GENSRCS=bool_table.h bool_vars.h int_table.h int_vars.h raw_table.h \
	raw_vars.h str_table.h str_vars.h time_table.h time_vars.h

SRCS=	${PSRCS} ${GENSRCS}

CLEANFILES+= ${GENSRCS}

CPPFLAGS+= -I${.OBJDIR}

DPADD+= ${LIBPGLOBAL} ${LIBPUTIL}
LDADD+= ${LIBPGLOBAL} ${LIBPUTIL}

NOMAN=1

# XXX Note that there is no ${AWK} variable in our build system. This
# is probably a deficiency.
${GENSRCS}: ${DIST}/../global/mail_params.h
	awk -f ${DIST}/extract.awk ${DIST}/../*/*.c

.include <bsd.prog.mk>

${OBJS}: ${GENSRCS}
