.. 
   Copyright (C) Internet Systems Consortium, Inc. ("ISC")
   
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/.
   
   See the COPYRIGHT file distributed with this work for additional
   information regarding copyright ownership.

.. highlight: console

.. _man_dnssec-coverage:

dnssec-coverage - checks future DNSKEY coverage for a zone
----------------------------------------------------------

Synopsis
~~~~~~~~

``dnssec-coverage`` [**-K**\ *directory*] [**-l**\ *length*]
[**-f**\ *file*] [**-d**\ *DNSKEY TTL*] [**-m**\ *max TTL*]
[**-r**\ *interval*] [**-c**\ *compilezone path*] [**-k**] [**-z**]
[zone...]

Description
~~~~~~~~~~~

``dnssec-coverage`` verifies that the DNSSEC keys for a given zone or a
set of zones have timing metadata set properly to ensure no future
lapses in DNSSEC coverage.

If ``zone`` is specified, then keys found in the key repository matching
that zone are scanned, and an ordered list is generated of the events
scheduled for that key (i.e., publication, activation, inactivation,
deletion). The list of events is walked in order of occurrence. Warnings
are generated if any event is scheduled which could cause the zone to
enter a state in which validation failures might occur: for example, if
the number of published or active keys for a given algorithm drops to
zero, or if a key is deleted from the zone too soon after a new key is
rolled, and cached data signed by the prior key has not had time to
expire from resolver caches.

If ``zone`` is not specified, then all keys in the key repository will
be scanned, and all zones for which there are keys will be analyzed.
(Note: This method of reporting is only accurate if all the zones that
have keys in a given repository share the same TTL parameters.)

Options
~~~~~~~

**-K** *directory*

   Sets the directory in which keys can be found. Defaults to the
   current working directory.

**-f** *file*

   If a ``file`` is specified, then the zone is read from that file; the
   largest TTL and the DNSKEY TTL are determined directly from the zone
   data, and the ``-m`` and ``-d`` options do not need to be specified
   on the command line.

**-l** *duration*

   The length of time to check for DNSSEC coverage. Key events scheduled
   further into the future than ``duration`` will be ignored, and
   assumed to be correct.

   The value of ``duration`` can be set in seconds, or in larger units
   of time by adding a suffix: mi for minutes, h for hours, d for days,
   w for weeks, mo for months, y for years.

**-m** *maximum TTL*

   Sets the value to be used as the maximum TTL for the zone or zones
   being analyzed when determining whether there is a possibility of
   validation failure. When a zone-signing key is deactivated, there
   must be enough time for the record in the zone with the longest TTL
   to have expired from resolver caches before that key can be purged
   from the DNSKEY RRset. If that condition does not apply, a warning
   will be generated.

   The length of the TTL can be set in seconds, or in larger units of
   time by adding a suffix: mi for minutes, h for hours, d for days, w
   for weeks, mo for months, y for years.

   This option is not necessary if the ``-f`` has been used to specify a
   zone file. If ``-f`` has been specified, this option may still be
   used; it will override the value found in the file.

   If this option is not used and the maximum TTL cannot be retrieved
   from a zone file, a warning is generated and a default value of 1
   week is used.

**-d** *DNSKEY TTL*

   Sets the value to be used as the DNSKEY TTL for the zone or zones
   being analyzed when determining whether there is a possibility of
   validation failure. When a key is rolled (that is, replaced with a
   new key), there must be enough time for the old DNSKEY RRset to have
   expired from resolver caches before the new key is activated and
   begins generating signatures. If that condition does not apply, a
   warning will be generated.

   The length of the TTL can be set in seconds, or in larger units of
   time by adding a suffix: mi for minutes, h for hours, d for days, w
   for weeks, mo for months, y for years.

   This option is not necessary if ``-f`` has been used to specify a
   zone file from which the TTL of the DNSKEY RRset can be read, or if a
   default key TTL was set using ith the ``-L`` to ``dnssec-keygen``. If
   either of those is true, this option may still be used; it will
   override the values found in the zone file or the key file.

   If this option is not used and the key TTL cannot be retrieved from
   the zone file or the key file, then a warning is generated and a
   default value of 1 day is used.

**-r** *resign interval*

   Sets the value to be used as the resign interval for the zone or
   zones being analyzed when determining whether there is a possibility
   of validation failure. This value defaults to 22.5 days, which is
   also the default in ``named``. However, if it has been changed by the
   ``sig-validity-interval`` option in named.conf, then it should also
   be changed here.

   The length of the interval can be set in seconds, or in larger units
   of time by adding a suffix: mi for minutes, h for hours, d for days,
   w for weeks, mo for months, y for years.

**-k**

   Only check KSK coverage; ignore ZSK events. Cannot be used with
   ``-z``.

**-z**

   Only check ZSK coverage; ignore KSK events. Cannot be used with
   ``-k``.

**-c** *compilezone path*

   Specifies a path to a ``named-compilezone`` binary. Used for testing.

See Also
~~~~~~~~

``dnssec-checkds``\ (8), ``dnssec-dsfromkey``\ (8),
``dnssec-keygen``\ (8), ``dnssec-signzone``\ (8)
