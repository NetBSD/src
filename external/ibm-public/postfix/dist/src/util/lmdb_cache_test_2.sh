#!/bin/sh

# Torture test: run update and delete ${1-10000}perations in
# parallel processes. None should remain.

set -e

rm -f foo.lmdb
./dict_cache <<EOF
lmdb_map_size 20000
cache lmdb:foo
EOF

(./dict_cache <<EOF
cache lmdb:foo
update x ${1-10000}
run 
update y ${1-10000}
delete x ${1-10000}
run 
delete y ${1-10000}
run 
EOF
) &

(./dict_cache <<EOF
cache lmdb:foo
update a ${1-10000}
run 
update b ${1-10000}
delete a ${1-10000}
run 
delete b ${1-10000}
run 
EOF
) &

wait

../../bin/postmap -s lmdb:foo | diff /dev/null -

rm -f foo.lmdb
