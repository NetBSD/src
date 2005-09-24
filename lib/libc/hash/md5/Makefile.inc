# $NetBSD: Makefile.inc,v 1.1 2005/09/24 19:04:52 elad Exp $

# hash functions
.PATH: ${.CURDIR}/hash/md5

SRCS+=	hmac_md5.c
