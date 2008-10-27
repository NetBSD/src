# $NetBSD: Makefile,v 1.11 2008/10/27 07:48:27 mrg Exp $
# Copyright 1998 Juniper Networks, Inc.
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
# $FreeBSD: src/lib/libpam/modules/Makefile,v 1.19 2003/03/09 20:06:34 obrien Exp $

.include <bsd.own.mk>

SUBDIR=	pam_chroot pam_deny pam_echo pam_exec pam_ftpusers pam_group \
	pam_guest pam_lastlog pam_login_access pam_nologin pam_permit \
	pam_radius pam_rhosts pam_rootok pam_securetty pam_self pam_unix

.if (${MKSKEY} != "no")
SUBDIR+= pam_skey
.endif

.if (${MKKERBEROS} != "no")
SUBDIR+= pam_afslog pam_krb5 pam_ksu
.endif

.if (${MKCRYPTO} != "no")
SUBDIR+= pam_ssh
.endif

libinstall::		# disable install rule in <bsd.lib.mk>

.include <bsd.lib.mk>
.include <bsd.subdir.mk>
