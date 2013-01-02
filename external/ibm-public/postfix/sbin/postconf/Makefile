#	$NetBSD: Makefile,v 1.4 2013/01/02 19:45:49 tron Exp $

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

PSRCS=	postconf.c postconf_builtin.c postconf_dbms.c postconf_edit.c \
	postconf_main.c postconf_master.c postconf_misc.c postconf_node.c \
	postconf_other.c postconf_service.c postconf_unused.c postconf_user.c
GENSRCS=bool_table.h bool_vars.h int_table.h int_vars.h str_table.h \
	str_vars.h time_table.h time_vars.h raw_table.h raw_vars.h \
	nint_table.h nint_vars.h nbool_table.h nbool_vars.h long_table.h \
	long_vars.h

SRCS=	${PSRCS} ${GENSRCS}
DPSRCS=	${GENSRCS}

CLEANFILES+= ${GENSRCS}

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

.include <bsd.prog.mk>
