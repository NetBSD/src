# $NetBSD: include-subsub.mk,v 1.2 2020/09/05 16:59:19 rillig Exp $

.if ${.INCLUDEDFROMFILE:T} == "include-sub.mk"
.  info subsub-ok
.else
.  warning subsub-fail
.endif
