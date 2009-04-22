#	$NetBSD: Makefile,v 1.12 2009/04/22 15:23:09 lukem Exp $
#

WARNS?=	1	# XXX: many issues in lib/libwrap to address first

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
