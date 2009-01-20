#!/bin/sh

# Make a GPG keyring for testing purposes...

rm -f ../test/t2.pub ../test/t2.sec

gpg --gen-key --batch <<EOF
#%dry-run
%pubring ../test/t2.pub
%secring ../test/t2.sec
Key-Type: rsa
Name-Real: OPS Test
Name-Comment: This is a test
Name-Email: ops@links.org
Passphrase: xxx
%commit
EOF
