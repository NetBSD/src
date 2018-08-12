# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

# Set known locale for the tests

if locale -a | grep -qE "^C\\.(UTF-8|utf8)"; then
    LC_ALL="C.UTF-8"
elif locale -a | grep -qE "^en_US\\.(UTF-8|utf8)"; then
    LC_ALL="en_US.UTF-8"
fi
export LC_ALL

# This set of tests check the behavior of the IDNA options in "dig".
#
# "dig" supports two IDNA-related options:
#
# +[no]idnin -  Translates a domain name into punycode format before sending
#               the query to the server.
#
#               Should the input name be a punycode name, "dig +idnin" will also
#               validate the punycode, rejecting it if it is invalid.
#
# +[no]idnout - Translates the received punycode domain names into appropriate
#               unicode characters before displaying.
#
# The tests run "dig" against an authoritative server configured with a minimal
# root zone and nothing else.  As a result, all queries will result in an
# NXDOMAIN.  The server will return the qname sent, which "dig" will display
# according to the options selected.  This returned string is compared with
# the qname originally sent.
#
# In the comments below, the following nomenclature (taken from RFC 5890) is
# used:
#
# A-label: Label comprising ASCII characters that starts xn-- and whose
#          characters after the xn-- are a valid output of the Punycode
#          algorithm.
#
# Fake A-label: An A-label whose characters after the xn-- are not valid
#          Punycode output.
#
# U-label: Unicode (native character) form of a label.
#
# For the purpose of this test script, U-labels do not include labels that
# comprise purely ASCII characters, which are referred to as "ASCII-labels"
# here. Valid ASCII-labels comprise letters, digits and hyphens and do not
# start with a hyphen.
#
# References:
# 1. http://www.unicode.org/reports/tr46/#Deviations
# 2. http://www.unicode.org/reports/tr46/#IDNAComparison

# Using dig insecure mode as we are not testing DNSSEC here
DIGCMD="$DIG -i -p ${PORT} @10.53.0.1"

# Initialize test count and status return
n=0
status=0


# Function for extracting the qname from the response
#
# This is the first field in the line after the line starting
# ";; QUESTION SECTION:".
#
# The string returned includes the trailing period.

qname() {
    awk 'BEGIN { qs = 0; } \
        /;; QUESTION SECTION:/ { qs = 1; next; } \
        qs == 1 {sub(";", "", $1) ; print $1; exit 0; }' \
        $1
}

# Function for performing a test where "dig" is expected to succeed.
#
#   $1 - Description of the test
#   $2 - Dig command additional options
#   $3 - Name being queried
#   $4 - The name that is expected to be displayed by "dig".  Note that names
#        displayed by "dig" will always have a trailing period, so this
#        parameter should have that period as well.

idna_test() {
    n=`expr $n + 1`
    description=$1
    if [ "$2" != "" ]; then
        description="${description}: $2"
    fi
    echo_i "$description ($n)"

    ret=0
    $DIGCMD $2 $3 > dig.out.$n 2>&1
    if [ $? -ne 0 ]; then
        echo_i "failed: dig command returned non-zero status"
        ret=1
    else
        actual=`qname dig.out.$n`
        if [ "$4" != "$actual" ]; then
            echo_i "failed: expected answer $4, actual result $actual"
            ret=1
        fi
    fi
    status=`expr $status + $ret`
}

# Function for performing a test where "dig" is expected to fail
#
#   $1 - Description of the test
#   $2 - Dig command additional options
#   $3 - Name being queried

idna_fail() {
    n=`expr $n + 1`
    description=$1
    if [ "$2" != "" ]; then
        description="${description}: $2"
    fi
    echo_i "$description ($n)"

    ret=0
    $DIGCMD $2 $3 > dig.out.$n 2>&1
    if [ $? -eq 0 ]; then
        echo_i "failed: dig command unexpectedly succeeded"
        ret=1
    fi
    status=`expr $status + $ret`
}

# Check if current version of libidn2 is >= a given version
#
# This requires that:
# a) "pkg-config" exists on the system
# b) The libidn2 installed has an associated ".pc" file
# c) The system sort command supports "-V"
#
# $1 - Minimum version required
#
# Returns:
# 0 - Version check is OK, libidn2 at required version or greater.
# 1 - Version check was made, but libidn2 not at required version.
# 2 - Could not carry out version check

libidn_version_check() {
    ret=2
    if [ -n "`command -v pkg-config`" ]; then
        version=`pkg-config --modversion --silence-errors libidn2`
        if [ -n "$version" ]; then
            # Does the sort command have a "-V" flag on this system?
            sort -V 2>&1 > /dev/null << .
.
            if [ $? -eq 0 ]; then
                # Sort -V exists.  Sort the IDN version and the minimum version
                # required.  If the IDN version is greater than or equal to that
                # version, it will appear last in the list.
                last_version=`printf "%s\n" $version $1 | sort -V | tail -1`
                if [ "$version" = "$last_version" ]; then
                    ret=0
                else
                    ret=1
                fi
            fi
        fi
    fi

    return $ret
}


# Function to check that case is preserved for an all-ASCII label.
#
# Without IDNA support, case-preservation is the expected behavior.
#
# With IDNA support... not really.  IDNA maps uppercase ASCII characters to
# their lower-case equivalent.  When IDNA support in "dig" was updated to
# non-transitional IDNA 2008, the switch "+idnin" was added and made the default
# behaviour. This meant that the command "dig LocalhosT" (no command switches)
# sends the qname "localhost", a change in behavior from earlier versions.
#
# This was felt to be confusing to the significant number of users who are
# not interested in IDNA. For this reason, after "dig" passes the input qname
# through the IDNA conversion, is does a case-insensitive comparison with the
# result.  If the two are the same, "dig" can conclude that the qname is
# entirely ASCII and is uses the entered string instead of the converted string
# as the qname.

ascii_case_preservation_test() {
    text="Checking valid ASCII label"
    idna_test "$text" ""                   LocalhosT LocalhosT.
    idna_test "$text" "+noidnin +noidnout" LocalhosT LocalhosT.
    idna_test "$text" "+noidnin +idnout"   LocalhosT LocalhosT.
    idna_test "$text" "+idnin   +noidnout" LocalhosT LocalhosT.
    idna_test "$text" "+idnin   +idnout"   LocalhosT LocalhosT.
}

# Function to perform the tests if IDNA is enabled.

idna_enabled_test() {
    echo_i "IDNA is enabled, all IDNA tests will be performed"
    # Check that case is preserved on an ASCII label.

    ascii_case_preservation_test


    # Test of a valid U-label
    #
    # +noidnin +noidnout: The label is sent as a unicode octet stream and dig
    #                     will display the string in the \nnn format.
    # +noidnin +idnout:   As for the previous case.
    # +idnin   +noidnout: The label is converted to the xn-- format.  "dig"
    #                     displays the returned xn-- text.
    # +idnin   +idnout:   The label is converted to the xn-- format.  "dig"
    #                     converts the returned xn-- string back to the original
    #                     unicode text.
    #
    # Note that ASCII characters are converted to lower-case.

    text="Checking valid non-ASCII label"
    idna_test "$text" ""                   "München" "münchen." 
    idna_test "$text" "+noidnin +noidnout" "München" "M\195\188nchen."
    idna_test "$text" "+noidnin +idnout"   "München" "M\195\188nchen."
    idna_test "$text" "+idnin   +noidnout" "München" "xn--mnchen-3ya."
    idna_test "$text" "+idnin   +idnout"   "München" "münchen." 


    # Tests of transitional processing of a valid U-label
    #
    # IDNA2003 introduced national character sets but, unfortunately, didn't
    # support several characters properly.  One of those was the German
    # character "ß" (the "Eszett" or "sharp s"), which was interpreted as "ss".
    # So the domain “faß.de” domain (for example) was processed as “fass.de”.
    #
    # This was corrected in IDNA2008, although some vendors that adopted this
    # standard chose to keep the existing IDNA2003 translation for this
    # character to prevent problems (e.g. people visiting www.faß.example would,
    # under IDNA2003, go to www.fass.example but under IDNA2008 would end up at
    # www.fa\195\159.example - a different web site).
    #
    # BIND has adopted a hard transition, so this test checks that these
    # transitional mapping is not used.  The tests are essentially the same as
    # for the valid U-label.

    text="Checking that non-transitional IDNA processing is used"
    idna_test "$text" ""                   "faß.de" "faß.de."
    idna_test "$text" "+noidnin +noidnout" "faß.de" "fa\195\159.de."
    idna_test "$text" "+noidnin +idnout"   "faß.de" "fa\195\159.de."
    idna_test "$text" "+idnin   +noidnout" "faß.de" "xn--fa-hia.de."
    idna_test "$text" "+idnin   +idnout"   "faß.de" "faß.de."

    # Another problem character.  The final character in the first label mapped
    # onto the Greek sigma character ("σ") in IDNA2003.

    text="Second check that non-transitional IDNA processing is used"
    idna_test "$text" ""                   "βόλος.com" "βόλος.com." 
    idna_test "$text" "+noidnin +noidnout" "βόλος.com" "\206\178\207\140\206\187\206\191\207\130.com."
    idna_test "$text" "+noidnin +idnout"   "βόλος.com" "\206\178\207\140\206\187\206\191\207\130.com."
    idna_test "$text" "+idnin   +noidnout" "βόλος.com" "xn--nxasmm1c.com."
    idna_test "$text" "+idnin   +idnout"   "βόλος.com" "βόλος.com." 



    # Tests of a valid A-label (i.e. starting xn--)
    #
    # +noidnout: The string is sent as-is to the server and the returned qname
    #            is displayed in the same form.
    # +idnout:   The string is sent as-is to the server and the returned qname
    #            is displayed as the corresponding U-label.
    #
    # The "+[no]idnin" flag has no effect in these cases.

    text="Checking valid A-label"
    idna_test "$text" ""                   "xn--nxasmq6b.com" "βόλοσ.com." 
    idna_test "$text" "+noidnin +noidnout" "xn--nxasmq6b.com" "xn--nxasmq6b.com."
    idna_test "$text" "+noidnin +idnout"   "xn--nxasmq6b.com" "βόλοσ.com." 
    idna_test "$text" "+idnin +noidnout"   "xn--nxasmq6b.com" "xn--nxasmq6b.com."
    idna_test "$text" "+idnin +idnout"     "xn--nxasmq6b.com" "βόλοσ.com."



    # Tests of invalid A-labels
    #
    # +noidnin: The label is sent as-is to the server and dig will display the
    #           returned fake A-label in the same form.
    # +idnin:   "dig" should report that the label is not correct.
    #
    # +[no]idnout: If the label makes it to the server (via +noidnin), "dig"
    #           should report an error if +idnout is specified.

    # The minimum length of a punycode A-label is 7 characters.  Check that
    # a shorter label is detected and rejected.

    text="Checking punycode label shorter than minimum valid length"
    idna_fail "$text" ""                   "xn--xx"
    idna_test "$text" "+noidnin +noidnout" "xn--xx" "xn--xx."
    idna_fail "$text" "+noidnin   +idnout" "xn--xx"
    idna_fail "$text" "+idnin   +noidnout" "xn--xx"
    idna_fail "$text" "+idnin     +idnout" "xn--xx"

    # Fake A-label - the string does not translate to anything.

    text="Checking fake A-label"
    idna_fail "$text" ""                   "xn--ahahah"
    idna_test "$text" "+noidnin +noidnout" "xn--ahahah" "xn--ahahah."

    # Owing to issues with libdns, the next test will fail for versions of
    # libidn earlier than 2.0.5.  For this reason, get the version (if
    # available) and compare with 2.0.5.
    libidn_version_check 2.0.5
    if [ $? -ne 0 ]; then
        echo_i "Skipping fake A-label +noidnin +idnout test (libidn2 version issues)"
    else
        idna_test "$text" "+noidnin   +idnout" "xn--ahahah" "xn--ahahah."
    fi
    idna_fail "$text" "+idnin   +noidnout" "xn--ahahah"
    idna_fail "$text" "+idnin     +idnout" "xn--ahahah"

    # Too long a label. The punycode string is too long (at 64 characters).
    # BIND rejects such labels: with +idnin

    label="xn--xflod18hstflod18hstflod18hstflod18hstflod18hstflod18-1iejjjj"
    text="Checking punycode label shorter than minimum valid length"
    idna_fail "$text" ""                   "$label"
    idna_fail "$text" "+noidnin +noidnout" "$label"
    idna_fail "$text" "+noidnin   +idnout" "$label"
    idna_fail "$text" "+idnin   +noidnout" "$label"
    idna_fail "$text" "+idnin     +idnout" "$label"

    


    # Tests of a valid unicode string but an invalid U-label
    #
    # Symbols are not valid IDNA names.
    #
    # +noidnin: "dig" should send unicode octets to the server and display the
    #           returned qname in the same form.
    # +idnin:   "dig" should generate an error.
    #
    # The +[no]idnout options should not have any effect on the test.

    text="Checking invalid U-label"
    idna_fail "$text" ""                   "🧦.com"
    idna_test "$text" "+noidnin +noidnout" "🧦.com" "\240\159\167\166.com."
    idna_test "$text" "+noidnin +idnout"   "🧦.com" "\240\159\167\166.com."
    idna_fail "$text" "+idnin   +noidnout" "🧦.com"
    idna_fail "$text" "+idnin   +idnout"   "🧦.com"
}


# Function to perform tests if IDNA is not enabled.

idna_disabled_test() {
    echo_i "IDNA is disabled, only case mapping tests will be performed"
    ascii_case_preservation_test
}


# Main test begins here

$FEATURETEST --with-idn
if [ $? -eq 0 ]; then
    idna_enabled_test
else
    idna_disabled_test
fi

exit $status
