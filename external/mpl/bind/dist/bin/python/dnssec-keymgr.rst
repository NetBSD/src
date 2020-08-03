.. 
   Copyright (C) Internet Systems Consortium, Inc. ("ISC")
   
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/.
   
   See the COPYRIGHT file distributed with this work for additional
   information regarding copyright ownership.

.. highlight: console

.. _man_dnssec-keymgr:

dnssec-keymgr - Ensures correct DNSKEY coverage based on a defined policy
-------------------------------------------------------------------------

Synopsis
~~~~~~~~

:program:``dnssec-keymgr`` [**-K**\ *directory*] [**-c**\ *file*] [**-f**]
[**-k**] [**-q**] [**-v**] [**-z**] [**-g**\ *path*] [**-s**\ *path*]
[zone...]

Description
~~~~~~~~~~~

``dnssec-keymgr`` is a high level Python wrapper to facilitate the key
rollover process for zones handled by BIND. It uses the BIND commands
for manipulating DNSSEC key metadata: ``dnssec-keygen`` and
``dnssec-settime``.

DNSSEC policy can be read from a configuration file (default
/etc/dnssec-policy.conf), from which the key parameters, publication and
rollover schedule, and desired coverage duration for any given zone can
be determined. This file may be used to define individual DNSSEC
policies on a per-zone basis, or to set a "default" policy used for all
zones.

When ``dnssec-keymgr`` runs, it examines the DNSSEC keys for one or more
zones, comparing their timing metadata against the policies for those
zones. If key settings do not conform to the DNSSEC policy (for example,
because the policy has been changed), they are automatically corrected.

A zone policy can specify a duration for which we want to ensure the key
correctness (``coverage``). It can also specify a rollover period
(``roll-period``). If policy indicates that a key should roll over
before the coverage period ends, then a successor key will automatically
be created and added to the end of the key series.

If zones are specified on the command line, ``dnssec-keymgr`` will
examine only those zones. If a specified zone does not already have keys
in place, then keys will be generated for it according to policy.

If zones are *not* specified on the command line, then ``dnssec-keymgr``
will search the key directory (either the current working directory or
the directory set by the ``-K`` option), and check the keys for all the
zones represented in the directory.

Key times that are in the past will not be updated unless the ``-f`` is
used (see below). Key inactivation and deletion times that are less than
five minutes in the future will be delayed by five minutes.

It is expected that this tool will be run automatically and unattended
(for example, by ``cron``).

Options
~~~~~~~

**-c** *file*

   If ``-c`` is specified, then the DNSSEC policy is read from ``file``.
   (If not specified, then the policy is read from
   /etc/dnssec-policy.conf; if that file doesnt exist, a built-in global
   default policy is used.)

**-f**

   Force: allow updating of key events even if they are already in the
   past. This is not recommended for use with zones in which keys have
   already been published. However, if a set of keys has been generated
   all of which have publication and activation dates in the past, but
   the keys have not been published in a zone as yet, then this option
   can be used to clean them up and turn them into a proper series of
   keys with appropriate rollover intervals.

**-g** *keygen-path*

   Specifies a path to a ``dnssec-keygen`` binary. Used for testing. See
   also the ``-s`` option.

**-h**

   Print the ``dnssec-keymgr`` help summary and exit.

**-K** *directory*

   Sets the directory in which keys can be found. Defaults to the
   current working directory.

**-k**

   Only apply policies to KSK keys. See also the ``-z`` option.

**-q**

   Quiet: suppress printing of ``dnssec-keygen`` and ``dnssec-settime``.

**-s** *settime-path*

   Specifies a path to a ``dnssec-settime`` binary. Used for testing.
   See also the ``-g`` option.

**-v**

   Print the ``dnssec-keymgr`` version and exit.

**-z**

   Only apply policies to ZSK keys. See also the ``-k`` option.

Policy Configuration
~~~~~~~~~~~~~~~~~~~~

The dnssec-policy.conf file can specify three kinds of policies:

   · *Policy classes* (``policy``\ *name*\ ``{ ... };``) can be
   inherited by zone policies or other policy classes; these can be used
   to create sets of different security profiles. For example, a policy
   class ``normal`` might specify 1024-bit key sizes, but a class
   ``extra`` might specify 2048 bits instead; ``extra`` would be used
   for zones that had unusually high security needs.

..

   · *Algorithm policies:* (``algorithm-policy``\ *algorithm*\ ``{ ...
   };`` ) override default per-algorithm settings. For example, by
   default, RSASHA256 keys use 2048-bit key sizes for both KSK and ZSK.
   This can be modified using ``algorithm-policy``, and the new key
   sizes would then be used for any key of type RSASHA256.

   · *Zone policies:* (``zone``\ *name*\ ``{ ... };`` ) set policy for a
   single zone by name. A zone policy can inherit a policy class by
   including a ``policy`` option. Zone names beginning with digits
   (i.e., 0-9) must be quoted. If a zone does not have its own policy
   then the "default" policy applies.

Options that can be specified in policies:

``algorithm`` *name*;

   The key algorithm. If no policy is defined, the default is RSASHA256.

``coverage`` *duration*;

   The length of time to ensure that keys will be correct; no action
   will be taken to create new keys to be activated after this time.
   This can be represented as a number of seconds, or as a duration
   using human-readable units (examples: "1y" or "6 months"). A default
   value for this option can be set in algorithm policies as well as in
   policy classes or zone policies. If no policy is configured, the
   default is six months.

``directory`` *path*;

   Specifies the directory in which keys should be stored.

``key-size`` *keytype* *size*;

   Specifies the number of bits to use in creating keys. The keytype is
   either "zsk" or "ksk". A default value for this option can be set in
   algorithm policies as well as in policy classes or zone policies. If
   no policy is configured, the default is 2048 bits for RSA keys.

``keyttl`` *duration*;

   The key TTL. If no policy is defined, the default is one hour.

``post-publish`` *keytype* *duration*;

   How long after inactivation a key should be deleted from the zone.
   Note: If ``roll-period`` is not set, this value is ignored. The
   keytype is either "zsk" or "ksk". A default duration for this option
   can be set in algorithm policies as well as in policy classes or zone
   policies. The default is one month.

``pre-publish`` *keytype* *duration*;

   How long before activation a key should be published. Note: If
   ``roll-period`` is not set, this value is ignored. The keytype is
   either "zsk" or "ksk". A default duration for this option can be set
   in algorithm policies as well as in policy classes or zone policies.
   The default is one month.

``roll-period`` *keytype* *duration*;

   How frequently keys should be rolled over. The keytype is either
   "zsk" or "ksk". A default duration for this option can be set in
   algorithm policies as well as in policy classes or zone policies. If
   no policy is configured, the default is one year for ZSKs. KSKs do
   not roll over by default.

``standby`` *keytype* *number*;

   Not yet implemented.

Remaining Work
~~~~~~~~~~~~~~

   · Enable scheduling of KSK rollovers using the ``-P sync`` and ``-D
   sync`` options to ``dnssec-keygen`` and ``dnssec-settime``. Check the
   parent zone (as in ``dnssec-checkds``) to determine when its safe for
   the key to roll.

..

   · Allow configuration of standby keys and use of the REVOKE bit, for
   keys that use RFC 5011 semantics.

See Also
~~~~~~~~

``dnssec-coverage``\ (8), ``dnssec-keygen``\ (8),
``dnssec-settime``\ (8), ``dnssec-checkds``\ (8)
