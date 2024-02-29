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

.. _zone_file:

.. _soa_rr:

Zone File
---------

This section, largely borrowed from :rfc:`1034`, describes the concept of a
Resource Record (RR) and explains how to use them.

Resource Records
~~~~~~~~~~~~~~~~

A domain name identifies a node in the DNS tree namespace. Each node has a set of resource
information, which may be empty. The set of resource information
associated with a particular name is composed of separate RRs. The order
of RRs in a set is not significant and need not be preserved by name
servers, resolvers, or other parts of the DNS. However, sorting of
multiple RRs is permitted for optimization purposes: for example, to
specify that a particular nearby server be tried first. See
:any:`sortlist` and :ref:`rrset_ordering`.

The components of a Resource Record are:

.. glossary::

   owner name
      The domain name where the RR is found.

   RR type
      An encoded 16-bit value that specifies the type of the resource record.
      For a list of *types* of valid RRs, including those that have been obsoleted, please refer to
      `https://www.iana.org/assignments/dns-parameters/dns-parameters.xhtml#dns-parameters-4`.

   TTL
      The time-to-live of the RR. This field is a 32-bit integer in units of seconds,
      and is primarily used by resolvers when they cache RRs. The TTL describes how long
      a RR can be cached before it should be discarded.

   class
      An encoded 16-bit value that identifies a protocol family or an instance of a protocol.

   RDATA
      The resource data. The format of the data is type- and sometimes class-specific.


The following *classes* of resource records are currently valid in the
DNS:

.. glossary::

   IN
      The Internet. The only widely :term:`class` used today.

   CH
      Chaosnet, a LAN protocol created at MIT in the mid-1970s. It was rarely used for its historical purpose, but was reused for BIND's built-in server information zones, e.g., **version.bind**.

   HS
      Hesiod, an information service developed by MIT's Project Athena. It was used to share information about various systems databases, such as users, groups, printers, etc.

The :term:`owner name` is often implicit, rather than forming an integral part
of the RR. For example, many name servers internally form tree or hash
structures for the name space, and chain RRs off nodes. The remaining RR
parts are the fixed header (type, class, TTL), which is consistent for
all RRs, and a variable part (RDATA) that fits the needs of the resource
being described.

The TTL field is a time limit on how long an RR can be
kept in a cache. This limit does not apply to authoritative data in
zones; that also times out, but follows the refreshing policies for the
zone. The TTL is assigned by the administrator for the zone where the
data originates. While short TTLs can be used to minimize caching, and a
zero TTL prohibits caching, the realities of Internet performance
suggest that these times should be on the order of days for the typical
host. If a change is anticipated, the TTL can be reduced prior to
the change to minimize inconsistency, and then
increased back to its former value following the change.

The data in the RDATA section of RRs is carried as a combination of
binary strings and domain names. The domain names are frequently used as
"pointers" to other data in the DNS.

.. _rr_text:

Textual Expression of RRs
^^^^^^^^^^^^^^^^^^^^^^^^^

RRs are represented in binary form in the packets of the DNS protocol,
and are usually represented in highly encoded form when stored in a name
server or resolver. In the examples provided in :rfc:`1034`, a style
similar to that used in primary files was employed in order to show the
contents of RRs. In this format, most RRs are shown on a single line,
although continuation lines are possible using parentheses.

The start of the line gives the owner of the RR. If a line begins with a
blank, then the owner is assumed to be the same as that of the previous
RR. Blank lines are often included for readability.

Following the owner are listed the TTL, type, and class of the RR. Class
and type use the mnemonics defined above, and TTL is an integer before
the type field. To avoid ambiguity in parsing, type and class
mnemonics are disjoint, TTLs are integers, and the type mnemonic is
always last. The IN class and TTL values are often omitted from examples
in the interest of clarity.

The resource data or RDATA section of the RR is given using knowledge
of the typical representation for the data.

For example, the RRs carried in a message might be shown as:

 +---------------------+---------------+--------------------------------+
 | **ISI.EDU.**        | **MX**        | **10 VENERA.ISI.EDU.**         |
 +---------------------+---------------+--------------------------------+
 |                     | **MX**        | **10 VAXA.ISI.EDU**            |
 +---------------------+---------------+--------------------------------+
 | **VENERA.ISI.EDU**  | **A**         | **128.9.0.32**                 |
 +---------------------+---------------+--------------------------------+
 |                     | **A**         | **10.1.0.52**                  |
 +---------------------+---------------+--------------------------------+
 | **VAXA.ISI.EDU**    | **A**         | **10.2.0.27**                  |
 +---------------------+---------------+--------------------------------+
 |                     | **A**         | **128.9.0.33**                 |
 +---------------------+---------------+--------------------------------+

The MX RRs have an RDATA section which consists of a 16-bit number
followed by a domain name. The address RRs use a standard IP address
format to contain a 32-bit Internet address.

The above example shows six RRs, with two RRs at each of three domain
names.

Here is another possible example:

 +----------------------+---------------+-------------------------------+
 | **XX.LCS.MIT.EDU.**  | **IN A**      | **10.0.0.44**                 |
 +----------------------+---------------+-------------------------------+
 |                      | **CH A**      | **MIT.EDU. 2420**             |
 +----------------------+---------------+-------------------------------+

This shows two addresses for **XX.LCS.MIT.EDU**, each of a
different class.

.. _mx_records:

Discussion of MX Records
~~~~~~~~~~~~~~~~~~~~~~~~

As described above, domain servers store information as a series of
resource records, each of which contains a particular piece of
information about a given domain name (which is usually, but not always,
a host). The simplest way to think of an RR is as a typed pair of data, a
domain name matched with a relevant datum and stored with some
additional type information, to help systems determine when the RR is
relevant.

MX records are used to control delivery of email. The data specified in
the record is a priority and a domain name. The priority controls the
order in which email delivery is attempted, with the lowest number
first. If two priorities are the same, a server is chosen randomly. If
no servers at a given priority are responding, the mail transport agent
falls back to the next largest priority. Priority numbers do not
have any absolute meaning; they are relevant only respective to other
MX records for that domain name. The domain name given is the machine to
which the mail is delivered. It *must* have an associated address
record (A or AAAA); CNAME is not sufficient.

For a given domain, if there is both a CNAME record and an MX record,
the MX record is in error and is ignored. Instead, the mail is
delivered to the server specified in the MX record pointed to by the
CNAME. For example:

 +------------------------+--------+--------+--------------+------------------------+
 | **example.com.**       | **IN** | **MX** | **10**       | **mail.example.com.**  |
 +------------------------+--------+--------+--------------+------------------------+
 |                        | **IN** | **MX** | **10**       | **mail2.example.com.** |
 +------------------------+--------+--------+--------------+------------------------+
 |                        | **IN** | **MX** | **20**       | **mail.backup.org.**   |
 +------------------------+--------+--------+--------------+------------------------+
 | **mail.example.com.**  | **IN** | **A**  | **10.0.0.1** |                        |
 +------------------------+--------+--------+--------------+------------------------+
 | **mail2.example.com.** | **IN** | **A**  | **10.0.0.2** |                        |
 +------------------------+--------+--------+--------------+------------------------+

Mail delivery is attempted to **mail.example.com** and
**mail2.example.com** (in any order); if neither of those succeeds,
delivery to **mail.backup.org** is attempted.

.. _Setting_TTLs:

Setting TTLs
~~~~~~~~~~~~

The time-to-live (TTL) of the RR field is a 32-bit integer represented in
units of seconds, and is primarily used by resolvers when they cache
RRs. The TTL describes how long an RR can be cached before it should be
discarded. The following three types of TTLs are currently used in a zone
file.

.. glossary::

   SOA minimum
       The last field in the SOA is the negative caching TTL.
       This controls how long other servers cache no-such-domain (NXDOMAIN)
       responses from this server. Further details can be found in :rfc:`2308`.

       The maximum time for negative caching is 3 hours (3h).

   $TTL
       The $TTL directive at the top of the zone file (before the SOA) gives a default TTL for every RR without a specific TTL set.

   RR TTLs
       Each RR can have a TTL as the second field in the RR, which controls how long other servers can cache it.

All of these TTLs default to units of seconds, though units can be
explicitly specified: for example, **1h30m**.

.. _ipv4_reverse:

Inverse Mapping in IPv4
~~~~~~~~~~~~~~~~~~~~~~~

Reverse name resolution (that is, translation from IP address to name)
is achieved by means of the **in-addr.arpa** domain and PTR records.
Entries in the in-addr.arpa domain are made in least-to-most significant
order, read left to right. This is the opposite order to the way IP
addresses are usually written. Thus, a machine with an IP address of
10.1.2.3 would have a corresponding in-addr.arpa name of
3.2.1.10.in-addr.arpa. This name should have a PTR resource record whose
data field is the name of the machine or, optionally, multiple PTR
records if the machine has more than one name. For example, in the
**example.com** domain:

 +--------------+-------------------------------------------------------+
 | **$ORIGIN**  | **2.1.10.in-addr.arpa**                               |
 +--------------+-------------------------------------------------------+
 | **3**        | **IN PTR foo.example.com.**                           |
 +--------------+-------------------------------------------------------+

.. note::

   The **$ORIGIN** line in this example is only to provide context;
   it does not necessarily appear in the actual
   usage. It is only used here to indicate that the example is
   relative to the listed origin.

.. _zone_directives:

Other Zone File Directives
~~~~~~~~~~~~~~~~~~~~~~~~~~

The DNS "master file" format was initially defined in :rfc:`1035` and has
subsequently been extended. While the format itself is class-independent,
all records in a zone file must be of the same class.

Master file directives include **$ORIGIN**, **$INCLUDE**, and **$TTL.**

.. _atsign:

The **@** (at-sign)
^^^^^^^^^^^^^^^^^^^

When used in the label (or name) field, the asperand or at-sign (@)
symbol represents the current origin. At the start of the zone file, it
is the <**zone_name**>, followed by a trailing dot (.).

.. _origin_directive:

The **$ORIGIN** Directive
^^^^^^^^^^^^^^^^^^^^^^^^^

Syntax: **$ORIGIN** domain-name [comment]

**$ORIGIN** sets the domain name that is appended to any
unqualified records. When a zone is first read, there is an implicit
``$ORIGIN <zone_name>.``; note the trailing dot. The
current **$ORIGIN** is appended to the domain specified in the
**$ORIGIN** argument if it is not absolute.

::

   $ORIGIN example.com.
   WWW     CNAME   MAIN-SERVER

is equivalent to

::

   WWW.EXAMPLE.COM. CNAME MAIN-SERVER.EXAMPLE.COM.

The **$INCLUDE** Directive
^^^^^^^^^^^^^^^^^^^^^^^^^^

Syntax: **$INCLUDE** filename [origin] [comment]

This reads and processes the file **filename** as if it were included in the
file at this point. The **filename** can be an absolute path, or a relative
path. In the latter case it is read from :iscman:`named`'s working directory. If
**origin** is specified, the file is processed with **$ORIGIN** set to that
value; otherwise, the current **$ORIGIN** is used.

The origin and the current domain name revert to the values they had
prior to the **$INCLUDE** once the file has been read.

.. note::

   :rfc:`1035` specifies that the current origin should be restored after
   an **$INCLUDE**, but it is silent on whether the current domain name
   should also be restored. BIND 9 restores both of them. This could be
   construed as a deviation from :rfc:`1035`, a feature, or both.

.. _ttl_directive:

The **$TTL** Directive
^^^^^^^^^^^^^^^^^^^^^^

Syntax: **$TTL** default-ttl [comment]

This sets the default Time-To-Live (TTL) for subsequent records with undefined
TTLs. Valid TTLs are of the range 0-2147483647 seconds.

**$TTL** is defined in :rfc:`2308`.

.. _generate_directive:

BIND Primary File Extension: the **$GENERATE** Directive
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Syntax: **$GENERATE** range owner [ttl] [class] type rdata [comment]

**$GENERATE** is used to create a series of resource records that only
differ from each other by an iterator.

**range**
    This can be one of two forms: start-stop or start-stop/step.
    If the first form is used, then step is set to 1. "start",
    "stop", and "step" must be positive integers between 0 and
    (2^31)-1. "start" must not be larger than "stop".

**owner**
    This describes the owner name of the resource records to be created.

    The **owner** string may include one or more **$** (dollar sign)
    symbols, which will be replaced with the iterator value when
    generating records; see below for details.

**ttl**
    This specifies the time-to-live of the generated records. If
    not specified, this is inherited using the normal TTL inheritance
    rules.

    **class** and **ttl** can be entered in either order.

**class**
    This specifies the class of the generated records. This must
    match the zone class if it is specified.

    **class** and **ttl** can be entered in either order.

**type**
    This can be any valid type.

**rdata**
    This is a string containing the RDATA of the resource record
    to be created. As with **owner**, the **rdata** string may
    include one or more **$** symbols, which are replaced with the
    iterator value. **rdata** may be quoted if there are spaces in
    the string; the quotation marks do not appear in the generated
    record.

    Any single **$** (dollar sign) symbols within the **owner** or
    **rdata** strings are replaced by the iterator value. To get a **$**
    in the output, escape the **$** using a backslash **\\**, e.g.,
    ``\$``. (For compatibility with earlier versions, **$$** is also
    recognized as indicating a literal **$** in the output.)

    The **$** may optionally be followed by modifiers which change
    the offset from the iterator, field width, and base.  Modifiers
    are introduced by a **{** (left brace) immediately following
    the **$**, as in  **${offset[,width[,base]]}**. For example,
    **${-20,3,d}** subtracts 20 from the current value and prints
    the result as a decimal in a zero-padded field of width 3.
    Available output forms are decimal (**d**), octal (**o**),
    hexadecimal (**x** or **X** for uppercase), and nibble (**n**
    or **N** for uppercase). The modfiier cannot contain whitespace
    or newlines.

    The default modifier is **${0,0,d}**. If the **owner** is not
    absolute, the current **$ORIGIN** is appended to the name.

    In nibble mode, the value is treated as if it were a reversed
    hexadecimal string, with each hexadecimal digit as a separate
    label. The width field includes the label separator.

Examples:

**$GENERATE** can be used to easily generate the sets of records required
to support sub-/24 reverse delegations described in :rfc:`2317`:

::

   $ORIGIN 0.0.192.IN-ADDR.ARPA.
   $GENERATE 1-2 @ NS SERVER$.EXAMPLE.
   $GENERATE 1-127 $ CNAME $.0

is equivalent to

::

   0.0.0.192.IN-ADDR.ARPA. NS SERVER1.EXAMPLE.
   0.0.0.192.IN-ADDR.ARPA. NS SERVER2.EXAMPLE.
   1.0.0.192.IN-ADDR.ARPA. CNAME 1.0.0.0.192.IN-ADDR.ARPA.
   2.0.0.192.IN-ADDR.ARPA. CNAME 2.0.0.0.192.IN-ADDR.ARPA.
   ...
   127.0.0.192.IN-ADDR.ARPA. CNAME 127.0.0.0.192.IN-ADDR.ARPA.

This example creates a set of A and MX records. Note the MX's **rdata**
is a quoted string; the quotes are stripped when **$GENERATE** is processed:

::

   $ORIGIN EXAMPLE.
   $GENERATE 1-127 HOST-$ A 1.2.3.$
   $GENERATE 1-127 HOST-$ MX "0 ."

is equivalent to

::

   HOST-1.EXAMPLE.   A  1.2.3.1
   HOST-1.EXAMPLE.   MX 0 .
   HOST-2.EXAMPLE.   A  1.2.3.2
   HOST-2.EXAMPLE.   MX 0 .
   HOST-3.EXAMPLE.   A  1.2.3.3
   HOST-3.EXAMPLE.   MX 0 .
   ...
   HOST-127.EXAMPLE. A  1.2.3.127
   HOST-127.EXAMPLE. MX 0 .


This example generates A and AAAA records using modifiers; the AAAA
**owner** names are generated using nibble mode:

::

   $ORIGIN EXAMPLE.
   $GENERATE 0-2 HOST-${0,4,d} A 1.2.3.${1,0,d}
   $GENERATE 1024-1026 ${0,3,n} AAAA 2001:db8::${0,4,x}

is equivalent to:

::

   HOST-0000.EXAMPLE.   A      1.2.3.1
   HOST-0001.EXAMPLE.   A      1.2.3.2
   HOST-0002.EXAMPLE.   A      1.2.3.3
   0.0.4.EXAMPLE.       AAAA   2001:db8::400
   1.0.4.EXAMPLE.       AAAA   2001:db8::401
   2.0.4.EXAMPLE.       AAAA   2001:db8::402

The **$GENERATE** directive is a BIND extension and not part of the
standard zone file format.

.. _zonefile_format:

Additional File Formats
~~~~~~~~~~~~~~~~~~~~~~~

In addition to the standard text format, BIND 9 supports the ability
to read or dump to zone files in other formats.

The **raw** format is a binary representation of zone data in a manner
similar to that used in zone transfers. Since it does not require
parsing text, load time is significantly reduced.

For a primary server, a zone file in **raw** format is expected
to be generated from a text zone file by the :iscman:`named-compilezone` command.
For a secondary server or a dynamic zone, the zone file is automatically
generated when :iscman:`named` dumps the zone contents after zone transfer or
when applying prior updates, if one of these formats is specified by the
**masterfile-format** option.

If a zone file in **raw** format needs manual modification, it first must
be converted to **text** format by the :iscman:`named-compilezone` command,
then converted back after editing.  For example:

::

    named-compilezone -f raw -F text -o zonefile.text <origin> zonefile.raw
    [edit zonefile.text]
    named-compilezone -f text -F raw -o zonefile.raw <origin> zonefile.text
