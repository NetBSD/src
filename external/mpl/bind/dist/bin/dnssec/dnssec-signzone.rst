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


.. highlight: console

.. _man_dnssec-signzone:

dnssec-signzone - DNSSEC zone signing tool
------------------------------------------

Synopsis
~~~~~~~~

:program:`dnssec-signzone` [**-a**] [**-c** class] [**-d** directory] [**-D**] [**-E** engine] [**-e** end-time] [**-f** output-file] [**-g**] [**-h**] [**-i** interval] [**-I** input-format] [**-j** jitter] [**-K** directory] [**-k** key] [**-L** serial] [**-M** maxttl] [**-N** soa-serial-format] [**-o** origin] [**-O** output-format] [**-P**] [**-Q**] [**-q**] [**-R**] [**-S**] [**-s** start-time] [**-T** ttl] [**-t**] [**-u**] [**-v** level] [**-V**] [**-X** extended end-time] [**-x**] [**-z**] [**-3** salt] [**-H** iterations] [**-A**] {zonefile} [key...]

Description
~~~~~~~~~~~

``dnssec-signzone`` signs a zone. It generates NSEC and RRSIG records
and produces a signed version of the zone. The security status of
delegations from the signed zone (that is, whether the child zones are
secure or not) is determined by the presence or absence of a ``keyset``
file for each child zone.

Options
~~~~~~~

**-a**
   Verify all generated signatures.

**-c** class
   Specifies the DNS class of the zone.

**-C**
   Compatibility mode: Generate a ``keyset-zonename`` file in addition
   to ``dsset-zonename`` when signing a zone, for use by older versions
   of ``dnssec-signzone``.

**-d** directory
   Look for ``dsset-`` or ``keyset-`` files in ``directory``.

**-D**
   Output only those record types automatically managed by
   ``dnssec-signzone``, i.e. RRSIG, NSEC, NSEC3 and NSEC3PARAM records.
   If smart signing (``-S``) is used, DNSKEY records are also included.
   The resulting file can be included in the original zone file with
   ``$INCLUDE``. This option cannot be combined with ``-O raw``,
   ``-O map``, or serial number updating.

**-E** engine
   When applicable, specifies the hardware to use for cryptographic
   operations, such as a secure key store used for signing.

   When BIND is built with OpenSSL PKCS#11 support, this defaults to the
   string "pkcs11", which identifies an OpenSSL engine that can drive a
   cryptographic accelerator or hardware service module. When BIND is
   built with native PKCS#11 cryptography (--enable-native-pkcs11), it
   defaults to the path of the PKCS#11 provider library specified via
   "--with-pkcs11".

**-g**
   Generate DS records for child zones from ``dsset-`` or ``keyset-``
   file. Existing DS records will be removed.

**-K** directory
   Key repository: Specify a directory to search for DNSSEC keys. If not
   specified, defaults to the current directory.

**-k** key
   Treat specified key as a key signing key ignoring any key flags. This
   option may be specified multiple times.

**-M** maxttl
   Sets the maximum TTL for the signed zone. Any TTL higher than maxttl
   in the input zone will be reduced to maxttl in the output. This
   provides certainty as to the largest possible TTL in the signed zone,
   which is useful to know when rolling keys because it is the longest
   possible time before signatures that have been retrieved by resolvers
   will expire from resolver caches. Zones that are signed with this
   option should be configured to use a matching ``max-zone-ttl`` in
   ``named.conf``. (Note: This option is incompatible with ``-D``,
   because it modifies non-DNSSEC data in the output zone.)

**-s** start-time
   Specify the date and time when the generated RRSIG records become
   valid. This can be either an absolute or relative time. An absolute
   start time is indicated by a number in YYYYMMDDHHMMSS notation;
   20000530144500 denotes 14:45:00 UTC on May 30th, 2000. A relative
   start time is indicated by +N, which is N seconds from the current
   time. If no ``start-time`` is specified, the current time minus 1
   hour (to allow for clock skew) is used.

**-e** end-time
   Specify the date and time when the generated RRSIG records expire. As
   with ``start-time``, an absolute time is indicated in YYYYMMDDHHMMSS
   notation. A time relative to the start time is indicated with +N,
   which is N seconds from the start time. A time relative to the
   current time is indicated with now+N. If no ``end-time`` is
   specified, 30 days from the start time is used as a default.
   ``end-time`` must be later than ``start-time``.

**-X** extended end-time
   Specify the date and time when the generated RRSIG records for the
   DNSKEY RRset will expire. This is to be used in cases when the DNSKEY
   signatures need to persist longer than signatures on other records;
   e.g., when the private component of the KSK is kept offline and the
   KSK signature is to be refreshed manually.

   As with ``start-time``, an absolute time is indicated in
   YYYYMMDDHHMMSS notation. A time relative to the start time is
   indicated with +N, which is N seconds from the start time. A time
   relative to the current time is indicated with now+N. If no
   ``extended end-time`` is specified, the value of ``end-time`` is used
   as the default. (``end-time``, in turn, defaults to 30 days from the
   start time.) ``extended end-time`` must be later than ``start-time``.

**-f** output-file
   The name of the output file containing the signed zone. The default
   is to append ``.signed`` to the input filename. If ``output-file`` is
   set to ``"-"``, then the signed zone is written to the standard
   output, with a default output format of "full".

**-h**
   Prints a short summary of the options and arguments to
   ``dnssec-signzone``.

**-V**
   Prints version information.

**-i** interval
   When a previously-signed zone is passed as input, records may be
   resigned. The ``interval`` option specifies the cycle interval as an
   offset from the current time (in seconds). If a RRSIG record expires
   after the cycle interval, it is retained. Otherwise, it is considered
   to be expiring soon, and it will be replaced.

   The default cycle interval is one quarter of the difference between
   the signature end and start times. So if neither ``end-time`` or
   ``start-time`` are specified, ``dnssec-signzone`` generates
   signatures that are valid for 30 days, with a cycle interval of 7.5
   days. Therefore, if any existing RRSIG records are due to expire in
   less than 7.5 days, they would be replaced.

**-I** input-format
   The format of the input zone file. Possible formats are ``"text"``
   (default), ``"raw"``, and ``"map"``. This option is primarily
   intended to be used for dynamic signed zones so that the dumped zone
   file in a non-text format containing updates can be signed directly.
   The use of this option does not make much sense for non-dynamic
   zones.

**-j** jitter
   When signing a zone with a fixed signature lifetime, all RRSIG
   records issued at the time of signing expires simultaneously. If the
   zone is incrementally signed, i.e. a previously-signed zone is passed
   as input to the signer, all expired signatures have to be regenerated
   at about the same time. The ``jitter`` option specifies a jitter
   window that will be used to randomize the signature expire time, thus
   spreading incremental signature regeneration over time.

   Signature lifetime jitter also to some extent benefits validators and
   servers by spreading out cache expiration, i.e. if large numbers of
   RRSIGs don't expire at the same time from all caches there will be
   less congestion than if all validators need to refetch at mostly the
   same time.

**-L** serial
   When writing a signed zone to "raw" or "map" format, set the "source
   serial" value in the header to the specified serial number. (This is
   expected to be used primarily for testing purposes.)

**-n** ncpus
   Specifies the number of threads to use. By default, one thread is
   started for each detected CPU.

**-N** soa-serial-format
   The SOA serial number format of the signed zone. Possible formats are
   ``"keep"`` (default), ``"increment"``, ``"unixtime"``, and
   ``"date"``.

   ``"keep"``
      Do not modify the SOA serial number.

   ``"increment"``
      Increment the SOA serial number using :rfc:`1982` arithmetic.

   ``"unixtime"``
      Set the SOA serial number to the number of seconds since epoch.

   ``"date"``
      Set the SOA serial number to today's date in YYYYMMDDNN format.

**-o** origin
   The zone origin. If not specified, the name of the zone file is
   assumed to be the origin.

**-O** output-format
   The format of the output file containing the signed zone. Possible
   formats are ``"text"`` (default), which is the standard textual
   representation of the zone; ``"full"``, which is text output in a
   format suitable for processing by external scripts; and ``"map"``,
   ``"raw"``, and ``"raw=N"``, which store the zone in binary formats
   for rapid loading by ``named``. ``"raw=N"`` specifies the format
   version of the raw zone file: if N is 0, the raw file can be read by
   any version of ``named``; if N is 1, the file can be read by release
   9.9.0 or higher; the default is 1.

**-P**
   Disable post sign verification tests.

   The post sign verification test ensures that for each algorithm in
   use there is at least one non revoked self signed KSK key, that all
   revoked KSK keys are self signed, and that all records in the zone
   are signed by the algorithm. This option skips these tests.

**-Q**
   Remove signatures from keys that are no longer active.

   Normally, when a previously-signed zone is passed as input to the
   signer, and a DNSKEY record has been removed and replaced with a new
   one, signatures from the old key that are still within their validity
   period are retained. This allows the zone to continue to validate
   with cached copies of the old DNSKEY RRset. The ``-Q`` forces
   ``dnssec-signzone`` to remove signatures from keys that are no longer
   active. This enables ZSK rollover using the procedure described in
   :rfc:`4641#4.2.1.1` ("Pre-Publish Key Rollover").

``-q``
   Quiet mode: Suppresses unnecessary output.  Without this option, when
   ``dnssec-signzone`` is run it will print to standard output the number of
   keys in use, the algorithms used to verify the zone was signed correctly and
   other status information, and finally the filename containing the signed
   zone.  With it, that output is suppressed, leaving only the filename.

**-R**
   Remove signatures from keys that are no longer published.

   This option is similar to ``-Q``, except it forces
   ``dnssec-signzone`` to signatures from keys that are no longer
   published. This enables ZSK rollover using the procedure described in
   :rfc:`4641#4.2.1.2` ("Double Signature Zone Signing Key
   Rollover").

**-S**
   Smart signing: Instructs ``dnssec-signzone`` to search the key
   repository for keys that match the zone being signed, and to include
   them in the zone if appropriate.

   When a key is found, its timing metadata is examined to determine how
   it should be used, according to the following rules. Each successive
   rule takes priority over the prior ones:

      If no timing metadata has been set for the key, the key is
      published in the zone and used to sign the zone.

      If the key's publication date is set and is in the past, the key
      is published in the zone.

      If the key's activation date is set and in the past, the key is
      published (regardless of publication date) and used to sign the
      zone.

      If the key's revocation date is set and in the past, and the key
      is published, then the key is revoked, and the revoked key is used
      to sign the zone.

      If either of the key's unpublication or deletion dates are set and
      in the past, the key is NOT published or used to sign the zone,
      regardless of any other metadata.

      If key's sync publication date is set and in the past,
      synchronization records (type CDS and/or CDNSKEY) are created.

      If key's sync deletion date is set and in the past,
      synchronization records (type CDS and/or CDNSKEY) are removed.

**-T** ttl
   Specifies a TTL to be used for new DNSKEY records imported into the
   zone from the key repository. If not specified, the default is the
   TTL value from the zone's SOA record. This option is ignored when
   signing without ``-S``, since DNSKEY records are not imported from
   the key repository in that case. It is also ignored if there are any
   pre-existing DNSKEY records at the zone apex, in which case new
   records' TTL values will be set to match them, or if any of the
   imported DNSKEY records had a default TTL value. In the event of a a
   conflict between TTL values in imported keys, the shortest one is
   used.

**-t**
   Print statistics at completion.

**-u**
   Update NSEC/NSEC3 chain when re-signing a previously signed zone.
   With this option, a zone signed with NSEC can be switched to NSEC3,
   or a zone signed with NSEC3 can be switch to NSEC or to NSEC3 with
   different parameters. Without this option, ``dnssec-signzone`` will
   retain the existing chain when re-signing.

**-v** level
   Sets the debugging level.

**-x**
   Only sign the DNSKEY, CDNSKEY, and CDS RRsets with key-signing keys,
   and omit signatures from zone-signing keys. (This is similar to the
   ``dnssec-dnskey-kskonly yes;`` zone option in ``named``.)

**-z**
   Ignore KSK flag on key when determining what to sign. This causes
   KSK-flagged keys to sign all records, not just the DNSKEY RRset.
   (This is similar to the ``update-check-ksk no;`` zone option in
   ``named``.)

**-3** salt
   Generate an NSEC3 chain with the given hex encoded salt. A dash
   (salt) can be used to indicate that no salt is to be used when
   generating the NSEC3 chain.

**-H** iterations
   When generating an NSEC3 chain, use this many iterations. The default
   is 10.

**-A**
   When generating an NSEC3 chain set the OPTOUT flag on all NSEC3
   records and do not generate NSEC3 records for insecure delegations.

   Using this option twice (i.e., ``-AA``) turns the OPTOUT flag off for
   all records. This is useful when using the ``-u`` option to modify an
   NSEC3 chain which previously had OPTOUT set.

**zonefile**
   The file containing the zone to be signed.

**key**
   Specify which keys should be used to sign the zone. If no keys are
   specified, then the zone will be examined for DNSKEY records at the
   zone apex. If these are found and there are matching private keys, in
   the current directory, then these will be used for signing.

Example
~~~~~~~

The following command signs the ``example.com`` zone with the
ECDSAP256SHA256 key generated by key generated by ``dnssec-keygen``
(Kexample.com.+013+17247). Because the ``-S`` option is not being used,
the zone's keys must be in the master file (``db.example.com``). This
invocation looks for ``dsset`` files, in the current directory, so that
DS records can be imported from them (``-g``).

::

   % dnssec-signzone -g -o example.com db.example.com \
   Kexample.com.+013+17247
   db.example.com.signed
   %

In the above example, ``dnssec-signzone`` creates the file
``db.example.com.signed``. This file should be referenced in a zone
statement in a ``named.conf`` file.

This example re-signs a previously signed zone with default parameters.
The private keys are assumed to be in the current directory.

::

   % cp db.example.com.signed db.example.com
   % dnssec-signzone -o example.com db.example.com
   db.example.com.signed
   %

See Also
~~~~~~~~

:manpage:`dnssec-keygen(8)`, BIND 9 Administrator Reference Manual, :rfc:`4033`,
:rfc:`4641`.
