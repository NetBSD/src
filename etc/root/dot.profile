#	$NetBSD: dot.profile,v 1.8 1998/01/17 19:36:40 thorpej Exp $

PATH=/sbin:/usr/sbin:/bin:/usr/bin:/usr/pkg/sbin:/usr/pkg/bin
PATH=${PATH}:/usr/X11R6/bin:/usr/local/sbin:/usr/local/bin
export PATH

BLOCKSIZE=1k
export BLOCKSIZE

HOME=/root
export HOME

if [ -x /usr/bin/tset ]; then
	eval `/usr/bin/tset -sQ \?$TERM`
fi

umask 022

echo "Don't login as root, use the su command."
