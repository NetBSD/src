#	$NetBSD: Makefile,v 1.2 1995/11/23 02:41:20 cgd Exp $

PROG=	makexxboot
NOMAN=
CLEANFILES+= xxboot

all: xxboot

xxboot: makexxboot
	/bin/rm -f xxboot
	./makexxboot > xxboot

install:
	install -c -o bin -g bin -m 444 xxboot ${DESTDIR}/usr/mdec/sdboot

.include <bsd.prog.mk>
