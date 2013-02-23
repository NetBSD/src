# $NetBSD: Makefile,v 1.1 2013/02/23 14:16:49 jmmv Exp $

SUBDIR= lib .WAIT share tests usr.bin

.for dir in share tests usr.bin
includes-${dir}: .PHONY
	@true
.endfor

.include <bsd.subdir.mk>
