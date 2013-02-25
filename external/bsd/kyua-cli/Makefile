# $NetBSD: Makefile,v 1.2.4.2 2013/02/25 00:26:08 tls Exp $

SUBDIR= etc lib .WAIT share tests usr.bin

.for dir in share tests usr.bin
includes-${dir}: .PHONY
	@true
.endfor

.include <bsd.subdir.mk>
