.. 
   Copyright (C) Internet Systems Consortium, Inc. ("ISC")
   
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/.
   
   See the COPYRIGHT file distributed with this work for additional
   information regarding copyright ownership.

..
   Copyright (C) Internet Systems Consortium, Inc. ("ISC")

   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/.

   See the COPYRIGHT file distributed with this work for additional
   information regarding copyright ownership.

.. General:

General DNS Reference Information
=================================

.. _ipv6addresses:

IPv6 Addresses (AAAA)
---------------------

IPv6 addresses are 128-bit identifiers, for interfaces and sets of
interfaces, which were introduced in the DNS to facilitate scalable
Internet routing. There are three types of addresses: *Unicast*, an
identifier for a single interface; *Anycast*, an identifier for a set of
interfaces; and *Multicast*, an identifier for a set of interfaces. Here
we describe the global Unicast address scheme. For more information, see
:rfc:`3587`, "IPv6 Global Unicast Address Format."

IPv6 unicast addresses consist of a *global routing prefix*, a *subnet
identifier*, and an *interface identifier*.

The global routing prefix is provided by the upstream provider or ISP,
and roughly corresponds to the IPv4 *network* section of the address
range. The subnet identifier is for local subnetting, much like
subnetting an IPv4 /16 network into /24 subnets. The interface
identifier is the address of an individual interface on a given network;
in IPv6, addresses belong to interfaces rather than to machines.

The subnetting capability of IPv6 is much more flexible than that of
IPv4: subnetting can be carried out on bit boundaries, in much the same
way as Classless InterDomain Routing (CIDR), and the DNS PTR
representation ("nibble" format) makes setting up reverse zones easier.

The interface identifier must be unique on the local link, and is
usually generated automatically by the IPv6 implementation, although it
is usually possible to override the default setting if necessary. A
typical IPv6 address might look like:
``2001:db8:201:9:a00:20ff:fe81:2b32``

IPv6 address specifications often contain long strings of zeros, so the
architects have included a shorthand for specifying them. The double
colon (``::``) indicates the longest possible string of zeros that can
fit, and can be used only once in an address.

.. _bibliography:

Bibliography (and Suggested Reading)
------------------------------------

.. _rfcs:

Request for Comments (RFCs)
~~~~~~~~~~~~~~~~~~~~~~~~~~~

BIND 9 strives for strict compliance with IETF standards. To the best
of our knowledge, BIND 9 complies with the following RFCs, with
the caveats and exceptions listed in the numbered notes below. Many
of these RFCs were written by current or former ISC staff members.
The list is non-exhaustive.

Specification documents for the Internet protocol suite, including the
DNS, are published as part of the Request for Comments (RFCs) series of
technical notes. The standards themselves are defined by the Internet
Engineering Task Force (IETF) and the Internet Engineering Steering
Group (IESG). RFCs can be viewed online at: https://datatracker.ietf.org/doc/.

Some of these RFCs, though DNS-related, are not concerned with implementing
software.

Internet Standards
------------------

:rfc:`1034` - P. Mockapetris. *Domain Names — Concepts and Facilities.* November
1987.

:rfc:`1035` - P. Mockapetris. *Domain Names — Implementation and Specification.*
November 1987. [1] [2]

:rfc:`1123` - R. Braden. *Requirements for Internet Hosts - Application and
Support.* October 1989.

:rfc:`3596` - S. Thomson, C. Huitema, V. Ksinant, and M. Souissi. *DNS Extensions to
Support IP Version 6.* October 2003.

:rfc:`5011` - M. StJohns. *Automated Updates of DNS Security (DNSSEC) Trust Anchors.*

:rfc:`6891` - J. Damas, M. Graff, and P. Vixie. *Extension Mechanisms for DNS
(EDNS(0)).* April 2013.

.. _proposed_standards:

Proposed Standards
------------------

:rfc:`1982` - R. Elz and R. Bush. *Serial Number Arithmetic.* August 1996.

:rfc:`1995` - M. Ohta. *Incremental Zone Transfer in DNS.* August 1996.

:rfc:`1996` - P. Vixie. *A Mechanism for Prompt Notification of Zone Changes (DNS NOTIFY).*
August 1996.

:rfc:`2136` - P. Vixie, S. Thomson, Y. Rekhter, and J. Bound. *Dynamic Updates in the
Domain Name System (DNS UPDATE).* April 1997.

:rfc:`2163` - A. Allocchio. *Using the Internet DNS to Distribute MIXER
Conformant Global Address Mapping (MCGAM).* January 1998.

:rfc:`2181` - R. Elz and R. Bush. *Clarifications to the DNS Specification.* July 1997.

:rfc:`2308` - M. Andrews. *Negative Caching of DNS Queries (DNS NCACHE).* March 1998.

:rfc:`2539` - D. Eastlake, 3rd. *Storage of Diffie-Hellman Keys in the Domain Name
System (DNS).* March 1999.

:rfc:`2782` - A. Gulbrandsen, P. Vixie, and L. Esibov. *A DNS RR for Specifying the
Location of Services (DNS SRV).* February 2000.

:rfc:`2845` - P. Vixie, O. Gudmundsson, D. Eastlake, 3rd, and B. Wellington. *Secret Key
Transaction Authentication for DNS (TSIG).* May 2000.

:rfc:`2930` - D. Eastlake, 3rd. *Secret Key Establishment for DNS (TKEY RR).*
September 2000.

:rfc:`2931` - D. Eastlake, 3rd. *DNS Request and Transaction Signatures (SIG(0)s).*
September 2000. [3]

:rfc:`3007` - B. Wellington. *Secure Domain Name System (DNS) Dynamic Update.*
November 2000.

:rfc:`3110` - D. Eastlake, 3rd. *RSA/SHA-1 SIGs and RSA KEYs in the Domain Name
System (DNS).* May 2001.

:rfc:`3225` - D. Conrad. *Indicating Resolver Support of DNSSEC.* December 2001.

:rfc:`3226` - O. Gudmundsson. *DNSSEC and IPv6 A6 Aware Server/Resolver
Message Size Requirements.* December 2001.

:rfc:`3492` - A. Costello. *Punycode: A Bootstring Encoding of Unicode for
Internationalized Domain Names in Applications (IDNA).* March 2003.

:rfc:`3597` - A. Gustafsson. *Handling of Unknown DNS Resource Record (RR) Types.*
September 2003.

:rfc:`3645` - S. Kwan, P. Garg, J. Gilroy, L. Esibov, J. Westhead, and R. Hall. *Generic
Security Service Algorithm for Secret Key Transaction Authentication for
DNS (GSS-TSIG).* October 2003.

:rfc:`4025` - M. Richardson. *A Method for Storing IPsec Keying Material in
DNS.* March 2005.

:rfc:`4033` - R. Arends, R. Austein, M. Larson, D. Massey, and S. Rose. *DNS Security
Introduction and Requirements.* March 2005. [4]

:rfc:`4034` - R. Arends, R. Austein, M. Larson, D. Massey, and S. Rose. *Resource Records for
the DNS Security Extensions.* March 2005.

:rfc:`4035` - R. Arends, R. Austein, M. Larson, D. Massey, and S. Rose. *Protocol
Modifications for the DNS Security Extensions.* March 2005.

:rfc:`4255` - J. Schlyter and W. Griffin. *Using DNS to Securely Publish Secure
Shell (SSH) Key Fingerprints.* January 2006.

:rfc:`4343` - D. Eastlake, 3rd. *Domain Name System (DNS) Case Insensitivity
Clarification.* January 2006.

:rfc:`4398` - S. Josefsson. *Storing Certificates in the Domain Name System (DNS).* March 2006.

:rfc:`4470` - S. Weiler and J. Ihren. *Minimally Covering NSEC Records and
DNSSEC On-line Signing.* April 2006. [5]

:rfc:`4509` - W. Hardaker. *Use of SHA-256 in DNSSEC Delegation Signer
(DS) Resource Records (RRs).* May 2006.

:rfc:`4592` - E. Lewis. *The Role of Wildcards in the Domain Name System.* July 2006.

:rfc:`4635` - D. Eastlake, 3rd. *HMAC SHA (Hashed Message Authentication
Code, Secure Hash Algorithm) TSIG Algorithm Identifiers.* August 2006.

:rfc:`4701` - M. Stapp, T. Lemon, and A. Gustafsson. *A DNS Resource Record
(RR) for Encoding Dynamic Host Configuration Protocol (DHCP) Information (DHCID
RR).* October 2006.

:rfc:`4955` - D. Blacka. *DNS Security (DNSSEC) Experiments.* July 2007. [6]

:rfc:`5001` - R. Austein. *DNS Name Server Identifier (NSID) Option.* August 2007.

:rfc:`5155` - B. Laurie, G. Sisson, R. Arends, and D. Blacka. *DNS Security
(DNSSEC) Hashed Authenticated Denial of Existence.* March 2008.

:rfc:`5452` - A. Hubert and R. van Mook. *Measures for Making DNS More
Resilient Against Forged Answers.* January 2009. [7]

:rfc:`5702` - J. Jansen. *Use of SHA-2 Algorithms with RSA in DNSKEY and
RRSIG Resource Records for DNSSEC.* October 2009.

:rfc:`5936` - E. Lewis and A. Hoenes, Ed. *DNS Zone Transfer Protocol (AXFR).*
June 2010.

:rfc:`5952` - S. Kawamura and M. Kawashima. *A Recommendation for IPv6 Address
Text Representation.* August 2010.

:rfc:`6052` - C. Bao, C. Huitema, M. Bagnulo, M. Boucadair, and X. Li. *IPv6
Addressing of IPv4/IPv6 Translators.* October 2010.

:rfc:`6147` - M. Bagnulo, A. Sullivan, P. Matthews, and I. van Beijnum.
*DNS64: DNS Extensions for Network Address Translation from IPv6 Clients to
IPv4 Servers.* April 2011. [8]

:rfc:`6594` - O. Sury. *Use of the SHA-256 Algorithm with RSA, Digital
Signature Algorithm (DSA), and Elliptic Curve DSA (ECDSA) in SSHFP Resource
Records.* April 2012.

:rfc:`6604` - D. Eastlake, 3rd. *xNAME RCODE and Status Bits Clarification.*
April 2012.

:rfc:`6605` - P. Hoffman and W. C. A. Wijngaards. *Elliptic Curve Digital
Signature Algorithm (DSA) for DNSSEC.* April 2012. [9]

:rfc:`6672` - S. Rose and W. Wijngaards. *DNAME Redirection in the DNS.*
June 2012.

:rfc:`6698` - P. Hoffman and J. Schlyter. *The DNS-Based Authentication of
Named Entities (DANE) Transport Layer Security (TLS) Protocol: TLSA.*
August 2012.

:rfc:`6725` - S. Rose. *DNS Security (DNSSEC) DNSKEY Algorithm IANA Registry
Updates.* August 2012. [10]

:rfc:`6840` - S. Weiler, Ed., and D. Blacka, Ed. *Clarifications and
Implementation Notes for DNS Security (DNSSEC).* February 2013. [11]

:rfc:`7216` - M. Thomson and R. Bellis. *Location Information Server (LIS)
Discovery Using IP Addresses and Reverse DNS.* April 2014.

:rfc:`7344` - W. Kumari, O. Gudmundsson, and G. Barwood. *Automating DNSSEC
Delegation Trust Maintenance.* September 2014. [12]

:rfc:`7477` - W. Hardaker. *Child-to-Parent Synchronization in DNS.* March
2015.

:rfc:`7766` - J. Dickinson, S. Dickinson, R. Bellis, A. Mankin, and D.
Wessels. *DNS Transport over TCP - Implementation Requirements.* March 2016.

:rfc:`7828` - P. Wouters, J. Abley, S. Dickinson, and R. Bellis.
*The edns-tcp-keepalive EDNS0 Option.* April 2016.

:rfc:`7830` - A. Mayrhofer. *The EDNS(0) Padding Option.* May 2016. [13]

:rfc:`8080` - O. Sury and R. Edmonds. *Edwards-Curve Digital Security Algorithm
(EdDSA) for DNSSEC.* February 2017.

:rfc:`8482` - J. Abley, O. Gudmundsson, M. Majkowski, and E. Hunt. *Providing
Minimal-Sized Responses to DNS Queries That Have QTYPE=ANY.* January 2019.

:rfc:`8490` - R. Bellis, S. Cheshire, J. Dickinson, S. Dickinson, T. Lemon,
and T. Pusateri. *DNS Stateful Operations.* March 2019.

:rfc:`8624` - P. Wouters and O. Sury. *Algorithm Implementation Requirements
and Usage Guidance for DNSSEC.* June 2019.

:rfc:`8749` - W. Mekking and D. Mahoney. *Moving DNSSEC Lookaside Validation
(DLV) to Historic Status.* March 2020.

Informational RFCs
------------------

:rfc:`1535` - E. Gavron. *A Security Problem and Proposed Correction With Widely
Deployed DNS Software.* October 1993.

:rfc:`1536` - A. Kumar, J. Postel, C. Neuman, P. Danzig, and S. Miller. *Common DNS
Implementation Errors and Suggested Fixes.* October 1993.

:rfc:`1591` - J. Postel. *Domain Name System Structure and Delegation.* March 1994.

:rfc:`1706` - B. Manning and R. Colella. *DNS NSAP Resource Records.* October 1994.

:rfc:`1713` - A. Romao. *Tools for DNS Debugging.* November 1994.

:rfc:`1794` - T. Brisco. *DNS Support for Load Balancing.* April 1995.

:rfc:`1912` - D. Barr. *Common DNS Operational and Configuration Errors.* February
1996.

:rfc:`2230` - R. Atkinson. *Key Exchange Delegation Record for the DNS.* November
1997.

:rfc:`2352` - O. Vaughan. *A Convention for Using Legal Names as Domain Names.* May
1998.

:rfc:`2825` - IAB and L. Daigle. *A Tangled Web: Issues of I18N, Domain Names, and
the Other Internet Protocols.* May 2000.

:rfc:`2826` - Internet Architecture Board. *IAB Technical Comment on the Unique
DNS Root.* May 2000.

:rfc:`3071` - J. Klensin. *Reflections on the DNS, RFC 1591, and Categories of
Domains.* February 2001.

:rfc:`3258` - T. Hardie. *Distributing Authoritative Name Servers via Shared
Unicast Addresses.* April 2002.

:rfc:`3363` - R. Bush, A. Durand, B. Fink, O. Gudmundsson, and T. Hain.
*Representing Internet Protocol Version 6 (IPv6) Addresses in the Domain Name
System (DNS).* August 2002. [14]

:rfc:`3493` - R. Gilligan, S. Thomson, J. Bound, J. McCann, and W. Stevens.
*Basic Socket Interface Extensions for IPv6.* March 2003.

:rfc:`3496` - A. G. Malis and T. Hsiao. *Protocol Extension for Support of
Asynchronous Transfer Mode (ATM) Service Class-aware Multiprotocol Label
Switching (MPLS) Traffic Engineering.* March 2003.

:rfc:`3833` - D. Atkins and R. Austein. *Threat Analysis of the Domain Name System
(DNS).* August 2004.

:rfc:`4074` - Y. Morishita and T. Jinmei. *Common Misbehavior Against DNS Queries for
IPv6 Addresses.* June 2005.

:rfc:`4892` - S. Woolf and D. Conrad. *Requirements for a Mechanism
Identifying a Name Server Instance.* June 2007.

:rfc:`6781` - O. Kolkman, W. Mekking, and R. Gieben. *DNSSEC Operational
Practices, Version 2.* December 2012.

:rfc:`7043` - J. Abley. *Resource Records for EUI-48 and EUI-64 Addresses
in the DNS.* October 2013.

:rfc:`7129` - R. Gieben and W. Mekking. *Authenticated Denial of Existence
in the DNS.* February 2014.

:rfc:`7553` - P. Faltstrom and O. Kolkman. *The Uniform Resource Identifier
(URI) DNS Resource Record.* June 2015.

:rfc:`7583` - S. Morris, J. Ihren, J. Dickinson, and W. Mekking. *DNSSEC Key
Rollover Timing Considerations.* October 2015.

Experimental RFCs
-----------------

:rfc:`1183` - C. F. Everhart, L. A. Mamakos, R. Ullmann, P. Mockapetris. *New DNS RR
Definitions.* October 1990.

:rfc:`1464` - R. Rosenbaum. *Using the Domain Name System to Store Arbitrary
String Attributes.* May 1993.

:rfc:`1712` - C. Farrell, M. Schulze, S. Pleitner, and D. Baldoni. *DNS Encoding of
Geographical Location.* November 1994.

:rfc:`1876` - C. Davis, P. Vixie, T. Goodwin, and I. Dickinson. *A Means for Expressing
Location Information in the Domain Name System.* January 1996.

:rfc:`2345` - J. Klensin, T. Wolf, and G. Oglesby. *Domain Names and Company Name
Retrieval.* May 1998.

:rfc:`2540` - D. Eastlake, 3rd. *Detached Domain Name System (DNS) Information.*
March 1999.

:rfc:`3123` - P. Koch. *A DNS RR Type for Lists of Address Prefixes (APL RR).* June
2001.

:rfc:`6742` - RJ Atkinson, SN Bhatti, U. St. Andrews, and S. Rose. *DNS
Resource Records for the Identifier-Locator Network Protocol (ILNP).*
November 2012.

:rfc:`7314` - M. Andrews. *Extension Mechanisms for DNS (EDNS) EXPIRE Option.*
July 2014.

:rfc:`7929` - P. Wouters. *DNS-Based Authentication of Named Entities (DANE)
Bindings for OpenPGP.* August 2016.

Best Current Practice RFCs
--------------------------

:rfc:`2219` - M. Hamilton and R. Wright. *Use of DNS Aliases for Network Services.*
October 1997.

:rfc:`2317` - H. Eidnes, G. de Groot, and P. Vixie. *Classless IN-ADDR.ARPA Delegation.*
March 1998.

:rfc:`2606` - D. Eastlake, 3rd and A. Panitz. *Reserved Top Level DNS Names.* June
1999. [15]

:rfc:`3901` - A. Durand and J. Ihren. *DNS IPv6 Transport Operational Guidelines.*
September 2004.

:rfc:`5625` - R. Bellis. *DNS Proxy Implementation Guidelines.* August 2009.

:rfc:`6303` - M. Andrews. *Locally Served DNS Zones.* July 2011.

:rfc:`7793` - M. Andrews. *Adding 100.64.0.0/10 Prefixes to the IPv4
Locally-Served DNS Zones Registry.* May 2016.

Historic RFCs
-------------

:rfc:`2874` - M. Crawford and C. Huitema. *DNS Extensions to Support IPv6 Address
Aggregation and Renumbering.* July 2000. [4]

:rfc:`4431` - M. Andrews and S. Weiler. *The DNSSEC Lookaside Validation
(DLV) DNS Resource Record.* February 2006.

RFCs of Type "Unknown"
----------------------

:rfc:`1033` - M. Lottor. *Domain Administrators Operations Guide.* November 1987.

:rfc:`1101` - P. Mockapetris. *DNS Encoding of Network Names and Other Types.*
April 1989.

Obsoleted and Unimplemented Experimental RFCs
---------------------------------------------

:rfc:`974` - C. Partridge. *Mail Routing and the Domain System.* January 1986.

:rfc:`1521` - N. Borenstein and N. Freed. *MIME (Multipurpose Internet Mail
Extensions) Part One: Mechanisms for Specifying and Describing the Format of
Internet Message Bodies.* September 1993 [16]

:rfc:`1537` - P. Beertema. *Common DNS Data File Configuration Errors.* October
1993.

:rfc:`1750` - D. Eastlake, 3rd, S. Crocker, and J. Schiller. *Randomness
Recommendations for Security.* December 1994.

:rfc:`2010` - B. Manning and P. Vixie. *Operational Criteria for Root Name Servers.*
October 1996.

:rfc:`2052` - A. Gulbrandsen and P. Vixie. *A DNS RR for Specifying the Location of
Services.* October 1996.

:rfc:`2065` - D. Eastlake, 3rd and C. Kaufman. *Domain Name System Security Extensions.*
January 1997.

:rfc:`2137` - D. Eastlake, 3rd. *Secure Domain Name System Dynamic Update.* April
1997.

:rfc:`2168` - R. Daniel and M. Mealling. *Resolution of Uniform Resource Identifiers
Using the Domain Name System.* June 1997.

:rfc:`2240` - O. Vaughan. *A Legal Basis for Domain Name Allocation.* November 1997.

:rfc:`2535` - D. Eastlake, 3rd. *Domain Name System Security Extensions.*
March 1999. [17] [18]

:rfc:`2537` - D. Eastlake, 3rd. *RSA/MD5 KEYs and SIGs in the Domain Name System
(DNS).* March 1999.

:rfc:`2538` - D. Eastlake, 3rd and O. Gudmundsson. *Storing Certificates in the Domain
Name System (DNS).* March 1999.

:rfc:`2671` - P. Vixie. *Extension Mechanisms for DNS (EDNS0).* August 1999.

:rfc:`2672` - M. Crawford. *Non-Terminal DNS Name Redirection.* August 1999.

:rfc:`2673` - M. Crawford. *Binary Labels in the Domain Name System.* August 1999.

:rfc:`2915` - M. Mealling and R. Daniel. *The Naming Authority Pointer (NAPTR) DNS
Resource Record.* September 2000.

:rfc:`2929` - D. Eastlake, 3rd, E. Brunner-Williams, and B. Manning. *Domain Name System
(DNS) IANA Considerations.* September 2000.

:rfc:`3008` - B. Wellington. *Domain Name System Security (DNSSEC) Signing
Authority.* November 2000.

:rfc:`3090` - E. Lewis. *DNS Security Extension Clarification on Zone Status.*
March 2001.

:rfc:`3152` - R. Bush. *Delegation of IP6.ARPA.* August 2001.

:rfc:`3445` - D. Massey and S. Rose. *Limiting the Scope of the KEY Resource Record
(RR).* December 2002.

:rfc:`3490` - P. Faltstrom, P. Hoffman, and A. Costello. *Internationalizing Domain Names
in Applications (IDNA).* March 2003. [19]

:rfc:`3491` - P. Hoffman and M. Blanchet. *Nameprep: A Stringprep Profile for
Internationalized Domain Names (IDN).* March 2003. [19]

:rfc:`3655` - B. Wellington and O. Gudmundsson. *Redefinition of DNS Authenticated
Data (AD) Bit.* November 2003.

:rfc:`3658` - O. Gudmundsson. *Delegation Signer (DS) Resource Record (RR).*
December 2003.

:rfc:`3755` - S. Weiler. *Legacy Resolver Compatibility for Delegation Signer
(DS).* May 2004.

:rfc:`3757` - O. Kolkman, J. Schlyter, and E. Lewis. *Domain Name System KEY (DNSKEY)
Resource Record (RR) Secure Entry Point (SEP) Flag.* May 2004.

:rfc:`3845` - J. Schlyter. *DNS Security (DNSSEC) NextSECure (NSEC) RDATA Format.*
August 2004.

:rfc:`4294` - J. Loughney, Ed. *IPv6 Node Requirements.* [20]

:rfc:`4408` - M. Wong and W. Schlitt. *Sender Policy Framework (SPF) for
Authorizing Use of Domains in E-Mail, Version 1.* April 2006.

:rfc:`5966` - R. Bellis. *DNS Transport Over TCP - Implementation
Requirements.* August 2010.

:rfc:`6844` - P. Hallam-Baker and R. Stradling. *DNS Certification Authority
Authorization (CAA) Resource Record.* January 2013.

:rfc:`6944` - S. Rose. *Applicability Statement: DNS Security (DNSSEC) DNSKEY
Algorithm Implementation Status.* April 2013.

RFCs No Longer Supported in BIND 9
----------------------------------

:rfc:`2536` - D. Eastlake, 3rd. *DSA KEYs and SIGs in the Domain Name System
(DNS).* March 1999.

Notes
~~~~~

[1] Queries to zones that have failed to load return SERVFAIL rather
than a non-authoritative response. This is considered a feature.

[2] CLASS ANY queries are not supported. This is considered a
feature.

[3] When receiving a query signed with a SIG(0), the server is
only able to verify the signature if it has the key in its local
authoritative data; it cannot do recursion or validation to
retrieve unknown keys.

[4] Compliance is with loading and serving of A6 records only. A6 records were moved
to the experimental category by :rfc:`3363`.

[5] Minimally covering NSEC records are accepted but not generated.

[6] BIND 9 interoperates with correctly designed experiments.

[7] ``named`` only uses ports to extend the ID space; addresses are not
used.

[8] Section 5.5 does not match reality. ``named`` uses the presence
of DO=1 to detect if validation may be occurring. CD has no bearing
on whether validation occurs.

[9] Compliance is conditional on the OpenSSL library being linked against
a supporting ECDSA.

[10] RSAMD5 support has been removed. See :rfc:`6944`.

[11] Section 5.9 - Always set CD=1 on queries. This is *not* done, as
it prevents DNSSEC from working correctly through another recursive server.

When talking to a recursive server, the best algorithm is to send
CD=0 and then send CD=1 iff SERVFAIL is returned, in case the recursive
server has a bad clock and/or bad trust anchor. Alternatively, one
can send CD=1 then CD=0 on validation failure, in case the recursive
server is under attack or there is stale/bogus authoritative data.

[12] Updating of parent zones is not yet implemented.

[13] ``named`` does not currently encrypt DNS requests, so the PAD option
is accepted but not returned in responses.

[14] Section 4 is ignored.

[15] This does not apply to DNS server implementations.

[16] Only the Base 64 encoding specification is supported.

[17] Wildcard records are not supported in DNSSEC secure zones.

[18] Servers authoritative for secure zones being resolved by BIND
9 must support EDNS0 (RFC2671), and must return all relevant SIGs
and NXTs in responses, rather than relying on the resolving server
to perform separate queries for missing SIGs and NXTs.

[19] BIND 9 requires ``--with-idn`` to enable entry of IDN labels within dig,
host, and nslookup at compile time.  ACE labels are supported
everywhere with or without ``--with-idn``.

[20] Section 5.1 - DNAME records are fully supported.

.. _internet_drafts:

Internet Drafts
~~~~~~~~~~~~~~~

Internet Drafts (IDs) are rough-draft working documents of the Internet
Engineering Task Force (IETF). They are, in essence, RFCs in the preliminary
stages of development. Implementors are cautioned not to regard IDs as
archival, and they should not be quoted or cited in any formal documents
unless accompanied by the disclaimer that they are "works in progress."
IDs have a lifespan of six months, after which they are deleted unless
updated by their authors.

.. _more_about_bind:

Other Documents About BIND
~~~~~~~~~~~~~~~~~~~~~~~~~~

Paul Albitz and Cricket Liu. *DNS and BIND.* Copyright 1998 Sebastopol, CA: O'Reilly and
Associates.
