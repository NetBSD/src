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

.. iscman:: named-rrchecker
.. program:: named-rrchecker
.. _man_named-rrchecker:

named-rrchecker - syntax checker for individual DNS resource records
--------------------------------------------------------------------

Synopsis
~~~~~~~~

:program:`named-rrchecker` [**-h**] [**-o** origin] [**-p**] [**-u**] [**-C**] [**-T**] [**-P**]

Description
~~~~~~~~~~~

:program:`named-rrchecker` reads a single DNS resource record (RR) from standard
input and checks whether it is syntactically correct.

The input format is a minimal subset of the DNS zone file format. The entire input must be:
  CLASS TYPE RDATA

* Input must not start with an owner (domain) name
* The `CLASS` field is mandatory (typically ``IN``).
* The `TTL` field **must not** be present.
* RDATA format is specific to each RRTYPE.
* Leading and trailing whitespace in each field is ignored.

Format details can be found in :rfc:`1035#section-5.1` under ``<rr>``
specification. :rfc:`3597` format is also accepted in any of the input fields.
See :ref:`Examples`.


Options
~~~~~~~

.. option:: -o origin

   This option specifies the origin to be used when interpreting names in the record:
   it defaults to root (`.`). The specified origin is always taken as an absolute name.

.. option:: -p

   This option prints out the resulting record in canonical form. If there
   is no canonical form defined, the record is printed in :rfc:`3597` unknown
   record format.

.. option:: -u

   This option prints out the resulting record in :rfc:`3597` unknown record
   format.

.. option:: -C, -T, -P

   These options do not read input. They print out known classes, standard types,
   and private type mnemonics. Each item is printed on a separate line.
   The resulting list of private types may be empty

.. option:: -h

   This option prints out the help menu.


.. _examples:

Examples
~~~~~~~~
Pay close attention to the :manpage:`echo` command line options `-e` and `-n`, as they affect whitespace in the input to ``named-rrchecker``.

echo -n 'IN A 192.0.2.1' | named-rrchecker
  * Valid input is in :rfc:`1035` format with no newline at the end of the input.
  * Return code 0.

echo -e '\\n  \\n IN\\tA 192.0.2.1 \\t  \\n\\n  ' | named-rrchecker -p
  * Valid input with leading and trailing whitespace.
  * Output: ``IN	A	192.0.2.1``
  * Leading and trailing whitespace is not part of the output.


Relative names and origin
^^^^^^^^^^^^^^^^^^^^^^^^^
echo 'IN CNAME target' | named-rrchecker -p
  * Valid input with a relative name as the CNAME target.
  * Output: ``IN	CNAME	target.``
  * Relative name `target` from the input is converted to an absolute name using the default origin ``.`` (root).

echo 'IN CNAME target' | named-rrchecker -p -o origin.test
  * Valid input with a relative name as the CNAME target.
  * Output: ``IN	CNAME	target.origin.test.``
  * Relative name `target` from the input is converted to an absolute name using the specified origin ``origin.test``
echo 'IN CNAME target.' | named-rrchecker -p -o origin.test
  * Valid input with an absolute name as the CNAME target.
  * Output: ``IN	CNAME	target.``
  * The specified origin has no influence if `target` from the input is already absolute.


Special characters
^^^^^^^^^^^^^^^^^^
Special characters allowed in zone files by :rfc:`1035#section-5.1` are accepted.

echo 'IN CNAME t\\097r\\get\\.' | named-rrchecker -p -o origin.test
  * Valid input with backslash escapes.
  * Output: ``IN	CNAME	target\..origin.test.``
  * ``\097`` denotes an ASCII value in decimal, which, in this example, is the character ``a``.
  * ``\g`` is converted to a plain ``g`` because the ``g`` character does not have a special meaning and so the ``\`` prefix does nothing in this case.
  * ``\.`` denotes a literal ASCII dot (here as a part of the CNAME target name). Special meaning of ``.`` as the DNS label separator was disabled by the preceding ``\`` prefix.

echo 'IN CNAME @' | named-rrchecker -p -o origin.test
  * Valid input with ``@`` used as a reference to the specified origin.
  * Output: ``IN	CNAME	origin.test.``

echo 'IN CNAME \\@' | named-rrchecker -p -o origin.test
  * Valid input with a literal ``@`` character (escaped).
  * Output: ``IN	CNAME	\@.origin.test.``

echo 'IN CNAME prefix.@' | named-rrchecker -p -o origin.test
  * Valid input with ``@`` used as a reference to the specifed origin.
  * Output: ``IN	CNAME	prefix.\@.origin.test.``
  * ``@`` has special meaning only if it is free-standing.

echo 'IN A 192.0.2.1; comment' | named-rrchecker -p
  * Valid input with a trailing comment. Note the lack of whitespace before the start of the comment.
  * Output: ``IN	A	192.0.2.1``

For multi-line examples see the next section.

Multi-token records
^^^^^^^^^^^^^^^^^^^
echo -e 'IN TXT two words \\n' | named-rrchecker -p
  * Valid TXT RR with two unquoted words and trailing whitespace.
  * Output: ``IN	TXT	"two" "words"``
  * Two unquoted words in the input are treated as two `<character-string>`\ s per :rfc:`1035#section-3.3.14`.
  * Trailing whitespace is omitted from the last `<character-string>`.

echo -e 'IN TXT "two words" \\n' | named-rrchecker -p
  * Valid TXT RR with one `character-string` and trailing whitespace.
  * Output: ``IN	TXT	"two words"``

echo -e 'IN TXT "problematic newline\\n"' | named-rrchecker -p
  * Invalid input - the closing ``"`` is not detected before the end of the line.

echo 'IN TXT "with newline\\010"' | named-rrchecker -p
  * Valid input with an escaped newline character inside `character-string`.
  * Output: ``IN	TXT	"with newline\010"``

echo -e 'IN TXT ( two\\nwords )' | named-rrchecker -p
  * Valid multi-line input with line continuation allowed inside optional parentheses in the RDATA field.
  * Output: ``IN	TXT	"two" "words"``

echo -e 'IN TXT ( two\\nwords ; misplaced comment )' | named-rrchecker -p
  * Invalid input - comments, starting with ";", are ignored by the parser, so the closing parenthesis should be before the semicolon.

echo -e 'IN TXT ( two\\nwords ; a working comment\\n )' | named-rrchecker -p
  * Valid input - the comment is terminated with a newline.
  * Output: ``IN	TXT	"two" "words"``

echo 'IN HTTPS 1 . alpn="h2,h3"' | named-rrchecker -p
  * Valid HTTPS record
  * Output: ``IN	HTTPS	1 . alpn="h2,h3"``

echo -e 'IN HTTPS ( 1 \\n . \\n alpn="dot")port=853' | named-rrchecker -p
  * Valid HTTPS record with individual sub-fields split across multiple lines
    using :rfc:`1035#section-5.1` parentheses syntax to group data that crosses
    a line boundary.
  * Note the missing whitespace between the closing parenthesis and adjacent tokens.
  * Output: ``IN	HTTPS	1 . alpn="dot" port=853``


Unknown type handling
^^^^^^^^^^^^^^^^^^^^^

echo 'IN A 192.0.2.1' | named-rrchecker -u
  * Valid input in :rfc:`1035` format.
  * Output in :rfc:`3957` format: ``CLASS1	TYPE1	\# 4 C0000201``

echo 'CLASS1 TYPE1 \\# 4 C0000201' | named-rrchecker -p
  * Valid input in :rfc:`3597` format.
  * Output in :rfc:`1035` format: ``IN	A	192.0.2.1``

echo 'IN A \\# 4 C0000201' | named-rrchecker -p
  * Valid input with class and type in :rfc:`1035` format and rdata in :rfc:`3597` format.
  * Output in :rfc:`1035` format: ``IN	A	192.0.2.1``

echo 'IN HTTPS 1 . key3=\\001\\000' | named-rrchecker -p
  * Valid input with :rfc:`9460` syntax for an unknown `key3` field. Syntax ``\001\000`` produces two octets with values 1 and 0, respectively.
  * Output: ``IN	HTTPS	1 . port=256``
  * `key3` matches the standardized key name `port`.
  * Octets 1 and 0 were decoded as integer values in big-endian encoding.

echo 'IN HTTPS 1 . key3=\\001' | named-rrchecker -p
  * Invalid input - the length of the value for `key3` (i.e. port) does not match the known standard format for that parameter in the SVCB RRTYPE.

echo 'IN HTTPS 1 . port=\\001\\000' | named-rrchecker -p
  * Invalid input - the key `port`, when specified using its standard mnemonic name, **must** use standard key-specific syntax.

Meta values
^^^^^^^^^^^

echo 'IN AXFR' | named-rrchecker
  * Invalid input - AXFR is a meta type, not a genuine RRTYPE.

echo 'ANY A 192.0.2.1' | named-rrchecker
  * Invalid input - ANY is meta class, not a true class.

echo 'A 192.0.2.1' | named-rrchecker
  * Invalid input - the class field is missing, so the parser would try and fail to interpret the RRTYPE A as the class.


Return Codes
~~~~~~~~~~~~

0
  The whole input was parsed as one syntactically valid resource record.

1
  The input is not a syntactically valid resource record, or the given type is not
  supported, or either/both class and type are meta-values, which should not appear in zone files.


See Also
~~~~~~~~

:rfc:`1034`, :rfc:`1035`, :rfc:`3957`, :iscman:`named(8) <named>`.
