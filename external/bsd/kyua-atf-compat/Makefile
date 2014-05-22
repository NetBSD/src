# $NetBSD: Makefile,v 1.1.12.2 2014/05/22 15:45:21 yamt Exp $

SUBDIR= share tests usr.bin

.for dir in share tests usr.bin
includes-${dir}: .PHONY
	@true
.endfor

.include <bsd.subdir.mk>
