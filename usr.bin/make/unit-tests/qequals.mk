# $NetBSD: qequals.mk,v 1.2 2020/10/24 08:34:59 rillig Exp $

M= i386
V.i386= OK
V.$M ?= bug

all:
	@echo 'V.$M ?= ${V.$M}'
