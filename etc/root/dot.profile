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
