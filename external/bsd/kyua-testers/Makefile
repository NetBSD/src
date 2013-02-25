# $NetBSD: Makefile,v 1.2.4.2 2013/02/25 00:26:27 tls Exp $

SUBDIR= lib .WAIT libexec share tests

.for dir in libexec share tests
includes-${dir}: .PHONY
	@true
.endfor

.include <bsd.subdir.mk>
