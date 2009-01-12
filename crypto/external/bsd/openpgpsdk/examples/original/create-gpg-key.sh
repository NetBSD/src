#!/bin/sh

# Make a GPG keyring for testing purposes...

mkdir -p ../scratch

rm -f ../scratch/t1.pub ../scratch/t1.sec

gpg --gen-key --batch <<EOF
#%dry-run
%pubring ../scratch/t1.pub
%secring ../scratch/t1.sec
Key-Type: rsa
Name-Real: OPS Test
Name-Comment: This is a test
Name-Email: ops@links.org
%commit
EOF
