#	$NetBSD: Makefile,v 1.11 2002/09/19 12:08:38 lukem Exp $
#

.include <bsd.own.mk>

PROG=	tcpdmatch
SRCS=	tcpdmatch.c fakelog.c inetcf.c scaffold.c percent_m.c
MAN=	tcpdmatch.8
TCPDCHK=${NETBSDSRCDIR}/usr.sbin/tcpdchk
.PATH: ${TCPDCHK}
CPPFLAGS+= -I${TCPDCHK} -I${NETBSDSRCDIR}/lib/libwrap -DSYS_ERRLIST_DEFINED
LDADD=	-lwrap
DPADD=	${LIBWRAP}

.include "${NETBSDSRCDIR}/lib/libwrap/Makefile.cflags"

.include <bsd.prog.mk>
