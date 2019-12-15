# NSD

[![Travis Build Status](https://travis-ci.org/NLnetLabs/nsd.svg?branch=master)](https://travis-ci.org/NLnetLabs/nsd)
[![Packaging status](https://repology.org/badge/tiny-repos/nsd.svg)](https://repology.org/project/nsd/versions)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/1462/badge)](https://bestpractices.coreinfrastructure.org/projects/1462)

The NLnet Labs Name Server Daemon (NSD) is an authoritative DNS name server.
It has been developed for operations in environments where speed,
reliability, stability and security are of high importance.  If you
have any feedback, we would love to hear from you. Don’t hesitate to
[create an issue on Github](https://github.com/NLnetLabs/nsd/issues/new)
or post a message on the
[NSD mailing list](https://nlnetlabs.nl/mailman/listinfo/nsd-users).
You can lean more about NSD by reading our
[documentation](https://nlnetlabs.nl/documentation/nsd/).

## Compiling

Make sure you have the C toolchain, OpenSSL and its include files, and
libevent with its include files and flex and bison installed.
The repository does not contain ./configure and you can generate it like
this (./configure is included in release tarballs, and then you do not
have to generate it first):

```
aclocal && autoconf && autoheader
```

NSD can be compiled and installed using:

```
./configure && make && make install
```

## NSD configuration

The configuration options for NSD are described in the man pages, which are
installed (use `man nsd.conf`) and are available on the NSD
[documentation page](https://nlnetlabs.nl/documentation/nsd/).

An example configuration file is located in
[nsd.conf.sample](https://github.com/NLnetLabs/nsd/blob/master/nsd.conf.sample.in).
