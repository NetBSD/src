# $NetBSD: include-sub.mk,v 1.2 2020/09/05 16:59:19 rillig Exp $

.if ${.INCLUDEDFROMFILE} == "include-main.mk"
.  info sub-before-ok
.else
.  warning sub-before-fail
.endif

.include "include-subsub.mk"

.if ${.INCLUDEDFROMFILE} == "include-main.mk"
.  info sub-after-ok
.else
.  warning sub-after-fail(${.INCLUDEDFROMFILE})
.endif
