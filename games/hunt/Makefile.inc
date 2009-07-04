#	$NetBSD: Makefile.inc,v 1.11 2009/07/04 07:51:34 dholland Exp $
#
#
#  Hunt
#
# Copyright (c) 1983-2003, Regents of the University of California.
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions are 
# met:
# 
# + Redistributions of source code must retain the above copyright 
#   notice, this list of conditions and the following disclaimer.
# + Redistributions in binary form must reproduce the above copyright 
#   notice, this list of conditions and the following disclaimer in the 
#   documentation and/or other materials provided with the distribution.
# + Neither the name of the University of California, San Francisco nor 
#   the names of its contributors may be used to endorse or promote 
#   products derived from this software without specific prior written 
#   permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#
# Game parameter flags are:
#	RANDOM	Include doors which disperse shots randomly
#	REFLECT	Include diagonal walls that reflect shots
#	MONITOR	Include code for watching the game from the sidelines
#	OOZE	Include slime shots
#	FLY	Make people fly when walls regenerate under them
#	VOLCANO	Include occasional large slime explosions
#	DRONE	Include occasional drone shots
#	BOOTS	Include boots (which makes you immune to slime)
#	OTTO	Reserved for CGL automatic player
#
GAME_PARAM=	-DRANDOM -DREFLECT -DMONITOR -DOOZE -DFLY -DVOLCANO -DBOOTS \
		-DOTTO

#
# System parameter flags are:
#	DEBUG		Don't trust everything in the code
#	INTERNET	Use the Internet domain IPC instead of UNIX domain
#	LOG		Use syslog error-logging in driver
#	OLDIPC		Use 4.1a internet system calls (must also define
#			INTERNET)
#	LOG		Use syslog(3) for logging errors
#	HPUX		do special HPUX only hacks
#
DEFS_BSD43=	-DINTERNET -DLOG
DEFS_BSD44=	-DINTERNET -DLOG
DEFS_SUN=	-DINTERNET -DLOG
DEFS_SUN4_0=	-DINTERNET -DLOG
DEFS_ULTRIX=	-DINTERNET -DLOG
DEFS_HPUX=	-DINTERNET -DLOG -DHPUX
DEFS_SGI=	-DINTERNET -DLOG
DEFS_NEXT=	-DINTERNET -DLOG -bsd -Dconst=
DEFS_OSF1=	-DINTERNET -DLOG -D_BSD

#
# The following flags are used for system specific compilation arguments.
# Change them to include the appropriate arguments.  For example, on SGI
# machines, they should look like
# SYSCFLAGS=	-I/usr/include/bsd
# SYSLIBS=	-lbsd
#
SYSCFLAGS=
SYSLIBS=

#
# Generic definitions
#
DEFS=		$(GAME_PARAM) $(DEFS_BSD44)
CPPFLAGS+=	$(SYSCFLAGS) $(DEFS) -DHUNTD=\"/usr/games/huntd\"

.include "../Makefile.inc"
