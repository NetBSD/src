#	$NetBSD: dot.profile,v 1.9 1999/03/30 02:36:05 hubertf Exp $

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

if [ `/usr/bin/id -u` = 0 ]; then 
	PS1="`hostname -s`# "
else
	PS1="`hostname -s`$ "
fi

echo "Don't login as root, use the su command."
