# $NetBSD: Makefile,v 1.5 2023/06/20 17:23:04 christos Exp $

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

USE_FORT?= yes	# network client

.PATH: ${HEIMDIST}/kadmin

PROG=	kadmin
MAN=	kadmin.8

HEIMSRCS= kadmin-commands.in

SRCS=	add_enctype.c				\
	ank.c					\
	check.c					\
	cpw.c					\
	del.c					\
	del_enctype.c				\
	dump.c					\
	ext.c					\
	get.c					\
	init.c					\
	kadmin.c				\
	load.c					\
	mod.c					\
	pw_quality.c				\
	random_password.c			\
	rename.c				\
	stash.c					\
	util.c

CPPFLAGS+= 				\
	-I${HEIMDIST}/base		\
	-I${HEIMDIST}/lib/krb5		\
	-I${HEIMDIST}/lib/asn1		\
	-I${HEIMDIST}/lib/wind		\
	-I${HEIMBASE}/lib/libwind	\
	-I${DESTDIR}/usr/include/kadm5

PROGDPLIBS+= ${DPLIBASN1} ${DPLIBKADM5SRV} ${DPLIBHDB} ${DPLIBKADM5CLNT}
PROGDPLIBS+= ${DPLIBSL}
LDADD+= -lcrypto -lutil -ledit -lterminfo
DPADD+= ${LIBCRYPTO} ${LIBUTIL} ${LIBEDIT} ${LIBTERMINFO}

kadmin.8: kadmin.1
	@${TOOL_SED} -e 's/KADMIN 1/KADMIN 8/' < ${.ALLSRC} > ${.TARGET}

CLEANFILES+=kadmin.8

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>
