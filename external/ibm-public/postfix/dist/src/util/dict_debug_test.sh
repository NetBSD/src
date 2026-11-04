#!/bin/sh
set -e

echo ">>> dict_open 'debug:' read"
! ${VALGRIND} ./dict_open 'debug:' read </dev/null || exit 1

echo ">>> dict_open 'debug:missing_colon_and_name' read"
! ${VALGRIND} ./dict_open 'debug:missing_colon_and_name' read </dev/null || exit 1

echo ">>> dict_open 'debug:static:{space in name}' read"
${VALGRIND} ./dict_open 'debug:static:{space in name}' read <<EOF
get k
EOF

echo ">>> dict_open 'debug:debug:static:value' read"
${VALGRIND} ./dict_open 'debug:debug:static:value' read <<EOF
get k
EOF

echo ">>> dict_open 'debug:internal:name' write"
${VALGRIND} ./dict_open 'debug:internal:name' write <<EOF
get k
put k=v
get k
first
next
del k
get k
del k
first
EOF

echo ">>> dict_open 'debug:fail:{oh no}' read"
${VALGRIND} ./dict_open 'debug:fail:{oh no}' read <<EOF
get k
first
EOF
