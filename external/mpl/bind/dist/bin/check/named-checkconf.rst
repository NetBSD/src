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

.. _man_named-checkconf:

named-checkconf - named configuration file syntax checking tool
---------------------------------------------------------------

Synopsis
~~~~~~~~

:program:`named-checkconf` [**-chjlvz**] [**-p** [**-x** ]] [**-t** directory] {filename}

Description
~~~~~~~~~~~

``named-checkconf`` checks the syntax, but not the semantics, of a
``named`` configuration file. The file is parsed and checked for syntax
errors, along with all files included by it. If no file is specified,
``/etc/named.conf`` is read by default.

Note: files that ``named`` reads in separate parser contexts, such as
``rndc.key`` and ``bind.keys``, are not automatically read by
``named-checkconf``. Configuration errors in these files may cause
``named`` to fail to run, even if ``named-checkconf`` was successful.
``named-checkconf`` can be run on these files explicitly, however.

Options
~~~~~~~

**-h**
   Print the usage summary and exit.

**-j**
   When loading a zonefile read the journal if it exists.

**-l**
   List all the configured zones. Each line of output contains the zone
   name, class (e.g. IN), view, and type (e.g. master or slave).

**-c**
   Check "core" configuration only. This suppresses the loading of
   plugin modules, and causes all parameters to ``plugin`` statements to
   be ignored.

**-i**
   Ignore warnings on deprecated options.

**-p**
   Print out the ``named.conf`` and included files in canonical form if
   no errors were detected. See also the ``-x`` option.

**-t** directory
   Chroot to ``directory`` so that include directives in the
   configuration file are processed as if run by a similarly chrooted
   ``named``.

**-v**
   Print the version of the ``named-checkconf`` program and exit.

**-x**
   When printing the configuration files in canonical form, obscure
   shared secrets by replacing them with strings of question marks
   ('?'). This allows the contents of ``named.conf`` and related files
   to be shared MDASH for example, when submitting bug reports MDASH
   without compromising private data. This option cannot be used without
   ``-p``.

**-z**
   Perform a test load of all master zones found in ``named.conf``.

filename
   The name of the configuration file to be checked. If not specified,
   it defaults to ``/etc/named.conf``.

Return Values
~~~~~~~~~~~~~

``named-checkconf`` returns an exit status of 1 if errors were detected
and 0 otherwise.

See Also
~~~~~~~~

:manpage:`named(8)`, :manpage:`named-checkzone(8)`, BIND 9 Administrator Reference Manual.
