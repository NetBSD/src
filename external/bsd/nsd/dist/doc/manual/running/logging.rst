Logging
=======

NSD does not provide any DNS logging. We believe that this is a separate task
and has to be done independently from the core operation. This decision was taken
in order to keep NSD focused and minimise its complexity.
It is better to leave logging and tracing to separate dedicated tools. Do note,
however, that NSD can be compiled with support for DNSTAP (see ``nsd.conf(5)``).

If some visibility on individual queries is required, consider running
``tcpdump(1)`` on the server, using an appropriate filter rule to capture UDP
and TCP packets to port 53. The tcpdump on most systems will decode the packets
into readable requests and responses.

The `CAIDA dnsstat tool <https://www.caida.org/catalog/software/dnsstat/>`_ can
easily be configured and/or modified to suit local statistics requirements
without any danger of affecting the name server itself. We have run ``dnsstat``
on the same machine as NSD, and we would recommend using a multiprocessor if
performance is an issue. Of course, ``dnsstat`` can also run on a separate
machine that has MAC layer access to the network of the server.

The :command:`nsd-control` tool can output some statistics, with
:command:`nsd-control stats` and :command:`nsd-control stats_noreset`.  In
`contrib/nsd_munin_
<https://github.com/NLnetLabs/nsd/blob/master/contrib/nsd_munin_>`_ there is a
Munin grapher plugin that uses it.  The output of :command:`nsd-control stats`
is easy to read (text only) with scripts.  The output values are documented on
the :command:`nsd-control` man page.

Another available tool is `dnstop
<http://dns.measurement-factory.com/tools/dnstop/>`_, which displays DNS
statistics on your network.
