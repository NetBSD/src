#	$NetBSD: dot.profile,v 1.10 1999/04/04 09:58:57 mycroft Exp $

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

hup(){ kill -HUP `cat /var/run/$1.pid` ; }
ll(){ ls -l $* ; }
x(){ exit ; }

PS1="`hostname -s`$PS1"

echo "Don't login as root, use the su command."
