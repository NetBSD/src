#	$NetBSD: Makefile.inc,v 1.17 2021/04/15 01:54:38 mrg Exp $

BINDIR=			/usr/mdec

release:

S?=		${.CURDIR}/../../../..

COPTS+=		-fcommon
