Interfaces
==========

NSD will by default bind itself to the system default interface and service IPv4
and if available also IPv6. It is possible to service only IPv4 or IPv6 using
the :option:`-4`, :option:`-6` command line options, or the ``ip4-only`` and
``ip6-only`` config file options.

The command line option :option:`-a` and config file option ip-address can be
given to bind to specific interfaces. Multiple interfaces can be specified,
which is useful for two reasons:

- The specific interface bound will result in the OS bypassing routing tables
  for the interface selection. This results in a small performance gain. It is
  not the performance gain that is the problem: sometimes the routing tables can
  give the wrong answer, see the next point.
- The answer will be routed via the interface the query came from. This makes
  sure that the return address on the DNS replies is the same as the query was
  sent to. Many resolvers require the source address of the replies to be
  correct.  The ``ip-address:`` option is easier than configuring the OS routing
  table to return the DNS replies via the correct interface.

The above means that even for systems with multiple interfaces where you intend
to provide DNS service to all interfaces, it is prudent to specify all the
interfaces as ``ip-address`` config file options.

With the config file option ``ip-transparent`` you can allow NSD to bind to
non-local addresses.