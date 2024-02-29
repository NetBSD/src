.. Copyright (C) Internet Systems Consortium, Inc. ("ISC")
..
.. SPDX-License-Identifier: MPL-2.0
..
.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0.  If a copy of the MPL was not distributed with this
.. file, you can obtain one at https://mozilla.org/MPL/2.0/.
..
.. See the COPYRIGHT file distributed with this work for additional
.. information regarding copyright ownership.

.. General:

General DNS Reference Information
=================================

.. _rfcs:

Requests for Comment (RFCs)
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Specification documents for the Internet protocol suite, including the
DNS, are published as part of the `Request for Comments`_ (RFCs) series
of technical notes. The standards themselves are defined by the
`Internet Engineering Task Force`_ (IETF) and the `Internet Engineering
Steering Group`_ (IESG). RFCs can be viewed online at:
https://www.rfc-editor.org/.

While reading RFCs, please keep in mind that :rfc:`not all RFCs are
standards <1796>`, and also that the validity of documents does change
over time. Every RFC needs to be interpreted in the context of other
documents.

BIND 9 strives for strict compliance with IETF standards. To the best
of our knowledge, BIND 9 complies with the following RFCs, with
the caveats and exceptions listed in the numbered notes below. Many
of these RFCs were written by current or former ISC staff members.
The list is non-exhaustive.

.. _Internet Engineering Steering Group: https://www.ietf.org/about/groups/iesg/
.. _Internet Engineering Task Force: https://www.ietf.org/about/
.. _Request for Comments: https://www.ietf.org/standards/rfcs/

Some of these RFCs, though DNS-related, are not concerned with implementing
software.

Protocol Specifications
-----------------------

:rfc:`1034` - P. Mockapetris. *Domain Names — Concepts and Facilities.* November
1987.

:rfc:`1035` - P. Mockapetris. *Domain Names — Implementation and Specification.*
November 1987. [#rfc1035_1]_ [#rfc1035_2]_

:rfc:`1183` - C. F. Everhart, L. A. Mamakos, R. Ullmann, P. Mockapetris. *New DNS RR
Definitions.* October 1990.

:rfc:`1706` - B. Manning and R. Colella. *DNS NSAP Resource Records.* October 1994.

:rfc:`1712` - C. Farrell, M. Schulze, S. Pleitner, and D. Baldoni. *DNS Encoding of
Geographical Location.* November 1994.

:rfc:`1876` - C. Davis, P. Vixie, T. Goodwin, and I. Dickinson. *A Means for Expressing
Location Information in the Domain Name System.* January 1996.

:rfc:`1982` - R. Elz and R. Bush. *Serial Number Arithmetic.* August 1996.

:rfc:`1995` - M. Ohta. *Incremental Zone Transfer in DNS.* August 1996.

:rfc:`1996` - P. Vixie. *A Mechanism for Prompt Notification of Zone Changes (DNS NOTIFY).*
August 1996.

:rfc:`2136` - P. Vixie, S. Thomson, Y. Rekhter, and J. Bound. *Dynamic Updates in the
Domain Name System (DNS UPDATE).* April 1997.

:rfc:`2163` - A. Allocchio. *Using the Internet DNS to Distribute MIXER
Conformant Global Address Mapping (MCGAM).* January 1998.

:rfc:`2181` - R. Elz and R. Bush. *Clarifications to the DNS Specification.* July 1997.

:rfc:`2230` - R. Atkinson. *Key Exchange Delegation Record for the DNS.* November
1997.

:rfc:`2308` - M. Andrews. *Negative Caching of DNS Queries (DNS NCACHE).* March 1998.

:rfc:`2539` - D. Eastlake, 3rd. *Storage of Diffie-Hellman Keys in the Domain Name
System (DNS).* March 1999.

:rfc:`2782` - A. Gulbrandsen, P. Vixie, and L. Esibov. *A DNS RR for Specifying the
Location of Services (DNS SRV).* February 2000.

:rfc:`2930` - D. Eastlake, 3rd. *Secret Key Establishment for DNS (TKEY RR).*
September 2000.

:rfc:`2931` - D. Eastlake, 3rd. *DNS Request and Transaction Signatures (SIG(0)s).*
September 2000. [#rfc2931]_

:rfc:`3007` - B. Wellington. *Secure Domain Name System (DNS) Dynamic Update.*
November 2000.

:rfc:`3110` - D. Eastlake, 3rd. *RSA/SHA-1 SIGs and RSA KEYs in the Domain Name
System (DNS).* May 2001.

:rfc:`3123` - P. Koch. *A DNS RR Type for Lists of Address Prefixes (APL RR).* June
2001.

:rfc:`3225` - D. Conrad. *Indicating Resolver Support of DNSSEC.* December 2001.

:rfc:`3226` - O. Gudmundsson. *DNSSEC and IPv6 A6 Aware Server/Resolver
Message Size Requirements.* December 2001.

:rfc:`3363` - R. Bush, A. Durand, B. Fink, O. Gudmundsson, and T. Hain.
*Representing Internet Protocol Version 6 (IPv6) Addresses in the Domain Name
System (DNS).* August 2002. [#rfc3363]_

:rfc:`3403` - M. Mealling.
*Dynamic Delegation Discovery System (DDDS). Part Three: The Domain Name System
(DNS) Database.*
October 2002.

:rfc:`3492` - A. Costello. *Punycode: A Bootstring Encoding of Unicode for
Internationalized Domain Names in Applications (IDNA).* March 2003.

:rfc:`3493` - R. Gilligan, S. Thomson, J. Bound, J. McCann, and W. Stevens.
*Basic Socket Interface Extensions for IPv6.* March 2003.

:rfc:`3496` - A. G. Malis and T. Hsiao. *Protocol Extension for Support of
Asynchronous Transfer Mode (ATM) Service Class-aware Multiprotocol Label
Switching (MPLS) Traffic Engineering.* March 2003.

:rfc:`3596` - S. Thomson, C. Huitema, V. Ksinant, and M. Souissi. *DNS Extensions to
Support IP Version 6.* October 2003.

:rfc:`3597` - A. Gustafsson. *Handling of Unknown DNS Resource Record (RR) Types.*
September 2003.

:rfc:`3645` - S. Kwan, P. Garg, J. Gilroy, L. Esibov, J. Westhead, and R. Hall. *Generic
Security Service Algorithm for Secret Key Transaction Authentication for
DNS (GSS-TSIG).* October 2003.

:rfc:`4025` - M. Richardson. *A Method for Storing IPsec Keying Material in
DNS.* March 2005.

:rfc:`4033` - R. Arends, R. Austein, M. Larson, D. Massey, and S. Rose. *DNS Security
Introduction and Requirements.* March 2005.

:rfc:`4034` - R. Arends, R. Austein, M. Larson, D. Massey, and S. Rose. *Resource Records for
the DNS Security Extensions.* March 2005.

:rfc:`4035` - R. Arends, R. Austein, M. Larson, D. Massey, and S. Rose. *Protocol
Modifications for the DNS Security Extensions.* March 2005.

:rfc:`4255` - J. Schlyter and W. Griffin. *Using DNS to Securely Publish Secure
Shell (SSH) Key Fingerprints.* January 2006.

:rfc:`4343` - D. Eastlake, 3rd. *Domain Name System (DNS) Case Insensitivity
Clarification.* January 2006.

:rfc:`4398` - S. Josefsson. *Storing Certificates in the Domain Name System (DNS).* March 2006.

:rfc:`4470` - S. Weiler and J. Ihren. *Minimally covering NSEC Records and
DNSSEC On-line Signing.* April 2006. [#rfc4470]_

:rfc:`4509` - W. Hardaker. *Use of SHA-256 in DNSSEC Delegation Signer
(DS) Resource Records (RRs).* May 2006.

:rfc:`4592` - E. Lewis. *The Role of Wildcards in the Domain Name System.* July 2006.

:rfc:`4635` - D. Eastlake, 3rd. *HMAC SHA (Hashed Message Authentication
Code, Secure Hash Algorithm) TSIG Algorithm Identifiers.* August 2006.

:rfc:`4701` - M. Stapp, T. Lemon, and A. Gustafsson. *A DNS Resource Record
(RR) for Encoding Dynamic Host Configuration Protocol (DHCP) Information (DHCID
RR).* October 2006.

:rfc:`4955` - D. Blacka. *DNS Security (DNSSEC) Experiments.* July 2007. [#rfc4955]_

:rfc:`5001` - R. Austein. *DNS Name Server Identifier (NSID) Option.* August 2007.

:rfc:`5011` - M. StJohns. *Automated Updates of DNS Security (DNSSEC) Trust Anchors.*

:rfc:`5155` - B. Laurie, G. Sisson, R. Arends, and D. Blacka. *DNS Security
(DNSSEC) Hashed Authenticated Denial of Existence.* March 2008.

:rfc:`5205` - P. Nikander and J. Laganier. *Host Identity Protocol (HIP)
Domain Name System (DNS) Extension.* April 2008.

:rfc:`5452` - A. Hubert and R. van Mook. *Measures for Making DNS More
Resilient Against Forged Answers.* January 2009. [#rfc5452]_

:rfc:`5702` - J. Jansen. *Use of SHA-2 Algorithms with RSA in DNSKEY and
RRSIG Resource Records for DNSSEC.* October 2009.

:rfc:`5891` - J. Klensin.
*Internationalized Domain Names in Applications (IDNA): Protocol.*
August 2010

:rfc:`5936` - E. Lewis and A. Hoenes, Ed. *DNS Zone Transfer Protocol (AXFR).*
June 2010.

:rfc:`5952` - S. Kawamura and M. Kawashima. *A Recommendation for IPv6 Address
Text Representation.* August 2010.

:rfc:`6052` - C. Bao, C. Huitema, M. Bagnulo, M. Boucadair, and X. Li. *IPv6
Addressing of IPv4/IPv6 Translators.* October 2010.

:rfc:`6147` - M. Bagnulo, A. Sullivan, P. Matthews, and I. van Beijnum.
*DNS64: DNS Extensions for Network Address Translation from IPv6 Clients to
IPv4 Servers.* April 2011. [#rfc6147]_

:rfc:`6604` - D. Eastlake, 3rd. *xNAME RCODE and Status Bits Clarification.*
April 2012.

:rfc:`6605` - P. Hoffman and W. C. A. Wijngaards. *Elliptic Curve Digital
Signature Algorithm (DSA) for DNSSEC.* April 2012. [#rfc6605]_

:rfc:`6672` - S. Rose and W. Wijngaards. *DNAME Redirection in the DNS.*
June 2012.

:rfc:`6698` - P. Hoffman and J. Schlyter. *The DNS-Based Authentication of
Named Entities (DANE) Transport Layer Security (TLS) Protocol: TLSA.*
August 2012.

:rfc:`6725` - S. Rose. *DNS Security (DNSSEC) DNSKEY Algorithm IANA Registry
Updates.* August 2012. [#rfc6725]_

:rfc:`6742` - RJ Atkinson, SN Bhatti, U. St. Andrews, and S. Rose. *DNS
Resource Records for the Identifier-Locator Network Protocol (ILNP).*
November 2012.

:rfc:`6840` - S. Weiler, Ed., and D. Blacka, Ed. *Clarifications and
Implementation Notes for DNS Security (DNSSEC).* February 2013. [#rfc6840]_

:rfc:`6891` - J. Damas, M. Graff, and P. Vixie. *Extension Mechanisms for DNS
(EDNS(0)).* April 2013.

:rfc:`7043` - J. Abley. *Resource Records for EUI-48 and EUI-64 Addresses
in the DNS.* October 2013.

:rfc:`7050` - T. Savolainen, J. Korhonen, and D. Wing. *Discovery of the IPv6
Prefix Used for IPv6 Address Synthesis.* November 2013. [#rfc7050]_

:rfc:`7208` - S. Kitterman.
*Sender Policy Framework (SPF) for Authorizing Use of Domains in Email,
Version 1.*
April 2014.

:rfc:`7314` - M. Andrews. *Extension Mechanisms for DNS (EDNS) EXPIRE Option.*
July 2014.

:rfc:`7344` - W. Kumari, O. Gudmundsson, and G. Barwood. *Automating DNSSEC
Delegation Trust Maintenance.* September 2014. [#rfc7344]_

:rfc:`7477` - W. Hardaker. *Child-to-Parent Synchronization in DNS.* March
2015.

:rfc:`7553` - P. Faltstrom and O. Kolkman. *The Uniform Resource Identifier
(URI) DNS Resource Record.* June 2015.

:rfc:`7583` - S. Morris, J. Ihren, J. Dickinson, and W. Mekking. *DNSSEC Key
Rollover Timing Considerations.* October 2015.

:rfc:`7766` - J. Dickinson, S. Dickinson, R. Bellis, A. Mankin, and D.
Wessels. *DNS Transport over TCP - Implementation Requirements.* March 2016.

:rfc:`7828` - P. Wouters, J. Abley, S. Dickinson, and R. Bellis.
*The edns-tcp-keepalive EDNS0 Option.* April 2016.

:rfc:`7830` - A. Mayrhofer. *The EDNS(0) Padding Option.* May 2016. [#rfc7830]_

:rfc:`7858` - Z. Hu, L. Zhu, J. Heidemann, A. Mankin, D. Wessels,
and P. Hoffman. *Specification for DNS over Transport Layer Security (TLS).*
May 2016. [#noencryptedfwd]_

:rfc:`7929` - P. Wouters. *DNS-Based Authentication of Named Entities (DANE)
Bindings for OpenPGP.* August 2016.

:rfc:`8078` - O. Gudmundsson and P. Wouters. *Managing DS Records from the
Parent via CDS/CDNSKEY.* March 2017. [#rfc8078]_

:rfc:`8080` - O. Sury and R. Edmonds. *Edwards-Curve Digital Security Algorithm
(EdDSA) for DNSSEC.* February 2017.

:rfc:`8484` - P. Hoffman and P. McManus. *DNS Queries over HTTPS (DoH).*
October 2018. [#noencryptedfwd]_

:rfc:`8624` - P. Wouters and O. Sury. *Algorithm Implementation Requirements
and Usage Guidance for DNSSEC.* June 2019.

:rfc:`8659` - P. Hallam-Baker, R. Stradling, and J. Hoffman-Andrews.
*DNS Certification Authority Authorization (CAA) Resource Record.*
November 2019.

:rfc:`8880` - S. Cheshire and D. Schinazi. *Special Use Domain Name
'ipv4only.arpa'.* August 2020.

:rfc:`8945` - F. Dupont, S. Morris, P. Vixie, D. Eastlake 3rd, O. Gudmundsson,
and B. Wellington.
*Secret Key Transaction Authentication for DNS (TSIG).*
November 2020.

:rfc:`9103` - W. Toorop, S. Dickinson, S. Sahib, P. Aras, and A. Mankin.
*DNS Zone Transfer over TLS.* August 2021. [#rfc9103]_

Best Current Practice RFCs
--------------------------

:rfc:`2219` - M. Hamilton and R. Wright. *Use of DNS Aliases for Network Services.*
October 1997.

:rfc:`2317` - H. Eidnes, G. de Groot, and P. Vixie. *Classless IN-ADDR.ARPA Delegation.*
March 1998.

:rfc:`2606` - D. Eastlake, 3rd and A. Panitz. *Reserved Top Level DNS Names.* June
1999. [#rfc2606]_

:rfc:`3901` - A. Durand and J. Ihren. *DNS IPv6 Transport Operational Guidelines.*
September 2004.

:rfc:`5625` - R. Bellis. *DNS Proxy Implementation Guidelines.* August 2009.

:rfc:`6303` - M. Andrews. *Locally Served DNS Zones.* July 2011.

:rfc:`7793` - M. Andrews. *Adding 100.64.0.0/10 Prefixes to the IPv4
Locally-Served DNS Zones Registry.* May 2016.

:rfc:`8906` - M. Andrews and R. Bellis. *A Common Operational Problem in DNS
Servers: Failure to Communicate.* September 2020.

For Your Information
--------------------

:rfc:`1101` - P. Mockapetris. *DNS Encoding of Network Names and Other Types.*
April 1989.

:rfc:`1123` - R. Braden. *Requirements for Internet Hosts - Application and
Support.* October 1989.

:rfc:`1535` - E. Gavron. *A Security Problem and Proposed Correction With Widely
Deployed DNS Software.* October 1993.

:rfc:`1536` - A. Kumar, J. Postel, C. Neuman, P. Danzig, and S. Miller. *Common DNS
Implementation Errors and Suggested Fixes.* October 1993.

:rfc:`1912` - D. Barr. *Common DNS Operational and Configuration Errors.* February
1996.

:rfc:`2874` - M. Crawford and C. Huitema. *DNS Extensions to Support IPv6 Address
Aggregation and Renumbering.* July 2000. [#rfc2874]_

:rfc:`3833` - D. Atkins and R. Austein. *Threat Analysis of the Domain Name System
(DNS).* August 2004.

:rfc:`4074` - Y. Morishita and T. Jinmei. *Common Misbehavior Against DNS Queries for
IPv6 Addresses.* June 2005.

:rfc:`4431` - M. Andrews and S. Weiler. *The DNSSEC Lookaside Validation
(DLV) DNS Resource Record.* February 2006. [#rfc4431]_

:rfc:`4892` - S. Woolf and D. Conrad. *Requirements for a Mechanism
Identifying a Name Server Instance.* June 2007.

:rfc:`6781` - O. Kolkman, W. Mekking, and R. Gieben. *DNSSEC Operational
Practices, Version 2.* December 2012.

:rfc:`7129` - R. Gieben and W. Mekking. *Authenticated Denial of Existence
in the DNS.* February 2014.

:rfc:`8749` - W. Mekking and D. Mahoney. *Moving DNSSEC Lookaside Validation
(DLV) to Historic Status.* March 2020.

Notes
~~~~~

.. [#rfc1035_1] Queries to zones that have failed to load return SERVFAIL rather
   than a non-authoritative response. This is considered a feature.

.. [#rfc1035_2] CLASS ANY queries are not supported. This is considered a
   feature.

.. [#rfc2931] When receiving a query signed with a SIG(0), the server is
   only able to verify the signature if it has the key in its local
   authoritative data; it cannot do recursion or validation to
   retrieve unknown keys.

.. [#rfc2874] Compliance is with loading and serving of A6 records only.
   A6 records were moved to the experimental category by :rfc:`3363`.

.. [#rfc4431] Compliance is with loading and serving of DLV records only.
   DLV records were moved to the historic category by :rfc:`8749`.

.. [#rfc4470] Minimally Covering NSEC records are accepted but not generated.

.. [#rfc4955] BIND 9 interoperates with correctly designed experiments.

.. [#rfc5452] :iscman:`named` only uses ports to extend the ID space; addresses are not
   used.

.. [#rfc6147] Section 5.5 does not match reality. :iscman:`named` uses the presence
   of DO=1 to detect if validation may be occurring. CD has no bearing
   on whether validation occurs.

.. [#rfc6605] Compliance is conditional on the OpenSSL library being linked against
   a supporting ECDSA.

.. [#rfc6725] RSAMD5 support has been removed. See :rfc:`8624`.

.. [#rfc6840] Section 5.9 - Always set CD=1 on queries. This is *not* done, as
   it prevents DNSSEC from working correctly through another recursive server.

   When talking to a recursive server, the best algorithm is to send
   CD=0 and then send CD=1 iff SERVFAIL is returned, in case the recursive
   server has a bad clock and/or bad trust anchor. Alternatively, one
   can send CD=1 then CD=0 on validation failure, in case the recursive
   server is under attack or there is stale/bogus authoritative data.

.. [#rfc7344] Updating of parent zones is not yet implemented.

.. [#rfc7830] :iscman:`named` does not currently encrypt DNS requests, so the PAD option
   is accepted but not returned in responses.

.. [#rfc3363] Section 4 is ignored.

.. [#rfc2606] This does not apply to DNS server implementations.

.. [#rfc1521] Only the Base 64 encoding specification is supported.

.. [#idna] BIND 9 requires ``--with-libidn2`` to enable entry of IDN labels within
   dig, host, and nslookup at compile time.  ACE labels are supported
   everywhere with or without ``--with-libidn2``.

.. [#rfc4294] Section 5.1 - DNAME records are fully supported.

.. [#rfc7050] RFC 7050 is updated by RFC 8880.

.. [#noencryptedfwd] Forwarding DNS queries over encrypted transports is not
   supported yet.

.. [#rfc8078] Updating of parent zones is not yet implemented.

.. [#rfc9103] Strict TLS and Mutual TLS authentication mechanisms are
   not supported yet.

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
