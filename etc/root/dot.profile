#	$NetBSD: dot.profile,v 1.10.2.1 1999/12/27 18:28:58 wrstuden Exp $

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
