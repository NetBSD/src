#!/bin/sh

PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin

# Correctly flush automatically generated SAD and SPD
# This should go away the day racoon will properly do the job.

echo "
deleteall ${REMOTE_ADDR} ${LOCAL_ADDR} esp;
deleteall ${LOCAL_ADDR} ${REMOTE_ADDR} esp;
spddelete ${INTERNAL_ADDR4}/32[any] 0.0.0.0/0[any] any 
	-P in ipsec esp/tunnel/${REMOTE_ADDR}-${LOCAL_ADDR}/require;
spddelete 0.0.0.0/0[any] ${INTERNAL_ADDR4}/32[any] any 
	-P out ipsec esp/tunnel/${LOCAL_ADDR}-${REMOTE_ADDR}/require;
"|setkey -c
