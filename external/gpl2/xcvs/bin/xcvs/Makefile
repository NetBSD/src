#	$NetBSD: Makefile,v 1.1.2.2 2009/05/13 18:52:48 jym Exp $

.include "${.CURDIR}/../../Makefile.inc"
CFLAGS+=-g
PROG=	xcvs

# So that cleandir works on case-insensitive file systems.
PROGNAME= cvs

DIST=	${IDIST}/src
.PATH:	${DIST} ${IDIST}/doc ${IDIST}/man

MAN=	cvs.1 cvs.5
SRCS=	add.c admin.c annotate.c buffer.c checkin.c checkout.c classify.c \
	client.c commit.c create_adm.c cvsrc.c diff.c edit.c \
	entries.c error.c exithandle.c expand_path.c fileattr.c filesubr.c \
	find_names.c hardlink.c hash.c history.c ignore.c import.c \
	lock.c log.c log-buffer.c login.c logmsg.c ls.c main.c mkmodules.c \
	modules.c ms-buffer.c myndbm.c no_diff.c parseinfo.c patch.c rcs.c \
	rcscmds.c recurse.c release.c remove.c repos.c root.c rsh-client.c \
	run.c scramble.c server.c stack.c status.c subr.c tag.c update.c \
	vers_ts.c version.c watch.c wrapper.c zlib.c

DPADD+= ${LIBDIFF} ${LIBCVS}
LDADD+= ${LIBDIFF} ${LIBCVS}

.if defined(USE_KERBEROS) && (${USE_KERBEROS} != "no")
CPPFLAGS+= -DHAVE_GSSAPI -I${DESTDIR}/usr/include/krb5
DPADD+= ${LIBGSSAPI} ${LIBKRB5}
SRCS+=	gssapi-client.c
LDADD+= -lgssapi -lkrb5

# libheimntlm is required by libgssapi
DPADD+= ${LIBHEIMNTLM}
LDADD+= -lheimntlm

# libhx509 is required by libkrb5
DPADD+= ${LIBHX509}
LDADD+= -lhx509

# libasn1 is required by all the above libs
DPADD+= ${LIBASN1}
LDADD+= -lasn1

# libcom_err is required by libasn1
# libroken is required by libgssapi, libkrb5, libheimntlm, and libhx509
# libcrypto is required by libgssapi, libkrb5, and libhx509
CPPFLAGS+= -DENCRYPTION
DPADD+= ${LIBCOM_ERR} ${LIBROKEN} ${LIBCRYPTO}
LDADD+= -lcom_err -lroken -lcrypto

# libutil is required by libroken
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.endif

DPADD+= ${LIBCRYPT} ${LIBZ}
LDADD+= -lcrypt -lz

.include <bsd.prog.mk>
