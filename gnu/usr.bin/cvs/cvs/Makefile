#	$NetBSD: Makefile,v 1.9 2003/07/24 00:25:24 lha Exp $

.include "${.CURDIR}/../Makefile.inc"

PROG=	cvs

DIST=	${IDIST}/src
.PATH:	${DIST} ${IDIST}/man

MAN=	cvs.1 cvs.5
SRCS=	add.c admin.c annotate.c buffer.c checkin.c checkout.c classify.c \
	client.c commit.c create_adm.c cvsrc.c diff.c edit.c \
	entries.c error.c expand_path.c fileattr.c filesubr.c \
	find_names.c hardlink.c hash.c history.c ignore.c import.c \
	lock.c log.c login.c logmsg.c main.c mkmodules.c modules.c \
	myndbm.c no_diff.c parseinfo.c patch.c rcs.c rcscmds.c \
	recurse.c release.c remove.c repos.c root.c run.c \
	scramble.c server.c status.c subr.c tag.c update.c vers_ts.c \
	version.c watch.c wrapper.c zlib.c

DPADD+= ${LIBDIFF} ${LIBCVS} ${LIBCRYPT} ${LIBZ}
LDADD+= ${LIBDIFF} ${LIBCVS} -lcrypt -lz

.if (${USE_KERBEROS4} != "no")
CPPFLAGS+= -DHAVE_KERBEROS -I${DESTDIR}/usr/include/kerberosIV
DPADD+= ${LIBKRB}
LDADD+= -lkrb
.endif

.if (${USE_KERBEROS} != "no")
CPPFLAGS+= -DHAVE_GSSAPI -I${DESTDIR}/usr/include/krb5
DPADD+= ${LIBGSSAPI} ${LIBKRB5} ${LIBASN1}
LDADD+= -lgssapi -lkrb5 -lasn1
.endif

.if (${USE_KERBEROS4} != "no" || ${USE_KERBEROS} != "no")
CPPFLAGS+= -DENCRYPTION
DPADD+= ${LIBCRYPTO} ${LIBCOM_ERR} ${LIBROKEN} ${LIBCRYPT}
LDADD+= -lcrypto -lcom_err -lroken -lcrypt
.endif


.include <bsd.prog.mk>
