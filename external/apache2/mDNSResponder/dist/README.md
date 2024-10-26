# Apple’s Open Source DNS Service Discovery Collection
This is Apple’s Open Source DNS Service Discovery Collection. The collection consists of a set of daemons, tools and libraries which can be used either together or separately when deploying and using DNS Service Discovery.  The collection consists of the following subsystems:
- Daemons
	- mDNS Responder Daemon
	- DNSSD Discovery Proxy
	- DNSSD Service Registration Protocol Advertising Proxy
	- DNSSD Service Registration Protocol Client
	- DNSSD Service Registration Protocol Update Proxy
- Systems:
	- OpenThread Stub Network Border Router
- Tools:
	- DNSSD Command-line tool
- Libraries:
	- DNSSD Client Library
For more information on the various parts of the collection, see the descriptions below.
## mDNS Responder Daemon
The mDNS Responder Daemon (mDNSResponder) serves both as a DNS Stub Resolver, as a resolver for information published using multicast DNS (mDNS), and as a publisher of mDNS information. mDNSResponder monitors multicast traffic on port 5353, the mDNS port, to keep track of services advertised on the local network. mDNSResponder performs DNS resolution for non-local queries, and resolves queries in the special “.local” domain using mDNS. mDNSResponder is used on macOS as the system resolver as well as providing Bonjour service advertising and discovery, and can provide the same services on other platforms, such as Linux and BSD.

[Click here to learn how to set up and use mDNSResponder.][1]
## OpenThread Stub Network Border Router
The OpenThread Stub Network Border Router can be used to provide _stub router_ service for Thread (802.15.4 mesh) networks using OpenThread. A stub router is a router that serves one or more isolated (stub) networks, and can connect automatically to an _infrastructure network_, such as a home Wi-Fi network. The purpose of a stub router is to allow:
- devices on the stub network to discover and be discovered by devices on the infrastructure network
- devices on the stub network to be reached by devices on the infrastructure network
- devices on the infrastructure network to reach devices on the stub network
What makes a stub router different than other routers is that the stub router only provides a way to reach one or more isolated networks. It will never provide a default route (for example to the Internet). It is not possible to reach a network other than the directly-connected stub network through a stub router. Stub routers need not participate in a routing protocol on the infrastructure network, and therefore do not require operator intervention in order to function.

[Click here to learn how to set up and use an Open Thread Stub Network Border Router][2]
## DNSSD Discovery Proxy
The DNSSD Discovery Proxy implements the IETF DNSSD Discovery Proxy ([RFC8766][3]) and DNS Push ([RFC 8765][4]). Together, these provide authoritative DNS service, for the purpose of DNS Service Discovery, using mDNS instead of a stateful DNS database. This allows the network infrastructure to provide DNS service discovery automatically over DNS, which eliminates the common problem on multi-link networks where services can only be discovered by a host when it happens to be connected to the correct link.

[Click here to learn how to set up and use a DNSSD Discovery Proxy][5]
## DNSSD Service Registration Protocol Advertising Proxy
The DNSSD Service Registration Protocol Advertising Proxy implements acts as a [DNSSD Service Registration Protocol][6] server: it accepts service registrations from SRP clients. Service registrations are then advertised on one or more infrastructure links using multicast DNS.

[Click here to learn how to set up and use a DNSSD Service Registration Protocol Advertising Proxy][7]
## DNSSD Service Registration Protocol Client
The DNSSD Service Registration Protocol Client implements the client side of the [DNSSD Service Registration Protocol][8]. The core client implementation is implemented in such a way that it can be readily embedded using a small API that must be implemented in the embedded environment. Two example APIs are provided, one for Thread, and another for Posix. The Posix implementation builds a command-line client that can either be used as a daemon to register a service, or used to validate various aspects of Service Registration Protocol implementations.

[Click here for more information about the DNSSD Service Registration Protocol Client][9]
## DNSSD Service Registration Protocol Update Proxy
The DNSSD Service Registration Protocol Update Proxy acts as a [DNSSD Service Registration Protocol][10] server: it accepts service registrations from SRP clients. The SRP registration is then used to generate a series of DNS Updates ([RFC2136][11]). These updates can be authenticated using TSIG. The SRP server responds to the client after all of the DNS Updates have completed, or responds when one part of the DNS update fails. The effect of running the SRP Protocol Update Proxy is as if the DNS server being updated were itself an SRP server.

[Click here to learn how to set up and use a DNSSD Service Registration Protocol Update Proxy][12]
## DNSSD command-line tool
The DNSSD command line tool (dns-sd) provides a way to exercise the services provided by mDNSResponder. Services can be advertised, browsed, and resolved. The tool provides a wide variety of different command-line options and is a great way to explore the functionality of DNS-SD.

[Click here to learn about the DNSSD command-line tool][13]
## DNSSD Client Library
The DNSSD Client Library provides, when used with the mDNS Responder daemon, a fully-featured DNS stub name resolution service, DNSSD advertising service, and DNSSD browsing and resolution service. The library is asynchronous, and can be easily integrated into an existing asynchronous server flow.

[Click here to learn about the DNSSD Client Library][14]

[Click here to learn about how mDNSResponder deals with time][15]
[1]:	Documents/mDNSResponder.md
[2]:	Documents/openthread-border-router.md
[3]:	https://www.rfc-editor.org/rfc/rfc8766.html "RFC8766"
[4]:	https://www.rfc-editor.org/rfc/rfc8765.html
[5]:	Documents/discovery-proxy.md
[6]:	https://datatracker.ietf.org/doc/draft-ietf-dnssd-srp/
[7]:	Documents/advertising-proxy.md
[8]:	https://datatracker.ietf.org/doc/draft-ietf-dnssd-srp/
[9]:	Documents/srp-client.md
[10]:	https://datatracker.ietf.org/doc/draft-ietf-dnssd-srp/
[11]:	https://tools.ietf.org/html/rfc2136
[12]:	Documents/srp-update-proxy.md
[13]:	Documents/dns-sd.md
[14]:	Documents/dnssd-client-library.md
[15]:   Documents/relative-time-in-mDNSResponder.md
