# $NetBSD: Makefile,v 1.17 2010/12/03 17:57:43 phx Exp $

NOOBJ=	# defined

FILES=	\
	amikbd.pl.qwertz.iso8859-2 \
	mkbd.pt.iso8859-1 \
	pckbd.be.azerty pckbd.br.abnt2 \
	pckbd.de_CH.iso8859-1 \
	pckbd.fr_CA.iso8859-1 \
	pckbd.fr_CH.iso8859-1 \
	pckbd.hu.iso8859-2 \
	pckbd.pl.iso8859-2 \
	pckbd.ru.koi8-r \
	pckbd.sv.svascii \
	ukbd.be.azerty

.if ${MACHINE} == "macppc"
FILES+=	ukbd.any.powerbook
.endif

FILESDIR= /usr/share/wscons/keymaps

.include <bsd.prog.mk>
