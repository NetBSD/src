# dhcpcd

dhcpcd is a
[DHCP](http://en.wikipedia.org/wiki/Dynamic_Host_Configuration_Protocol) and a
[DHCPv6](http://en.wikipedia.org/wiki/DHCPv6) client.
It's also an IPv4LL (aka [ZeroConf](http://en.wikipedia.org/wiki/Zeroconf))
client.
In layman's terms, dhcpcd runs on your machine and silently configures your
computer to work on the attached networks without trouble and mostly without
configuration.

If you're a desktop user then you may also be interested in
[Network Configurator (dhcpcd-ui)](http://roy.marples.name/projects/dhcpcd-ui)
which sits in the notification area and monitors the state of the network via
dhcpcd.
It also has a nice configuration dialog and the ability to enter a pass phrase
for wireless networks.

dhcpcd may not be the only daemon running that wants to configure DNS on the
host, so it uses [openresolv](http://roy.marples.name/projects/openresolv)
to ensure they can co-exist.

See [BUILDING.md](BUILDING.md) for how to build dhcpcd.

If you wish to file a support ticket or help out with development, please
[visit the Development Area](https://dev.marples.name/project/profile/101/)
or join the mailing list below.

## Configuration

You should read the
[dhcpcd.conf man page](http://roy.marples.name/man/html5/dhcpcd.conf.html)
and put your options into `/etc/dhcpcd.conf`.
The default configuration file should work for most people just fine.
Here it is, in case you lose it.

```
# A sample configuration for dhcpcd.
# See dhcpcd.conf(5) for details.

# Allow users of this group to interact with dhcpcd via the control socket.
#controlgroup wheel

# Inform the DHCP server of our hostname for DDNS.
hostname

# Use the hardware address of the interface for the Client ID.
#clientid
# or
# Use the same DUID + IAID as set in DHCPv6 for DHCPv4 ClientID as per RFC4361.
# Some non-RFC compliant DHCP servers do not reply with this set.
# In this case, comment out duid and enable clientid above.
duid

# Persist interface configuration when dhcpcd exits.
persistent

# Rapid commit support.
# Safe to enable by default because it requires the equivalent option set
# on the server to actually work.
option rapid_commit

# A list of options to request from the DHCP server.
option domain_name_servers, domain_name, domain_search, host_name
option classless_static_routes
# Respect the network MTU. This is applied to DHCP routes.
option interface_mtu

# Most distributions have NTP support.
#option ntp_servers

# A ServerID is required by RFC2131.
require dhcp_server_identifier

# Generate SLAAC address using the Hardware Address of the interface
#slaac hwaddr
# OR generate Stable Private IPv6 Addresses based from the DUID
slaac private
```

The [dhcpcd man page](/man/html8/dhcpcd.html) has a lot of the same options and more, which only apply to calling dhcpcd from the command line.


## Compatibility
dhcpcd-5 is only fully command line compatible with dhcpcd-4
For compatibility with older versions, use dhcpcd-4

## Upgrading
dhcpcd-7 defaults the database directory to `/var/db/dhcpcd` instead of
`/var/db` and now stores dhcpcd.duid and dhcpcd.secret in there instead of
in /etc.
The Makefile `_confinstall` target will attempt to move the files correctly from
the old locations to the new locations.
Of course this won't work if dhcpcd-7 is packaged up, so packagers will need to
install similar logic into their dhcpcd package.

## ChangeLog
We no longer supply a ChangeLog.
However, you're more than welcome to read the
[commit log](http://roy.marples.name/git/dhcpcd.git/log/) and
[archived release announcements](http://roy.marples.name/archives/dhcpcd-discuss/).
