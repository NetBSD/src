#	$NetBSD: Makefile,v 1.42 2000/06/20 06:00:34 thorpej Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/4/94

PROG=	ftpd
SRCS=	cmds.c conf.c ftpd.c ftpcmd.y logutmp.c logwtmp.c popen.c
CPPFLAGS+=-DHASSETPROCTITLE
# CPPFLAGS+=-DDEBUG		# XXX for lukem testing
DPADD+=	${LIBCRYPT} ${LIBUTIL}
LDADD+=	-lcrypt -lutil
MAN=	ftpd.conf.5 ftpusers.5 ftpd.8
MLINKS=	ftpusers.5 ftpchroot.5

# for `internal' ls
SRCS+=	ls.c cmp.c print.c stat_flags.c util.c
.PATH:	${.CURDIR}/../../bin/ls

CPPFLAGS+=-DINET6
.include <bsd.own.mk>

.ifdef SKEY
CPPFLAGS+=-DSKEY
DPADD+= ${LIBSKEY}
LDADD+= -lskey
.endif

ftpd.o ftpcmd.o: version.h

# XXX Kerberos support is broken right now.
#.PATH:	${.CURDIR}/../../usr.bin/login

#.ifdef KERBEROS5
#SRCS+=	k5login.c
#CPPFLAGS+=-DKERBEROS5
#DPADD+= ${LIBKRB5} ${LIBK5CRYPTO} ${LIBCOM_ERR}
#LDADD+= -lkrb5 -lk5crypto -lcom_err
#.else
#SRCS+=	klogin.c
#CPPFLAGS+=-DKERBEROS
#DPADD+= ${LIBKRB} ${LIBDES} ${LIBCOM_ERR}
#LDADD+= -lkrb -kdes -lcom_err
#.endif

.include <bsd.prog.mk>
