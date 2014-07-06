#! /bin/bash

set -e

DOMAIN=example.com
HOST=mail.${DOMAIN}
TEST=./tls_dane

key() {
    local key=$1; shift

    if [ ! -f "${key}.pem" ]; then
    	openssl genpkey 2>/dev/null \
	    -paramfile <(openssl ecparam -name prime256v1) \
	    -out "${key}.pem"
    fi
}

req() {
    local key=$1; shift
    local cn=$1; shift

    key "$key"
    openssl req -new -sha256 -key "${key}.pem" 2>/dev/null \
	-config <(printf "[req]\n%s\n%s\n[dn]\nCN=%s\n" \
		   "prompt = no" "distinguished_name = dn" "${cn}") 
}

req_nocn() {
    local key=$1; shift

    key "$key"
    openssl req -new -sha256 -subj / -key "${key}.pem" 2>/dev/null \
	-config <(printf "[req]\n%s\n[dn]\nCN_default =\n" \
		   "distinguished_name = dn") 
}

cert() {
    local cert=$1; shift
    local exts=$1; shift

    openssl x509 -req -sha256 -out "${cert}.pem" 2>/dev/null \
	-extfile <(printf "%s\n" "$exts") "$@"
}

genroot() {
    local cn=$1; shift
    local key=$1; shift
    local cert=$1; shift
    local skid=$1; shift
    local akid=$1; shift

    exts=$(printf "%s\n%s\n%s\n" "$skid" "$akid" "basicConstraints = CA:true")
    req "$key" "$cn" |
    	cert "$cert" "$exts" -signkey "${key}.pem" -set_serial 1 -days 30
}

genca() {
    local cn=$1; shift
    local key=$1; shift
    local cert=$1; shift
    local skid=$1; shift
    local akid=$1; shift
    local ca=$1; shift
    local cakey=$1; shift

    exts=$(printf "%s\n%s\n%s\n" "$skid" "$akid" "basicConstraints = CA:true")
    req "$key" "$cn" |
    	cert "$cert" "$exts" -CA "${ca}.pem" -CAkey "${cakey}.pem" \
	    -set_serial 2 -days 30 "$@"
}

genee() {
    local cn=$1; shift
    local key=$1; shift
    local cert=$1; shift
    local ca=$1; shift
    local cakey=$1; shift

    exts=$(printf "%s\n%s\n%s\n%s\n%s\n[alts]\n%s\n" \
	    "subjectKeyIdentifier = hash" \
	    "authorityKeyIdentifier = keyid, issuer" \
	    "basicConstraints = CA:false" \
	    "extendedKeyUsage = serverAuth" \
	    "subjectAltName = @alts" "DNS=${cn}")
    req "$key" "$cn" |
    	cert "$cert" "$exts" -CA "${ca}.pem" -CAkey "${cakey}.pem" \
	    -set_serial 2 -days 30 "$@"
}

genss() {
    local cn=$1; shift
    local key=$1; shift
    local cert=$1; shift

    exts=$(printf "%s\n%s\n%s\n%s\n%s\n[alts]\n%s\n" \
	    "subjectKeyIdentifier   = hash" \
	    "authorityKeyIdentifier = keyid, issuer" \
	    "basicConstraints = CA:true" \
	    "extendedKeyUsage = serverAuth" \
	    "subjectAltName = @alts" "DNS=${cn}")
    req "$key" "$cn" |
	cert "$cert" "$exts" -set_serial 1 -days 30 -signkey "${key}.pem" "$@"
}

gennocn() {
    local key=$1; shift
    local cert=$1; shift

    req_nocn "$key" |
	cert "$cert" "" -signkey "${key}.pem" -set_serial 1 -days -1 "$@"
}

runtest() {
    local desc=$1; shift
    local usage=$1; shift
    local selector=$1; shift
    local mtype=$1; shift
    local tlsa=$1; shift
    local ca=$1; shift
    local chain=$1; shift
    local digest

    case $mtype in
    0) digest="";;
    1) digest=sha256;;
    2) digest=sha512;;
    *) echo "bad mtype: $mtype"; exit 1;;
    esac

    printf "%d %d %d %-24s %s: " "$usage" "$selector" "$mtype" "$tlsa" "$desc"

    if [ -n "$ca" ]; then ca="$ca.pem"; fi
    "$TEST" "$usage" "$selector" "$digest" "$tlsa.pem" "$ca" "$chain.pem" \
    	"$@" > /dev/null
}

checkpass() { runtest "$@" && { echo pass; } || { echo fail; exit 1; }; }
checkfail() { runtest "$@" && { echo fail; exit 1; } || { echo pass; }; }

#---------

genss "$HOST" sskey sscert
gennocn akey acert

# Tests that might depend on akid/skid chaining
#
for rakid in "" \
	"authorityKeyIdentifier  = keyid,issuer" \
	"authorityKeyIdentifier  = issuer" \
	"authorityKeyIdentifier  = keyid"
do
for cakid in "" \
	"authorityKeyIdentifier  = keyid,issuer" \
	"authorityKeyIdentifier  = issuer" \
	"authorityKeyIdentifier  = keyid"
do
for rskid in "" "subjectKeyIdentifier = hash"
do
for caskid in "" "subjectKeyIdentifier = hash"
do

genroot "Root CA" rootkey rootcert "$rskid" "$rakid"
genca "CA 1" cakey1 cacert1 "$caskid" "$cakid" rootcert rootkey
genca "CA 2" cakey2 cacert2 "$caskid" "$cakid" cacert1 cakey1
genee "$HOST" eekey eecert cacert2 cakey2
cat eecert.pem cacert2.pem cacert1.pem rootcert.pem > chain.pem
cat eecert.pem cacert2.pem cacert1.pem > chain1.pem

for s in 0 1
do
  checkpass "OOB root TA" 2 "$s" 0 rootcert "" chain1 "$HOST"
  checkpass "OOB TA" 2 "$s" 0 cacert2 "" eecert "$HOST"
  checkpass "in-chain root TA" 2 "$s" 1 rootcert "" chain "$HOST"

  for m in 0 1 2
  do
    checkpass "valid TA" 2 "$s" "$m" rootcert "" chain "$HOST"
    for ca in "cacert1" "cacert2"; do
	checkpass "valid TA" 2 "$s" "$m" "$ca" "" chain "$HOST"
	checkpass "valid TA" 2 "$s" "$m" "$ca" "" chain1 "$HOST"
	checkpass "valid TA+CA" 2 "$s" "$m" "$ca" rootcert chain1 "$HOST"
	checkpass "sub-domain" 2 "$s" "$m" "$ca" "" chain1 whatever ".$DOMAIN"
	checkfail "wrong name" 2 "$s" "$m" "$ca" "" chain1 "whatever"
    done
  done
done

done
done
done
done

# These tests don't depend in the akid/skid chaining:
#
for s in 0 1
do
  checkfail "missing TA" 2 "$s" 1 rootcert "" chain1 "$HOST"
  for m in 0 1 2
  do
    checkpass "depth 0 TA" 2 "$s" "$m" sscert "" sscert "$HOST" 
    checkfail "non-TA" 2 "$s" "$m" eecert rootcert chain "$HOST"
    checkfail "depth 0 TA namecheck" 2 "$s" "$m" sscert sscert sscert whatever

    checkpass "valid EE" 3 "$s" "$m" eecert "" chain whatever
    checkpass "key-only EE" 3 "$s" "$m" acert "" acert whatever
    checkfail "wrong EE" 3 "$s" "$m" cacert2 "" chain whatever
  done
done

rm -f *.pem
