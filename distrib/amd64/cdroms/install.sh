#! /bin/sh
# $NetBSD: install.sh,v 1.1 2011/01/18 00:16:13 jym Exp $
#
# -
#  Copyright (c) 2010 The NetBSD Foundation, Inc.
#  All rights reserved.
# 
#  This code is derived from software contributed to The NetBSD Foundation
#  by Martin Husemann <martin@NetBSD.org>.
# 
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
# 
#  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
#  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
#  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
#  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
#  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.


# setup basic environment
PATH=/sbin:/bin:/usr/bin:/usr/sbin:/
export PATH

# Check if we are on a framebuffer or on serial console and default
# the terminal type accordingly.
# There is no /var/db/dev.db, so sysctl might not map the devicename properly;
# ttyE0 is 90,0 -> 0x5a00
case $(sysctl -nx kern.consdev) in
 002f000000000000)
   TERM=wsvt25
   ;;
 *)
   TERM=vt220
   ;;
esac

export TERM
HOME=/
export HOME
BLOCKSIZE=1k
export BLOCKSIZE
EDITOR=ed
export EDITOR

umask 022

stty newcrt werase ^W intr ^C kill ^U erase ^?
if [ $TERM != "wsvt25" ]; then
	cat << "EOM"


You are using a serial console, we do not know your terminal emulation.
Please select one, typical values are:

	vt100
	ansi
	xterm

EOM
	echo -n "Terminal type (just hit ENTER for '$TERM'): "
	read ans
	if [ -n "$ans" ];then
	    TERM=$ans
	fi
fi

# run the installation or upgrade script.
/sysinst || {
	    echo "Oops, something went wrong - we will try again"; exit; }

echo "To return to the installer, quit this shell by typing 'exit' or ^D."
exec /bin/sh
