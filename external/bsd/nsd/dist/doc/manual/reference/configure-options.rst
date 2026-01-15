Configure Options
=================

NSD can be configured using GNU autoconf's configure script. In addition to
standard configure options, one may use the following:

CC=compiler
    Specify the C compiler. The default is gcc or cc. The compiler must support
    ANSI C89.

CPPFLAGS=flags
    Specify the C preprocessor flags.  Such as ``-I<includedir>``.

CFLAGS=flags
    Specify the C compiler flags. These include code generation, optimisation,
    warning, and debugging flags. These flags are also passed to the linker.

    The default for gcc is ``-g -O2``.

LD=linker
    Specify the linker (defaults to the C compiler).

LDFLAGS=flags
    Specify linker flags.

LIBS=libs
    Specify additional libraries to link with.

--disable-ipv6
    Disables IPv6 support in NSD.

--enable-checking
    Enable some internal development checks.  Useful if you want to modify NSD.
    This option enables the standard C "assert" macro and compiler warnings.

    This will instruct NSD to be stricter when validating its input. This could
    lead to a reduced service level.

--disable-bind8-stats
    Disables BIND8-like statistics.

--disable-zone-stats

    Disable per-zone statistics gathering (if enabled, it needs
    bind8-stats).

--disable-ratelimit
    Disables rate limiting, based on query name, type and source.

--disable-ratelimit-default-is-off
    Disable this to set default of ratelimit to on (this controls
    the default, ratelimits can be enabled and disabled in nsd.conf).

--disable-dnstap

    Disable dnstap support (requires fstrm-devel, protobuf-c).

--enable-systemd

    Compile with systemd support, and then the server notifies libsystemd
    when the server is up (it needs pkg-config and systemd-devel).

--with-configdir=dir
    Specified, NSD configuration directory, default :file:`/etc/nsd`.

--with-nsd_conf_file=path
    Pathname to the NSD configuration file, default :file:`/etc/nsd/nsd.conf`.

--with-pidfile=path
    Pathname to the NSD pidfile, default is platform specific, mostly
    :file:`/var/run/nsd.pid`.

--with-zonesdir=dir
    NSD default location for master zone files, default :file:`/etc/nsd/`.

--with-user=username
    User name or ID to answer the queries with, default is ``nsd``.

--with-facility=facility
    Specify the syslog facility to use.  The default is LOG_DAEMON. See the
    syslog(3) manual page for the available facilities.

--with-libevent=path
    Specity the location of the ``libevent`` library (or libev).
    ``--with-libevent=no`` uses a builtin portable implementation (select()).

--with-ssl=path
    Specify the location of the OpenSSL libraries. OpenSSL 0.9.7 or higher is
    required for TSIG support.

--with-start_priority=number
    Startup priority for NSD.

--with-kill_priority=number
    Shutdown priority for NSD.

--with-tcp-timeout=number
    Set the default TCP timeout (in seconds). The default is 120 seconds.

--disable-nsec3
    Disable NSEC3 support. With NSEC3 support enabled, very large zones, also
    non-NSEC3 zones, use about 20% more memory.

--disable-minimal-responses
    Disable minimal responses. If disabled, responses are more likely to get
    truncated, resulting in TCP fallback.  When enabled (by default) NSD will
    leave out RRsets to make responses fit inside one datagram, but for shorter
    responses the full normal response is carried.

--disable-largefile
    Disable large file support (64 bit file lengths). Makes off_t a 32bit length
    during compilation.
