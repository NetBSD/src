# $NetBSD: Makefile,v 1.4 2003/07/01 13:45:45 agc Exp $

# $FreeBSD: src/gnu/usr.bin/groff/contrib/mm/Makefile,v 1.6 2002/10/11 08:58:13 ru Exp $

DIST_SUBDIR=	contrib/mm

# Perl script
#SCRIPTS=	mmroff.pl
#MAN=		mmroff.1

#TMACDIR?=	${SHAREDIR}/tmac
# sync with ../../src/include/config.h
TMACDIR?=	/usr/share/tmac

FILES=		m.tmac mm.tmac mse.tmac mmse.tmac
FILESNAME_m.tmac=../m.tmac
FILESNAME_mse.tmac=../mse.tmac
FILESNAME_mm.tmac=../mm.tmac
FILESNAME_mmse.tmac=../mmse.tmac
FILES+=		0.MT 5.MT 4.MT ms.cov se_ms.cov
FILESDIR=	${TMACDIR}/mm
LOCALE=		locale se_locale

MAN+=		groff_mm.7 groff_mmse.7
MLINKS=		groff_mm.7 mm.7 groff_mmse.7 mmse.7

#CLEANFILES+=	${MAN}

.include <bsd.prog.mk>

.PATH: ${DIST_DIR}/mm

beforeinstall:
.for locale in ${LOCALE}
.if !exists(${DESTDIR}${TMACDIR}/mm/${locale})
	${INSTALL_FILE} -o ${FILESOWN} -g ${FILESGRP} -m ${FILESMODE} \
		/dev/null ${DESTDIR}${TMACDIR}/mm/${locale}
.endif
.endfor
