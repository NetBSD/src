# $NetBSD: ec.inc,v 1.2 2025/07/20 22:53:34 christos Exp $
ECNI=yes
ECNONISTZ=yes
EC_SRCS+= ecp_nistp384.c

.include "../../ec.inc"
