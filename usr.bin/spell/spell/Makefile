#	$NetBSD: Makefile,v 1.2 2008/10/30 21:37:55 mrg Exp $

MAN=    spell.1
SCRIPTS=spell.sh

BINDIR=	/usr/bin

.include <bsd.own.mk>

.if ${MKSHARE} != "no"

SPECIALFILES=	special.math special.netbsd
FILES=		american british stop ${SPECIALFILES}
FILESMODE=	${NONBINMODE}
FILESDIR=	/usr/share/dict

.for F in ${SPECIALFILES}
FILESNAME_${F}= ${F:S/special.//}
FILESDIR_${F}=  /usr/share/dict/special
.endfor

.endif

.include <bsd.prog.mk>
