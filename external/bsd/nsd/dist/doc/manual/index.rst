Name Server Daemon (NSD) by NLnet Labs
======================================

Welcome to the documentation of the NLnet Labs Name Server Daemon (NSD), an
authoritative DNS name server. It has been developed for operations in
environments where speed, reliability, stability and security are of high
importance.

NSD is designed with a pure philosophy that prioritises raw performance. This
means that if you serve hundreds of thousands or even millions of queries per
second, NSD is the leading implementation in the world. This authoritative DNS
name server strives to be a reference implementation for emerging standards of
the Internet Engineering Task Force (IETF). NSD is distributed free of charge in
open source form under the BSD license. For most platforms, `packages
<https://repology.org/project/nsd/versions>`_ are available.

This documentation is an open source project maintained by NLnet Labs. is edited
via text files in the `reStructuredText
<http://www.sphinx-doc.org/en/stable/rest.html>`_ markup language and then
compiled into a static website/offline document using the open source `Sphinx
<http://www.sphinx-doc.org>`_ and `ReadTheDocs <https://readthedocs.org/>`_
tools.

We always appreciate your feedback and improvements. You can submit an issue or
pull request on the `GitHub repository
<https://github.com/NLnetLabs/nsd-manual/issues>`_, or post a message on the
`NSD users <https://lists.nlnetlabs.nl/mailman/listinfo/nsd-users>`_ mailing
list. All the contents are under the permissive Creative Commons Attribution 3.0
(`CC-BY 3.0 <https://creativecommons.org/licenses/by/3.0/>`_) license, with
attribution to NLnet Labs.

.. toctree::
   :maxdepth: 2
   :caption: Getting Started

   installation
   configuration
   zonefile
   catalog-zones
   xdp

.. toctree::
   :maxdepth: 2
   :caption: Running

   running/logging
   running/using-tsig
   running/zone-expiry
   running/interfaces
   running/tuning

.. toctree::
   :maxdepth: 1
   :caption: Manual Pages

   manpages/nsd
   manpages/nsd-checkconf
   manpages/nsd-checkzone
   manpages/nsd.conf
   manpages/nsd-control

.. toctree::
   :maxdepth: 2
   :caption: Reference

   reference/configure-options
   reference/log-diagnosis
   reference/rfc-compliance

.. history
.. authors
.. license
