# $NetBSD: Makefile,v 1.1 2016/01/13 19:02:01 christos Exp $

# $FreeBSD: src/gnu/usr.bin/groff/contrib/mm/Makefile,v 1.6 2002/10/11 08:58:13 ru Exp $

DIST_SUBDIR=	contrib/mm

.include <bsd.init.mk>

.if ${MKSHARE} != "no"					# {


# Perl script
#SCRIPTS=	mmroff.pl
#MAN=		mmroff.1

#TMACDIR?=	${SHAREDIR}/tmac
# sync with ../../src/include/config.h
TMACDIR?=	/usr/share/tmac

FILESDIR=		${TMACDIR}/mm
FILES=			m.tmac mm.tmac mse.tmac mmse.tmac
FILESDIR_m.tmac=	${TMACDIR}
FILESDIR_mse.tmac=	${TMACDIR}
FILESDIR_mm.tmac=	${TMACDIR}
FILESDIR_mmse.tmac=	${TMACDIR}
FILES+=			0.MT 5.MT 4.MT ms.cov se_ms.cov
LOCALE=			locale se_locale

MAN+=		groff_mm.7 groff_mmse.7
MLINKS=		groff_mm.7 mm.7 groff_mmse.7 mmse.7

CLEANFILES+=	${MAN}

.endif							# }

.include <bsd.prog.mk>

.if ${MKSHARE} != "no"					# {

.PATH: ${DIST_DIR}/mm

beforeinstall:
.for locale in ${LOCALE}
.if !exists(${DESTDIR}${TMACDIR}/mm/${locale})
	${INSTALL_FILE} -o ${FILESOWN} -g ${FILESGRP} -m ${FILESMODE} \
		/dev/null ${DESTDIR}${TMACDIR}/mm/${locale}
.endif
.endfor

.endif							# }
