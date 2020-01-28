# $NetBSD: dot.profile,v 1.15.50.1 2020/01/28 10:20:25 msaitoh Exp $
#
# Copyright (c) 1995 Jason R. Thorpe
# Copyright (c) 1994 Christopher G. Demetriou
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
#          This product includes software developed for the
#          NetBSD Project.  See http://www.NetBSD.org/ for
#          information about NetBSD.
# 4. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 
# <<Id: LICENSE,v 1.2 2000/06/14 15:57:33 cgd Exp>>

PATH=/sbin:/bin:/usr/bin:/usr/sbin:/
export PATH
TERM=vt100
export TERM
HOME=/
export HOME

umask 022

if [ "X${DONEPROFILE}" = "X" ]; then
	DONEPROFILE=YES
	export DONEPROFILE

	# set up some sane defaults
	echo 'erase ^H, werase ^W, kill ^U, intr ^C'
	stty newcrt werase ^W intr ^C kill ^U erase ^H

	# get the terminal type
	_forceloop=""
	while [ "X${_forceloop}" = X"" ]; do
		eval `tset -s -m ":?$TERM"`
		if [ "X${TERM}" != X"unknown" ]; then
			_forceloop="done"
		fi
	done

	# Installing or upgrading?
	_forceloop=""
	while [ "X${_forceloop}" = X"" ]; do
		cat <<'EOF'

This installer now uses the new `sysinst' installer tool by default.  To
use the old install or upgrade shell scripts instead, enter the options
(OI) for Old Install or (OU) for Old Upgrade.

The script-based installers may be removed in a future release.

EOF
		echo -n '(I)nstall/Upgrade, (H)alt or (S)hell? '
		read _forceloop
		case "$_forceloop" in
			i*|I*|u*|U*)
				# setup a writable /tmp directory
				mount_mfs -s 1m swap /tmp || continue
				/sysinst || stty sane
				;;

			oi*|OI*)
				/install
				;;

			ou*|OU*)
				/upgrade
				;;

			h*|H*)
				#
				# XXX - if we're piggybacking a microroot, then
				# exit from this (chroot) environment: the
				# microroot's .profile will halt the machine.
				#
				if [ "$BOOTFS_DONEPROFILE" = YES ]; then
					exit
				else
					/sbin/halt
				fi
				;;

			s*|S*)
				/bin/sh
				continue
				;;

			*)
				_forceloop=""
				;;
		esac
	done
fi
