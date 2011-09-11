#!/bin/sh
#
# Clean up after tsiggss tests.
#

rm -f ns1/*.jnl ns1/update.txt ns1/auth.sock
rm -f ns1/*.db ns1/K*.key ns1/K*.private
rm -f ns1/_default.tsigkeys
rm -f */named.memstats
rm -f authsock.pid
rm -f ns1/core
