Diagnosing NSD Log Entries
==========================

NSD will print log messages to the system log (or ``logfile:`` configuration
entry). Some of these messages are covered here.

Reload process <pid> failed with status <s>, continuing with old database
    This log message indicates the reload process of NSD has failed for some
    reason.  This can be anything from a missing database file to internal
    errors.

snipping off trailing partial part of <ixfr.db>
    The file :file:`ixfr.db` contains only part of expected data. The corruption
    is removed by snipping off the trailing part.

memory recyclebin holds <num> bytes
    This is printed for every reload. NSD allocates and deallocates memory to
    service IXFR updates. The recycle bin holds deallocated memory ready for
    future use. If the number grows too large, a restart resets it.

xfrd: max number of tcp connections (32) reached
    This line is printed when more than 32 zones need a zone transfer at the
    same time.  The value is a compile constant (``xfrd-tcp.h``), but if this
    happens often for you, we could make this a config option.  NSD will reuse
    existing TCP connections to the same primary (determined by IP address) to
    transfer up to 64k zones from that primary.  Thus this error should only
    happen with more than 32 primaries or more than 64\*32=2M zones that need to
    be updated at the same time.

    If this happens, more zones have to wait until a zone transfer completes
    (or is aborted) before they can have a zone transfer too. This waiting
    list has no size limit.

error: <zone> NSEC3PARAM entry <num> has unknown hash algo <number>
    This error means that the zone has NSEC3 chain(s) with hash algorithms that
    are not supported by this version of NSD, and thus cannot be served by NSD.
    If there are also no NSECs or NSEC3 chain(s) with known hash algorithms, NSD
    will not be able to serve DNSSEC authenticated denials for the zone.
