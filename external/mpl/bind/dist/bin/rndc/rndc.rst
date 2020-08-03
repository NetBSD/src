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

.. _man_rndc:

rndc - name server control utility
----------------------------------

Synopsis
~~~~~~~~

:program:`rndc` [**-b** source-address] [**-c** config-file] [**-k** key-file] [**-s** server] [**-p** port] [**-q**] [**-r**] [**-V**] [**-y** key_id] [[**-4**] | [**-6**]] {command}

Description
~~~~~~~~~~~

``rndc`` controls the operation of a name server. It supersedes the
``ndc`` utility that was provided in old BIND releases. If ``rndc`` is
invoked with no command line options or arguments, it prints a short
summary of the supported commands and the available options and their
arguments.

``rndc`` communicates with the name server over a TCP connection,
sending commands authenticated with digital signatures. In the current
versions of ``rndc`` and ``named``, the only supported authentication
algorithms are HMAC-MD5 (for compatibility), HMAC-SHA1, HMAC-SHA224,
HMAC-SHA256 (default), HMAC-SHA384 and HMAC-SHA512. They use a shared
secret on each end of the connection. This provides TSIG-style
authentication for the command request and the name server's response.
All commands sent over the channel must be signed by a key_id known to
the server.

``rndc`` reads a configuration file to determine how to contact the name
server and decide what algorithm and key it should use.

Options
~~~~~~~

**-4**
   Use IPv4 only.

**-6**
   Use IPv6 only.

**-b** source-address
   Use source-address as the source address for the connection to the
   server. Multiple instances are permitted to allow setting of both the
   IPv4 and IPv6 source addresses.

**-c** config-file
   Use config-file as the configuration file instead of the default,
   ``/etc/rndc.conf``.

**-k** key-file
   Use key-file as the key file instead of the default,
   ``/etc/rndc.key``. The key in ``/etc/rndc.key`` will be used to
   authenticate commands sent to the server if the config-file does not
   exist.

**-s** server
   server is the name or address of the server which matches a server
   statement in the configuration file for ``rndc``. If no server is
   supplied on the command line, the host named by the default-server
   clause in the options statement of the ``rndc`` configuration file
   will be used.

**-p** port
   Send commands to TCP port port instead of BIND 9's default control
   channel port, 953.

**-q**
   Quiet mode: Message text returned by the server will not be printed
   except when there is an error.

**-r**
   Instructs ``rndc`` to print the result code returned by ``named``
   after executing the requested command (e.g., ISC_R_SUCCESS,
   ISC_R_FAILURE, etc).

**-V**
   Enable verbose logging.

**-y** key_id
   Use the key key_id from the configuration file. key_id must be known
   by ``named`` with the same algorithm and secret string in order for
   control message validation to succeed. If no key_id is specified,
   ``rndc`` will first look for a key clause in the server statement of
   the server being used, or if no server statement is present for that
   host, then the default-key clause of the options statement. Note that
   the configuration file contains shared secrets which are used to send
   authenticated control commands to name servers. It should therefore
   not have general read or write access.

Commands
~~~~~~~~

A list of commands supported by ``rndc`` can be seen by running ``rndc``
without arguments.

Currently supported commands are:

``addzone`` *zone* [*class* [*view*]] *configuration*
   Add a zone while the server is running. This command requires the
   ``allow-new-zones`` option to be set to ``yes``. The configuration
   string specified on the command line is the zone configuration text
   that would ordinarily be placed in :manpage:`named.conf(5)`.

   The configuration is saved in a file called ``viewname.nzf`` (or, if
   :manpage:`named(8)` is compiled with liblmdb, an LMDB database file called
   ``viewname.nzd``). viewname is the name of the view, unless the view
   name contains characters that are incompatible with use as a file
   name, in which case a cryptographic hash of the view name is used
   instead. When :manpage:`named(8)` is restarted, the file will be loaded into
   the view configuration, so that zones that were added can persist
   after a restart.

   This sample ``addzone`` command would add the zone ``example.com`` to
   the default view:

   ``$``\ ``rndc addzone example.com '{ type master; file "example.com.db"; };'``

   (Note the brackets and semi-colon around the zone configuration
   text.)

   See also ``rndc delzone`` and ``rndc modzone``.

``delzone`` [**-clean**] *zone* [*class* [*view*]]
   Delete a zone while the server is running.

   If the ``-clean`` argument is specified, the zone's master file (and
   journal file, if any) will be deleted along with the zone. Without
   the ``-clean`` option, zone files must be cleaned up by hand. (If the
   zone is of type "slave" or "stub", the files needing to be cleaned up
   will be reported in the output of the ``rndc delzone`` command.)

   If the zone was originally added via ``rndc addzone``, then it will
   be removed permanently. However, if it was originally configured in
   ``named.conf``, then that original configuration is still in place;
   when the server is restarted or reconfigured, the zone will come
   back. To remove it permanently, it must also be removed from
   ``named.conf``

   See also ``rndc addzone`` and ``rndc modzone``.

``dnssec`` [**-status** *zone* [*class* [*view*]]
   Show the DNSSEC signing state for the specified zone.  Requires the
   zone to have a "dnssec-policy".

``dnstap`` ( **-reopen** | **-roll** [*number*] )
   Close and re-open DNSTAP output files. ``rndc dnstap -reopen`` allows
   the output file to be renamed externally, so that :manpage:`named(8)` can
   truncate and re-open it. ``rndc dnstap -roll`` causes the output file
   to be rolled automatically, similar to log files; the most recent
   output file has ".0" appended to its name; the previous most recent
   output file is moved to ".1", and so on. If number is specified, then
   the number of backup log files is limited to that number.

``dumpdb`` [**-all** | **-cache** | **-zones** | **-adb** | **-bad** | **-fail**] [*view ...*]
   Dump the server's caches (default) and/or zones to the dump file for
   the specified views. If no view is specified, all views are dumped.
   (See the ``dump-file`` option in the BIND 9 Administrator Reference
   Manual.)

``flush``
   Flushes the server's cache.

``flushname`` *name* [*view*]
   Flushes the given name from the view's DNS cache and, if applicable,
   from the view's nameserver address database, bad server cache and
   SERVFAIL cache.

``flushtree`` *name* [*view*]
   Flushes the given name, and all of its subdomains, from the view's
   DNS cache, address database, bad server cache, and SERVFAIL cache.

``freeze`` [*zone* [*class* [*view*]]]
   Suspend updates to a dynamic zone. If no zone is specified, then all
   zones are suspended. This allows manual edits to be made to a zone
   normally updated by dynamic update. It also causes changes in the
   journal file to be synced into the master file. All dynamic update
   attempts will be refused while the zone is frozen.

   See also ``rndc thaw``.

``halt`` [**-p**]
   Stop the server immediately. Recent changes made through dynamic
   update or IXFR are not saved to the master files, but will be rolled
   forward from the journal files when the server is restarted. If
   ``-p`` is specified :manpage:`named(8)`'s process id is returned. This allows
   an external process to determine when :manpage:`named(8)` had completed
   halting.

   See also ``rndc stop``.

``loadkeys`` [*zone* [*class* [*view*]]]
   Fetch all DNSSEC keys for the given zone from the key directory. If
   they are within their publication period, merge them into the
   zone's DNSKEY RRset. Unlike ``rndc sign``, however, the zone is not
   immediately re-signed by the new keys, but is allowed to
   incrementally re-sign over time.

   This command requires that zone is configured with a ``dnssec-policy``, or
   the ``auto-dnssec`` zone option be set to ``maintain``, and also requires the
   zone to be configured to allow dynamic DNS. (See "Dynamic Update Policies" in
   the Administrator Reference Manual for more details.)

``managed-keys`` (*status* | *refresh* | *sync* | *destroy*) [*class* [*view*]]
   Inspect and control the "managed-keys" database which handles
   :rfc:`5011` DNSSEC trust anchor maintenance. If a view is specified, these
   commands are applied to that view; otherwise they are applied to all
   views.

   -  When run with the ``status`` keyword, prints the current status of
      the managed-keys database.

   -  When run with the ``refresh`` keyword, forces an immediate refresh
      query to be sent for all the managed keys, updating the
      managed-keys database if any new keys are found, without waiting
      the normal refresh interval.

   -  When run with the ``sync`` keyword, forces an immediate dump of
      the managed-keys database to disk (in the file
      ``managed-keys.bind`` or (``viewname.mkeys``). This synchronizes
      the database with its journal file, so that the database's current
      contents can be inspected visually.

   -  When run with the ``destroy`` keyword, the managed-keys database
      is shut down and deleted, and all key maintenance is terminated.
      This command should be used only with extreme caution.

      Existing keys that are already trusted are not deleted from
      memory; DNSSEC validation can continue after this command is used.
      However, key maintenance operations will cease until :manpage:`named(8)` is
      restarted or reconfigured, and all existing key maintenance state
      will be deleted.

      Running ``rndc reconfig`` or restarting :manpage:`named(8)` immediately
      after this command will cause key maintenance to be reinitialized
      from scratch, just as if the server were being started for the
      first time. This is primarily intended for testing, but it may
      also be used, for example, to jumpstart the acquisition of new
      keys in the event of a trust anchor rollover, or as a brute-force
      repair for key maintenance problems.

``modzone`` *zone* [*class* [*view*]] *configuration*
   Modify the configuration of a zone while the server is running. This
   command requires the ``allow-new-zones`` option to be set to ``yes``.
   As with ``addzone``, the configuration string specified on the
   command line is the zone configuration text that would ordinarily be
   placed in ``named.conf``.

   If the zone was originally added via ``rndc addzone``, the
   configuration changes will be recorded permanently and will still be
   in effect after the server is restarted or reconfigured. However, if
   it was originally configured in ``named.conf``, then that original
   configuration is still in place; when the server is restarted or
   reconfigured, the zone will revert to its original configuration. To
   make the changes permanent, it must also be modified in
   ``named.conf``

   See also ``rndc addzone`` and ``rndc delzone``.

``notify`` *zone* [*class* [*view*]]
   Resend NOTIFY messages for the zone.

``notrace``
   Sets the server's debugging level to 0.

   See also ``rndc trace``.

``nta`` [( **-class** *class* | **-dump** | **-force** | **-remove** | **-lifetime** *duration*)] *domain* [*view*]
   Sets a DNSSEC negative trust anchor (NTA) for ``domain``, with a
   lifetime of ``duration``. The default lifetime is configured in
   ``named.conf`` via the ``nta-lifetime`` option, and defaults to one
   hour. The lifetime cannot exceed one week.

   A negative trust anchor selectively disables DNSSEC validation for
   zones that are known to be failing because of misconfiguration rather
   than an attack. When data to be validated is at or below an active
   NTA (and above any other configured trust anchors), :manpage:`named(8)` will
   abort the DNSSEC validation process and treat the data as insecure
   rather than bogus. This continues until the NTA's lifetime is
   elapsed.

   NTAs persist across restarts of the :manpage:`named(8)` server. The NTAs for a
   view are saved in a file called ``name.nta``, where name is the name
   of the view, or if it contains characters that are incompatible with
   use as a file name, a cryptographic hash generated from the name of
   the view.

   An existing NTA can be removed by using the ``-remove`` option.

   An NTA's lifetime can be specified with the ``-lifetime`` option.
   TTL-style suffixes can be used to specify the lifetime in seconds,
   minutes, or hours. If the specified NTA already exists, its lifetime
   will be updated to the new value. Setting ``lifetime`` to zero is
   equivalent to ``-remove``.

   If the ``-dump`` is used, any other arguments are ignored, and a list
   of existing NTAs is printed (note that this may include NTAs that are
   expired but have not yet been cleaned up).

   Normally, :manpage:`named(8)` will periodically test to see whether data below
   an NTA can now be validated (see the ``nta-recheck`` option in the
   Administrator Reference Manual for details). If data can be
   validated, then the NTA is regarded as no longer necessary, and will
   be allowed to expire early. The ``-force`` overrides this behavior
   and forces an NTA to persist for its entire lifetime, regardless of
   whether data could be validated if the NTA were not present.

   The view class can be specified with ``-class``. The default is class
   ``IN``, which is the only class for which DNSSEC is currently
   supported.

   All of these options can be shortened, i.e., to ``-l``, ``-r``,
   ``-d``, ``-f``, and ``-c``.

   Unrecognized options are treated as errors. To reference a domain or
   view name that begins with a hyphen, use a double-hyphen on the
   command line to indicate the end of options.

``querylog`` [(*on* | *off*)]
   Enable or disable query logging. (For backward compatibility, this
   command can also be used without an argument to toggle query logging
   on and off.)

   Query logging can also be enabled by explicitly directing the
   ``queries`` ``category`` to a ``channel`` in the ``logging`` section
   of ``named.conf`` or by specifying ``querylog yes;`` in the
   ``options`` section of ``named.conf``.

``reconfig``
   Reload the configuration file and load new zones, but do not reload
   existing zone files even if they have changed. This is faster than a
   full ``reload`` when there is a large number of zones because it
   avoids the need to examine the modification times of the zones files.

``recursing``
   Dump the list of queries :manpage:`named(8)` is currently recursing on, and the
   list of domains to which iterative queries are currently being sent.
   (The second list includes the number of fetches currently active for
   the given domain, and how many have been passed or dropped because of
   the ``fetches-per-zone`` option.)

``refresh`` *zone* [*class* [*view*]]
   Schedule zone maintenance for the given zone.

``reload``
   Reload configuration file and zones.

``reload`` *zone* [*class* [*view*]]
   Reload the given zone.

``retransfer`` *zone* [*class* [*view*]]
   Retransfer the given slave zone from the master server.

   If the zone is configured to use ``inline-signing``, the signed
   version of the zone is discarded; after the retransfer of the
   unsigned version is complete, the signed version will be regenerated
   with all new signatures.

``scan``
   Scan the list of available network interfaces for changes, without
   performing a full ``reconfig`` or waiting for the
   ``interface-interval`` timer.

``secroots`` [**-**] [*view* ...]
   Dump the security roots (i.e., trust anchors configured via
   ``trust-anchors``, or the ``managed-keys`` or ``trusted-keys`` statements
   (both deprecated), or ``dnssec-validation auto``) and negative trust anchors
   for the specified views. If no view is specified, all views are
   dumped. Security roots will indicate whether they are configured as trusted
   keys, managed keys, or initializing managed keys (managed keys that have not
   yet been updated by a successful key refresh query).

   If the first argument is "-", then the output is returned via the
   ``rndc`` response channel and printed to the standard output.
   Otherwise, it is written to the secroots dump file, which defaults to
   ``named.secroots``, but can be overridden via the ``secroots-file``
   option in ``named.conf``.

   See also ``rndc managed-keys``.

``serve-stale`` (**on** | **off** | **reset** | **status**) [*class* [*view*]]
   Enable, disable, reset, or report the current status of the serving
   of stale answers as configured in ``named.conf``.

   If serving of stale answers is disabled by ``rndc-serve-stale off``,
   then it will remain disabled even if :manpage:`named(8)` is reloaded or
   reconfigured. ``rndc serve-stale reset`` restores the setting as
   configured in ``named.conf``.

   ``rndc serve-stale status`` will report whether serving of stale
   answers is currently enabled, disabled by the configuration, or
   disabled by ``rndc``. It will also report the values of
   ``stale-answer-ttl`` and ``max-stale-ttl``.

``showzone`` *zone* [*class* [*view*]]
   Print the configuration of a running zone.

   See also ``rndc zonestatus``.

``sign`` *zone* [*class* [*view*]]
   Fetch all DNSSEC keys for the given zone from the key directory (see
   the ``key-directory`` option in the BIND 9 Administrator Reference
   Manual). If they are within their publication period, merge them into
   the zone's DNSKEY RRset. If the DNSKEY RRset is changed, then the
   zone is automatically re-signed with the new key set.

   This command requires that the zone is configure with a ``dnssec-policy``, or
   that the ``auto-dnssec`` zone option be set to ``allow`` or ``maintain``,
   and also requires the zone to be configured to allow dynamic DNS. (See
   "Dynamic Update Policies" in the Administrator Reference Manual for more
   details.)

   See also ``rndc loadkeys``.

``signing`` [(**-list** | **-clear** *keyid/algorithm* | **-clear** *all* | **-nsec3param** ( *parameters* | none ) | **-serial** *value* ) *zone* [*class* [*view*]]
   List, edit, or remove the DNSSEC signing state records for the
   specified zone. The status of ongoing DNSSEC operations (such as
   signing or generating NSEC3 chains) is stored in the zone in the form
   of DNS resource records of type ``sig-signing-type``.
   ``rndc signing -list`` converts these records into a human-readable
   form, indicating which keys are currently signing or have finished
   signing the zone, and which NSEC3 chains are being created or
   removed.

   ``rndc signing -clear`` can remove a single key (specified in the
   same format that ``rndc signing -list`` uses to display it), or all
   keys. In either case, only completed keys are removed; any record
   indicating that a key has not yet finished signing the zone will be
   retained.

   ``rndc signing -nsec3param`` sets the NSEC3 parameters for a zone.
   This is the only supported mechanism for using NSEC3 with
   ``inline-signing`` zones. Parameters are specified in the same format
   as an NSEC3PARAM resource record: hash algorithm, flags, iterations,
   and salt, in that order.

   Currently, the only defined value for hash algorithm is ``1``,
   representing SHA-1. The ``flags`` may be set to ``0`` or ``1``,
   depending on whether you wish to set the opt-out bit in the NSEC3
   chain. ``iterations`` defines the number of additional times to apply
   the algorithm when generating an NSEC3 hash. The ``salt`` is a string
   of data expressed in hexadecimal, a hyphen (`-') if no salt is to be
   used, or the keyword ``auto``, which causes :manpage:`named(8)` to generate a
   random 64-bit salt.

   So, for example, to create an NSEC3 chain using the SHA-1 hash
   algorithm, no opt-out flag, 10 iterations, and a salt value of
   "FFFF", use: ``rndc signing -nsec3param 1 0 10 FFFF zone``. To set
   the opt-out flag, 15 iterations, and no salt, use:
   ``rndc signing -nsec3param 1 1 15 - zone``.

   ``rndc signing -nsec3param none`` removes an existing NSEC3 chain and
   replaces it with NSEC.

   ``rndc signing -serial value`` sets the serial number of the zone to
   value. If the value would cause the serial number to go backwards it
   will be rejected. The primary use is to set the serial on inline
   signed zones.

``stats``
   Write server statistics to the statistics file. (See the
   ``statistics-file`` option in the BIND 9 Administrator Reference
   Manual.)

``status``
   Display status of the server. Note that the number of zones includes
   the internal ``bind/CH`` zone and the default ``./IN`` hint zone if
   there is not an explicit root zone configured.

``stop`` **-p**
   Stop the server, making sure any recent changes made through dynamic
   update or IXFR are first saved to the master files of the updated
   zones. If ``-p`` is specified :manpage:`named(8)`'s process id is returned.
   This allows an external process to determine when :manpage:`named(8)` had
   completed stopping.

   See also ``rndc halt``.

``sync`` **-clean** [*zone* [*class* [*view*]]]
   Sync changes in the journal file for a dynamic zone to the master
   file. If the "-clean" option is specified, the journal file is also
   removed. If no zone is specified, then all zones are synced.

``tcp-timeouts`` [*initial* *idle* *keepalive* *advertised*]
   When called without arguments, display the current values of the
   ``tcp-initial-timeout``, ``tcp-idle-timeout``,
   ``tcp-keepalive-timeout`` and ``tcp-advertised-timeout`` options.
   When called with arguments, update these values. This allows an
   administrator to make rapid adjustments when under a denial of
   service attack. See the descriptions of these options in the BIND 9
   Administrator Reference Manual for details of their use.

``thaw`` [*zone* [*class* [*view*]]]
   Enable updates to a frozen dynamic zone. If no zone is specified,
   then all frozen zones are enabled. This causes the server to reload
   the zone from disk, and re-enables dynamic updates after the load has
   completed. After a zone is thawed, dynamic updates will no longer be
   refused. If the zone has changed and the ``ixfr-from-differences``
   option is in use, then the journal file will be updated to reflect
   changes in the zone. Otherwise, if the zone has changed, any existing
   journal file will be removed.

   See also ``rndc freeze``.

``trace``
   Increment the servers debugging level by one.

``trace`` *level*
   Sets the server's debugging level to an explicit value.

   See also ``rndc notrace``.

``tsig-delete`` *keyname* [*view*]
   Delete a given TKEY-negotiated key from the server. (This does not
   apply to statically configured TSIG keys.)

``tsig-list``
   List the names of all TSIG keys currently configured for use by
   :manpage:`named(8)` in each view. The list both statically configured keys and
   dynamic TKEY-negotiated keys.

``validation`` (**on** | **off** | **status**) [*view* ...]``
   Enable, disable, or check the current status of DNSSEC validation. By
   default, validation is enabled.

   The cache is flushed when validation is turned on or off to avoid using data
   that might differ between states.

``zonestatus`` *zone* [*class* [*view*]]
   Displays the current status of the given zone, including the master
   file name and any include files from which it was loaded, when it was
   most recently loaded, the current serial number, the number of nodes,
   whether the zone supports dynamic updates, whether the zone is DNSSEC
   signed, whether it uses automatic DNSSEC key management or inline
   signing, and the scheduled refresh or expiry times for the zone.

   See also ``rndc showzone``.

``rndc`` commands that specify zone names, such as ``reload``,
``retransfer`` or ``zonestatus``, can be ambiguous when applied to zones
of type ``redirect``. Redirect zones are always called ".", and can be
confused with zones of type ``hint`` or with slaved copies of the root
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
