#	$NetBSD: dot.profile,v 1.11 1999/11/05 11:30:13 mycroft Exp $

export PATH=/sbin:/usr/sbin:/bin:/usr/bin:/usr/pkg/sbin:/usr/pkg/bin
export PATH=${PATH}:/usr/X11R6/bin:/usr/local/sbin:/usr/local/bin

export BLOCKSIZE=1k
export HOME=/root

if [ -x /usr/bin/tset ]; then
	eval `/usr/bin/tset -sQ \?$TERM`
fi

umask 022
#ulimit -c 0

export ENV=$HOME/.shrc

echo "Don't login as root, use the su command."
