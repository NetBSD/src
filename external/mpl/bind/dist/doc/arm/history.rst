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

.. History:

A Brief History of the DNS and BIND
===================================

Although the Domain Name System "officially" began in
1984 with the publication of :rfc:`920`, the core of the new system was
described in 1983 in :rfc:`882` and :rfc:`883`. From 1984 to 1987, the ARPAnet
(the precursor to today's Internet) became a testbed of experimentation
for developing the new naming/addressing scheme in a rapidly expanding,
operational network environment. New RFCs were written and published in
1987 that modified the original documents to incorporate improvements
based on the working model. :rfc:`1034`, "Domain Names-Concepts and
Facilities," and :rfc:`1035`, "Domain Names-Implementation and
Specification," were published and became the standards upon which all
DNS implementations are built.

The first working domain name server, called "Jeeves," was written in
1983-84 by Paul Mockapetris for operation on DEC Tops-20 machines
located at the University of Southern California's Information Sciences
Institute (USC-ISI) and SRI International's Network Information Center
(SRI-NIC). A DNS server for Unix machines, the Berkeley Internet Name
Domain (BIND) package, was written soon after by a group of graduate
students at the University of California at Berkeley under a grant from
the US Defense Advanced Research Projects Administration (DARPA).

Versions of BIND through 4.8.3 were maintained by the Computer Systems
Research Group (CSRG) at UC Berkeley. Douglas Terry, Mark Painter, David
Riggle, and Songnian Zhou made up the initial BIND project team. After
that, additional work on the software package was done by Ralph
Campbell. Kevin Dunlap, a Digital Equipment Corporation employee on loan
to the CSRG, worked on BIND for 2 years, from 1985 to 1987. Many other
people also contributed to BIND development during that time: Doug
Kingston, Craig Partridge, Smoot Carl-Mitchell, Mike Muuss, Jim Bloom,
and Mike Schwartz. BIND maintenance was subsequently handled by Mike
Karels and Øivind Kure.

BIND versions 4.9 and 4.9.1 were released by Digital Equipment
Corporation (now Compaq Computer Corporation). Paul Vixie, then a DEC
employee, became BIND's primary caretaker. He was assisted by Phil
Almquist, Robert Elz, Alan Barrett, Paul Albitz, Bryan Beecher, Andrew
Partan, Andy Cherenson, Tom Limoncelli, Berthold Paffrath, Fuat Baran,
Anant Kumar, Art Harkin, Win Treese, Don Lewis, Christophe Wolfhugel,
and others.

In 1994, BIND version 4.9.2 was sponsored by Vixie Enterprises. Paul
Vixie became BIND's principal architect/programmer.

BIND versions from 4.9.3 onward have been developed and maintained by
the Internet Systems Consortium and its predecessor, the Internet
Software Consortium, with support provided by ISC's sponsors.

As co-architects/programmers, Bob Halley and Paul Vixie released the
first production-ready version of BIND version 8 in May 1997.

BIND version 9 was released in September 2000 and is a major rewrite of
nearly all aspects of the underlying BIND architecture.

BIND versions 4 and 8 are officially deprecated. No additional
development is done on BIND version 4 or BIND version 8.

BIND development work is made possible today by the sponsorship of
corporations who purchase professional support services from ISC (https://www.isc.org/contact/) and/or donate to our mission, and by the tireless efforts of numerous
individuals.
