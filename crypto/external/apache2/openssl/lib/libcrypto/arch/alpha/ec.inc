# $NetBSD: ec.inc,v 1.2 2025/07/19 11:31:42 christos Exp $
ECNI=yes
ECNONISTZ=yes
EC_SRCS+= ecp_nistp384.c

.include "../../ec.inc"
