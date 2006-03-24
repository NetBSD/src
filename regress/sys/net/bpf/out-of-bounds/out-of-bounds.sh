#!/bin/sh

# an out-of-bounds read in the BPF expression should exit the bpf program,
# regardless of other expressions. Pass an expression to tcpdump which
# performs an out-of-bound read and ping with a packet which would cause
# the expression to succeed if the out-of-bounds read is not handled.

# exit status: 0 - pass, 1 - fail, 2 - skip

IFACE=${IFACE:-lo0}
ADDR=${ADDR:-127.0.0.1}

tcpdump -c1 -np -i $IFACE \( link[34000:2]=0 or icmp \) and ip[36:2]=0xcafe > /dev/null &
TCPDUMP_PID=$!

sleep 1
if ! kill -0 $TCPDUMP_PID > /dev/null 2>&1 ; then
	echo "SKIPPED tcpdump exited - are you root?"; exit 2;
fi
if ! ping -c1 -p cafe $ADDR > /dev/null 2>&1 ; then
	echo "SKIPPED not able to ping localhost"; 
	kill $TCPDUMP_PID > /dev/null 2>&1 ; exit 2;
fi
sleep 2
if ! kill $TCPDUMP_PID > /dev/null 2>&1; then
	echo "FAILED"; exit 1;
fi
wait $TCPDUMP_PID; echo "PASSED"; exit 0;
