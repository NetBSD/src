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

.. highlight: console

.. _man_rndc:

rndc - name server control utility
----------------------------------

Synopsis
~~~~~~~~

:program:`rndc` [**-b** source-address] [**-c** config-file] [**-k** key-file] [**-s** server] [**-p** port] [**-q**] [**-r**] [**-V**] [**-y** key_id] [[**-4**] | [**-6**]] {command}

Description
~~~~~~~~~~~

``rndc`` controls the operation of a name server; it supersedes the
``ndc`` utility. If ``rndc`` is
invoked with no command line options or arguments, it prints a short
summary of the supported commands and the available options and their
arguments.

``rndc`` communicates with the name server over a TCP connection,
sending commands authenticated with digital signatures. In the current
versions of ``rndc`` and ``named``, the only supported authentication
algorithms are HMAC-MD5 (for compatibility), HMAC-SHA1, HMAC-SHA224,
HMAC-SHA256 (default), HMAC-SHA384, and HMAC-SHA512. They use a shared
secret on each end of the connection, which provides TSIG-style
authentication for the command request and the name server's response.
All commands sent over the channel must be signed by a key_id known to
the server.

``rndc`` reads a configuration file to determine how to contact the name
server and decide what algorithm and key it should use.

Options
~~~~~~~

``-4``
   This option indicates use of IPv4 only.

``-6``
   This option indicates use of IPv6 only.

``-b source-address``
   This option indicates ``source-address`` as the source address for the connection to the
   server. Multiple instances are permitted, to allow setting of both the
   IPv4 and IPv6 source addresses.

``-c config-file``
   This option indicates ``config-file`` as the configuration file instead of the default,
   ``/etc/rndc.conf``.

``-k key-file``
   This option indicates ``key-file`` as the key file instead of the default,
   ``/etc/rndc.key``. The key in ``/etc/rndc.key`` is used to
   authenticate commands sent to the server if the config-file does not
   exist.

``-s server``
   ``server`` is the name or address of the server which matches a server
   statement in the configuration file for ``rndc``. If no server is
   supplied on the command line, the host named by the default-server
   clause in the options statement of the ``rndc`` configuration file
   is used.

``-p port``
   This option instructs BIND 9 to send commands to TCP port ``port`` instead of its default control
   channel port, 953.

``-q``
   This option sets quiet mode, where message text returned by the server is not printed
   unless there is an error.

``-r``
   This option instructs ``rndc`` to print the result code returned by ``named``
   after executing the requested command (e.g., ISC_R_SUCCESS,
   ISC_R_FAILURE, etc.).

``-V``
   This option enables verbose logging.

``-y key_id``
   This option indicates use of the key ``key_id`` from the configuration file. For control message validation to succeed, ``key_id`` must be known
   by ``named`` with the same algorithm and secret string. If no ``key_id`` is specified,
   ``rndc`` first looks for a key clause in the server statement of
   the server being used, or if no server statement is present for that
   host, then in the default-key clause of the options statement. Note that
   the configuration file contains shared secrets which are used to send
   authenticated control commands to name servers, and should therefore
   not have general read or write access.

Commands
~~~~~~~~

A list of commands supported by ``rndc`` can be seen by running ``rndc``
without arguments.

Currently supported commands are:

``addzone`` *zone* [*class* [*view*]] *configuration*
   This command adds a zone while the server is running. This command requires the
   ``allow-new-zones`` option to be set to ``yes``. The configuration
   string specified on the command line is the zone configuration text
   that would ordinarily be placed in ``named.conf``.

   The configuration is saved in a file called ``viewname.nzf`` (or, if
   ``named`` is compiled with liblmdb, an LMDB database file called
   ``viewname.nzd``). ``viewname`` is the name of the view, unless the view
   name contains characters that are incompatible with use as a file
   name, in which case a cryptographic hash of the view name is used
   instead. When ``named`` is restarted, the file is loaded into
   the view configuration so that zones that were added can persist
   after a restart.

   This sample ``addzone`` command adds the zone ``example.com`` to
   the default view:

   ``rndc addzone example.com '{ type master; file "example.com.db"; };'``

   (Note the brackets around and semi-colon after the zone configuration
   text.)

   See also ``rndc delzone`` and ``rndc modzone``.

``delzone`` [**-clean**] *zone* [*class* [*view*]]
   This command deletes a zone while the server is running.

   If the ``-clean`` argument is specified, the zone's master file (and
   journal file, if any) are deleted along with the zone. Without
   the ``-clean`` option, zone files must be deleted manually. (If the
   zone is of type ``secondary`` or ``stub``, the files needing to be removed
   are reported in the output of the ``rndc delzone`` command.)

   If the zone was originally added via ``rndc addzone``, then it is
   removed permanently. However, if it was originally configured in
   ``named.conf``, then that original configuration remains in place;
   when the server is restarted or reconfigured, the zone is
   recreated. To remove it permanently, it must also be removed from
   ``named.conf``.

   See also ``rndc addzone`` and ``rndc modzone``.

``dnssec`` ( **-status** | **-rollover** **-key** id [**-alg** *algorithm*] [**-when** *time*] | **-checkds** [**-key** *id* [**-alg** *algorithm*]] [**-when** *time*] ( *published* | *withdrawn* )) *zone* [*class* [*view*]]
   This command allows you to interact with the "dnssec-policy" of a given
   zone.

   ``rndc dnssec -status`` show the DNSSEC signing state for the specified
   zone.

   ``rndc dnssec -rollover`` allows you to schedule key rollover for a
   specific key (overriding the original key lifetime).

   ``rndc dnssec -checkds`` informs :iscman:`named` that the DS for
   a specified zone's key-signing key has been confirmed to be published
   in, or withdrawn from, the parent zone. This is required in order to
   complete a KSK rollover.  The ``-key id`` and ``-alg algorithm`` arguments
   can be used to specify a particular KSK, if necessary; if there is only
   one key acting as a KSK for the zone, these arguments can be omitted.
   The time of publication or withdrawal for the DS is set to the current
   time by default, but can be overridden to a specific time with the
   argument ``-when time``, where ``time`` is expressed in YYYYMMDDHHMMSS
   notation.

``dnstap`` ( **-reopen** | **-roll** [*number*] )
   This command closes and re-opens DNSTAP output files.

   ``rndc dnstap -reopen`` allows
   the output file to be renamed externally, so that ``named`` can
   truncate and re-open it.

   ``rndc dnstap -roll`` causes the output file
   to be rolled automatically, similar to log files. The most recent
   output file has ".0" appended to its name; the previous most recent
   output file is moved to ".1", and so on. If ``number`` is specified, then
   the number of backup log files is limited to that number.

``dumpdb`` [**-all** | **-cache** | **-zones** | **-adb** | **-bad** | **-expired** | **-fail**] [*view ...*]
   This command dumps the server's caches (default) and/or zones to the dump file for
   the specified views. If no view is specified, all views are dumped.
   (See the ``dump-file`` option in the BIND 9 Administrator Reference
   Manual.)

``flush``
   This command flushes the server's cache.

``flushname`` *name* [*view*]
   This command flushes the given name from the view's DNS cache and, if applicable,
   from the view's nameserver address database, bad server cache, and
   SERVFAIL cache.

``flushtree`` *name* [*view*]
   This command flushes the given name, and all of its subdomains, from the view's
   DNS cache, address database, bad server cache, and SERVFAIL cache.

``freeze`` [*zone* [*class* [*view*]]]
   This command suspends updates to a dynamic zone. If no zone is specified, then all
   zones are suspended. This allows manual edits to be made to a zone
   normally updated by dynamic update, and causes changes in the
   journal file to be synced into the master file. All dynamic update
   attempts are refused while the zone is frozen.

   See also ``rndc thaw``.

``halt`` [**-p**]
   This command stops the server immediately. Recent changes made through dynamic
   update or IXFR are not saved to the master files, but are rolled
   forward from the journal files when the server is restarted. If
   ``-p`` is specified, ``named``'s process ID is returned. This allows
   an external process to determine when ``named`` has completed
   halting.

   See also ``rndc stop``.

``loadkeys`` [*zone* [*class* [*view*]]]
   This command fetches all DNSSEC keys for the given zone from the key directory. If
   they are within their publication period, they are merged into the
   zone's DNSKEY RRset. Unlike ``rndc sign``, however, the zone is not
   immediately re-signed by the new keys, but is allowed to
   incrementally re-sign over time.

   This command requires that the zone be configured with a ``dnssec-policy``, or
   that the ``auto-dnssec`` zone option be set to ``maintain``, and also requires the
   zone to be configured to allow dynamic DNS. (See "Dynamic Update Policies" in
   the Administrator Reference Manual for more details.)

``managed-keys`` (*status* | *refresh* | *sync* | *destroy*) [*class* [*view*]]
   This command inspects and controls the "managed-keys" database which handles
   :rfc:`5011` DNSSEC trust anchor maintenance. If a view is specified, these
   commands are applied to that view; otherwise, they are applied to all
   views.

   -  When run with the ``status`` keyword, this prints the current status of
      the managed-keys database.

   -  When run with the ``refresh`` keyword, this forces an immediate refresh
      query to be sent for all the managed keys, updating the
      managed-keys database if any new keys are found, without waiting
      the normal refresh interval.

   -  When run with the ``sync`` keyword, this forces an immediate dump of
      the managed-keys database to disk (in the file
      ``managed-keys.bind`` or (``viewname.mkeys``). This synchronizes
      the database with its journal file, so that the database's current
      contents can be inspected visually.

   -  When run with the ``destroy`` keyword, the managed-keys database
      is shut down and deleted, and all key maintenance is terminated.
      This command should be used only with extreme caution.

      Existing keys that are already trusted are not deleted from
      memory; DNSSEC validation can continue after this command is used.
      However, key maintenance operations cease until ``named`` is
      restarted or reconfigured, and all existing key maintenance states
      are deleted.

      Running ``rndc reconfig`` or restarting ``named`` immediately
      after this command causes key maintenance to be reinitialized
      from scratch, just as if the server were being started for the
      first time. This is primarily intended for testing, but it may
      also be used, for example, to jumpstart the acquisition of new
      keys in the event of a trust anchor rollover, or as a brute-force
      repair for key maintenance problems.

``modzone`` *zone* [*class* [*view*]] *configuration*
   This command modifies the configuration of a zone while the server is running. This
   command requires the ``allow-new-zones`` option to be set to ``yes``.
   As with ``addzone``, the configuration string specified on the
   command line is the zone configuration text that would ordinarily be
   placed in ``named.conf``.

   If the zone was originally added via ``rndc addzone``, the
   configuration changes are recorded permanently and are still
   in effect after the server is restarted or reconfigured. However, if
   it was originally configured in ``named.conf``, then that original
   configuration remains in place; when the server is restarted or
   reconfigured, the zone reverts to its original configuration. To
   make the changes permanent, it must also be modified in
   ``named.conf``.

   See also ``rndc addzone`` and ``rndc delzone``.

``notify`` *zone* [*class* [*view*]]
   This command resends NOTIFY messages for the zone.

``notrace``
   This command sets the server's debugging level to 0.

   See also ``rndc trace``.

``nta`` [( **-class** *class* | **-dump** | **-force** | **-remove** | **-lifetime** *duration*)] *domain* [*view*]
   This command sets a DNSSEC negative trust anchor (NTA) for ``domain``, with a
   lifetime of ``duration``. The default lifetime is configured in
   ``named.conf`` via the ``nta-lifetime`` option, and defaults to one
   hour. The lifetime cannot exceed one week.

   A negative trust anchor selectively disables DNSSEC validation for
   zones that are known to be failing because of misconfiguration rather
   than an attack. When data to be validated is at or below an active
   NTA (and above any other configured trust anchors), ``named``
   aborts the DNSSEC validation process and treats the data as insecure
   rather than bogus. This continues until the NTA's lifetime has
   elapsed.

   NTAs persist across restarts of the ``named`` server. The NTAs for a
   view are saved in a file called ``name.nta``, where ``name`` is the name
   of the view; if it contains characters that are incompatible with
   use as a file name, a cryptographic hash is generated from the name of
   the view.

   An existing NTA can be removed by using the ``-remove`` option.

   An NTA's lifetime can be specified with the ``-lifetime`` option.
   TTL-style suffixes can be used to specify the lifetime in seconds,
   minutes, or hours. If the specified NTA already exists, its lifetime
   is updated to the new value. Setting ``lifetime`` to zero is
   equivalent to ``-remove``.

   If ``-dump`` is used, any other arguments are ignored and a list
   of existing NTAs is printed. Note that this may include NTAs that are
   expired but have not yet been cleaned up.

   Normally, ``named`` periodically tests to see whether data below
   an NTA can now be validated (see the ``nta-recheck`` option in the
   Administrator Reference Manual for details). If data can be
   validated, then the NTA is regarded as no longer necessary and is
   allowed to expire early. The ``-force`` parameter overrides this behavior
   and forces an NTA to persist for its entire lifetime, regardless of
   whether data could be validated if the NTA were not present.

   The view class can be specified with ``-class``. The default is class
   ``IN``, which is the only class for which DNSSEC is currently
   supported.

   All of these options can be shortened, i.e., to ``-l``, ``-r``,
   ``-d``, ``-f``, and ``-c``.

   Unrecognized options are treated as errors. To refer to a domain or
   view name that begins with a hyphen, use a double-hyphen (--) on the
   command line to indicate the end of options.

``querylog`` [(*on* | *off*)]
   This command enables or disables query logging. For backward compatibility, this
   command can also be used without an argument to toggle query logging
   on and off.

   Query logging can also be enabled by explicitly directing the
   ``queries`` ``category`` to a ``channel`` in the ``logging`` section
   of ``named.conf``, or by specifying ``querylog yes;`` in the
   ``options`` section of ``named.conf``.

``reconfig``
   This command reloads the configuration file and loads new zones, but does not reload
   existing zone files even if they have changed. This is faster than a
   full ``reload`` when there is a large number of zones, because it
   avoids the need to examine the modification times of the zone files.

``recursing``
   This command dumps the list of queries ``named`` is currently
   recursing on, and the list of domains to which iterative queries
   are currently being sent.

   The first list includes all unique clients that are waiting for
   recursion to complete, including the query that is awaiting a
   response and the timestamp (seconds since the Unix epoch) of
   when named started processing this client query.

   The second list comprises of domains for which there are active
   (or recently active) fetches in progress.  It reports the number
   of active fetches for each domain and the number of queries that
   have been passed (allowed) or dropped (spilled) as a result of
   the ``fetches-per-zone`` limit.  (Note: these counters are not
   cumulative over time; whenever the number of active fetches for
   a domain drops to zero, the counter for that domain is deleted,
   and the next time a fetch is sent to that domain, it is recreated
   with the counters set to zero).

``refresh`` *zone* [*class* [*view*]]
   This command schedules zone maintenance for the given zone.

``reload``
   This command reloads the configuration file and zones.

``reload`` *zone* [*class* [*view*]]
   This command reloads the given zone.

``retransfer`` *zone* [*class* [*view*]]
   This command retransfers the given secondary zone from the primary server.

   If the zone is configured to use ``inline-signing``, the signed
   version of the zone is discarded; after the retransfer of the
   unsigned version is complete, the signed version is regenerated
   with new signatures.

``scan``
   This command scans the list of available network interfaces for changes, without
   performing a full ``reconfig`` or waiting for the
   ``interface-interval`` timer.

``secroots`` [**-**] [*view* ...]
   This command dumps the security roots (i.e., trust anchors configured via
   ``trust-anchors``, or the ``managed-keys`` or ``trusted-keys`` statements
   [both deprecated], or ``dnssec-validation auto``) and negative trust anchors
   for the specified views. If no view is specified, all views are
   dumped. Security roots indicate whether they are configured as trusted
   keys, managed keys, or initializing managed keys (managed keys that have not
   yet been updated by a successful key refresh query).

   If the first argument is ``-``, then the output is returned via the
   ``rndc`` response channel and printed to the standard output.
   Otherwise, it is written to the secroots dump file, which defaults to
   ``named.secroots``, but can be overridden via the ``secroots-file``
   option in ``named.conf``.

   See also ``rndc managed-keys``.

``serve-stale`` (**on** | **off** | **reset** | **status**) [*class* [*view*]]
   This command enables, disables, resets, or reports the current status of the serving
   of stale answers as configured in ``named.conf``.

   If serving of stale answers is disabled by ``rndc-serve-stale off``,
   then it remains disabled even if ``named`` is reloaded or
   reconfigured. ``rndc serve-stale reset`` restores the setting as
   configured in ``named.conf``.

   ``rndc serve-stale status`` reports whether serving of stale
   answers is currently enabled, disabled by the configuration, or
   disabled by ``rndc``. It also reports the values of
   ``stale-answer-ttl`` and ``max-stale-ttl``.

``showzone`` *zone* [*class* [*view*]]
   This command prints the configuration of a running zone.

   See also ``rndc zonestatus``.

``sign`` *zone* [*class* [*view*]]
   This command fetches all DNSSEC keys for the given zone from the key directory (see
   the ``key-directory`` option in the BIND 9 Administrator Reference
   Manual). If they are within their publication period, they are merged into
   the zone's DNSKEY RRset. If the DNSKEY RRset is changed, then the
   zone is automatically re-signed with the new key set.

   This command requires that the zone be configured with a ``dnssec-policy``, or
   that the ``auto-dnssec`` zone option be set to ``allow`` or ``maintain``,
   and also requires the zone to be configured to allow dynamic DNS. (See
   "Dynamic Update Policies" in the BIND 9 Administrator Reference Manual for more
   details.)

   See also ``rndc loadkeys``.

``signing`` [(**-list** | **-clear** *keyid/algorithm* | **-clear** *all* | **-nsec3param** ( *parameters* | none ) | **-serial** *value* ) *zone* [*class* [*view*]]
   This command lists, edits, or removes the DNSSEC signing-state records for the
   specified zone. The status of ongoing DNSSEC operations, such as
   signing or generating NSEC3 chains, is stored in the zone in the form
   of DNS resource records of type ``sig-signing-type``.
   ``rndc signing -list`` converts these records into a human-readable
   form, indicating which keys are currently signing or have finished
   signing the zone, and which NSEC3 chains are being created or
   removed.

   ``rndc signing -clear`` can remove a single key (specified in the
   same format that ``rndc signing -list`` uses to display it), or all
   keys. In either case, only completed keys are removed; any record
   indicating that a key has not yet finished signing the zone is
   retained.

   ``rndc signing -nsec3param`` sets the NSEC3 parameters for a zone.
   This is the only supported mechanism for using NSEC3 with
   ``inline-signing`` zones. Parameters are specified in the same format
   as an NSEC3PARAM resource record: ``hash algorithm``, ``flags``, ``iterations``,
   and ``salt``, in that order.

   Currently, the only defined value for ``hash algorithm`` is ``1``,
   representing SHA-1. The ``flags`` may be set to ``0`` or ``1``,
   depending on whether the opt-out bit in the NSEC3
   chain should be set. ``iterations`` defines the number of additional times to apply
   the algorithm when generating an NSEC3 hash. The ``salt`` is a string
   of data expressed in hexadecimal, a hyphen (``-``) if no salt is to be
   used, or the keyword ``auto``, which causes ``named`` to generate a
   random 64-bit salt.

   The only recommended configuration is ``rndc signing -nsec3param 1 0 0 - zone``,
   i.e. no salt, no additional iterations, no opt-out.

   .. warning::
      Do not use extra iterations, salt, or opt-out unless all their implications
      are fully understood. A higher number of iterations causes interoperability
      problems and opens servers to CPU-exhausting DoS attacks.

   ``rndc signing -nsec3param none`` removes an existing NSEC3 chain and
   replaces it with NSEC.

   ``rndc signing -serial value`` sets the serial number of the zone to
   ``value``. If the value would cause the serial number to go backwards, it
   is rejected. The primary use of this parameter is to set the serial number on inline
   signed zones.

``stats``
   This command writes server statistics to the statistics file. (See the
   ``statistics-file`` option in the BIND 9 Administrator Reference
   Manual.)

``status``
   This command displays the status of the server. Note that the number of zones includes
   the internal ``bind/CH`` zone and the default ``./IN`` hint zone, if
   there is no explicit root zone configured.

``stop`` **-p**
   This command stops the server, making sure any recent changes made through dynamic
   update or IXFR are first saved to the master files of the updated
   zones. If ``-p`` is specified, ``named(8)`'s process ID is returned.
   This allows an external process to determine when ``named`` has
   completed stopping.

   See also ``rndc halt``.

``sync`` **-clean** [*zone* [*class* [*view*]]]
   This command syncs changes in the journal file for a dynamic zone to the master
   file. If the "-clean" option is specified, the journal file is also
   removed. If no zone is specified, then all zones are synced.

``tcp-timeouts`` [*initial* *idle* *keepalive* *advertised*]
   When called without arguments, this command displays the current values of the
   ``tcp-initial-timeout``, ``tcp-idle-timeout``,
   ``tcp-keepalive-timeout``, and ``tcp-advertised-timeout`` options.
   When called with arguments, these values are updated. This allows an
   administrator to make rapid adjustments when under a
   denial-of-service (DoS) attack. See the descriptions of these options in the BIND 9
   Administrator Reference Manual for details of their use.

``thaw`` [*zone* [*class* [*view*]]]
   This command enables updates to a frozen dynamic zone. If no zone is specified,
   then all frozen zones are enabled. This causes the server to reload
   the zone from disk, and re-enables dynamic updates after the load has
   completed. After a zone is thawed, dynamic updates are no longer
   refused. If the zone has changed and the ``ixfr-from-differences``
   option is in use, the journal file is updated to reflect
   changes in the zone. Otherwise, if the zone has changed, any existing
   journal file is removed.

   See also ``rndc freeze``.

``trace``
   This command increments the server's debugging level by one.

``trace`` *level*
   This command sets the server's debugging level to an explicit value.

   See also ``rndc notrace``.

``tsig-delete`` *keyname* [*view*]
   This command deletes a given TKEY-negotiated key from the server. This does not
   apply to statically configured TSIG keys.

``tsig-list``
   This command lists the names of all TSIG keys currently configured for use by
   ``named`` in each view. The list includes both statically configured keys and
   dynamic TKEY-negotiated keys.

``validation`` (**on** | **off** | **status**) [*view* ...]``
   This command enables, disables, or checks the current status of DNSSEC validation. By
   default, validation is enabled.

   The cache is flushed when validation is turned on or off to avoid using data
   that might differ between states.

``zonestatus`` *zone* [*class* [*view*]]
   This command displays the current status of the given zone, including the master
   file name and any include files from which it was loaded, when it was
   most recently loaded, the current serial number, the number of nodes,
   whether the zone supports dynamic updates, whether the zone is DNSSEC
   signed, whether it uses automatic DNSSEC key management or inline
   signing, and the scheduled refresh or expiry times for the zone.

   See also ``rndc showzone``.

``rndc`` commands that specify zone names, such as ``reload``,
``retransfer``, or ``zonestatus``, can be ambiguous when applied to zones
of type ``redirect``. Redirect zones are always called ``.``, and can be
confused with zones of type ``hint`` or with secondary copies of the root
zone. To specify a redirect zone, use the special zone name
``-redirect``, without a trailing period. (With a trailing period, this
would specify a zone called "-redirect".)

Limitations
~~~~~~~~~~~

There is currently no way to provide the shared secret for a ``key_id``
without using the configuration file.

Several error messages could be clearer.

See Also
~~~~~~~~

:manpage:`rndc.conf(5)`, :manpage:`rndc-confgen(8)`,
:manpage:`named(8)`, :manpage:`named.conf(5)`, :manpage:`ndc(8)`, BIND 9 Administrator
Reference Manual.
