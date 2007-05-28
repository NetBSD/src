# $NetBSD: Makefile.inc,v 1.6 2007/05/28 12:06:20 tls Exp $
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
# $FreeBSD: src/lib/libpam/Makefile.inc,v 1.15 2004/03/15 13:23:20 des Exp $

USE_FORT?= yes	# network client software *and* authentication software

# Include support for static modules.
CPPFLAGS+= -DOPENPAM_STATIC_MODULES

# Enable the static module macros.
CPPFLAGS+= -DOPENPAM_STATIC_MODULE

CPPFLAGS+= -DOPENPAM_MODULES_DIR=\"/usr/lib/security\"
# No debugging
#CPPFLAGS+= -DDEBUG

# ...and don't do either of those on the shared library.
# XXX CSHLIBFLAGS isn't perfect, but it'll do for now.
CSHLIBFLAGS+= -DNO_STATIC_MODULES

# Define the shared library version here.  libpam and the modules share a
# version, and we need these variables early for module install rules.
SHLIB_MAJOR=	0
SHLIB_MINOR=	0
