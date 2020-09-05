# $NetBSD: include-sub.mk,v 1.3 2020/09/05 18:13:47 rillig Exp $

.if ${.INCLUDEDFROMFILE} == "include-main.mk"
.  info sub-before-ok
.else
.  warning sub-before-fail(${.INCLUDEDFROMFILE})
.endif

# As of 2020-09-05, the .for loop is implemented as "including a file"
# with a custom buffer.  Therefore this loop has side effects on these
# variables.
.for i in once
.  if ${.INCLUDEDFROMFILE} == "include-main.mk"
.    info sub-before-for-ok
.  else
.    warning sub-before-for-fail(${.INCLUDEDFROMFILE})
.  endif
.endfor

.include "include-subsub.mk"

.if ${.INCLUDEDFROMFILE} == "include-main.mk"
.  info sub-after-ok
.else
.  warning sub-after-fail(${.INCLUDEDFROMFILE})
.endif

.for i in once
.  if ${.INCLUDEDFROMFILE} == "include-main.mk"
.    info sub-after-for-ok
.  else
.    warning sub-after-for-fail(${.INCLUDEDFROMFILE})
.  endif
.endfor
