.. 
   Copyright (C) Internet Systems Consortium, Inc. ("ISC")
   
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, you can obtain one at https://mozilla.org/MPL/2.0/.
   
   See the COPYRIGHT file distributed with this work for additional
   information regarding copyright ownership.

.. _dnssec_troubleshooting:

Basic DNSSEC Troubleshooting
----------------------------

In this chapter, we cover some basic troubleshooting
techniques, some common DNSSEC symptoms, and their causes and solutions. This
is not a comprehensive "how to troubleshoot any DNS or DNSSEC problem"
guide, because that could easily be an entire book by itself.

.. _troubleshooting_query_path:

Query Path
~~~~~~~~~~

The first step in troubleshooting DNS or DNSSEC should be to
determine the query path. Whenever you are working with a DNS-related issue, it is
always a good idea to determine the exact query path to identify the
origin of the problem.

End clients, such as laptop computers or mobile phones, are configured
to talk to a recursive name server, and the recursive name server may in
turn forward requests on to other recursive name servers before arriving at the
authoritative name server. The giveaway is the presence of the
Authoritative Answer (``aa``) flag in a query response: when present, we know we are talking
to the authoritative server; when missing, we are talking to a recursive
server. The example below shows an answer to a query for
``www.example.com`` without the Authoritative Answer flag:

::

   $ dig @10.53.0.3 www.example.com A

   ; <<>> DiG 9.16.0 <<>> @10.53.0.3 www.example.com a
   ; (1 server found)
   ;; global options: +cmd
   ;; Got answer:
   ;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 62714
   ;; flags: qr rd ra ad; QUERY: 1, ANSWER: 1, AUTHORITY: 0, ADDITIONAL: 1

   ;; OPT PSEUDOSECTION:
   ; EDNS: version: 0, flags:; udp: 4096
   ; COOKIE: c823fe302625db5b010000005e722b504d81bb01c2227259 (good)
   ;; QUESTION SECTION:
   ;www.example.com.       IN  A

   ;; ANSWER SECTION:
   www.example.com.    60  IN  A   10.1.0.1

   ;; Query time: 3 msec
   ;; SERVER: 10.53.0.3#53(10.53.0.3)
   ;; WHEN: Wed Mar 18 14:08:16 GMT 2020
   ;; MSG SIZE  rcvd: 88

Not only do we not see the ``aa`` flag, we see an ``ra``
flag, which indicates Recursion Available. This indicates that the
server we are talking to (10.53.0.3 in this example) is a recursive name
server: although we were able to get an answer for
``www.example.com``, we know that the answer came from somewhere else.

If we query the authoritative server directly, we get:

::

   $ dig @10.53.0.2 www.example.com A

   ; <<>> DiG 9.16.0 <<>> @10.53.0.2 www.example.com a
   ; (1 server found)
   ;; global options: +cmd
   ;; Got answer:
   ;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 39542
   ;; flags: qr aa rd; QUERY: 1, ANSWER: 1, AUTHORITY: 0, ADDITIONAL: 1
   ;; WARNING: recursion requested but not available
   ...

The ``aa`` flag tells us that we are now talking to the
authoritative name server for ``www.example.com``, and that this is not a
cached answer it obtained from some other name server; it served this
answer to us right from its own database. In fact,
the Recursion Available (``ra``) flag is not present, which means this
name server is not configured to perform recursion (at least not for
this client), so it could not have queried another name server to get
cached results.

.. _troubleshooting_visible_symptoms:

Visible DNSSEC Validation Symptoms
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

After determining the query path, it is necessary to
determine whether the problem is actually related to DNSSEC
validation. You can use the ``+cd`` flag in ``dig`` to disable
validation, as described in
:ref:`how_do_i_know_validation_problem`.

When there is indeed a DNSSEC validation problem, the visible symptoms,
unfortunately, are very limited. With DNSSEC validation enabled, if a
DNS response is not fully validated, it results in a generic
SERVFAIL message, as shown below when querying against a recursive name
server at 192.168.1.7:

::

   $ dig @10.53.0.3 www.example.org. A

   ; <<>> DiG 9.16.0 <<>> @10.53.0.3 www.example.org A
   ; (1 server found)
   ;; global options: +cmd
   ;; Got answer:
   ;; ->>HEADER<<- opcode: QUERY, status: SERVFAIL, id: 28947
   ;; flags: qr rd ra; QUERY: 1, ANSWER: 0, AUTHORITY: 0, ADDITIONAL: 1

   ;; OPT PSEUDOSECTION:
   ; EDNS: version: 0, flags:; udp: 4096
   ; COOKIE: d1301968aca086ad010000005e723a7113603c01916d136b (good)
   ;; QUESTION SECTION:
   ;www.example.org.       IN  A

   ;; Query time: 3 msec
   ;; SERVER: 10.53.0.3#53(10.53.0.3)
   ;; WHEN: Wed Mar 18 15:12:49 GMT 2020
   ;; MSG SIZE  rcvd: 72

With ``delv``, a "resolution failed" message is output instead:

::

   $ delv @10.53.0.3 www.example.org. A +rtrace
   ;; fetch: www.example.org/A
   ;; resolution failed: SERVFAIL
   
BIND 9 logging features may be useful when trying to identify
DNSSEC errors.

.. _troubleshooting_logging:

Basic Logging
~~~~~~~~~~~~~

DNSSEC validation error messages show up in ``syslog`` as a
query error by default. Here is an example of what it may look like:

::

   validating www.example.org/A: no valid signature found
   RRSIG failed to verify resolving 'www.example.org/A/IN': 10.53.0.2#53

Usually, this level of error logging is sufficient.
Debug logging, described in
:ref:`troubleshooting_logging_debug`, gives information on how
to get more details about why DNSSEC validation may have
failed.

.. _troubleshooting_logging_debug:

BIND DNSSEC Debug Logging
~~~~~~~~~~~~~~~~~~~~~~~~~

A word of caution: before you enable debug logging, be aware that this
may dramatically increase the load on your name servers. Enabling debug
logging is thus not recommended for production servers.

With that said, sometimes it may become necessary to temporarily enable
BIND debug logging to see more details of how and whether DNSSEC is
validating. DNSSEC-related messages are not recorded in ``syslog`` by default,
even if query log is enabled; only DNSSEC errors show up in ``syslog``.

The example below shows how to enable debug level 3 (to see full DNSSEC
validation messages) in BIND 9 and have it sent to ``syslog``:

::

   logging {
      channel dnssec_log {
           syslog daemon;
           severity debug 3;
           print-category yes;
       };
       category dnssec { dnssec_log; };
   };

The example below shows how to log DNSSEC messages to their own file
(here, ``/var/log/dnssec.log``):

::

   logging {
       channel dnssec_log {
           file "/var/log/dnssec.log";
           severity debug 3;
       };
       category dnssec { dnssec_log; };
   };

After turning on debug logging and restarting BIND, a large
number of log messages appear in
``syslog``. The example below shows the log messages as a result of
successfully looking up and validating the domain name ``ftp.isc.org``.

::

   validating ./NS: starting
   validating ./NS: attempting positive response validation
     validating ./DNSKEY: starting
     validating ./DNSKEY: attempting positive response validation
     validating ./DNSKEY: verify rdataset (keyid=20326): success
     validating ./DNSKEY: marking as secure (DS)
   validating ./NS: in validator_callback_dnskey
   validating ./NS: keyset with trust secure
   validating ./NS: resuming validate
   validating ./NS: verify rdataset (keyid=33853): success
   validating ./NS: marking as secure, noqname proof not needed
   validating ftp.isc.org/A: starting
   validating ftp.isc.org/A: attempting positive response validation
   validating isc.org/DNSKEY: starting
   validating isc.org/DNSKEY: attempting positive response validation
     validating isc.org/DS: starting
     validating isc.org/DS: attempting positive response validation
   validating org/DNSKEY: starting
   validating org/DNSKEY: attempting positive response validation
     validating org/DS: starting
     validating org/DS: attempting positive response validation
     validating org/DS: keyset with trust secure
     validating org/DS: verify rdataset (keyid=33853): success
     validating org/DS: marking as secure, noqname proof not needed
   validating org/DNSKEY: in validator_callback_ds
   validating org/DNSKEY: dsset with trust secure
   validating org/DNSKEY: verify rdataset (keyid=9795): success
   validating org/DNSKEY: marking as secure (DS)
     validating isc.org/DS: in fetch_callback_dnskey
     validating isc.org/DS: keyset with trust secure
     validating isc.org/DS: resuming validate
     validating isc.org/DS: verify rdataset (keyid=33209): success
     validating isc.org/DS: marking as secure, noqname proof not needed
   validating isc.org/DNSKEY: in validator_callback_ds
   validating isc.org/DNSKEY: dsset with trust secure
   validating isc.org/DNSKEY: verify rdataset (keyid=7250): success
   validating isc.org/DNSKEY: marking as secure (DS)
   validating ftp.isc.org/A: in fetch_callback_dnskey
   validating ftp.isc.org/A: keyset with trust secure
   validating ftp.isc.org/A: resuming validate
   validating ftp.isc.org/A: verify rdataset (keyid=27566): success
   validating ftp.isc.org/A: marking as secure, noqname proof not needed

Note that these log messages indicate that the chain of trust has been
established and ``ftp.isc.org`` has been successfully validated.

If validation had failed, you would see log messages indicating errors.
We cover some of the most validation problems in the next section.

.. _troubleshooting_common_problems:

Common Problems
~~~~~~~~~~~~~~~

.. _troubleshooting_security_lameness:

Security Lameness
^^^^^^^^^^^^^^^^^

Similar to lame delegation in traditional DNS, security lameness refers to the
condition when the parent zone holds a set of DS records that point to
something that does not exist in the child zone. As a result,
the entire child zone may "disappear," having been marked as bogus by
validating resolvers.

Below is an example attempting to resolve the A record for a test domain
name ``www.example.net``. From the user's perspective, as described in
:ref:`how_do_i_know_validation_problem`, only a SERVFAIL
message is returned. On the validating resolver, we see the
following messages in ``syslog``:

::

   named[126063]: validating example.net/DNSKEY: no valid signature found (DS)
   named[126063]: no valid RRSIG resolving 'example.net/DNSKEY/IN': 10.53.0.2#53
   named[126063]: broken trust chain resolving 'www.example.net/A/IN': 10.53.0.2#53

This gives us a hint that it is a broken trust chain issue. Let's take a
look at the DS records that are published for the zone (with the keys
shortened for ease of display):

::

   $ dig @10.53.0.3 example.net. DS

   ; <<>> DiG 9.16.0 <<>> @10.53.0.3 example.net DS
   ; (1 server found)
   ;; global options: +cmd
   ;; Got answer:
   ;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 59602
   ;; flags: qr rd ra ad; QUERY: 1, ANSWER: 1, AUTHORITY: 0, ADDITIONAL: 1

   ;; OPT PSEUDOSECTION:
   ; EDNS: version: 0, flags:; udp: 4096
   ; COOKIE: 7026d8f7c6e77e2a010000005e735d7c9d038d061b2d24da (good)
   ;; QUESTION SECTION:
   ;example.net.           IN  DS

   ;; ANSWER SECTION:
   example.net.        256 IN  DS  14956 8 2 9F3CACD...D3E3A396

   ;; Query time: 0 msec
   ;; SERVER: 10.53.0.3#53(10.53.0.3)
   ;; WHEN: Thu Mar 19 11:54:36 GMT 2020
   ;; MSG SIZE  rcvd: 116

Next, we query for the DNSKEY and RRSIG of ``example.net`` to see if
there's anything wrong. Since we are having trouble validating, we
can use the ``+cd`` option to temporarily disable checking and return
results, even though they do not pass the validation tests. The
``+multiline`` option tells ``dig`` to print the type, algorithm type,
and key id for DNSKEY records. Again,
some long strings are shortened for ease of display:

::

   $ dig @10.53.0.3 example.net. DNSKEY +dnssec +cd +multiline

   ; <<>> DiG 9.16.0 <<>> @10.53.0.3 example.net DNSKEY +cd +multiline +dnssec
   ; (1 server found)
   ;; global options: +cmd
   ;; Got answer:
   ;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 42980
   ;; flags: qr rd ra cd; QUERY: 1, ANSWER: 4, AUTHORITY: 0, ADDITIONAL: 1

   ;; OPT PSEUDOSECTION:
   ; EDNS: version: 0, flags: do; udp: 4096
   ; COOKIE: 4b5e7c88b3680c35010000005e73722057551f9f8be1990e (good)
   ;; QUESTION SECTION:
   ;example.net.       IN DNSKEY

   ;; ANSWER SECTION:
   example.net.        287 IN DNSKEY 256 3 8 (
                   AwEAAbu3NX...ADU/D7xjFFDu+8WRIn
                   ) ; ZSK; alg = RSASHA256 ; key id = 35328
   example.net.        287 IN DNSKEY 257 3 8 (
                   AwEAAbKtU1...PPP4aQZTybk75ZW+uL
                   6OJMAF63NO0s1nAZM2EWAVasbnn/X+J4N2rLuhk=
                   ) ; KSK; alg = RSASHA256 ; key id = 27247
   example.net.        287 IN RRSIG DNSKEY 8 2 300 (
                   20811123173143 20180101000000 27247 example.net.
                   Fz1sjClIoF...YEjzpAWuAj9peQ== )
   example.net.        287 IN RRSIG DNSKEY 8 2 300 (
                   20811123173143 20180101000000 35328 example.net.
                   seKtUeJ4/l...YtDc1rcXTVlWIOw= )

   ;; Query time: 0 msec
   ;; SERVER: 10.53.0.3#53(10.53.0.3)
   ;; WHEN: Thu Mar 19 13:22:40 GMT 2020
   ;; MSG SIZE  rcvd: 962

Here is the problem: the parent zone is telling the world that
``example.net`` is using the key 14956, but the authoritative server
indicates that it is using keys 27247 and 35328. There are several
potential causes for this mismatch: one possibility is that a malicious
attacker has compromised one side and changed the data. A more likely
scenario is that the DNS administrator for the child zone did not upload
the correct key information to the parent zone.

.. _troubleshooting_incorrect_time:

Incorrect Time
^^^^^^^^^^^^^^

In DNSSEC, every record comes with at least one RRSIG, and each RRSIG
contains two timestamps: one indicating when it becomes valid, and
one when it expires. If the validating resolver's current system time does
not fall within the two RRSIG timestamps, error messages
appear in the BIND debug log.

The example below shows a log message when the RRSIG appears to have
expired. This could mean the validating resolver system time is
incorrectly set too far in the future, or the zone administrator has not
kept up with RRSIG maintenance.

::

   validating example.com/DNSKEY: verify failed due to bad signature (keyid=19036): RRSIG has expired

The log below shows that the RRSIG validity period has not yet begun. This could mean
the validation resolver's system time is incorrectly set too far in the past, or
the zone administrator has incorrectly generated signatures for this
domain name.

::

   validating example.com/DNSKEY: verify failed due to bad signature (keyid=4521): RRSIG validity period has not begun

.. _troubleshooting_unable_to_load_keys:

Unable to Load Keys
^^^^^^^^^^^^^^^^^^^

This is a simple yet common issue. If the key files are present but
unreadable by ``named`` for some reason, the ``syslog`` returns clear error
messages, as shown below:

::

   named[32447]: zone example.com/IN (signed): reconfiguring zone keys
   named[32447]: dns_dnssec_findmatchingkeys: error reading key file Kexample.com.+008+06817.private: permission denied
   named[32447]: dns_dnssec_findmatchingkeys: error reading key file Kexample.com.+008+17694.private: permission denied
   named[32447]: zone example.com/IN (signed): next key event: 27-Nov-2014 20:04:36.521

However, if no keys are found, the error is not as obvious. Below shows
the ``syslog`` messages after executing ``rndc
reload`` with the key files missing from the key directory:

::

   named[32516]: received control channel command 'reload'
   named[32516]: loading configuration from '/etc/bind/named.conf'
   named[32516]: reading built-in trusted keys from file '/etc/bind/bind.keys'
   named[32516]: using default UDP/IPv4 port range: [1024, 65535]
   named[32516]: using default UDP/IPv6 port range: [1024, 65535]
   named[32516]: sizing zone task pool based on 6 zones
   named[32516]: the working directory is not writable
   named[32516]: reloading configuration succeeded
   named[32516]: reloading zones succeeded
   named[32516]: all zones loaded
   named[32516]: running
   named[32516]: zone example.com/IN (signed): reconfiguring zone keys
   named[32516]: zone example.com/IN (signed): next key event: 27-Nov-2014 20:07:09.292

This happens to look exactly the same as if the keys were present and
readable, and appears to indicate that ``named`` loaded the keys and signed the zone. It
even generates the internal (raw) files:

::

   # cd /etc/bind/db
   # ls
   example.com.db  example.com.db.jbk  example.com.db.signed

If ``named`` really loaded the keys and signed the zone, you should see
the following files:

::

   # cd /etc/bind/db
   # ls
   example.com.db  example.com.db.jbk  example.com.db.signed  example.com.db.signed.jnl

So, unless you see the ``*.signed.jnl`` file, your zone has not been
signed.

.. _troubleshooting_invalid_trust_anchors:

Invalid Trust Anchors
^^^^^^^^^^^^^^^^^^^^^

In most cases, you never need to explicitly configure trust
anchors. ``named`` supplies the current root trust anchor and,
with the default setting of ``dnssec-validation``, updates it on the
infrequent occasions when it is changed.

However, in some circumstances you may need to explicitly configure
your own trust anchor. As we saw in the :ref:`trust_anchors_description`
section, whenever a DNSKEY is received by the validating resolver, it is
compared to the list of keys the resolver explicitly trusts to see if
further action is needed. If the two keys match, the validating resolver
stops performing further verification and returns the answer(s) as
validated.

But what if the key file on the validating resolver is misconfigured or
missing? Below we show some examples of log messages when things are not
working properly.

First of all, if the key you copied is malformed, BIND does not even
start and you will likely find this error message in syslog:

::

   named[18235]: /etc/bind/named.conf.options:29: bad base64 encoding
   named[18235]: loading configuration: failure

If the key is a valid base64 string but the key algorithm is incorrect,
or if the wrong key is installed, the first thing you will notice is
that virtually all of your DNS lookups result in SERVFAIL, even when
you are looking up domain names that have not been DNSSEC-enabled. Below
shows an example of querying a recursive server 10.53.0.3:

::

   $ dig @10.53.0.3 www.example.com. A

   ; <<>> DiG 9.16.0 <<>> @10.53.0.3 www.example.org A +dnssec
   ; (1 server found)
   ;; global options: +cmd
   ;; Got answer:
   ;; ->>HEADER<<- opcode: QUERY, status: SERVFAIL, id: 29586
   ;; flags: qr rd ra; QUERY: 1, ANSWER: 0, AUTHORITY: 0, ADDITIONAL: 1

   ;; OPT PSEUDOSECTION:
   ; EDNS: version: 0, flags: do; udp: 4096
   ; COOKIE: ee078fc321fa1367010000005e73a58bf5f205ca47e04bed (good)
   ;; QUESTION SECTION:
   ;www.example.org.       IN  A

``delv`` shows a similar result:

::

   $ delv @192.168.1.7 www.example.com. +rtrace
   ;; fetch: www.example.com/A
   ;; resolution failed: SERVFAIL

The next symptom you see is in the DNSSEC log messages:

::

   managed-keys-zone: DNSKEY set for zone '.' could not be verified with current keys
   validating ./DNSKEY: starting
   validating ./DNSKEY: attempting positive response validation
   validating ./DNSKEY: no DNSKEY matching DS
   validating ./DNSKEY: no DNSKEY matching DS
   validating ./DNSKEY: no valid signature found (DS)

These errors are indications that there are problems with the trust
anchor.

.. _troubleshooting_nta:

Negative Trust Anchors
~~~~~~~~~~~~~~~~~~~~~~

BIND 9.11 introduced Negative Trust Anchors (NTAs) as a means to
*temporarily* disable DNSSEC validation for a zone when you know that
the zone's DNSSEC is misconfigured.

NTAs are added using the ``rndc`` command, e.g.:

::

   $ rndc nta example.com
    Negative trust anchor added: example.com/_default, expires 19-Mar-2020 19:57:42.000
    

The list of currently configured NTAs can also be examined using
``rndc``, e.g.:

::

   $ rndc nta -dump
    example.com/_default: expiry 19-Mar-2020 19:57:42.000
    

The default lifetime of an NTA is one hour, although by default, BIND
polls the zone every five minutes to see if the zone correctly
validates, at which point the NTA automatically expires. Both the
default lifetime and the polling interval may be configured via
``named.conf``, and the lifetime can be overridden on a per-zone basis
using the ``-lifetime duration`` parameter to ``rndc nta``. Both timer
values have a permitted maximum value of one week.

.. _troubleshooting_nsec3:

NSEC3 Troubleshooting
~~~~~~~~~~~~~~~~~~~~~

BIND includes a tool called ``nsec3hash`` that runs through the same
steps as a validating resolver, to generate the correct hashed name
based on NSEC3PARAM parameters. The command takes the following
parameters in order: salt, algorithm, iterations, and domain. For
example, if the salt is 1234567890ABCDEF, hash algorithm is 1, and
iteration is 10, to get the NSEC3-hashed name for ``www.example.com`` we
would execute a command like this:

::

   $ nsec3hash 1234567890ABCEDF 1 10 www.example.com
   RN7I9ME6E1I6BDKIP91B9TCE4FHJ7LKF (salt=1234567890ABCEDF, hash=1, iterations=10)

While it is unlikely you would construct a rainbow table of your own
zone data, this tool may be useful when troubleshooting NSEC3 problems.
