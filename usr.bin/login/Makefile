#	$NetBSD: Makefile,v 1.45 2005/03/04 20:41:09 he Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

.include <bsd.own.mk>

WARNS=	2
PROG=	login
SRCS=	copyrightstr.c
DPADD+=	${LIBUTIL} ${LIBCRYPT}
LDADD+=	-lutil -lcrypt
BINOWN=	root
BINMODE=4555

.if (${USE_PAM} != "no")
SRCS+=	login_pam.c
LDADD+=	-lpam ${PAM_STATIC_LDADD}
DPADD+= ${LIBPAM} ${PAM_STATIC_DPADD}
.else
SRCS+=	login.c
CPPFLAGS+=-DLOGIN_CAP -DSUPPORT_UTMP -DSUPPORT_UTMPX

.if (${USE_KERBEROS} != "no")
SRCS+= k5login.c
CPPFLAGS+=-DKERBEROS5 -I${DESTDIR}/usr/include/krb5
DPADD+=	${LIBKRB5} ${LIBASN1}
LDADD+= -lkrb5 -lasn1

.if (${USE_KERBEROS4} != "no")
SRCS+= klogin.c
CPPFLAGS+=-DKERBEROS -I${DESTDIR}/usr/include/kerberosIV
DPADD+=	${LIBKRB} ${LIBDES}
LDADD+= -lkrb -ldes
.endif

DPADD+=	${LIBCRYPTO} ${LIBROKEN} ${LIBCOM_ERR}
LDADD+=	-lcrypto -lroken -lcom_err
.endif

.if (${USE_SKEY} != "no")
CPPFLAGS+=-DSKEY
DPADD+= ${LIBSKEY}
LDADD+=	-lskey
.endif
.endif

CLEANFILES+=	copyrightstr.c

copyrightstr.c: ${NETBSDSRCDIR}/sys/conf/copyright
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	awk '\
		BEGIN { print "const char copyrightstr[] =" }\
		{ print "\""$$0"\\n\""}\
		END { print "\"\\n\";" }\
	' ${.ALLSRC} > ${.TARGET}


.include <bsd.prog.mk>
