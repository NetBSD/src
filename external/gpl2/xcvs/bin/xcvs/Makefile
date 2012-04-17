#	$NetBSD: Makefile,v 1.3.2.1 2012/04/17 00:04:09 yamt Exp $

.include "${.CURDIR}/../../Makefile.inc"
PROG=	xcvs

# So that cleandir works on case-insensitive file systems.
PROGNAME= cvs

DIST=	${IDIST}/src
.PATH:	${DIST} ${IDIST}/doc ${IDIST}/man

MAN=	cvs.1 cvs.5
SRCS=	acl.c add.c admin.c annotate.c buffer.c checkin.c checkout.c \
	classify.c client.c commit.c create_adm.c cvsrc.c diff.c edit.c \
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

# libheimbase is required by libkrb5
# libhx509 is required by libkrb5
DPADD+= ${LIBHEIMBASE} ${LIBHX509}
LDADD+= -lheimbase -lhx509

# libasn1 is required by all the above libs
DPADD+= ${LIBASN1}
LDADD+= -lasn1

# libcom_err is required by libasn1
# libroken is required by libgssapi, libkrb5, libheimntlm, and libhx509
# libcrypto is required by libgssapi, libkrb5, and libhx509
# libwind is required by libhx509
CPPFLAGS+= -DENCRYPTION
DPADD+= ${LIBCOM_ERR} ${LIBROKEN} ${LIBWIND} ${LIBCRYPTO}
LDADD+= -lcom_err -lroken -lwind -lcrypto


# libutil is required by libroken
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.endif

DPADD+= ${LIBCRYPT} ${LIBZ}
LDADD+= -lcrypt -lz

.include <bsd.prog.mk>
