#!/bin/sh
#
# Copyright (C) 2004, 2007, 2009, 2011-2017  Internet Systems Consortium, Inc. ("ISC")
# Copyright (C) 2001-2003  Internet Software Consortium.
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.

#
# Set up a test zone
#
# Usage: genzone.sh master-server-number slave-server-number...
#
# e.g., "genzone.sh 2 3 4" means ns2 is the master and ns3, ns4
# are slaves.
#

master="$1"

cat <<EOF
\$TTL 3600

@		86400	IN SOA	ns${master} hostmaster (
					1397051952 ; "SER0"
					5
					5
					1814400
					3600 )
EOF

for n
do
	cat <<EOF
@			NS	ns${n}
ns${n}			A	10.53.0.${n}
EOF
done

cat <<\EOF

; type 1
a01			A	0.0.0.0
a02			A	255.255.255.255

; type 2
; see NS records at top of file

; type 3
; md01			MD	madname
; 			MD	.

; type 4
; mf01			MF	madname
; mf01			MF	.

; type 5
cname01			CNAME	cname-target.
cname02			CNAME	cname-target
cname03			CNAME	.

; type 6
; see SOA record at top of file

; type 7
mb01			MG	madname
mb02			MG	.

; type 8
mg01			MG	mgmname
mg02			MG	.

; type 9
mr01			MR	mrname
mr02			MR	.

; type 10
; NULL RRs are not allowed in master files per RFC1035.
;null01			NULL

; type 11
wks01			WKS	10.0.0.1 tcp telnet ftp 0 1 2
wks02			WKS	10.0.0.1 udp domain 0 1 2
wks03			WKS	10.0.0.2 tcp 65535

; type 12
ptr01			PTR	@

; type 13
hinfo01			HINFO	"Generic PC clone" "NetBSD-1.4"
hinfo02			HINFO	PC NetBSD

; type 14
minfo01			MINFO	rmailbx emailbx
minfo02			MINFO	. . 

; type 15
mx01			MX	10 mail
mx02			MX	10 .

; type 16
txt01			TXT	"foo"
txt02			TXT	"foo" "bar"
txt03			TXT	foo
txt04			TXT	foo bar
txt05			TXT	"foo bar"
txt06			TXT	"foo\032bar"
txt07			TXT	foo\032bar
txt08			TXT	"foo\010bar"
txt09			TXT	foo\010bar
txt10			TXT	foo\ bar
txt11			TXT	"\"foo\""
txt12			TXT	\"foo\"
txt13			TXT	"foo;"
txt14			TXT	"foo\;"
txt15			TXT	"bar\\;"

; type 17
rp01			RP	mbox-dname txt-dname
rp02			RP	. . 

; type 18
afsdb01			AFSDB	0 hostname
afsdb02			AFSDB	65535 .

; type 19
x2501			X25	123456789
;x2502			X25	"123456789"

; type 20
isdn01			ISDN	"isdn-address"
isdn02			ISDN	"isdn-address" "subaddress"
isdn03			ISDN	isdn-address
isdn04			ISDN	isdn-address subaddress

; type 21
rt01			RT	0 intermediate-host
rt02			RT	65535 .

; type 22
nsap01			NSAP	(
	0x47.0005.80.005a00.0000.0001.e133.ffffff000161.00 )
nsap02			NSAP	(
	0x47.0005.80.005a00.0000.0001.e133.ffffff000161.00. )
;nsap03			NSAP	0x

; type 23
nsap-ptr01		NSAP-PTR foo.
nsap-ptr01		NSAP-PTR .

; type 24
;sig01			SIG	NXT 1 3 ( 3600 20000102030405
;				19961211100908 2143 foo.nil. 
;				MxFcby9k/yvedMfQgKzhH5er0Mu/vILz45I
;				kskceFGgiWCn/GxHhai6VAuHAoNUz4YoU1t
;				VfSCSqQYn6//11U6Nld80jEeC8aTrO+KKmCaY= )

; type 25
;key01			KEY	512 ( 255 1 AQMFD5raczCJHViKtLYhWGz8hMY
;				9UGRuniJDBzC7w0aRyzWZriO6i2odGWWQVucZqKV
;				sENW91IOW4vqudngPZsY3GvQ/xVA8/7pyFj6b7Esg
;				a60zyGW6LFe9r8n6paHrlG5ojqf0BaqHT+8= )

; type 26
px01			PX	65535 foo. bar.
px02			PX	65535 . .

; type 27
gpos01			GPOS    -22.6882 116.8652 250.0
gpos02			GPOS    "" "" ""

; type 28
aaaa01			AAAA    ::1
aaaa02			AAAA    fd92:7065:b8e:ffff::5

; type 29
loc01			LOC	60 9 N 24 39 E 10 20 2000 20
loc02			LOC 	60 09 00.000 N 24 39 00.000 E 10.00m 20.00m (
				  2000.00m 20.00m )

; type 30
;nxt01			NXT	a.secure.nil. ( NS SOA MX RRSIG KEY LOC NXT )
;nxt02			NXT	. NXT NSAP-PTR
;nxt03			NXT	. 1
;nxt04			NXT	. 127

; type 31 (EID - not implemented by BIND)
; type 32 (NIMLOC - not implemented by BIND)

; type 33
srv01			SRV 0 0 0 .
srv02			SRV 65535 65535 65535  old-slow-box

; type 34 (ATMA - not implemented by BIND)

; type 35
naptr01			NAPTR   0 0 "" "" "" . 
naptr02			NAPTR   65535 65535 blurgh blorf blllbb foo.
naptr02			NAPTR   65535 65535 "blurgh" "blorf" "blllbb" foo.

; type 36
kx01			KX	10 kdc
kx02			KX	10 .

; type 37
cert01			CERT	65534 65535 254 ( 
				MxFcby9k/yvedMfQgKzhH5er0Mu/vILz45I
				kskceFGgiWCn/GxHhai6VAuHAoNUz4YoU1t
				VfSCSqQYn6//11U6Nld80jEeC8aTrO+KKmCaY= )
; type 38
a601			A6	0 ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff
a601			A6	64 ::ffff:ffff:ffff:ffff foo.
a601			A6	127 ::1 foo.
a601			A6	128 .

; type 39
dname01			DNAME	dname-target.
dname02			DNAME	dname-target
dname03			DNAME	.

; type 40
sink01			SINK	1 0 0
sink02			SINK	8 0 2 l4ik

; type 41
; OPT is a meta-type and should never occur in master files.

; type 42
apl01			APL	!1:10.0.0.1/32 1:10.0.0.0/24

; type 43
ds01			DS	12892 5 2 26584835CA80C81C91999F31CFAF2A0E89D4FF1C8FAFD0DDB31A85C7 19277C13
ds01			NS	ns42
ds02			DS	12892 5 1 7AA4A3F416C2F2391FB7AB0D434F762CD62D1390
ds02			NS	ns43

; type 44
sshfp01			SSHFP	4 2 C76D8329954DA2835751E371544E963EFDA099080D6C58DD2BFD9A31 6E162C83
sshfp02			SSHFP	1 2 BF29468C83AC58CCF8C85AB7B3BEB054ECF1E38512B8353AB36471FA 88961DCC

; type 45
ipseckey01		IPSECKEY	10 1 2 192.0.2.38 AQNRU3mG7TVTO2BkR47usntb102uFJtugbo6BSGvgqt4AQ==
ipseckey02		IPSECKEY	10 0 2 . AQNRU3mG7TVTO2BkR47usntb102uFJtugbo6BSGvgqt4AQ==
ipseckey03		IPSECKEY	( 10 1 2
					  192.0.2.3
					  AQNRU3mG7TVTO2BkR47usntb102uFJtugbo6BSGvgqt4AQ== )
ipseckey04		IPSECKEY	( 10 3 2
					  mygateway.example.com.
					  AQNRU3mG7TVTO2BkR47usntb102uFJtugbo6BSGvgqt4AQ== )

ipseckey05		IPSECKEY	( 10 2 2
					  2001:0DB8:0:8002::2000:1
					  AQNRU3mG7TVTO2BkR47usntb102uFJtugbo6BSGvgqt4AQ== )

; type 46
rrsig01			RRSIG	NSEC 1 3 ( 3600 20000102030405
				19961211100908 2143 foo.nil. 
				MxFcby9k/yvedMfQgKzhH5er0Mu/vILz45I
				kskceFGgiWCn/GxHhai6VAuHAoNUz4YoU1t
				VfSCSqQYn6//11U6Nld80jEeC8aTrO+KKmCaY= )

; type 47
nsec01			NSEC	a.secure.nil. ( NS SOA MX RRSIG DNSKEY LOC NSEC )
nsec02			NSEC	. NSEC NSAP-PTR
nsec03			NSEC	. TYPE1
nsec04			NSEC	. TYPE127

; type 48
dnskey01		DNSKEY	512 ( 255 1 AQMFD5raczCJHViKtLYhWGz8hMY
				9UGRuniJDBzC7w0aRyzWZriO6i2odGWWQVucZqKV
				sENW91IOW4vqudngPZsY3GvQ/xVA8/7pyFj6b7Esg
				a60zyGW6LFe9r8n6paHrlG5ojqf0BaqHT+8= )

; type 49
dhcid01			DHCID	( AAIBY2/AuCccgoJbsaxcQc9TUapptP69l
				  OjxfNuVAA2kjEA= )
dhcid02			DHCID	( AAEBOSD+XR3Os/0LozeXVqcNc7FwCfQdW
				  L3b/NaiUDlW2No= )
dhcid03			DHCID	( AAABxLmlskllE0MVjd57zHcWmEH3pCQ6V
				  ytcKD//7es/deY= )

; type 50
8f1tmio9avcom2k0frp92lgcumak0cad	NSEC3 1 0 10 D2CF0294C020CE6C 8FPNS2UCT7FBS643THP2B77PEQ77K6IU  A NS SOA MX AAAA RRSIG DNSKEY NSEC3PARAM
kcd3juae64f9c5csl1kif1htaui7un0g	NSEC3 1 0 10 D2CF0294C020CE6C KD5MN2M20340DGO0BL7NTSB8JP4BSC7E
mr5ukvsk1l37btu4q7b1dfevft4hkqdk	NSEC3 1 0 10 D2CF0294C020CE6C MT38J6VG7S0SN5G17MCUF6IQIKFUAJ05  A AAAA RRSIG

; type 51
; @			NSEC3PARAM 1 0 1 868BCF7ED4108929

; type 52
tlsa			TLSA	( 1 1 2 92003ba34942dc74152e2f2c408d29ec
				a5a520e7f2e06bb944f4dca346baf63c
				1b177615d466f6c4b71c216a50292bd5
				8c9ebdd2f74e38fe51ffd48c43326cbc )

; type 53
smimea			SMIMEA	( 1 1 2 92003ba34942dc74152e2f2c408d29ec
				a5a520e7f2e06bb944f4dca346baf63c
				1b177615d466f6c4b71c216a50292bd5
				8c9ebdd2f74e38fe51ffd48c43326cbc )

; type 54 (unassigned)

; type 55
hip1			HIP	( 2 200100107B1A74DF365639CC39F1D578
				AwEAAbdxyhNuSutc5EMzxTs9LBPCIkOFH8cIvM4p9+LrV4e19WzK00+CI6zBCQTdtWsuxKbWIy87UOoJTwkUs7lBu+Upr1gsNrut79ryra+bSRGQb1slImA8YVJyuIDsj7kwzG7jnERNqnWxZ48AWkskmdHaVDP4BcelrTI3rMXdXF5D )

hip2			HIP	( 2 200100107B1A74DF365639CC39F1D578
                                AwEAAbdxyhNuSutc5EMzxTs9LBPCIkOFH8cIvM4p9+LrV4e19WzK00+CI6zBCQTdtWsuxKbWIy87UOoJTwkUs7lBu+Upr1gsNrut79ryra+bSRGQb1slImA8YVJyuIDsj7kwzG7jnERNqnWxZ48AWkskmdHaVDP4BcelrTI3rMXdXF5D
				rvs.example.com. )

; type 56
ninfo01			NINFO	"foo"
ninfo02			NINFO	"foo" "bar"
ninfo03			NINFO	foo
ninfo04			NINFO	foo bar
ninfo05			NINFO	"foo bar"
ninfo06			NINFO	"foo\032bar"
ninfo07			NINFO	foo\032bar
ninfo08			NINFO	"foo\010bar"
ninfo09			NINFO	foo\010bar
ninfo10			NINFO	foo\ bar
ninfo11			NINFO	"\"foo\""
ninfo12			NINFO	\"foo\"
ninfo13			NINFO	"foo;"
ninfo14			NINFO	"foo\;"
ninfo15			NINFO	"bar\\;"

; type 57
rkey01			RKEY	512 ( 255 1 AQMFD5raczCJHViKtLYhWGz8hMY
				9UGRuniJDBzC7w0aRyzWZriO6i2odGWWQVucZqKV
				sENW91IOW4vqudngPZsY3GvQ/xVA8/7pyFj6b7Esg
				a60zyGW6LFe9r8n6paHrlG5ojqf0BaqHT+8= )

; type 58
talink0			TALINK	. talink1
talink1			TALINK	talink0 talink2
talink2			TALINK	talink2 .

; type 59
cds01			CDS	30795 1 1 (
					310D27F4D82C1FC2400704EA9939FE6E1CEA
					A3B9 )

; type 60
cdnskey01		CDNSKEY	512 ( 255 1 AQMFD5raczCJHViKtLYhWGz8hMY
				9UGRuniJDBzC7w0aRyzWZriO6i2odGWWQVucZqKV
				sENW91IOW4vqudngPZsY3GvQ/xVA8/7pyFj6b7Esg
				a60zyGW6LFe9r8n6paHrlG5ojqf0BaqHT+8= )

; type 61
openpgpkey		OPENPGPKEY	( AQMFD5raczCJHViKtLYhWGz8hMY
				9UGRuniJDBzC7w0aRyzWZriO6i2odGWWQVucZqKV
				sENW91IOW4vqudngPZsY3GvQ/xVA8/7pyFj6b7Esg
				a60zyGW6LFe9r8n6paHrlG5ojqf0BaqHT+8= )

;type	62
csync01			CSYNC	0 0 A NS AAAA
csync02			CSYNC	0 0

; type 63 -- 98 (unassigned)

; type 99
spf01			SPF	"v=spf1 -all"
spf02			SPF	"v=spf1" " -all"

; type 100 (UINFO - not implemented by BIND)
; type 101 (UID - not implemented by BIND)
; type 102 (GID - not implemented by BIND)

; type 103 (UNSPEC - XXXMUKS TODO - this has some weird encoding - see btoa_totext())

; type 104
nid			NID	10 0014:4fff:ff20:ee64

; type 105
l32			L32	10 1.2.3.4

; type 106
l64			L64	10 0014:4fff:ff20:ee64

; type 107
lp			LP	10 example.net.

; type 108
eui48			EUI48	01-23-45-67-89-ab

; type 109
eui64			EUI64	01-23-45-67-89-ab-cd-ef

; type 110 -- 248 (unassigned)

; type 249
; TKEY is a meta-type and should never occur in master files.
; The text representation is not specified in the draft.
; This example was written based on the bind9 RR parsing code.
;tkey01			TKEY	928321914 928321915 (
;				255		; algorithm
;				65535 		; mode
;				0		; error
;				3 		; key size
;				aaaa		; key data
;				3 		; other size
;				bbbb		; other data
;				)
;; A TKEY with empty "other data"
;tkey02			TKEY	928321914 928321915 (
;				255		; algorithm
;				65535 		; mode
;				0		; error
;				3 		; key size
;				aaaa		; key data
;				0 		; other size
;						; other data
;				)

; type 255
; * is a meta-type and should never occur in master files.

; type 256
uri01			URI	10 20 "https://www.isc.org/"
uri02			URI	30 40 "https://www.isc.org/HolyCowThisSureIsAVeryLongURIRecordIDontEvenKnowWhatSomeoneWouldEverWantWithSuchAThingButTheSpecificationRequiresThatWesupportItSoHereWeGoTestingItLaLaLaLaLaLaLaSeriouslyThoughWhyWouldYouEvenConsiderUsingAURIThisLongItSeemsLikeASillyIdeaButEnhWhatAreYouGonnaDo/"
uri03			URI	30 40 ""

; type 257
caa01			CAA	0 issue "ca.example.net; policy=ev"
caa02			CAA	128 tbs "Unknown"
caa03			CAA	128 tbs ""

; type 258
avc			AVC	foo:bar

; type 259
doa01			DOA	( 1234567890 1234567890 1 "image/gif"
				  R0lGODlhKAAZAOMCAGZmZgBmmf///zOZzMz//5nM/zNmmWbM/5nMzMzMzACZ////
				  /////////////////yH5BAEKAA8ALAAAAAAoABkAAATH8IFJK5U2a4337F5ogRkp
				  noCJrly7PrCKyh8c3HgAhzT35MDbbtO7/IJIHbGiOiaTxVTpSVWWLqNq1UVyapNS
				  1wd3OAxug0LhnCubcVhsxysQnOt4ATpvvzHlFzl1AwODhWeFAgRpen5/UhheAYMF
				  dUB4SFcpGEGGdQeCAqBBLTuSk30EeXd9pEsAbKGxjHqDSE0Sp6ixN4N1BJmbc7lI
				  hmsBich1awPAjkY1SZR8bJWrz382SGqIBQQFQd4IsUTaX+ceuudPEQA7 )
doa02			DOA	0 1 2 "" aHR0cHM6Ly93d3cuaXNjLm9yZy8=

; type 260 -- 32767 (unassigned)

; type 32768
ta			TA	30795 1 1 (
					310D27F4D82C1FC2400704EA9939FE6E1CEA
					A3B9 )

; type 32769
dlv			DLV	30795 1 1 (
					310D27F4D82C1FC2400704EA9939FE6E1CEA
					A3B9 )

; type 32770 -- 65279 (unassigned)

; type 65280-65534 (private use)

; keydata (internal type used for managed-keys)
keydata			TYPE65533	\# 0
keydata			TYPE65533	\# 6 010203040506 
keydata			TYPE65533	\# 18 010203040506010203040506010203040506

; type 65535 (reserved)

EOF
