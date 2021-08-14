#!/bin/sh
openssl=$(which openssl)

if [ x"$openssl" = "x" ]; then
echo "OpenSSL command line binary not found, skipping..."
fi

KEY_BITS=4096
KEY_TYPE=rsa:$KEY_BITS

USAGE="$0 [-s] [-u <user@domain.com>]"
SERVER=0
USER=0
EMAIL=

while test $# -gt 0 ; do
	case "$1" in
		-s | -server)
			SERVER=1;
			shift;;
		-u | -user)
			if [ x"$2" = "x" ]; then
				echo "User cert requires an email address as an argument"
				exit;
			fi
			USER=1;
			EMAIL="$2";
			shift; shift;;
		-)
			shift;;
		-*)
			echo "$USAGE"; exit 1
			;;
		*)
			break;;
	esac
done

if [ $SERVER = 0 -a $USER = 0 ]; then
	echo "$USAGE";
	exit 1;
fi

rm -rf ./openssl.cnf cruft
mkdir -p private certs cruft/private cruft/certs

echo "00" > cruft/serial
touch cruft/index.txt
touch cruft/index.txt.attr
hn=$(hostname -f)
sed -e "s;@HOSTNAME@;$hn;" -e "s;@KEY_BITS@;$KEY_BITS;" conf/openssl.cnf >  ./openssl.cnf

if [ $SERVER = 1 ]; then
	rm -rf private/localhost.key certs/localhost.crt

	$openssl req -new -nodes -out localhost.csr -keyout private/localhost.key \
		-newkey $KEY_TYPE -config ./openssl.cnf \
		-subj "/CN=localhost/OU=OpenLDAP Test Suite/O=OpenLDAP Foundation/ST=CA/C=US" \
		-batch > /dev/null 2>&1

	$openssl ca -out certs/localhost.crt -notext -config ./openssl.cnf -days 183000 -in localhost.csr \
		-keyfile ca/private/testsuiteCA.key -extensions v3_req -cert ca/certs/testsuiteCA.crt \
		-batch >/dev/null 2>&1

	rm -rf ./openssl.cnf ./localhost.csr cruft
fi

if [ $USER = 1 ]; then
	rm -f certs/$EMAIL.crt private/$EMAIL.key $EMAIL.csr

	$openssl req -new -nodes -out $EMAIL.csr -keyout private/$EMAIL.key \
		-newkey $KEY_TYPE -config ./openssl.cnf \
		-subj "/emailAddress=$EMAIL/CN=$EMAIL/OU=OpenLDAP/O=OpenLDAP Foundation/ST=CA/C=US" \
		-batch >/dev/null 2>&1

	$openssl ca -out certs/$EMAIL.crt -notext -config ./openssl.cnf -days 183000 -in $EMAIL.csr \
		-keyfile ca/private/testsuiteCA.key -extensions req_distinguished_name \
		-cert ca/certs/testsuiteCA.crt -batch >/dev/null 2>&1

	rm -rf ./openssl.cnf ./$EMAIL.csr cruft
fi
