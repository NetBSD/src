# $NetBSD: Makefile,v 1.2.12.2 2014/05/22 15:45:27 yamt Exp $

SUBDIR= etc lib .WAIT share tests usr.bin

.for dir in share tests usr.bin
includes-${dir}: .PHONY
	@true
.endfor

.include <bsd.subdir.mk>
