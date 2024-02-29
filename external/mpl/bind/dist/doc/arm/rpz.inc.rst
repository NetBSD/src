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

.. highlight:: none

.. dns_firewalls_rpz:

DNS Firewalls and Response Policy Zones
---------------------------------------

A DNS firewall examines DNS traffic and allows some responses to pass
through while blocking others. This examination can be based on several
criteria, including the name requested, the data (such as an IP address)
associated with that name, or the name or IP address of the name server
that is authoritative for the requested name.  Based on these criteria, a
DNS firewall can be configured to discard, modify, or replace the original
response, allowing administrators more control over what systems can access
or be accessed from their networks.

DNS Response Policy Zones (RPZ) are a form of DNS firewall in which the
firewall rules are expressed within the DNS itself - encoded in an open,
vendor-neutral format as records in specially constructed DNS zones.

Using DNS zones to configure policy allows policy to be shared from
one server to another using the standard DNS zone transfer mechanism.
This allows a DNS operator to maintain their own firewall policies and
share them easily amongst their internal name servers, or to subscribe to
external firewall policies such as commercial or cooperative "threat
feeds," or both.

:iscman:`named` can subscribe to up to 64 Response Policy Zones, each of which
encodes a separate policy rule set.  Each rule is stored in a DNS resource
record set (RRset) within the RPZ, and consists of a **trigger** and an
**action**.  There are five types of triggers and six types of actions.

A response policy rule in a DNS RPZ can be triggered as follows:

- by the IP address of the client
- by the query name
- by an address which would be present in a truthful response
- by the name or address of an authoritative name server responsible for
  publishing the original response

A response policy action can be one of the following:

- to synthesize a "domain does not exist" (NXDOMAIN) response
- to synthesize a "name exists but there are no records of the requested
  type" (NODATA) response
- to drop the response
- to switch to TCP by sending a truncated UDP response that requires the
  DNS client to try again with TCP
- to replace/override the response's data with specific data (provided
  within the response policy zone)
- to exempt the response from further policy processing

The most common use of a DNS firewall is to "poison" a domain name, IP
address, name server name, or name server IP address. Poisoning is usually
done by forcing a synthetic "domain does not exist" (NXDOMAIN) response.
This means that if an administrator maintains a list of known "phishing"
domains, these names can be made unreachable by customers or end users just
by adding a firewall policy into the recursive DNS server, with a trigger
for each known "phishing" domain, and an action in every case forcing a
synthetic NXDOMAIN response. It is also possible to use a data-replacement
action such as answering for these known "phishing" domains with the name
of a local web server that can display a warning page. Such a web server
would be called a "walled garden."

.. note::

  Authoritative name servers can be responsible for many different domains.
  If DNS RPZ is used to poison all domains served by some authoritative
  name server name or address, the effects can be quite far-reaching. Users
  are advised to ensure that such authoritative name servers do not also
  serve domains that should not be poisoned.

.. _why_dns_firewall:

Why Use a DNS Firewall?
~~~~~~~~~~~~~~~~~~~~~~~

Criminal and network abuse traffic on the Internet often uses the Domain
Name System (DNS), so protection against these threats should include DNS
firewalling.  A DNS firewall can selectively intercept DNS queries for
known network assets including domain names, IP addresses, and name
servers. Interception can mean rewriting a DNS response to direct a web
browser to a "walled garden," or simply making any malicious network assets
invisible and unreachable.

.. _what_dns_firewalls_do:

What Can a DNS Firewall Do?
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Firewalls work by applying a set of rules to a traffic flow, where each
rule consists of a trigger and an action. Triggers determine which messages
within the traffic flow are handled specially, and actions determine what
that special handling is. For a DNS firewall, the traffic flow to be
controlled consists of responses sent by a recursive DNS server to its
end-user clients. Some true responses are not safe for all clients, so the
policy rules in a DNS firewall allow some responses to be intercepted and
replaced with safer content.

.. _rpz_rule_sets:

Creating and Maintaining RPZ Rule Sets
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In DNS RPZ, the DNS firewall policy rule set is stored in a DNS zone, which
is maintained and synchronized using the same tools and methods as for any
other DNS zone. The primary name server for a DNS RPZ may be an internal
server, if an administrator is creating and maintaining their own DNS
policy zone, or it may be an external name server (such as a security
vendor's server), if importing a policy zone published externally. The
primary copy of the DNS firewall policy can be a DNS "zone file" which is
either edited by hand or generated from a database. A DNS zone can also be
edited indirectly using DNS dynamic updates (for example, using the
"nsupdate" shell level utility).

DNS RPZ allows firewall rules to be expressed in a DNS zone format and then
carried to subscribers as DNS data. A recursive DNS server which is capable
of processing DNS RPZ synchronizes these DNS firewall rules using the same
standard DNS tools and protocols used for secondary name service. The DNS
policy information is then promoted to the DNS control plane inside the
customer's DNS resolver, making that server into a DNS firewall.

A security company whose products include threat intelligence feeds can use
a DNS Response Policy Zone (RPZ) as a delivery channel to customers.
Threats can be expressed as known-malicious IP addresses and subnets,
known-malicious domain names, and known-malicious domain name servers. By
feeding this threat information directly into customers' local DNS
resolvers, providers can transform these DNS servers into a distributed DNS
firewall.

When a customer's DNS resolver is connected by a realtime subscription to a
threat intelligence feed, the provider can protect the customer's end users
from malicious network elements (including IP addresses and subnets, domain
names, and name servers) immediately as they are discovered. While it may
take days or weeks to "take down" criminal and abusive infrastructure once
reported, a distributed DNS firewall can respond instantly.

Other distributed TCP/IP firewalls have been on the market for many years,
and enterprise users are now comfortable importing real-time threat
intelligence from their security vendors directly into their firewalls.
This intelligence can take the form of known-malicious IP addresses or
subnets, or of patterns which identify known-malicious email attachments,
file downloads, or web addresses (URLs). In some products it is also
possible to block DNS packets based on the names or addresses they carry.

.. _rpz_limitations:

Limitations of DNS RPZ
~~~~~~~~~~~~~~~~~~~~~~

We're often asked if DNS RPZ could be used to set up redirection to a CDN.
For example, if "mydomain.com" is a normal domain with SOA, NS, MX, TXT
records etc., then if someone sends an A or AAAA query for "mydomain.com",
can we use DNS RPZ on an authoritative nameserver to return "CNAME
mydomain.com.my-cdn-provider.net"?

The problem with this suggestion is that there is no way to CNAME only A
and AAAA queries, not even with RPZ.

The underlying reason is that if the authoritative server answers with a
CNAME, the recursive server making that query will cache the response.
Thereafter (while the CNAME is still in cache), it assumes that there are
no records of any non-CNAME type for the name that was being queried, and
directs subsequent queries for all other types directly to the target name
of the CNAME record.

To be clear, this is not a limitation of RPZ; it is a function of the way
the DNS protocol works. It's simply not possible to use "partial" CNAMES to
help when setting up CDNs because doing this will break other functionality
such as email routing.

Similarly, following the DNS protocol definition, wildcards in the form of
``*.example`` records might behave in unintuitive ways. For a detailed
definition of wildcards in DNS, please see :rfc:`4592`, especially section 2.

.. _dns_firewall_examples:

DNS Firewall Usage Examples
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Here are some scenarios in which a DNS firewall might be useful.

Some known threats are based on an IP address or subnet (IP address range).
For example, an analysis may show that all addresses in a "class C" network
are used by a criminal gang for "phishing" web servers. With a DNS firewall
based on DNS RPZ, a firewall policy can be created such as "if a DNS lookup
would result in an address from this class C network, then answer instead
with an NXDOMAIN indication." That simple rule would prevent any end users
inside customers' networks from being able to look up any domain name used
in this phishing attack – without having to know in advance what those
names might be.

Other known threats are based on domain names. An analysis may determine
that a certain domain name or set of domain names is being or will shortly
be used for spamming, phishing, or other Internet-based attacks which all
require working domain names. By adding name-triggered rules to a
distributed DNS firewall, providers can protect customers' end users from
any attacks which require them to be able to look up any of these malicious
names. The names can be wildcards (for example, \*.evil.com), and these
wildcards can have exceptions if some domains are not as malicious as
others (if \*.evil.com is bad, then not.evil.com might be an exception).

Alongside growth in electronic crime has come growth of electronic criminal
expertise. Many criminal gangs now maintain their own extensive DNS
infrastructure to support a large number of domain names and a diverse set
of IP addressing resources. Analyses show in many cases that the only truly
fixed assets criminal organizations have are their name servers, which are
by nature slightly less mobile than other network assets. In such cases,
DNS administrators can anchor their DNS firewall policies in the abusive
name server names or name server addresses, and thus protect their
customers' end users from threats where neither the domain name nor the IP
address of that threat is known in advance.

Electronic criminals rely on the full resiliency of DNS just as the rest of
digital society does. By targeting criminal assets at the DNS level we can
deny these criminals the resilience they need. A distributed DNS firewall
can leverage the high skills of a security company to protect a large
number of end users. DNS RPZ, as the first open and vendor-neutral
distributed DNS firewall, can be an effective way to deliver threat
intelligence to customers.

A Real-World Example of DNS RPZ's Value
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The Conficker malware worm (https://en.wikipedia.org/wiki/Conficker) was
first detected in 2008. Although it is no longer an active threat, the
techniques described here can be applied to other DNS threats.

Conficker used a domain generation algorithm (DGA) to choose up to 50,000
command and control domains per day. It would be impractical to create
an RPZ that contains so many domain names and changes so much on a daily
basis. Instead, we can trigger RPZ rules based on the names of the name
servers that are authoritative for the command and control domains, rather
than trying to trigger on each of 50,000 different (daily) query names.
Since the well-known name server names for Conficker's domain names are
never used by nonmalicious domains, it is safe to poison all lookups that
rely on these name servers.  Here is an example that achieves this result:

::

  $ORIGIN rpz.example.com.
  ns.0xc0f1c3a5.com.rpz-nsdname  CNAME  *.walled-garden.example.com.
  ns.0xc0f1c3a5.net.rpz-nsdname  CNAME  *.walled-garden.example.com.
  ns.0xc0f1c3a5.org.rpz-nsdname  CNAME  *.walled-garden.example.com.

The ``*`` at the beginning of these CNAME target names is special, and it
causes the original query name to be prepended to the CNAME target. So if a
user tries to visit the Conficker command and control domain
http://racaldftn.com.ai/ (which was a valid Conficker command and control
domain name on 19-October-2011), the RPZ-configured recursive name server
will send back this answer:

::

  racaldftn.com.ai.     CNAME     racaldftn.com.ai.walled-garden.example.com.
  racaldftn.com.ai.walled-garden.example.com.     A      192.168.50.3

This example presumes that the following DNS content has also been created,
which is not part of the RPZ zone itself but is in another domain:

::

  $ORIGIN walled-garden.example.com.
  *     A     192.168.50.3

Assuming that we're running a web server listening on 192.168.50.3 that
always displays a warning message no matter what uniform resource
identifier (URI) is used, the above RPZ configuration will instruct the web
browser of any infected end user to connect to a "server name" consisting
of their original lookup name (racaldftn.com.ai) prepended to the walled
garden domain name (walled-garden.example.com). This is the name that will
appear in the web server's log file, and having the full name in that log
file will facilitate an analysis as to which users are infected with what
virus.

.. _firewall_updates:

Keeping Firewall Policies Updated
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

It is vital for overall system performance that incremental zone transfers
(see :rfc:`1995`) and real-time change notification (see :rfc:`1996`) be
used to synchronize DNS firewall rule sets between the publisher's primary
copy of the rule set and the subscribers' working copies of the rule set.

If DNS dynamic updates are used to maintain a DNS RPZ rule set, the name
server automatically calculates a stream of deltas for use when sending
incremental zone transfers to the subscribing name servers. Sending a
stream of deltas – known as an "incremental zone transfer" or IXFR – is
usually much faster than sending the full zone every time it changes, so
it's worth the effort to use an editing method that makes such incremental
transfers possible.

Administrators who edit or periodically regenerate a DNS RPZ rule set and
whose primary name server uses BIND can enable the
:any:`ixfr-from-differences` option, which tells the primary name server to
calculate the differences between each new zone and the preceding version,
and to make these differences available as a stream of deltas for use in
incremental zone transfers to the subscribing name servers. This will look
something like the following:

.. code-block:: c

       options {
                 // ...
                 ixfr-from-differences yes;
                 // ...
       };

As mentioned above, the simplest and most common use of a DNS firewall is
to poison domain names known to be purely malicious, by simply making them
disappear. All DNS RPZ rules are expressed as resource record sets
(RRsets), and the way to express a "force a name-does-not-exist condition"
is by adding a CNAME pointing to the root domain (``.``). In practice this
looks like:

::

  $ORIGIN rpz.example.com.
  malicious1.org          CNAME .
  *.malicious1.org        CNAME .
  malicious2.org          CNAME .
  *.malicious2.org        CNAME .

Two things are noteworthy in this example. First, the malicious names are
made relative within the response policy zone. Since there is no trailing
dot following ".org" in the above example, the actual RRsets created within
this response policy zone are, after expansion:

::

  malicious1.org.rpz.example.com.         CNAME .
  *.malicious1.org.rpz.example.com.       CNAME .
  malicious2.org.rpz.example.com.         CNAME .
  *.malicious2.org.rpz.example.com.       CNAME .

Second, for each name being poisoned, a wildcard name is also listed.
This is because a malicious domain name probably has or may potentially
have malicious subdomains.

In the above example, the relative domain names `malicious1.org` and
`malicious2.org` will match only the real domain names `malicious1.org`
and `malicious2.org`, respectively. The relative domain names
`*.malicious1.org` and `*.malicious2.org` will match any
`subdomain.of.malicious1.org` or `subdomain.of.malicious2.org`,
respectively.

This example forces an NXDOMAIN condition as its policy action, but other
policy actions are also possible.

.. _multiple_rpz_performance:

Performance and Scalability When Using Multiple RPZs
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Since version 9.10, BIND can be configured to have different response
policies depending on the identity of the querying client and the nature of
the query. To configure BIND response policy, the information is placed
into a zone file whose only purpose is conveying the policy information to
BIND. A zone file containing response policy information is called a
Response Policy Zone, or RPZ, and the mechanism in BIND that uses the
information in those zones is called DNS RPZ.

It is possible to use as many as 64 separate RPZ files in a single instance
of BIND, and BIND is not significantly slowed by such heavy use of RPZ.

(Note: by default, BIND 9.11 only supports up to 32 RPZ files, but this
can be increased to 64 at compile time. All other supported versions of
BIND support 64 by default.)

Each one of the policy zone files can specify policy for as many
different domains as necessary. The limit of 64 is on the number of
independently-specified policy collections and not the number of zones
for which they specify policy.

Policy information from all of the policy zones together are stored in a
special data structure allowing simultaneous lookups across all policy
zones to be performed very rapidly. Looking up a policy rule is
proportional to the logarithm of the number of rules in the largest
single policy zone.

.. _rpz_practical_tips:

Practical Tips for DNS Firewalls and DNS RPZ
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Administrators who subscribe to an externally published DNS policy zone and
who have a large number of internal recursive name servers should create an
internal name server called a "distribution master" (DM). The DM is a
secondary (stealth secondary) name server from the publisher's point of
view; that is, the DM is fetching zone content from the external server.
The DM is also a primary name server from the internal recursive name
servers' point of view: they fetch zone content from the DM.  In this
configuration the DM is acting as a gateway between the external publisher
and the internal subscribers.

The primary server must know the unicast listener address of every
subscribing recursive server, and must enumerate all of these addresses as
destinations for real time zone change notification (as described in
:rfc:`1996`). So if an enterprise-wide RPZ is called "rpz.example.com" and
if the unicast listener addresses of four of the subscribing recursive name
servers are 192.0.200.1, 192.0.201.1, 192.0.202.1, and 192.0.203.1, the
primary server's configuration looks like this:

.. code-block:: c

  zone "rpz.example.com" {
       type primary;
       file "primary/rpz.example.com";
       notify explicit;
       also-notify { 192.0.200.1;
                     192.0.201.1;
                     192.0.202.1;
                     192.0.203.1; };
       allow-transfer { 192.0.200.1;
                        192.0.201.1;
                        192.0.202.1;
                        192.0.203.1; };
       allow-query { localhost; };
  };

Each recursive DNS server that subscribes to the policy zone must be
configured as a secondary server for the zone, and must also be configured
to use the policy zone for local response policy. To subscribe a recursive
name server to a response policy zone where the unicast listener address
of the primary server is 192.0.220.2, the server's configuration should
look like this:

.. code-block:: c

  options {
       // ...
       response-policy {
            zone "rpz.example.com";
       };
       // ...
  };

  zone "rpz.example.com";
       type secondary;
       primaries { 192.0.222.2; };
       file "secondary/rpz.example.com";
       allow-query { localhost; };
       allow-transfer { none; };
  };

Note that queries are restricted to "localhost," since query access is
never used by DNS RPZ itself, but may be useful to DNS operators for use in
debugging. Transfers should be disallowed to prevent policy information
leaks.

If an organization's business continuity depends on full connectivity with
another company whose ISP also serves some criminal or abusive customers,
it's possible that one or more external RPZ providers – that is, security
feed vendors – may eventually add some RPZ rules that could hurt a
company's connectivity to its business partner. Users can protect
themselves from this risk by using an internal RPZ in addition to any
external RPZs, and by putting into their internal RPZ some "pass-through"
rules to prevent any policy action from affecting a DNS response that
involves a business partner.

A recursive DNS server can be connected to more than one RPZ, and these are
searched in order. Therefore, to protect a network from dangerous policies
which may someday appear in external RPZ zones, administrators should list
the internal RPZ zones first.


.. code-block:: c

  options {
       // ...
       response-policy {
            zone "rpz.example.com";
            zone "rpz.security-vendor-1.com";
            zone "rpz.security-vendor-2.com";
       };
       // ...
  };

Within an internal RPZ, there need to be rules describing the network
assets of business partners whose communications need to be protected.
Although it is not generally possible to know what domain names they use,
administrators will be aware of what address space they have and perhaps
what name server names they use.

::

  $ORIGIN rpz.example.com.
  8.0.0.0.10.rpz-ip                CNAME   rpz-passthru.
  16.0.0.45.128.rpz-nsip           CNAME   rpz-passthru.
  ns.partner1.com.rpz-nsdname      CNAME   rpz-passthru.
  ns.partner2.com.rpz-nsdname      CNAME   rpz-passthru.

Here, we know that answers in address block 10.0.0.0/8 indicate a business
partner, as well as answers involving any name server whose address is in
the 128.45.0.0/16 address block, and answers involving the name servers
whose names are ns.partner1.com or ns.partner2.com.

The above example demonstrates that when matching by answer IP address (the
.rpz-ip owner), or by name server IP address (the .rpz-nsip owner) or by
name server domain name (the .rpz-nsdname owner), the special RPZ marker
(.rpz-ip, .rpz-nsip, or .rpz-nsdname) does not appear as part of the CNAME
target name.

By triggering these rules using the known network assets of a partner,
and using the "pass-through" policy action, no later RPZ processing
(which in the above example refers to the "rpz.security-vendor-1.com" and
"rpz.security-vendor-2.com" policy zones) will have any effect on DNS
responses for partner assets.

.. _walled_garden_ip_address:

Creating a Simple Walled Garden Triggered by IP Address
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

It may be the case that the only thing known about an attacker is the IP
address block they are using for their "phishing" web servers. If the
domain names and name servers they use are unknown, but it is known that
every one of their "phishing" web servers is within a small block of IP
addresses, a response can be triggered on all answers that would include
records in this address range, using RPZ rules that look like the following
example:

::

  $ORIGIN rpz.example.com.
  22.0.212.94.109.rpz-ip          CNAME   drop.garden.example.com.
  *.212.94.109.in-addr.arpa       CNAME   .
  *.213.94.109.in-addr.arpa       CNAME   .
  *.214.94.109.in-addr.arpa       CNAME   .
  *.215.94.109.in-addr.arpa       CNAME   .

Here, if a truthful answer would include an A (address) RR (resource
record) whose value were within the 109.94.212.0/22 address block, then a
synthetic answer is sent instead of the truthful answer. Assuming the query
is for www.malicious.net, the synthetic answer is:

::

  www.malicious.net.              CNAME   drop.garden.example.com.
  drop.garden.example.com.        A       192.168.7.89

This assumes that `drop.garden.example.com` has been created as real DNS
content, outside of the RPZ:

::

  $ORIGIN example.com.
  drop.garden                     A       192.168.7.89

In this example, there is no "\*" in the CNAME target name, so the original
query name will not be present in the walled garden web server's log file.
This is an undesirable loss of information, and is shown here for example
purposes only.

The above example RPZ rules would also affect address-to-name (also
known as "reverse DNS") lookups for the unwanted addresses. If a mail
or web server receives a connection from an address in the example's
109.94.212.0/22 address block, it will perform a PTR record lookup to
find the domain name associated with that IP address.

This kind of address-to-name translation is usually used for diagnostic or
logging purposes, but it is also common for email servers to reject any
email from IP addresses which have no address-to-name translation. Most
mail from such IP addresses is spam, so the lack of a PTR record here has
some predictive value.  By using the "force name-does-not-exist" policy
trigger on all lookups in the PTR name space associated with an address
block, DNS administrators can give their servers a hint that these IP
addresses are probably sending junk.

.. _known_rpz_inconsistency:

A Known Inconsistency in DNS RPZ's NSDNAME and NSIP Rules
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Response Policy Zones define several possible triggers for each rule, and
among these two are known to produce inconsistent results. This is not a
bug; rather, it relates to inconsistencies in the DNS delegation model.

DNS Delegation
^^^^^^^^^^^^^^

In DNS authority data, an NS RRset that is not at the apex of a DNS zone
creates a sub-zone.  That sub-zone’s data is separate from the current (or
"parent") zone, and it can have different authoritative name servers than
the current zone. In this way, the root zone leads to COM, NET, ORG, and so
on, each of which have their own name servers and their own way of managing
their authoritative data. Similarly, ORG has delegations to ISC.ORG and to
millions of other “dot-ORG” zones, each of which can have its own set of
authoritative name servers. In the parlance of the protocol, these NS
RRsets below the apex of a zone are called “delegation points.” An
NS RRset at a delegation point contains a list of authoritative servers
to which the parent zone is delegating authority for all names at or below
the delegation point.

At the apex of every zone there is also an NS RRset. Ideally, this
so-called “apex NS RRset” should be identical to the “delegation point NS
RRset” in the parent zone, but this ideal is not always achieved. In the
real DNS, it’s almost always easier for a zone administrator to update one
of these NS RRsets than the other, so that one will be correct and the
other out of date. This inconsistency is so common that it’s been
necessarily rendered harmless: domains that are inconsistent in this way
are less reliable and perhaps slower, but they still function as long as
there is some overlap between each of the NS RRsets and the truth. (“Truth”
in this case refers to the actual set of name servers that are
authoritative for the zone.)

A Quick Review of DNS Iteration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In DNS recursive name servers, an incoming query that cannot be answered
from the local cache is sent to the closest known delegation point for the
query name. For example, if a server is looking up XYZZY.ISC.ORG and it
the name servers for ISC.ORG, then it sends the query to those servers
directly; however, if it has never heard of ISC.ORG before, it must first
send the query to the name servers for ORG (or perhaps even to the root
zone that is the parent of ORG).

When it asks one of the parent name servers, that server will not have an
answer, so it sends a “referral” consisting only of the “delegation point
NS RRset.” Once the server receives this referral, it “iterates” by sending
the same query again, but this time to name servers for a more specific
part of the query name.  Eventually this iteration terminates, usually by
getting an answer or a “name error” (NXDOMAIN) from the query name’s
authoritative server, or by encountering some type of server failure.

When an authoritative server for the query name sends an answer, it has the
option of including a copy of the zone’s apex NS RRset. If this occurs, the
recursive name server caches this NS RRset, replacing the delegation point
NS RRset that it had received during the iteration process. In the parlance
of the DNS, the delegation point NS RRset is “glue,” meaning
non-authoritative data, or more of a hint than a real truth. On the other
hand, the apex NS RRset is authoritative data, coming as it does from the
zone itself, and it is considered more credible than the “glue.” For this
reason, it’s a little bit more important that the apex NS RRset be correct
than that the delegation point NS RRset be correct, since the former will
quickly replace the latter, and will be used more often for a longer total
period of time.

Importantly, the authoritative name server need not include its apex NS
RRset in any answers, and recursive name servers do not ordinarily query
directly for this RRset. Therefore it is possible for the apex NS RRset to
be completely wrong without any operational ill-effects, since the wrong
data need not be exposed. Of course, if a query comes in for this NS RRset,
most recursive name servers will forward the query to the zone’s authority
servers, since it’s bad form to return “glue” data when asked a specific
question. In these corner cases, bad apex NS RRset data can cause a zone to
become unreachable unpredictably, according to what other queries the
recursive name server has processed.

There is another kind of “glue," for name servers whose names are below
delegation points. If ORG delegates ISC.ORG to NS-EXT.ISC.ORG, the ORG
server needs to know an address for NS-EXT.ISC.ORG and return this address
as part of the delegation response. However, the name-to-address binding
for this name server is only authoritative inside the ISC.ORG zone;
therefore, the A or AAAA RRset given out with the delegation is
non-authoritative “glue,” which is replaced by an authoritative RRset if
one is seen. As with apex NS RRsets, the real A or AAAA RRset is not
automatically queried for by the recursive name server, but is queried for
if an incoming query asks for this RRset.

Enter RPZ
^^^^^^^^^

RPZ has two trigger types that are intended to allow policy zone authors to
target entire groups of domains based on those domains all being served by
the same DNS servers: NSDNAME and NSIP. The NSDNAME and NSIP rules are
matched against the name and IP address (respectively) of the nameservers
of the zone the answer is in, and all of its parent zones. In its default
configuration, BIND actively fetches any missing NS RRsets and address
records.  If, in the process of attempting to resolve the names of all of
these delegated server names, BIND receives a SERVFAIL response for any of
the queries, then it aborts the policy rule evaluation and returns SERVFAIL
for the query. This is technically neither a match nor a non-match of the
rule.

Every "." in a fully qualified domain name (FQDN) represents a potential
delegation point. When BIND goes searching for parent zone NS RRsets (and,
in the case of NSIP, their accompanying address records), it has to check
every possible delegation point. This can become a problem for some
specialized pseudo-domains, such as some domain name and network reputation
systems, that have many "." characters in the names. It is further
complicated if that system also has non-compliant DNS servers that silently
drop queries for NS and SOA records. This forces BIND to wait for those
queries to time out before it can finish evaluating the policy rule, even
if this takes longer than a reasonable client typically waits for an answer
(delays of over 60 seconds have been observed).

While both of these cases do involve configurations and/or servers that are
technically "broken," they may still "work" outside of RPZ NSIP and NSDNAME
rules because of redundancy and iteration optimizations.

There are two RPZ options, ``nsip-wait-recurse`` and ``nsdname-wait-recurse``,
that alter BIND's behavior by allowing it to use only those records that
already exist in the cache when evaluating NSIP and NSDNAME rules,
respectively.

Therefore NSDNAME and NSIP rules are unreliable. The rules may be matched
against either the apex NS RRset or the "glue" NS RRset, each with their
associated addresses (that also might or might not be "glue"). It’s in the
administrator's interests to discover both the delegation name server names
and addresses, and the apex name server names and authoritative address
records, to ensure correct use of NS and NSIP triggers in RPZ. Even then,
there may be collateral damage to completely unrelated domains that
otherwise "work," just by having NSIP and NSDNAME rules.

.. _rpz_disable_mozilla_doh:

Example: Using RPZ to Disable Mozilla DoH-by-Default
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Mozilla announced in September 2019 that they would enable DNS-over-HTTPS
(DoH) for all US-based users of the Firefox browser, sending all their DNS
queries to predefined DoH providers (Cloudflare's 1.1.1.1 service in
particular). This is a concern for some network administrators who do not
want their users' DNS queries to be rerouted unexpectedly. However,
Mozilla provides a mechanism to disable the DoH-by-default setting:
if the Mozilla-owned domain `use-application-dns.net
<https://use-application-dns.net>`_ returns an NXDOMAIN response code, Firefox
will not use DoH.

To accomplish this using RPZ:

1. Create a polizy zone file called ``mozilla.rpz.db`` configured so
   that NXDOMAIN will be returned for any query to ``use-application-dns.net``:

::

  $TTL	604800
  $ORIGIN	mozilla.rpz.
  @	IN	SOA	localhost. root.localhost. 1 604800 86400 2419200 604800
  @	IN	NS	localhost.
  use-application-dns.net CNAME .

2. Add the zone into the BIND configuration (usually :iscman:`named.conf`):

.. code-block:: c

  zone mozilla.rpz {
      type primary;
      file "/<PATH_TO>/mozilla.rpz.db";
      allow-query { localhost; };
  };

3. Enable use of the Response Policy Zone for all incoming queries
   by adding the :any:`response-policy` directive into the ``options {}``
   section:

.. code-block:: c

  options {
  	response-policy { zone mozilla.rpz; } break-dnssec yes;
  };

4. Reload the configuration and test whether the Response Policy
   Zone that was just added is in effect:

.. code-block:: shell-session

  # rndc reload
  # dig IN A use-application-dns.net @<IP_ADDRESS_OF_YOUR_RESOLVER>
  # dig IN AAAA use-application-dns.net @<IP_ADDRESS_OF_YOUR_RESOLVER>

The response should return NXDOMAIN instead of the list of IP addresses,
and the BIND 9 log should contain lines like this:

.. code-block:: none

  09-Sep-2019 18:50:49.439 client @0x7faf8e004a00 ::1#54175 (use-application-dns.net): rpz QNAME NXDOMAIN rewrite use-application-dns.net/AAAA/IN via use-application-dns.net.mozilla.rpz
  09-Sep-2019 18:50:49.439 client @0x7faf8e007800 127.0.0.1#62915 (use-application-dns.net): rpz QNAME NXDOMAIN rewrite use-application-dns.net/AAAA/IN via use-application-dns.net.mozilla.rpz

Note that this is the simplest possible configuration; specific
configurations may be different, especially for administrators who are
already using other response policy zones, or whose servers are configured
with multiple views.
