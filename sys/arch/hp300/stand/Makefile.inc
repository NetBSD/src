#	$NetBSD: Makefile.inc,v 1.16.78.1 2021/04/17 17:26:13 thorpej Exp $

BINDIR=			/usr/mdec

release:

S?=		${.CURDIR}/../../../..

COPTS+=		-fcommon
