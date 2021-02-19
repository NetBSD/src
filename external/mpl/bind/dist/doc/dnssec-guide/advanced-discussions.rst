.. 
   Copyright (C) Internet Systems Consortium, Inc. ("ISC")
   
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, you can obtain one at https://mozilla.org/MPL/2.0/.
   
   See the COPYRIGHT file distributed with this work for additional
   information regarding copyright ownership.

.. _dnssec_advanced_discussions:

Advanced Discussions
--------------------

.. _signature_validity_periods:

Signature Validity Periods and Zone Re-Signing Intervals
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In :ref:`how_are_answers_verified`, we saw that record signatures
have a validity period outside of which they are not valid. This means
that at some point, a signature will no longer be valid and a query for
the associated record will fail DNSSEC validation. But how long should a
signature be valid for?

The maximum value for the validity period should be determined by the impact of a
replay attack: if this is low, the period can be long; if high,
the period should be shorter. There is no "right" value, but periods of
between a few days to a month are common.

Deciding a minimum value is probably an easier task. Should something
fail (e.g., a hidden primary distributing to secondary servers that
actually answer queries), how long will it take before the failure is
noticed, and how long before it is fixed? If you are a large 24x7
operation with operators always on-site, the answer might be less than
an hour. In smaller companies, if the failure occurs
just after everyone has gone home for a long weekend, the answer might
be several days.

Again, there are no "right" values - they depend on your circumstances. The
signature validity period you decide to use should be a value between
the two bounds. At the time of this writing (mid-2020), the default policy used by BIND
sets a value of 14 days.

To keep the zone valid, the signatures must be periodically refreshed
since they expire - i.e., the zone must be periodically
re-signed. The frequency of the re-signing depends on your network's
individual needs. For example, signing puts a load on your server, so if
the server is very highly loaded, a lower re-signing frequency is better. Another
consideration is the signature lifetime: obviously the intervals between
signings must not be longer than the signature validity period. But if
you have set a signature lifetime close to the minimum (see above), the
signing interval must be much shorter. What would happen if the system
failed just before the zone was re-signed?

Again, there is no single "right" answer; it depends on your circumstances. The
BIND 9 default policy sets the signature refresh interval to 5 days.

.. _advanced_discussions_proof_of_nonexistence:

Proof of Non-Existence (NSEC and NSEC3)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

How do you prove that something does not exist? This zen-like question
is an interesting one, and in this section we provide an overview
of how DNSSEC solves the problem.

Why is it even important to have authenticated denial of existence in DNS?
Couldn't we just send back "hey, what you asked for does not exist,"
and somehow generate a digital signature to go with it, proving it
really is from the correct authoritative source? Aside from the technical
challenge of signing something that doesn't exist, this solution has flaws, one of
which is it gives an attacker a way to create the appearance of denial
of service by replaying this message on the network.

Let's use a little story, told three different ways, to
illustrate how proof of nonexistence works. In our story, we run a small
company with three employees: Alice, Edward, and Susan. For reasons that
are far too complicated to go into, they don't have email accounts;
instead, email for them is sent to a single account and a nameless
intern passes the message to them. The intern has access to our private
DNSSEC key to create signatures for their responses.

If we followed the approach of giving back the same answer no matter
what was asked, when people emailed and asked for the message to be
passed to "Bob," our intern would simply answer "Sorry, that person
doesn’t work here" and sign this message. This answer could be validated
because our intern signed the response with our private DNSSEC key.
However, since the signature doesn’t change, an attacker could record
this message. If the attacker were able to intercept our email, when the next
person emailed asking for the message to be passed to Susan, the attacker
could return the exact same message: "Sorry, that person doesn’t work
here," with the same signature. Now the attacker has successfully fooled
the sender into thinking that Susan doesn’t work at our company, and
might even be able to convince all senders that no one works at this
company.

To solve this problem, two different solutions were created. We will
look at the first one, NSEC, next.

.. _advanced_discussions_nsec:

NSEC
^^^^

The NSEC record is used to prove that something does not exist, by
providing the name before it and the name after it. Using our tiny
company example, this would be analogous to someone sending an email for
Bob and our nameless intern responding with with: "I'm sorry, that
person doesn't work here. The name before the location where 'Bob'
would be is Alice, and the name after that is Edward." Let's say
another email was received for a
non-existent person, this time Oliver; our intern would respond "I'm
sorry, that person doesn't work here. The name before the location
where 'Oliver' would be is Edward,
and the name after that is Susan." If another sender asked for Todd, the
answer would be: "I'm sorry, that person doesn't work here. The name
before the location where 'Todd' would be is Susan, and there are no
other names after that."

So we end up with four NSEC records:

::

   example.com.        300   IN  NSEC    alice.example.com.  A RRSIG NSEC
   alice.example.com.  300 IN  NSEC    edward.example.com. A RRSIG NSEC
   edward.example.com. 300 IN  NSEC    susan.example.com.  A RRSIG NSEC
   susan.example.com.  300 IN  NSEC    example.com.        A RRSIG NSEC

What if the attacker tried to use the same replay method described
earlier? If someone sent an email for Edward, none of the four answers
would fit. If attacker replied with message #2, "I'm sorry, that person
doesn't work here. The name before it is Alice, and the name after it is
Edward," it is obviously false, since "Edward" is in the response; and the same
goes for #3, Edward and Susan. As for #1 and #4, Edward does not fall in
the alphabetical range before Alice or after Susan, so the sender can logically deduce
that it was an incorrect answer.

When BIND signs your zone, the zone data is automatically sorted on
the fly before generating NSEC records, much like how a phone directory
is sorted.

The NSEC record allows for a proof of non-existence for record types. If
you ask a signed zone for a name that exists but for a record type that
doesn't (for that name), the signed NSEC record returned lists all of
the record types that *do* exist for the requested domain name.

NSEC records can also be used to show whether a record was generated as
the result of a wildcard expansion. The details of this are not
within the scope of this document, but are described well in
:rfc:`7129`.

Unfortunately, the NSEC solution has a few drawbacks, one of which is
trivial "zone walking." In our story, a curious person can keep sending emails, and
our nameless, gullible intern keeps divulging information about our
employees. Imagine if the sender first asked: "Is Bob there?" and
received back the names Alice and Edward. Our sender could then email
again: "Is Edwarda there?", and will get back Edward and Susan. (No,
"Edwarda" is not a real name. However, it is the first name
alphabetically after "Edward" and that is enough to get the intern to reply
with a message telling us the next valid name after Edward.) Repeat the
process enough times and the person sending the emails eventually
learns every name in our company phone directory. For many of you, this
may not be a problem, since the very idea of DNS is similar to a public
phone book: if you don't want a name to be known publicly, don't put it
in DNS! Consider using DNS views (split DNS) and only display your
sensitive names to a select audience.

The second drawback of NSEC is actually increased operational
overhead: there is no opt-out mechanism for insecure child zones. This generally
is a problem for parent-zone operators dealing with a lot of insecure
child zones, such as ``.com``. To learn more about opt-out, please see
:ref:`advanced_discussions_nsec3_optout`.

.. _advanced_discussions_nsec3:

NSEC3
^^^^^

NSEC3 adds two additional features that NSEC does not have:

1. It offers no easy zone enumeration.

2. It provides a mechanism for the parent zone to exclude insecure
   delegations (i.e., delegations to zones that are not signed) from the
   proof of non-existence.

Recall that in :ref:`advanced_discussions_nsec` we provided a range of
names to prove that something does not exist. But as it turns
out, even disclosing these ranges of names becomes a problem: this made
it very easy for the curious-minded to look at our entire zone. Not
only that, unlike a zone transfer, this "zone walking" is more
resource-intensive. So how do we disclose something without actually disclosing
it?

The answer is actually quite simple: hashing functions, or one-way
hashes. Without going into many details, think of it like a magical meat
grinder. A juicy piece of ribeye steak goes in one end, and out comes a
predictable shape and size of ground meat (hash) with a somewhat unique
pattern. No matter how hard you try, you cannot turn the ground meat
back into the ribeye steak: that's what we call a one-way hash.

NSEC3 basically runs the names through a one-way hash before giving them
out, so the recipients can verify the non-existence without any
knowledge of the actual names.

So let's tell our little story for the third time, this
time with NSEC3. In this version, our intern is not given a list of actual
names; he is given a list of "hashed" names. So instead of Alice,
Edward, and Susan, the list he is given reads like this (hashes
shortened for easier reading):

::

   FSK5.... (produced from Edward)
   JKMA.... (produced from Susan)
   NTQ0.... (produced from Alice)

Then, an email is received for Bob again. Our intern takes the name Bob
through a hash function, and the result is L8J2..., so he replies: "I'm
sorry, that person doesn't work here. The name before that is JKMA...,
and the name after that is NTQ0...". There, we proved Bob doesn't exist,
without giving away any names! To put that into proper NSEC3 resource
records, they would look like this (again, hashes shortened for
ease of display):

::

   FSK5....example.com. 300 IN NSEC3 1 0 10 1234567890ABCDEF  JKMA... A RRSIG
   JKMA....example.com. 300 IN NSEC3 1 0 10 1234567890ABCDEF  NTQ0... A RRSIG
   NTQ0....example.com. 300 IN NSEC3 1 0 10 1234567890ABCDEF  FSK5... A RRSIG

.. note::

   Just because we employed one-way hash functions does not mean there is
   no way for a determined individual to figure out our zone data.
   Someone could still gather all of our NSEC3 records and hashed
   names and perform an offline brute-force attack by trying all
   possible combinations to figure out what the original name is. In our
   meat-grinder analogy, this would be like someone
   buying all available cuts of meat and grinding them up at home using
   the same model of meat grinder, and comparing the output with the meat
   you gave him. It is expensive and time-consuming (especially with
   real meat), but like everything else in cryptography, if someone has
   enough resources and time, nothing is truly private forever. If you
   are concerned about someone performing this type of attack on your
   zone data, read more about adding salt as described in
   :ref:`advanced_discussions_nsec3_salt`.

.. _advanced_discussions_nsec3param:

NSEC3PARAM
++++++++++

The above NSEC3 examples used four parameters: 1, 0, 10, and
1234567890ABCDEF. 1 represents the algorithm, 0 represents the opt-out
flag, 10 represents the number of iterations, and 1234567890ABCDEF is the
salt. Let's look at how each one can be configured:

-  *Algorithm*: The only currently defined value is 1 for SHA-1, so there
   is no configuration field for it.

-  *Opt-out*: Set this to 1 for NSEC3 opt-out, which we
   discuss in :ref:`advanced_discussions_nsec3_optout`.

-  *Iterations*: Iterations defines the number of additional times to
   apply the algorithm when generating an NSEC3 hash. More iterations
   yield more secure results, but consume more resources for both
   authoritative servers and validating resolvers. The considerations
   here are similar to those seen in :ref:`key_sizes`, of
   security versus resources.

-  *Salt*: The salt cannot be configured explicitly, but you can provide
   a salt length and ``named`` generates a random salt of the given length.
   We learn more about salt in :ref:`advanced_discussions_nsec3_salt`.

If you want to use these NSEC3 parameters for a zone, you can add the
following configuration to your ``dnssec-policy``. For example, to create an
NSEC3 chain using the SHA-1 hash algorithm, with no opt-out flag,
5 iterations, and a salt that is 8 characters long, use:

::

   dnssec-policy "nsec3" {
       ...
       nsec3param iterations 5 optout no salt-length 8;
   };

To set the opt-out flag, 15 iterations, and no salt, use:

::

   dnssec-policy "nsec3" {
       ...
       nsec3param iterations 15 optout yes salt-length 0;
   };

.. _advanced_discussions_nsec3_optout:

NSEC3 Opt-Out
+++++++++++++

One of the advantages of NSEC3 over NSEC is the ability for a parent zone
to publish less information about its child or delegated zones. Why
would you ever want to do that? If a significant number of your
delegations are not yet DNSSEC-aware, meaning they are still insecure or
unsigned, generating DNSSEC-records for their NS and glue records is not
a good use of your precious name server resources.

The resources may not seem like a lot, but imagine that you are the
operator of busy top-level domains such as ``.com`` or ``.net``, with
millions of insecure delegated domain names: it quickly
adds up. As of mid-2020, less than 1.5% of all ``.com`` zones are
signed. Basically, without opt-out, with 1,000,000 delegations,
only 5 of which are secure, you still have to generate NSEC RRsets for
the other 999,995 delegations; with NSEC3 opt-out, you will have saved
yourself 999,995 sets of records.

For most DNS administrators who do not manage a large number of
delegations, the decision whether to use NSEC3 opt-out is
probably not relevant.

To learn more about how to configure NSEC3 opt-out, please see
:ref:`recipes_nsec3_optout`.

.. _advanced_discussions_nsec3_salt:

NSEC3 Salt
++++++++++

As described in :ref:`advanced_discussions_nsec3`, while NSEC3
does not put your zone data in plain public display, it is still not
difficult for an attacker to collect all the hashed names and perform
an offline attack. All that is required is running through all the
combinations to construct a database of plaintext names to hashed names,
also known as a "rainbow table."

There is one more feature NSEC3 gives us to provide additional
protection: salt. Basically, salt gives us the ability to introduce further
randomness into the hashed results. Whenever the salt is changed, any
pre-computed rainbow table is rendered useless, and a new rainbow table
must be re-computed. If the salt is changed periodically, it
becomes difficult to construct a useful rainbow table, and thus difficult to
walk the DNS zone data programmatically. How often you want to change
your NSEC3 salt is up to you.

To learn more about the steps to take to change NSEC3, please see
:ref:`recipes_nsec3_salt`.

.. _advanced_discussions_nsec_or_nsec3:

NSEC or NSEC3?
^^^^^^^^^^^^^^

So which one should you choose: NSEC or NSEC3? There is not a
single right answer here that fits everyone; it comes down to your
network's needs or requirements.

If you prefer not to make your zone easily enumerable, implementing
NSEC3 paired with a periodically changed salt provides a certain
level of privacy protection. However, someone could still randomly guess
the names in your zone (such as "ftp" or "www"), as in the traditional
insecure DNS.

If you have many delegations and need to be able to opt-out to save
resources, NSEC3 is for you.

In other situations, NSEC is typically a good choice for most zone
administrators, as it relieves the authoritative servers of the
additional cryptographic operations that NSEC3 requires, and NSEC is
comparatively easier to troubleshoot than NSEC3.

NSEC3 in conjunction with ``dnssec-policy`` is supported in BIND
as of version 9.16.9.

.. _advanced_discussions_key_generation:

DNSSEC Keys
~~~~~~~~~~~

Types of Keys
^^^^^^^^^^^^^

Although DNSSEC
documentation talks about three types of keys, they are all the same
thing - but they have different roles. The roles are:

Zone-Signing Key (ZSK)
   This is the key used to sign the zone. It signs all records in the
   zone apart from the DNSSEC key-related RRsets: DNSKEY, CDS, and
   CDNSKEY.

Key-Signing Key (KSK)
   This is the key used to sign the DNSSEC key-related RRsets and is the
   key used to link the parent and child zones. The parent zone stores a
   digest of the KSK. When a resolver verifies the chain of trust it
   checks to see that the DS record in the parent (which holds the
   digest of a key) matches a key in the DNSKEY RRset, and that it is
   able to use that key to verify the DNSKEY RRset. If it can do
   that, the resolver knows that it can trust the DNSKEY resource
   records, and so can use one of them to validate the other records in
   the zone.

Combined Signing Key (CSK)
   A CSK combines the functionality of a ZSK and a KSK. Instead of
   having one key for signing the zone and one for linking the parent
   and child zones, a CSK is a single key that serves both roles.

It is important to realize the terms ZSK, KSK, and CSK describe how the
keys are used - all these keys are represented by DNSKEY records. The
following examples are the DNSKEY records from a zone signed with a KSK
and ZSK:

::

   $ dig @192.168.1.12 example.com DNSKEY

   ; <<>> DiG 9.16.0 <<>> @192.168.1.12 example.com dnskey +multiline
   ; (1 server found)
   ;; global options: +cmd
   ;; Got answer:
   ;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 54989
   ;; flags: qr aa rd; QUERY: 1, ANSWER: 2, AUTHORITY: 0, ADDITIONAL: 1
   ;; WARNING: recursion requested but not available

   ;; OPT PSEUDOSECTION:
   ; EDNS: version: 0, flags:; udp: 4096
   ; COOKIE: 5258d7ed09db0d76010000005ea1cc8c672d8db27a464e37 (good)
   ;; QUESTION SECTION:
   ;example.com.       IN DNSKEY

   ;; ANSWER SECTION:
   example.com.        60 IN DNSKEY 256 3 13 (
                   tAeXLtIQ3aVDqqS/1UVRt9AE6/nzfoAuaT1Vy4dYl2CK
                   pLNcUJxME1Z//pnGXY+HqDU7Gr5HkJY8V0W3r5fzlw==
                   ) ; ZSK; alg = ECDSAP256SHA256 ; key id = 63722
   example.com.        60 IN DNSKEY 257 3 13 (
                   cxkNegsgubBPXSra5ug2P8rWy63B8jTnS4n0IYSsD9eW
                   VhiyQDmdgevKUhfG3SE1wbLChjJc2FAbvSZ1qk03Nw==
                   ) ; KSK; alg = ECDSAP256SHA256 ; key id = 42933

... and a zone signed with just a CSK:

::

   $ dig @192.168.1.13 example.com DNSKEY

   ; <<>> DiG 9.16.0 <<>> @192.168.1.13 example.com dnskey +multiline
   ; (1 server found)
   ;; global options: +cmd
   ;; Got answer:
   ;; ->>HEADER<<- opcode: QUERY, status: NOERROR, id: 22628
   ;; flags: qr aa rd; QUERY: 1, ANSWER: 1, AUTHORITY: 0, ADDITIONAL: 1
   ;; WARNING: recursion requested but not available

   ;; OPT PSEUDOSECTION:
   ; EDNS: version: 0, flags:; udp: 4096
   ; COOKIE: bf19ee914b5df46e010000005ea1cd02b66c06885d274647 (good)
   ;; QUESTION SECTION:
   ;example.com.       IN DNSKEY

   ;; ANSWER SECTION:
   example.com.        60 IN DNSKEY 257 3 13 (
                   p0XM6AJ68qid2vtOdyGaeH1jnrdk2GhZeVvGzXfP/PNa
                   71wGtzR6jdUrTbXo5Z1W5QeeJF4dls4lh4z7DByF5Q==
                   ) ; KSK; alg = ECDSAP256SHA256 ; key id = 1231

The only visible difference between the records (apart from the key data
itself) is the value of the flags fields; this is 256
for a ZSK and 257 for a KSK or CSK. Even then, the flags field is only a
hint to the software using it as to the role of the key: zones can be
signed by any key. The fact that a CSK and KSK both have the same flags
emphasizes this. A KSK usually only signs the DNSSEC key-related RRsets
in a zone, whereas a CSK is used to sign all records in the zone.

The original idea of separating the function of the key into a KSK and
ZSK was operational. With a single key, changing it for any reason is
"expensive," as it requires interaction with the parent zone
(e.g., uploading the key to the parent may require manual interaction
with the organization running that zone). By splitting it, interaction
with the parent is required only if the KSK is changed; the ZSK can be
changed as often as required without involving the parent.

The split also allows the keys to be of different lengths. So the ZSK,
which is used to sign the record in the zone, can be of a (relatively)
short length, lowering the load on the server. The KSK, which is used
only infrequently, can be of a much longer length. The relatively
infrequent use also allows the private part of the key to be stored in a
way that is more secure but that may require more overhead to access, e.g., on
an HSM (see :ref:`hardware_security_modules`).

In the early days of DNSSEC, the idea of splitting the key went more or
less unchallenged. However, with the advent of more powerful computers
and the introduction of signaling methods between the parent and child
zones (see :ref:`cds_cdnskey`), the advantages of a ZSK/KSK split are
less clear and, for many zones, a single key is all that is required.

As with many questions related to the choice of DNSSEC policy, the
decision on which is "best" is not clear and depends on your circumstances.

Which Algorithm?
^^^^^^^^^^^^^^^^

There are three algorithm choices for DNSSEC as of this writing
(mid-2020):

-  RSA

-  Elliptic Curve DSA (ECDSA)

-  Edwards Curve Digital Security Algorithm (EdDSA)

All are supported in BIND 9, but only RSA and ECDSA (specifically
RSASHA256 and ECDSAP256SHA256) are mandatory to implement in DNSSEC.
However, RSA is a little long in the tooth, and ECDSA/EdDSA are emerging
as the next new cryptographic standards. In fact, the US federal
government recommended discontinuing RSA use altogether by September 2015
and migrating to using ECDSA or similar algorithms.

For now, use ECDSAP256SHA256 but keep abreast of developments in this
area. For details about rolling over DNSKEYs to a new algorithm, see
:ref:`advanced_discussions_DNSKEY_algorithm_rollovers`.

.. _key_sizes:

Key Sizes
^^^^^^^^^

If using RSA keys, the choice of key sizes is a classic issue of finding
the balance between performance and security. The larger the key size,
the longer it takes for an attacker to crack the key; but larger keys
also mean more resources are needed both when generating signatures
(authoritative servers) and verifying signatures (recursive servers).

Of the two sets of keys, ZSK is used much more frequently. ZSK is used whenever zone
data changes or when signatures expire, so performance
certainly is of a bigger concern. As for KSK, it is used less
frequently, so performance is less of a factor, but its impact is bigger
because of its role in signing other keys.

In earlier versions of this guide, the following key lengths were
chosen for each set, with the recommendation that they be rotated more
frequently for better security:

-  *ZSK*: RSA 1024 bits, rollover every year

-  *KSK*: RSA 2048 bits, rollover every five years

These should be considered minimum RSA key sizes. At the time
of this writing (mid-2020), the root zone and many TLDs are already using 2048
bit ZSKs. If you choose to implement larger key sizes, keep in mind that
larger key sizes result in larger DNS responses, which this may mean more
load on network resources. Depending on your network configuration, end users
may even experience resolution failures due to the increased response
sizes, as discussed in :ref:`whats_edns0_all_about`.

ECDSA key sizes can be much smaller for the same level of security, e.g.,
an ECDSA key length of 224 bits provides the same level of security as a
2048-bit RSA key. Currently BIND 9 sets a key size of 256 for all ECDSA keys.

.. _advanced_discussions_key_storage:

Key Storage
^^^^^^^^^^^

Public Key Storage
++++++++++++++++++

The beauty of a public key cryptography system is that the public key
portion can and should be distributed to as many people as possible. As
the administrator, you may want to keep the public keys on an easily
accessible file system for operational ease, but there is no need to
securely store them, since both ZSK and KSK public keys are published in
the zone data as DNSKEY resource records.

Additionally, a hash of the KSK public key is also uploaded to the
parent zone (see :ref:`working_with_parent_zone` for more details),
and is published by the parent zone as DS records.

Private Key Storage
+++++++++++++++++++

Ideally, private keys should be stored offline, in secure devices such
as a smart card. Operationally, however, this creates certain
challenges, since the private key is needed to create RRSIG resource
records, and it is a hassle to bring the private key out of
storage every time the zone file changes or signatures expire.

A common approach to strike the balance between security and
practicality is to have two sets of keys: a ZSK set and a KSK set. A ZSK
private key is used to sign zone data, and can be kept online for ease
of use, while a KSK private key is used to sign just the DNSKEY (the ZSK); it is
used less frequently, and can be stored in a much more secure and
restricted fashion.

For example, a KSK private key stored on a USB flash drive that is kept
in a fireproof safe, only brought online once a year to sign a new pair
of ZSKs, combined with a ZSK private key stored on the network
file system and available for routine use, may be a good balance between
operational flexibility and security.

For more information on changing keys, please see
:ref:`key_rollovers`.

.. _hardware_security_modules:

Hardware Security Modules (HSMs)
++++++++++++++++++++++++++++++++

A Hardware Security Module (HSM) may come in different shapes and sizes,
but as the name indicates, it is a physical device or devices, usually
with some or all of the following features:

-  Tamper-resistant key storage

-  Strong random-number generation

-  Hardware for faster cryptographic operations

Most organizations do not incorporate HSMs into their security practices
due to cost and the added operational complexity.

BIND supports Public Key Cryptography Standard #11 (PKCS #11) for
communication with HSMs and other cryptographic support devices. For
more information on how to configure BIND to work with an HSM, please
refer to the `BIND 9 Administrator Reference
Manual <https://bind9.readthedocs.io/en/latest/index.html>`_.

.. _advanced_discussions_key_management:

Rollovers
~~~~~~~~~

.. _key_rollovers:

Key Rollovers
^^^^^^^^^^^^^

A key rollover is where one key in a zone is replaced by a new one.
There are arguments for and against regularly rolling keys. In essence
these are:

Pros:

1. Regularly changing the key hinders attempts at determination of the
   private part of the key by cryptanalysis of signatures.

2. It gives administrators practice at changing a key; should a key ever need to be
   changed in an emergency, they would not be doing it for the first time.

Cons:

1. A lot of effort is required to hack a key, and there are probably
   easier ways of obtaining it, e.g., by breaking into the systems on
   which it is stored.

2. Rolling the key adds complexity to the system and introduces the
   possibility of error. We are more likely to
   have an interruption to our service than if we had not rolled it.

Whether and when to roll the key is up to you. How serious would the
damage be if a key were compromised without you knowing about it? How
serious would a key roll failure be?

Before going any further, it is worth noting that if you sign your zone
with either of the fully automatic methods (described in ref:`signing_alternative_ways`),
you don't really need to
concern yourself with the details of a key rollover: BIND 9 takes care of
it all for you. If you are doing a manual key roll or are setting up the
keys for a semi-automatic key rollover, you do need to familiarize yourself
with the various steps involved and the timing details.

Rolling a key is not as simple as replacing the DNSKEY statement in the
zone. That is an essential part of it, but timing is everything. For
example, suppose that we run the ``example.com`` zone and that a friend
queries for the AAAA record of ``www.example.com``. As part of the
resolution process (described in
:ref:`how_does_dnssec_change_dns_lookup`), their recursive server
looks up the keys for the ``example.com`` zone and uses them to verify
the signature associated with the AAAA record. We'll assume that the
records validated successfully, so they can use the
address to visit ``example.com``'s website.

Let's also assume that immediately after the lookup, we want to roll the ZSK
for ``example.com``. Our first attempt at this is to remove the old
DNSKEY record and signatures, add a new DNSKEY record, and re-sign the
zone with it. So one minute our server is serving the old DNSKEY and
records signed with the old key, and the next minute it is serving the
new key and records signed with it. We've achieved our goal - we are
serving a zone signed with the new keys; to check this is really the
case, we booted up our laptop and looked up the AAAA record
``ftp.example.com``. The lookup succeeded so all must be well. Or is it?
Just to be sure, we called our friend and asked them to check. They
tried to lookup ``ftp.example.com`` but got a SERVFAIL response from
their recursive server. What's going on?

The answer, in a word, is "caching." When our friend looked up
``www.example.com``, their recursive server retrieved and cached
not only the AAAA record, but also a lot of other records. It cached
the NS records for ``com`` and ``example.com``, as well as
the AAAA (and A) records for those name servers (and this action may, in turn, have
caused the lookup and caching of other NS and AAAA/A records). Most
importantly for this example, it also looked up and cached the DNSKEY
records for the root, ``com``, and ``example.com`` zones. When a query
was made for ``ftp.example.com``, the recursive server believed it
already had most of the information
we needed. It knew what nameservers served ``example.com`` and their
addresses, so it went directly to one of those to get the AAAA record for
``ftp.example.com`` and its associated signature. But when it tried to
validate the signature, it used the cached copy of the DNSKEY, and that
is when our friend had the problem. Their recursive server had a copy of
the old DNSKEY in its cache, but the AAAA record for ``ftp.example.com``
was signed with the new key. So, not surprisingly, the signature could not
validate.

How should we roll the keys for ``example.com``? A clue to the
answer is to note that the problem came about because the DNSKEY records
were cached by the recursive server. What would have happened had our
friend flushed the DNSKEY records from the recursive server's cache before
making the query? That would have worked; those records would have been
retrieved from ``example.com``'s nameservers at the same time that we
retrieved the AAAA record for ``ftp.example.com``. Our friend's server would have
obtained the new key along with the AAAA record and associated signature
created with the new key, and all would have been well.

As it is obviously impossible for us to notify all recursive server
operators to flush our DNSKEY records every time we roll a key, we must
use another solution. That solution is to wait
for the recursive servers to remove old records from caches when they
reach their TTL. How exactly we do this depends on whether we are trying
to roll a ZSK, a KSK, or a CSK.

.. _zsk_rollover_methods:

ZSK Rollover Methods
++++++++++++++++++++

The ZSK can be rolled in one of the following two ways:

1. *Pre-Publication*: Publish the new ZSK into zone data before it is
   actually used. Wait at least one TTL interval, so the world's recursive servers
   know about both keys, then stop using the old key and generate a new
   RRSIG using the new key. Wait at least another TTL, so the cached old
   key data is expunged from the world's recursive servers, and then remove
   the old key.

   The benefit of the pre-publication approach is it does not
   dramatically increase the zone size; however, the duration of the rollover
   is longer. If insufficient time has passed after the new ZSK is
   published, some resolvers may only have the old ZSK cached when the
   new RRSIG records are published, and validation may fail. This is the
   method described in :ref:`recipes_zsk_rollover`.

2. *Double-Signature*: Publish the new ZSK and new RRSIG, essentially
   doubling the size of the zone. Wait at least one TTL interval, and then remove
   the old ZSK and old RRSIG.

   The benefit of the double-signature approach is that it is easier to
   understand and execute, but it causes a significantly increased zone size
   during a rollover event.

.. _ksk_rollover_methods:

KSK Rollover Methods
++++++++++++++++++++

Rolling the KSK requires interaction with the parent zone, so
operationally this may be more complex than rolling ZSKs. There are
three methods of rolling the KSK:

1. *Double-KSK*: Add the new KSK to the DNSKEY RRset, which is then
   signed with both the old and new keys. After waiting for the old RRset
   to expire from caches, change the DS record in the parent zone.
   After waiting a further TTL interval for this change to be reflected in
   caches, remove the old key from the RRset.

   Basically, the new KSK is added first at the child zone and
   used to sign the DNSKEY; then the DS record is changed, followed by the
   removal of the old KSK. Double-KSK keeps the interaction with the
   parent zone to a minimum, but for the duration of the rollover, the
   size of the DNSKEY RRset is increased.

2. *Double-DS*: Publish the new DS record. After waiting for this
   change to propagate into caches, change the KSK. After a further TTL
   interval during which the old DNSKEY RRset expires from caches, remove the
   old DS record.

   Double-DS is the reverse of Double-KSK: the new DS is published at
   the parent first, then the KSK at the child is updated, then
   the old DS at the parent is removed. The benefit is that the size of the DNSKEY
   RRset is kept to a minimum, but interactions with the parent zone are
   increased to two events. This is the method described in
   :ref:`recipes_ksk_rollover`.

3. *Double-RRset*: Add the new KSK to the DNSKEY RRset, which is
   then signed with both the old and new key, and add the new DS record
   to the parent zone. After waiting a suitable interval for the
   old DS and DNSKEY RRsets to expire from caches, remove the old DNSKEY and
   old DS record.

   Double-RRset is the fastest way to roll the KSK (i.e., it has the shortest rollover
   time), but has the drawbacks of both of the other methods: a larger
   DNSKEY RRset and two interactions with the parent.

.. _csk_rollover_methods:

CSK Rollover Methods
++++++++++++++++++++

Rolling the CSK is more complex than rolling either the ZSK or KSK, as
the timing constraints relating to both the parent zone and the caching
of records by downstream recursive servers must be taken into
account. There are numerous possible methods that are a combination of ZSK
rollover and KSK rollover methods. BIND 9 automatic signing uses a
combination of ZSK Pre-Publication and Double-KSK rollover.

.. _advanced_discussions_emergency_rollovers:

Emergency Key Rollovers
^^^^^^^^^^^^^^^^^^^^^^^

Keys are generally rolled on a regular schedule - if you choose
to roll them at all. But sometimes, you may have to rollover keys
out-of-schedule due to a security incident. The aim of an emergency
rollover is to re-sign the zone with a new key as soon as possible, because
when a key is suspected of being compromised, a malicious attacker (or
anyone who has access to the key) could impersonate your server and trick other
validating resolvers into believing that they are receiving authentic,
validated answers.

During an emergency rollover, follow the same operational
procedures described in :ref:`recipes_rollovers`, with the added
task of reducing the TTL of the current active (potentially compromised) DNSKEY
RRset, in an attempt to phase out the compromised key faster before the new
key takes effect. The time frame should be significantly reduced from
the 30-days-apart example, since you probably do not want to wait up to
60 days for the compromised key to be removed from your zone.

Another method is to carry a spare key with you at all times. If
you have a second key pre-published and that one
is not compromised at the same time as the first key,
you could save yourself some time by immediately
activating the spare key if the active
key is compromised. With pre-publication, all validating resolvers should already
have this spare key cached, thus saving you some time.

With a KSK emergency rollover, you also need to consider factors
related to your parent zone, such as how quickly they can remove the old
DS records and publish the new ones.

As with many other facets of DNSSEC, there are multiple aspects to take into
account when it comes to emergency key rollovers. For more in-depth
considerations, please check out :rfc:`7583`.

.. _advanced_discussions_DNSKEY_algorithm_rollovers:

Algorithm Rollovers
^^^^^^^^^^^^^^^^^^^

From time to time, new digital signature algorithms with improved
security are introduced, and it may be desirable for administrators to
roll over DNSKEYs to a new algorithm, e.g., from RSASHA1 (algorithm 5 or
7) to RSASHA256 (algorithm 8). The algorithm rollover steps must be followed with
care to avoid breaking DNSSEC validation.

If you are managing DNSSEC by using the ``dnssec-policy`` configuration,
``named`` handles the rollover for you. Simply change the algorithm
for the relevant keys, and ``named`` uses the new algorithm when the
key is next rolled. It performs a smooth transition to the new
algorithm, ensuring that the zone remains valid throughout rollover.

If you are using other methods to sign the zone, the administrator needs to do more work. As
with other key rollovers, when the zone is a primary zone, an algorithm
rollover can be accomplished using dynamic updates or automatic key
rollovers. For secondary zones, only automatic key rollovers are
possible, but the ``dnssec-settime`` utility can be used to control the
timing.

In any case, the first step is to put DNSKEYs in place using the new algorithm.
You must generate the ``K*`` files for the new algorithm and put
them in the zone's key directory, where ``named`` can access them. Take
care to set appropriate ownership and permissions on the keys. If the
``auto-dnssec`` zone option is set to ``maintain``, ``named``
automatically signs the zone with the new keys, based on their timing
metadata when the ``dnssec-loadkeys-interval`` elapses or when you issue the
``rndc loadkeys`` command. Otherwise, for primary zones, you can use
``nsupdate`` to add the new DNSKEYs to the zone; this causes ``named``
to use them to sign the zone. For secondary zones, e.g., on a
"bump in the wire" signing server, ``nsupdate`` cannot be used.

Once the zone has been signed by the new DNSKEYs (and you have waited
for at least one TTL period), you must inform the parent zone and any trust
anchor repositories of the new KSKs, e.g., you might place DS records in
the parent zone through your DNS registrar's website.

Before starting to remove the old algorithm from a zone, you must allow
the maximum TTL on its DS records in the parent zone to expire. This
assures that any subsequent queries retrieve the new DS records
for the new algorithm. After the TTL has expired, you can remove the DS
records for the old algorithm from the parent zone and any trust anchor
repositories. You must then allow another maximum TTL interval to elapse
so that the old DS records disappear from all resolver caches.

The next step is to remove the DNSKEYs using the old algorithm from your
zone. Again this can be accomplished using ``nsupdate`` to delete the
old DNSKEYs (for primary zones only) or by automatic key rollover when
``auto-dnssec`` is set to ``maintain``. You can cause the automatic key
rollover to take place immediately by using the ``dnssec-settime``
utility to set the *Delete* date on all keys to any time in the past.
(See the ``dnssec-settime -D <date/offset>`` option.)

After adjusting the timing metadata, the ``rndc loadkeys`` command
causes ``named`` to remove the DNSKEYs and
RRSIGs for the old algorithm from the zone. Note also that with the
``nsupdate`` method, removing the DNSKEYs also causes ``named`` to
remove the associated RRSIGs automatically.

Once you have verified that the old DNSKEYs and RRSIGs have been removed
from the zone, the final (optional) step is to remove the key files for
the old algorithm from the key directory.

Other Topics
~~~~~~~~~~~~

DNSSEC and Dynamic Updates
^^^^^^^^^^^^^^^^^^^^^^^^^^

Dynamic DNS (DDNS) is actually independent of DNSSEC. DDNS provides a
mechanism, separate from editing the zone file or zone database, to edit DNS
data. Most DNS clients and servers are able to handle dynamic
updates, and DDNS can also be integrated as part of your DHCP
environment.

When you have both DNSSEC and dynamic updates in your environment,
updating zone data works the same way as with traditional (insecure)
DNS: you can use ``rndc freeze`` before editing the zone file, and
``rndc thaw`` when you have finished editing, or you can use the
command ``nsupdate`` to add, edit, or remove records like this:

::

   $ nsupdate
   > server 192.168.1.13
   > update add xyz.example.com. 300 IN A 1.1.1.1
   > send
   > quit

The examples provided in this guide make ``named`` automatically
re-sign the zone whenever its content has changed. If you decide to sign
your own zone file manually, you need to remember to execute the
``dnssec-signzone`` command whenever your zone file has been updated.

As far as system resources and performance are concerned, be mindful that
with a DNSSEC zone that changes frequently, every time the zone
changes your system is executing a series of cryptographic operations
to (re)generate signatures and NSEC or NSEC3 records.

.. _dnssec_on_private_networks:

DNSSEC on Private Networks
^^^^^^^^^^^^^^^^^^^^^^^^^^

Let's clarify what we mean: in this section, "private networks" really refers to
a private or internal DNS view. Most DNS products offer the ability to
have different versions of DNS answers, depending on the origin of the
query. This feature is often called "DNS views" or "split DNS," and is most
commonly implemented as an "internal" versus an "external" setup.

For instance, your organization may have a version of ``example.com``
that is offered to the world, and its names most likely resolve to
publicly reachable IP addresses. You may also have an internal version
of ``example.com`` that is only accessible when you are on the company's
private networks or via a VPN connection. These private networks typically
fall under 10.0.0.0/8, 172.16.0.0/12, or 192.168.0.0/16 for IPv4.

So what if you want to offer DNSSEC for your internal version of
``example.com``? This can be done: the golden rule is to use the same
key for both the internal and external versions of the zones. This
avoids problems that can occur when machines (e.g., laptops) move
between accessing the internal and external zones, when it is possible
that they may have cached records from the wrong zone.

.. _introduction_to_dane:

Introduction to DANE
^^^^^^^^^^^^^^^^^^^^

With your DNS infrastructure secured with DNSSEC, information can
now be stored in DNS and its integrity and authenticity can be proved.
One of the new features that takes advantage of this is the DNS-Based
Authentication of Named Entities, or DANE. This improves security in a
number of ways, including:

-  The ability to store self-signed X.509 certificates and bypass having to pay a third
   party (such as a Certificate Authority) to sign the certificates
   (:rfc:`6698`).

-  Improved security for clients connecting to mail servers (:rfc:`7672`).

-  A secure way of getting public PGP keys (:rfc:`7929`).

Disadvantages of DNSSEC
~~~~~~~~~~~~~~~~~~~~~~~

DNSSEC, like many things in this world, is not without its problems.
Below are a few challenges and disadvantages that DNSSEC faces.

1. *Increased, well, everything*: With DNSSEC, signed zones are larger,
   thus taking up more disk space; for DNSSEC-aware servers, the
   additional cryptographic computation usually results in increased
   system load; and the network packets are bigger, possibly putting
   more strains on the network infrastructure.

2. *Different security considerations*: DNSSEC addresses many security
   concerns, most notably cache poisoning. But at the same time, it may
   introduce a set of different security considerations, such as
   amplification attack and zone enumeration through NSEC. These
   concerns are still being identified and addressed by the Internet
   community.

3. *More complexity*: If you have read this far, you have probably already
   concluded this yourself. With additional resource records, keys,
   signatures, and rotations, DNSSEC adds many more moving pieces on top of
   the existing DNS machine. The job of the DNS administrator changes,
   as DNS becomes the new secure repository of everything from spam
   avoidance to encryption keys, and the amount of work involved to
   troubleshoot a DNS-related issue becomes more challenging.

4. *Increased fragility*: The increased complexity means more
   opportunities for things to go wrong. Before DNSSEC, DNS
   was essentially "add something to the zone and forget it." With DNSSEC,
   each new component - re-signing, key rollover, interaction with
   parent zone, key management - adds more opportunity for error. It is
   entirely possible that a failure to validate a name may come down to
   errors on the part of one or more zone operators rather than the
   result of a deliberate attack on the DNS.

5. *New maintenance tasks*: Even if your new secure DNS infrastructure
   runs without any hiccups or security breaches, it still requires
   regular attention, from re-signing to key rollovers. While most of
   these can be automated, some of the tasks, such as KSK rollover,
   remain manual for the time being.

6. *Not enough people are using it today*: While it's estimated (as of
   mid-2020) that roughly 30% of the global Internet DNS traffic is
   validating  [#]_ , that doesn't mean that many of the DNS zones are
   actually signed. What this means is, even if your company's zone is
   signed today, fewer than 30% of the Internet's servers are taking
   advantage of this extra security. It gets worse: with less than 1.5%
   of the ``.com`` domains signed, even if your DNSSEC validation is enabled today,
   it's not likely to buy you or your users a whole lot more protection
   until these popular domain names decide to sign their zones.

The last point may have more impact than you realize. Consider this:
HTTP and HTTPS make up the majority of traffic on the Internet. While you may have
secured your DNS infrastructure through DNSSEC, if your web hosting is
outsourced to a third party that does not yet support DNSSEC in its
own domain, or if your web page loads contents and components from
insecure domains, end users may experience validation problems when
trying to access your web page. For example, although you may have signed
the zone ``company.com``, the web address ``www.company.com`` may actually be a
CNAME to ``foo.random-cloud-provider.com``. As long as
``random-cloud-provider.com`` remains an insecure DNS zone, users cannot
fully validate everything when they visit your web page and could be
redirected elsewhere by a cache poisoning attack.

.. [#]
   Based on APNIC statistics at
   `<https://stats.labs.apnic.net/dnssec/XA>`__
