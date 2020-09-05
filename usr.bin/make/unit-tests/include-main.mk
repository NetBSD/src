# $NetBSD: include-main.mk,v 1.4 2020/09/05 18:13:47 rillig Exp $
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

.for i in once
.  if !defined(${.INCLUDEDFROMFILE})
.    info main-before-for-ok
.  else
.    warning main-before-for-fail(${.INCLUDEDFROMFILE})
.  endif
.endfor

.include "include-sub.mk"

.if !defined(.INCLUDEDFROMFILE)
.  info main-after-ok
.else
.  warning main-after-fail(${.INCLUDEDFROMFILE})
.endif

.for i in once
.  if !defined(${.INCLUDEDFROMFILE})
.    info main-after-for-ok
.  else
.    warning main-after-for-fail(${.INCLUDEDFROMFILE})
.  endif
.endfor

all:	# nothing
