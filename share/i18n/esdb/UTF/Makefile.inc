# $NetBSD: Makefile.inc,v 1.1 2003/06/27 08:40:58 tshiozak Exp $

.PATH: ${.CURDIR}/UTF

CODE:=		UTF
.include "${.CURDIR}/Makefile.part"

UTF-16-mod=	UTF1632
UTF-16-var=	utf16
UTF-16BE-mod=	UTF1632
UTF-16BE-var=	utf16,big,force
UTF-16LE-mod=	UTF1632
UTF-16LE-var=	utf16,little,force
UTF-32-mod=	UTF1632
UTF-32-var=	utf32
UTF-32BE-mod=	UTF1632
UTF-32BE-var=	utf32,big,force
UTF-32LE-mod=	UTF1632
UTF-32LE-var=	utf32,little,force
UTF-8-mod=	UTF8
UTF-8-var=	utf8

.for i in ${UTF_PART}
UTF-$i.src: UTF.src
	sed 's/UTF-x/UTF-$i/;s/UTF-mod/${UTF-$i-mod}/;s/UTF-var/${UTF-$i-var}/' $> > $@
CLEANFILES+= UTF-$i.src
.endfor
