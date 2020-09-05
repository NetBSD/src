# $NetBSD: include-main.mk,v 1.3 2020/09/05 16:59:19 rillig Exp $
#
# Demonstrates that the .INCLUDEDFROMFILE magic variable does not behave
# as described in the manual page.
#
# The manual page says that it is the "filename of the file this Makefile
# was included from", while in reality it is the "filename in which the
# latest .include happened". See parse.c, function ParseSetIncludeFile.
#

.if !defined(.INCLUDEDFROMFILE)
.  info main-before-ok
.else
.  warning main-before-fail(${.INCLUDEDFROMFILE})
.endif

.include "include-sub.mk"

.if !defined(.INCLUDEDFROMFILE)
.  info main-after-ok
.else
.  warning main-after-fail(${.INCLUDEDFROMFILE})
.endif

all:	# nothing
