#	$NetBSD: Makefile,v 1.42.2.3 2004/08/26 04:59:56 jmc Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/4/94

.include <bsd.own.mk>

PROG=	ftpd
SRCS=	cmds.c conf.c ftpd.c ftpcmd.y logutmp.c logwtmp.c popen.c
SRCS+=	strsuftoll.c
CPPFLAGS+=-I${.CURDIR} -DSUPPORT_UTMP
DPADD+=	${LIBCRYPT} ${LIBUTIL}
LDADD+=	-lcrypt -lutil
MAN=	ftpd.conf.5 ftpusers.5 ftpd.8
MLINKS=	ftpusers.5 ftpchroot.5

NETBSDSRCDIR?=/usr/src
# for `internal' ls
SRCS+=	ls.c cmp.c print.c stat_flags.c util.c
.PATH:	${NETBSDSRCDIR}/bin/ls

CPPFLAGS+=-DINET6
.include <bsd.own.mk>

WARNS=1

USE_SKEY?="no" 

.if (${USE_SKEY} != "no")
CPPFLAGS+=-DSKEY
DPADD+= ${LIBSKEY}
LDADD+= -lskey
.endif

ftpd.o ftpcmd.o: version.h

#.if (${USE_KERBEROS} != "no")
#
#.PATH:	${NETBSDSRCDIR}/usr.bin/login
#
#SRCS+=	k5login.c
#CPPFLAGS+=-DKERBEROS5
#CPPFLAGS+=-DKERBEROS5 -I${DESTDIR}/usr/include/krb5
#DPADD+=	${LIBKRB5} ${LIBASN1}
#LDADD+= -lkrb5 -lasn1
#
#SRCS+= klogin.c
#CPPFLAGS+=-DKERBEROS -I${DESTDIR}/usr/include/kerberosIV
#DPADD+=	${LIBKRB}
#LDADD+= -lkrb
#
#DPADD+=	${LIBCRYPTO} ${LIBROKEN} ${LIBCOM_ERR}
#LDADD+=	-lcrypto -lroken -lcom_err
#
#.endif

.include <bsd.prog.mk>
