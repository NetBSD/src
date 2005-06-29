#	$NetBSD: Makefile,v 1.1 2005/06/29 21:06:12 perry Exp $

MAN=    spell.1
SCRIPTS=spell.sh

BINDIR=	/usr/bin

SPECIALFILES=	special.math special.netbsd
FILES=		american british stop ${SPECIALFILES}
FILESMODE=	${NONBINMODE}
FILESDIR=	/usr/share/dict

.for F in ${SPECIALFILES}
FILESNAME_${F}= ${F:S/special.//}
FILESDIR_${F}=  /usr/share/dict/special
.endfor


.include <bsd.prog.mk>
