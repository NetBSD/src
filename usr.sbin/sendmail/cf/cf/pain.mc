divert(-1)
#
# Copyright (c) 1993, 1994, 1995 Adam Glass
# Copyright (c) 1983 Eric P. Allman
# Copyright (c) 1988 The Regents of the University of California.
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

include(`../m4/cf.m4')
VERSIONID(`@(#)pain.mc	$Revision: 1.3 $')
OSTYPE(bsd4.4)dnl
define(`UUCP_RELAY', life.ai.mit.edu)dnl
define(`BITNET_RELAY', mitvma.mit.edu)dnl
define(`MASQUERADE_NAME', NetBSD.ORG)dnl
FEATURE(allmasquerade)dnl
MAILER(local)dnl
MAILER(smtp)dnl
define(`confCHECKPOINT_INTERVAL', 10)dnl
define(`confAUTO_REBUILD', True)dnl
define(`confMESSAGE_TIMEOUT', 5d)dnl
define(`confMIN_FREE_BLOCKS', 4096)dnl
define(`confMCI_CACHE_SIZE', 10)dnl
define(`confMCI_CACHE_TIMEOUT', 15m)dnl
define(`confREAD_TIMEOUT', 10m)dnl
define(`confQUEUE_LA', 12)dnl
define(`confWORK_RECIPIENT_FACTOR', 100)dnl
define(`confWORK_CLASS_FACTOR', 1800)dnl
define(`confWORK_TIME_FACTOR', 90000)dnl
CwNetBSD.ORG mail.NetBSD.ORG gnats.NetBSD.ORG ftp.NetBSD.ORG cvs.NetBSD.ORG sup.NetBSD.ORG
