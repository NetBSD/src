#	$NetBSD: Makefile,v 1.1 2003/07/04 04:52:59 atatat Exp $

NOPIC=		# defined
NOPROFILE=	# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist
.PATH: ${DIST}/sendmail/libmilter ${DIST}/sendmail/include/libmilter

LIB=		milter

CPPFLAGS+=	-DNOT_SENDMAIL

SRCS=		comm.c engine.c handler.c listener.c main.c signal.c \
		sm_gethost.c smfi.c

INCS=		mfapi.h mfdef.h
INCSDIR=	/usr/include/milter

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/sendmail
FILES=		README
FILESNAME_README=README.milter
.endif

.include <bsd.lib.mk>
