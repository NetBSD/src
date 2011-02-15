#!/bin/sh
#
# Clean up after tsiggss tests.
#

rm -f ns1/*.jnl ns1/update.txt ns1/auth.sock
rm -f */named.memstats
rm -f authsock.pid
