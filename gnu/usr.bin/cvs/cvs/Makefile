#	$NetBSD: Makefile,v 1.6 2002/04/10 22:04:51 thorpej Exp $

.include "${.CURDIR}/../Makefile.inc"

PROG=	cvs

DIST=	${IDIST}/src
.PATH:	${DIST} ${IDIST}/man

MAN=	cvs.1 cvs.5
SRCS=	add.c admin.c buffer.c checkin.c checkout.c classify.c \
	client.c commit.c create_adm.c cvsrc.c diff.c edit.c \
	entries.c error.c expand_path.c fileattr.c filesubr.c \
	find_names.c hardlink.c hash.c history.c ignore.c import.c \
	lock.c log.c login.c logmsg.c main.c mkmodules.c modules.c \
	myndbm.c no_diff.c parseinfo.c patch.c rcs.c rcscmds.c \
	recurse.c release.c remove.c repos.c root.c rtag.c run.c \
	scramble.c server.c status.c subr.c tag.c update.c vers_ts.c \
	version.c watch.c wrapper.c zlib.c

DPADD+= ${LIBDIFF} ${LIBCVS} ${LIBCRYPT} ${LIBZ}
LDADD+= ${LIBDIFF} ${LIBCVS} -lcrypt -lz

.if (${USE_KERBEROS} != "no")
CPPFLAGS+= -DENCRYPTION
CPPFLAGS+= -DHAVE_KERBEROS -I${DESTDIR}/usr/include/kerberosIV
DPADD+= ${LIBKRB} ${LIBDES} ${LIBCOM_ERR} ${LIBROKEN} ${LIBCRYPT}
LDADD+= -lkrb -ldes -lcom_err -lroken -lcrypt
CPPFLAGS+= -DHAVE_GSSAPI -I${DESTDIR}/usr/include/krb5
DPADD+= ${LIBGSSAPI} ${LIBKRB5} ${LIBDES} ${LIBASN1} ${LIBCOM_ERR} ${LIBROKEN} ${LIBCRYPT}
LDADD+= -lgssapi -lkrb5 -ldes -lasn1 -lcom_err -lroken -lcrypt
.endif

.include <bsd.prog.mk>
