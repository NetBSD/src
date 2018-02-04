# $NetBSD: Makefile,v 1.12 2018/02/04 03:19:53 christos Exp $
# Copyright 2001 FreeBSD, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# $FreeBSD: src/lib/libpam/modules/pam_krb5/Makefile,v 1.13 2004/08/06 07:27:04 cperciva Exp $

LIB=	pam_krb5
SRCS=	pam_krb5.c
MAN=	pam_krb5.8

LIBDPLIBS+= \
	krb5	${NETBSDSRCDIR}/crypto/external/bsd/heimdal/lib/libkrb5 \
	asn1	${NETBSDSRCDIR}/crypto/external/bsd/heimdal/lib/libasn1 \
	roken	${NETBSDSRCDIR}/crypto/external/bsd/heimdal/lib/libroken \
	com_err	${NETBSDSRCDIR}/crypto/external/bsd/heimdal/lib/libcom_err \
	crypt	${.CURDIR}/../../../libcrypt \
	crypto	${NETBSDSRCDIR}/crypto/external/bsd/${EXTERNAL_OPENSSL_SUBDIR}/lib/libcrypto

CWARNFLAGS.clang=	-Wno-format-security

.include "${.CURDIR}/../mod.mk"
