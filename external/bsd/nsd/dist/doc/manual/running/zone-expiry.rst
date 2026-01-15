Zone Expiry of Secondary Zones
==============================

NSD will keep track of the status of secondary zones, according to the timing
values in the SOA record for the zone. When the refresh time of a zone is
reached, the serial number is checked and a zone transfer is started if the zone
has changed. Each primary server is tried in turn.

Primary zones cannot expire so they are always served. Zones are interpreted
as primary zones if they have no ``request-xfr:`` statements in the config file.

After the expire timeout (from the SOA record at the zone apex) is reached, the
zone becomes expired. NSD will return ``SERVFAIL`` for expired zones, and will
attempt to perform a zone transfer from any of the primaries. After a zone
transfer succeeds, or if the primary indicates that the SOA serial number is
still the same, the zone returns to an operational state.

In contrast with e.g. BIND, the inception time for a secondary zone is stored on
disk (in ``xfrdfile: "xfrd.state"``), together with timeouts. If a secondary
zone acquisition time is recent enough, NSD can start serving a
zone immediately on loading, without querying the primary server.

If a secondary zone has expired and no primaries can be reached, but NSD
should still serve the zone, delete the :file:`xfrd.state`
file, but leave the zone file for the zone intact. Make sure to stop NSD before
you delete the file, as NSD writes it on exit. Upon loading NSD will treat the
zone file that you as operator have provided as recent and will serve the zone.
Even though NSD will start to serve the zone immediately, the zone will expire
after the timeout is reached again. NSD will also attempt to confirm that you
have provided the correct data by polling the primaries. So when the primary
servers come back up, it will transfer the updated zone within <retry timeout
from SOA> seconds.

It is possible to provide zone files for both primary and secondary
zones via alternative means (say from email or rsync). Reload with SIGHUP or
:command:`nsd-control reload` to read the new zone file contents into the name
database. When this is done the new zone will be served. For primary zones, NSD
will issue notifications to all configured ``notify:`` targets. For secondary
zones the above happens; NSD attempts to validate the zone from the primary
(checking its SOA serial number).
