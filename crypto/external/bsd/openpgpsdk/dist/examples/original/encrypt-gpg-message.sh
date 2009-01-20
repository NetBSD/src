#!/bin/sh

mkdir -p ../scratch

cat > ../scratch/to-be-encrypted <<EOF
Just a small test

Over a few lines, why not?

EOF

rm -f ../scratch/to-be-encrypted.gpg

gpg --trust-model always --no-default-keyring --secret-keyring ../scratch/t1.sec --keyring ../scratch/t1.pub --recipient ops@links.org --batch --encrypt ../scratch/to-be-encrypted
