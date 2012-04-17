# $NetBSD: d_qequals.mk,v 1.1.2.2 2012/04/17 00:09:21 yamt Exp $

M= i386
V.i386= OK
V.$M ?= bug

all:
	@echo 'V.$M ?= ${V.$M}'
