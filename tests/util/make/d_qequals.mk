# $NetBSD: d_qequals.mk,v 1.1.2.2 2009/05/13 19:19:40 jym Exp $

M= i386
V.i386= OK
V.$M ?= bug

all:
	@echo 'V.$M ?= ${V.$M}'
