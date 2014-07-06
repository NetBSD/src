#!/bin/sh

# Torture test: run update and purge operations in parallel processes.
# This will result in some purge operations not finding all entries,
# but the final sequential purge should eliminate all.

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
purge x
run 
purge y
run 
EOF
) &

(./dict_cache <<EOF
cache lmdb:foo
update a ${1-10000}
run 
update b ${1-10000}
purge a
run 
purge b
run 
EOF
) &

wait

./dict_cache <<EOF
cache lmdb:foo
purge a
run
purge b
run
purge x
run
purge y
run
EOF

../../bin/postmap -s lmdb:foo | diff /dev/null -

rm -f foo.lmdb
