# $NetBSD: netbsd-proto.mc,v 1.11 2003/03/24 15:04:03 atatat Exp $

divert(-1)
#
# Copyright (c) 1994 Adam Glass
# Copyright (c) 1983 Eric P. Allman
# Copyright (c) 1988, 1993
#	The Regents of the University of California.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#	This product includes software developed by the University of
#	California, Berkeley and its contributors.
# 4. Neither the name of the University nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

#
#  This is the prototype file for a configuration that supports SMTP
#  connections via TCP and some commonly required features.
#

include(`../m4/cf.m4')
VERSIONID(`@(#)netbsd-proto.mc	$Revision: 1.11 $')
OSTYPE(bsd4.4)dnl
DOMAIN(generic)dnl
FEATURE(genericstable,DATABASE_MAP_TYPE` -o 'MAIL_SETTINGS_DIR`genericstable')
FEATURE(mailertable,  DATABASE_MAP_TYPE` -o 'MAIL_SETTINGS_DIR`mailertable')
FEATURE(virtusertable,DATABASE_MAP_TYPE` -o 'MAIL_SETTINGS_DIR`virtusertable')
FEATURE(domaintable,  DATABASE_MAP_TYPE` -o 'MAIL_SETTINGS_DIR`domaintable')
FEATURE(access_db,    DATABASE_MAP_TYPE` -T<TMPF> -o 'MAIL_SETTINGS_DIR`access')
FEATURE(`redirect')
MAILER(local)dnl
MAILER(smtp)dnl

# Enable IPv6.  IPv6 is marked as optional so the configuration file
# can be used on IPV4-only kernel as well.
DAEMON_OPTIONS(`Family=inet, address=0.0.0.0, Name=MTA')dnl
DAEMON_OPTIONS(`Family=inet6, address=::, Name=MTA6, Modifiers=O')dnl
