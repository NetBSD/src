# $NetBSD: d_qequals.mk,v 1.1 2009/02/13 05:19:52 jmmv Exp $

M= i386
V.i386= OK
V.$M ?= bug

all:
	@echo 'V.$M ?= ${V.$M}'
