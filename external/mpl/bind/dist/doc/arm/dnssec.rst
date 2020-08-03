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

.. _dnssec.dynamic.zones:

DNSSEC, Dynamic Zones, and Automatic Signing
--------------------------------------------

Converting From Insecure to Secure
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Changing a zone from insecure to secure can be done in two ways: using a
dynamic DNS update, or via the ``auto-dnssec`` zone option.

For either method, ``named`` must be configured so that it can see
the ``K*`` files which contain the public and private parts of the keys
that are used to sign the zone. These files are generated
by ``dnssec-keygen``, and they should be placed in the
key-directory, as specified in ``named.conf``:

::

       zone example.net {
           type master;
           update-policy local;
           file "dynamic/example.net/example.net";
           key-directory "dynamic/example.net";
       };

If one KSK and one ZSK DNSKEY key have been generated, this
configuration causes all records in the zone to be signed with the
ZSK, and the DNSKEY RRset to be signed with the KSK. An NSEC
chain is generated as part of the initial signing process.

Dynamic DNS Update Method
~~~~~~~~~~~~~~~~~~~~~~~~~

To insert the keys via dynamic update:

::

       % nsupdate
       > ttl 3600
       > update add example.net DNSKEY 256 3 7 AwEAAZn17pUF0KpbPA2c7Gz76Vb18v0teKT3EyAGfBfL8eQ8al35zz3Y I1m/SAQBxIqMfLtIwqWPdgthsu36azGQAX8=
       > update add example.net DNSKEY 257 3 7 AwEAAd/7odU/64o2LGsifbLtQmtO8dFDtTAZXSX2+X3e/UNlq9IHq3Y0 XtC0Iuawl/qkaKVxXe2lo8Ct+dM6UehyCqk=
       > send

While the update request completes almost immediately, the zone is
not completely signed until ``named`` has had time to walk the zone
and generate the NSEC and RRSIG records. The NSEC record at the apex
is added last, to signal that there is a complete NSEC chain.

To sign using NSEC3 instead of NSEC, add an
NSEC3PARAM record to the initial update request. The OPTOUT bit in the NSEC3
chain can be set in the flags field of the
NSEC3PARAM record.

::

       % nsupdate
       > ttl 3600
       > update add example.net DNSKEY 256 3 7 AwEAAZn17pUF0KpbPA2c7Gz76Vb18v0teKT3EyAGfBfL8eQ8al35zz3Y I1m/SAQBxIqMfLtIwqWPdgthsu36azGQAX8=
       > update add example.net DNSKEY 257 3 7 AwEAAd/7odU/64o2LGsifbLtQmtO8dFDtTAZXSX2+X3e/UNlq9IHq3Y0 XtC0Iuawl/qkaKVxXe2lo8Ct+dM6UehyCqk=
       > update add example.net NSEC3PARAM 1 1 100 1234567890
       > send

Again, this update request completes almost immediately; however,
the record does not show up until ``named`` has had a chance to
build/remove the relevant chain. A private type record is created
to record the state of the operation (see below for more details), and
is removed once the operation completes.

While the initial signing and NSEC/NSEC3 chain generation is happening,
other updates are possible as well.

Fully Automatic Zone Signing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To enable automatic signing, add the ``auto-dnssec`` option to the zone
statement in ``named.conf``. ``auto-dnssec`` has two possible arguments:
``allow`` or ``maintain``.

With ``auto-dnssec allow``, ``named`` can search the key directory for
keys matching the zone, insert them into the zone, and use them to sign
the zone. It does so only when it receives an
``rndc sign <zonename>``.

``auto-dnssec maintain`` includes the above functionality, but also
automatically adjusts the zone's DNSKEY records on a schedule according to
the keys' timing metadata. (See :ref:`man_dnssec-keygen` and
:ref:`man_dnssec-settime` for more information.)

``named`` periodically searches the key directory for keys matching
the zone; if the keys' metadata indicates that any change should be
made to the zone, such as adding, removing, or revoking a key, then that
action is carried out. By default, the key directory is checked for
changes every 60 minutes; this period can be adjusted with the
``dnssec-loadkeys-interval``, up to a maximum of 24 hours. The
``rndc loadkeys`` command forces ``named`` to check for key updates immediately.

If keys are present in the key directory the first time the zone is
loaded, the zone is signed immediately, without waiting for an
``rndc sign`` or ``rndc loadkeys`` command. Those commands can still be
used when there are unscheduled key changes.

When new keys are added to a zone, the TTL is set to match that of any
existing DNSKEY RRset. If there is no existing DNSKEY RRset, the
TTL is set to the TTL specified when the key was created (using the
``dnssec-keygen -L`` option), if any, or to the SOA TTL.

To sign the zone using NSEC3 instead of NSEC, submit an
NSEC3PARAM record via dynamic update prior to the scheduled publication
and activation of the keys. The OPTOUT bit for the NSEC3 chain can be set
in the flags field of the NSEC3PARAM record. The
NSEC3PARAM record does not appear in the zone immediately, but it is
stored for later reference. When the zone is signed and the NSEC3
chain is completed, the NSEC3PARAM record appears in the zone.

Using the ``auto-dnssec`` option requires the zone to be configured to
allow dynamic updates, by adding an ``allow-update`` or
``update-policy`` statement to the zone configuration. If this has not
been done, the configuration fails.

Private-type Records
~~~~~~~~~~~~~~~~~~~~

The state of the signing process is signaled by private-type records
(with a default type value of 65534). When signing is complete, those
records with a nonzero initial octet have a nonzero value for the final octet.

If the first octet of a private-type record is non-zero, the
record indicates either that the zone needs to be signed with the key matching
the record, or that all signatures that match the record should be
removed.

   algorithm (octet 1)

   key id in network order (octet 2 and 3)

   removal flag (octet 4)
   
   complete flag (octet 5)

Only records flagged as "complete" can be removed via dynamic update.
Attempts to remove other private type records are silently ignored.

If the first octet is zero (this is a reserved algorithm number that
should never appear in a DNSKEY record), the record indicates
changes to the NSEC3 chains are in progress. The rest of the record
contains an NSEC3PARAM record, while the flag field tells what operation to
perform based on the flag bits.

   0x01 OPTOUT

   0x80 CREATE

   0x40 REMOVE

   0x20 NONSEC

DNSKEY Rollovers
~~~~~~~~~~~~~~~~

As with insecure-to-secure conversions, DNSSEC keyrolls can be done
in two ways: using a dynamic DNS update, or via the ``auto-dnssec`` zone
option.

Dynamic DNS Update Method
~~~~~~~~~~~~~~~~~~~~~~~~~

To perform key rollovers via dynamic update, the ``K*``
files for the new keys must be added so that ``named`` can find them.
The new DNSKEY RRs can then be added via dynamic update. ``named`` then causes the
zone to be signed with the new keys; when the signing is complete, the
private-type records are updated so that the last octet is non-zero.

If this is for a KSK, the parent and any trust anchor
repositories of the new KSK must be informed.

The maximum TTL in the zone must expire before removing the
old DNSKEY. If it is a KSK that is being updated,
the DS RRset in the parent must also be updated and its TTL allowed to expire. This
ensures that all clients are able to verify at least one signature
when the old DNSKEY is removed.

The old DNSKEY can be removed via UPDATE, taking care to specify the
correct key. ``named`` cleans out any signatures generated by the
old key after the update completes.

Automatic Key Rollovers
~~~~~~~~~~~~~~~~~~~~~~~

When a new key reaches its activation date (as set by ``dnssec-keygen``
or ``dnssec-settime``), if the ``auto-dnssec`` zone option is set to
``maintain``, ``named`` automatically carries out the key rollover.
If the key's algorithm has not previously been used to sign the zone,
then the zone is fully signed as quickly as possible. However, if
the new key replaces an existing key of the same algorithm, the
zone is re-signed incrementally, with signatures from the old key
replaced with signatures from the new key as their signature
validity periods expire. By default, this rollover completes in 30 days,
after which it is safe to remove the old key from the DNSKEY RRset.

NSEC3PARAM Rollovers via UPDATE
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The new NSEC3PARAM record can be added via dynamic update. When the new NSEC3
chain has been generated, the NSEC3PARAM flag field is set to zero. At
that point, the old NSEC3PARAM record can be removed. The old chain is
removed after the update request completes.

Converting From NSEC to NSEC3
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To do this, an NSEC3PARAM record must be added. When the
conversion is complete, the NSEC chain is removed and the
NSEC3PARAM record has a zero flag field. The NSEC3 chain is
generated before the NSEC chain is destroyed.

Converting From NSEC3 to NSEC
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To do this, use ``nsupdate`` to remove all NSEC3PARAM records with a
zero flag field. The NSEC chain is generated before the NSEC3 chain
is removed.

Converting From Secure to Insecure
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To convert a signed zone to unsigned using dynamic DNS, delete all the
DNSKEY records from the zone apex using ``nsupdate``. All signatures,
NSEC or NSEC3 chains, and associated NSEC3PARAM records are removed
automatically. This takes place after the update request completes.

This requires the ``dnssec-secure-to-insecure`` option to be set to
``yes`` in ``named.conf``.

In addition, if the ``auto-dnssec maintain`` zone statement is used, it
should be removed or changed to ``allow`` instead; otherwise it will re-sign.

Periodic Re-signing
~~~~~~~~~~~~~~~~~~~

In any secure zone which supports dynamic updates, ``named``
periodically re-signs RRsets which have not been re-signed as a result of
some update action. The signature lifetimes are adjusted to
spread the re-sign load over time rather than all at once.

NSEC3 and OPTOUT
~~~~~~~~~~~~~~~~

``named`` only supports creating new NSEC3 chains where all the NSEC3
records in the zone have the same OPTOUT state. ``named`` supports
UPDATES to zones where the NSEC3 records in the chain have mixed OPTOUT
state. ``named`` does not support changing the OPTOUT state of an
individual NSEC3 record; if the
OPTOUT state of an individual NSEC3 needs to be changed, the entire chain must be changed.
