#! /bin/sh

#	$KAME: racoonquestion.sh,v 1.1 2001/01/27 05:46:22 itojun Exp $

# sends question about racoon to sakane.
# % racoonquestion logfile conffile
#
# caveat: the script will tell everything about your system, and every secret
# keys, to sakane.

if [ $# != 2 ]; then
	echo usage: sendracoonquestion logfile conffile
	exit 1
fi
if [ -e /tmp/racoonbug ]; then
	echo fatal: clean /tmp/racoonbug first.
	exit 1
fi
if [ `whoami` != root ]; then
	echo fatal: must be a root to invoke this.
	exit 1
fi

# do not let others read the result
umask 0077
mkdir /tmp/racoonbug || exit 1
setkey -DP > /tmp/racoonbug/spd.$$
setkey -D > /tmp/racoonbug/sad.$$
ifconfig -a > /tmp/racoonbug/ifconfig.$$
netstat -rn >/tmp/racoonbug/netstat.$$
cp $1 /tmp/racoonbug/logfile.$$
cp $2 /tmp/racoonbug/conffile.$$
cd /tmp/racoonbug
shar spd.$$ sad.$$ ifconfig.$$ netstat.$$ logfile.$$ conffile.$$ | mail sakane@kame.net
cd /tmp
/bin/rm -fr /tmp/racoonbug
