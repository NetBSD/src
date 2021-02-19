.. 
   Copyright (C) Internet Systems Consortium, Inc. ("ISC")
   
   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, you can obtain one at https://mozilla.org/MPL/2.0/.
   
   See the COPYRIGHT file distributed with this work for additional
   information regarding copyright ownership.

.. _dnssec_commonly_asked_questions:

Commonly Asked Questions
------------------------

No questions are too stupid to ask. Below are some common
questions you may have and (hopefully) some answers that help.

Do I need IPv6 to have DNSSEC?
   No. DNSSEC can be deployed without IPv6.

Does DNSSEC encrypt my DNS traffic, so others cannot eavesdrop on my DNS queries?
   No. Although cryptographic keys and digital signatures
   are used in DNSSEC, they only provide authenticity and integrity, not
   privacy. Someone who sniffs network traffic can still see all the DNS
   queries and answers in plain text; DNSSEC just makes it very difficult
   for the eavesdropper to alter or spoof the DNS responses.

Does DNSSEC protect the communication between my laptop and my name server?
   Unfortunately, not at the moment. DNSSEC is designed to protect the
   communication between end clients (laptop) and name servers;
   however, there are few applications or stub resolver libraries as of
   mid-2020 that take advantage of this capability. While enabling DNSSEC today
   does little to enhance the security of communications between a recursive
   server and its clients (commonly called the "last mile"), we hope that
   will change in the near future as more applications become DNSSEC-aware.

Does DNSSEC secure zone transfers?
   No. You should consider using TSIG to secure zone transfers among your
   name servers.

Does DNSSEC protect my network from malicious websites?
   The answer in the early stages of DNSSEC deployment is, unfortunately,
   no. DNSSEC is designed to provide
   confidence that when you receive a DNS response for www.company.com over
   port 53, it really came from Company's name servers and the
   answers are authentic. But that does not mean the web server a user visits
   over port 80 or port 443 is necessarily safe. Furthermore, 98.5% of
   domain name operators (as of this writing in mid-2020) have not yet signed
   their zones, so DNSSEC cannot even validate their answers.
   
   The answer for sometime in
   the future is that, as more zones are signed and more
   recursive servers validate, DNSSEC will make it much more
   difficult for attackers to spoof DNS responses or perform cache
   poisoning. It will still not protect against users who visit a malicious
   website that an attacker owns and operates, or prevent users from
   mistyping a domain name; it will just become less likely that an attacker can
   hijack other domain names.

If I enable DNSSEC validation, will it break DNS lookup, since most domain names do not yet use DNSSEC?
   No, DNSSEC is backwards-compatible to "standard"
   DNS. As of this writing (in mid-2020), although 98.5% of the .com domains have yet to
   be signed, a DNSSEC-enabled validating resolver can still look up all of
   these domain names as it always has under standard DNS.
   
   There are four (4) categories of responses (see :rfc:`4035`):
   
   *Secure*:
      Domains that have DNSSEC deployed correctly.

   *Insecure*:
      Domains that have yet to deploy DNSSEC.
   
   *Bogus*:
      Domains that have deployed DNSSEC but have done it incorrectly.
   
   *Indeterminate*:
      Domains for which it is not possible to determine whether these domains use DNSSEC.

   A DNSSEC-enabled validating resolver still resolves #1 and #2; only #3
   and #4 result in a SERVFAIL. You may already be using DNSSEC
   validation without realizing it, since some ISPs have begun enabling
   DNSSEC validation on their recursive name servers. Google public DNS
   (8.8.8.8) also has enabled DNSSEC validation.

Do I need to have special client software to use DNSSEC?
   No. DNSSEC only changes the communication
   behavior among DNS servers, not between a DNS server (validating resolver) and
   a client (stub resolver). With DNSSEC validation enabled on your recursive
   server, if a domain name does not pass the checks, an error message
   (typically SERVFAIL) is returned to clients; to most client
   software today, it appears that the DNS query has failed or that the domain
   name does not exist.

Since DNSSEC uses public key cryptography, do I need Public Key Infrastructure (PKI) in order to use DNSSEC?
   No, DNSSEC does not depend on an existing PKI. Public keys are stored within
   the DNS hierarchy; the trustworthiness of each zone is guaranteed by
   its parent zone, all the way back to the root zone. A copy of the trust
   anchor for the root zone is distributed with BIND 9.

Do I need to purchase SSL certificates from a Certificate Authority (CA) to use DNSSEC?
   No. With DNSSEC, you generate and publish your own keys, and sign your own
   data as well. There is no need to pay someone else to do it for you.

My parent zone does not support DNSSEC; can I still sign my zone?
   Technically, yes, but you will not get
   the full benefit of DNSSEC, as other validating resolvers are not
   able to validate your zone data. Without the DS record(s) in your parent
   zone, other validating resolvers treat your zone as an insecure
   (traditional) zone, and no actual verification is carried out.
   To the rest of the world, your zone still appears to be
   insecure, and it will continue to be insecure until your parent zone can
   host the DS record(s) for you and tell the rest of the world
   that your zone is signed.

Is DNSSEC the same thing as TSIG?
   No. TSIG is typically used
   between primary and secondary name servers to secure zone transfers,
   while DNSSEC secures DNS lookup by validating answers. Even if you enable
   DNSSEC, zone transfers are still not validated; to
   secure the communication between your primary and secondary name
   servers, consider setting up TSIG or similar secure channels.

How are keys copied from primary to secondary server(s)?
   DNSSEC uses public cryptography, which results in two types of keys: public and
   private. The public keys are part of the zone data, stored as DNSKEY
   record types. Thus the public keys are synchronized from primary to
   secondary server(s) as part of the zone transfer. The private keys are
   not, and should not be, stored anywhere other than secured on the primary server.
   See :ref:`advanced_discussions_key_storage` for
   more information on key storage options and considerations.

Can I use the same key for multiple zones?
   Yes and no. Good security practice
   suggests that you should use unique key pairs for each zone, just as
   you should have different passwords for your email account, social
   media login, and online banking credentials. On a technical level, it
   is completely feasible to reuse a key, but multiple zones are at risk if one key
   pair is compromised. However, if you have hundreds or thousands
   of zones to administer, a single key pair for all might be
   less error-prone to manage. You may choose to use the same approach as
   with password management: use unique passwords for your bank accounts and
   shopping sites, but use a standard password for your not-very-important
   logins. First, categorize your zones: high-value zones (or zones that have
   specific key rollover requirements) get their own key pairs, while other,
   more "generic" zones can use a single key pair for easier management. Note that
   at present (mid-2020), fully automatic signing (using the ``dnssec-policy``
   clause in your ``named`` configuration file) does not support reuse of keys
   except when the same zone appears in multiple views (see next question).
   To use the same key for multiple zones, sign your
   zones using semi-automatic signing. Each zone wishing to use the key
   should point to the same key directory.

How do I sign the different instances of a zone that appears in multiple views?
   Add a ``dnssec-policy`` statement to each ``zone`` definition in the
   configuration file. To avoid problems when a single computer accesses
   different instances of the zone while information is still in its cache
   (e.g., a laptop moving from your office to a customer site), you
   should sign all instances with the same key. This means setting the
   same DNSSEC policy for all instances of the zone, and making sure that the
   key directory is the same for all instances of the zone.

Will there be any problems if I change the DNSSEC policy for a zone?
   If you are using fully automatic signing, no. Just change the parameters in the
   ``dnssec-policy`` statement and reload the configuration file. ``named``
   makes a smooth transition to the new policy, ensuring that your zone
   remains valid at all times.
