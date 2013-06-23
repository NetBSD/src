# $NetBSD: Makefile,v 1.1.6.2 2013/06/23 06:26:38 tls Exp $

SUBDIR= share tests usr.bin

.for dir in share tests usr.bin
includes-${dir}: .PHONY
	@true
.endfor

.include <bsd.subdir.mk>
