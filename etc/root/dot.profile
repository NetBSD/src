#	$NetBSD: dot.profile,v 1.7 1997/10/28 03:33:23 mrg Exp $

PATH=/sbin:/usr/sbin:/bin:/usr/bin
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
