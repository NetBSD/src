#	$NetBSD: Makefile,v 1.3 2003/09/22 12:54:49 atatat Exp $

NOPIC=		# defined
NOPROFILE=	# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/gnu/dist
.PATH: ${DIST}/sendmail/libmilter ${DIST}/sendmail/libsm \
	${DIST}/sendmail/include/libmilter

LIB=		milter

CPPFLAGS+=	-DNOT_SENDMAIL -Dsm_snprintf=snprintf

SRCS=		comm.c engine.c handler.c listener.c main.c signal.c \
		sm_gethost.c smfi.c errstring.c strl.c

INCS=		mfapi.h mfdef.h
INCSDIR=	/usr/include/libmilter

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/sendmail
FILES=		README
FILESNAME_README=README.milter
.endif

.include <bsd.lib.mk>
