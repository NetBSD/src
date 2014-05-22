# $NetBSD: Makefile,v 1.2.12.2 2014/05/22 15:45:38 yamt Exp $

SUBDIR= lib .WAIT libexec share tests

.for dir in libexec share tests
includes-${dir}: .PHONY
	@true
.endfor

.include <bsd.subdir.mk>
