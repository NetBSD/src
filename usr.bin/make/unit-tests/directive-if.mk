# $NetBSD: directive-if.mk,v 1.3 2020/11/10 20:44:18 rillig Exp $
#
# Tests for the .if directive.

# TODO: Implementation

.if 0
.  error
.else
.  info 0 evaluates to false.
.endif

.if 1
.  info 1 evaluates to true.
.else
.  error
.endif

# There is no '.ifx'.
#
# The commit from 2005-05-01 intended to detect this situation, but it failed
# to do this since the call to is_token has its arguments switched.  They are
# expected as (str, token, token_len) but are actually passed as (token, str,
# token_len).  This made is_token return true even if the directive was
# directly followed by alphanumerical characters.
#
# Back at that time, the commits only modified the main code but did not add
# the corresponding unit tests.  This allowed the bug to hide for more than
# 15 years.
.ifx 123
.  error
.else
.  error
.endif

# Missing condition.
.if
.  error
.else
.  error
.endif

all:
