#	$NetBSD: Makefile,v 1.1 2009/06/25 18:21:59 tron Exp $

# XXX Note we aren't building ../conf/main.cf.default
# The shipped makefiles construct it using postconf -d after building
# postconf. It isn't entirely clear how to deal with that in a cross
# build environment, and the .default file isn't that useful to
# the user anyway.

NOMAN=	# defined

.include <bsd.own.mk>

PROG=	postconf

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/src/${PROG}
.PATH:	${DIST}

PSRCS=	postconf.c
GENSRCS=bool_table.h bool_vars.h int_table.h int_vars.h str_table.h \
	str_vars.h time_table.h time_vars.h raw_table.h raw_vars.h
AUTOSRCS=auto_table.h auto_vars.h

SRCS=	${PSRCS} ${GENSRCS} ${AUTOSRCS}
DPSRCS=	${GENSRCS} ${AUTOSRCS}

CLEANFILES+= ${GENSRCS} ${AUTOSRCS}

CPPFLAGS+= -I.

DPADD+= ${LIBPGLOBAL} ${LIBPXSASL} ${LIBPUTIL}
LDADD+= ${LIBPGLOBAL} ${LIBPXSASL} ${LIBPUTIL}

${GENSRCS}: postconf-hdrs.stamp
CLEANFILES+= postconf-hdrs.stamp
postconf-hdrs.stamp: ${DIST}/../global/mail_params.h ${DIST}/../global/mail_params.c
	${_MKMSG_CREATE} ${GENSRCS}
	rm -f ${.TARGET}
	${TOOL_AWK} -f ${DIST}/extract.awk ${DIST}/../*/*.c | ${HOST_SH} -
	touch ${.TARGET}

auto_table.h auto_vars.h: ${DIST}/auto.awk
	${TOOL_AWK} -f ${DIST}/auto.awk

.include <bsd.prog.mk>
