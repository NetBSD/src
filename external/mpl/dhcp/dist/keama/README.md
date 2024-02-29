# KEA Migration Assistant Short Guide.

The KEA Migration Assistant (aka _keama_) is an experimental tool
which helps to translate ISC DHCP configurations to Kea.

## How to get last sources

From time to time the _keama_ is upgraded for bug fixes, support of
new or not yet ISC DHCP features or more likely support of new KEA
features.

As now _keama_ is included in ISC DHCP the most recent code can be
found with the most recent ISC DHCP code in the master branch of the
gitlab repository.

## How to build and install

After the ISC DHCP build go to the keama directory and type:
```console
make
```
To install it:
```console
make install
```

## Known limitations

_keama_ uses a subset of the ISC DHCP configuration file parser with a lot
of sanaity checks removed so it does not know how to handle an incorrect
ISC DHCP configuration file and eventually can even crash on it.

ISC DHCP and KEA have different models for many things, for instance
ISC DHCP supports the failover protocol when KEA supports High Availability.
In some cases _keama_ tries to cope with that, for instance for host
reservations which are global in ISC DHCP and by default per subnet in KEA.

## How to use

The manual explains how parameters guide _keama_ choices for lifetimes,
name literals, host reservation scope, etc. Directives were added to
the ISC DHCP syntax (they are valid but ignored) for options.

Each time _keama_ finds a feature it can't translate it emits a comment
with a reference to the feature description in a kea (not isc dhcp) gitlab
issue in the "ISC DHCP Migration" milestone. The number of reports is
returned by _keama_ when it exits.

## How to help

If you have configuration patterns you would like to see supported
by Keama please feel free to reach out to us. We are always looking
to improve the tool.
