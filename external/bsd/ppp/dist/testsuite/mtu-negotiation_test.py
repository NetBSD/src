#!/usr/bin/env python3
# LCP MRU negotiation: when side a requests "mru 512" the peer must limit
# what it sends, which shows up as MTU 512 on the peer's ppp interface.
# A large ping (forced through the small MTU) must still get through.

from pppfns import IP_A, IS_LINUX, PppPair, require_link_env, test_fail

require_link_env()

MRU = 512

with PppPair(a_options=['mru', str(MRU)]) as pair:
    pair.up()

    mtu_b = pair.b.link_mtu()
    print(f'b interface mtu: {mtu_b}')
    if mtu_b != MRU:
        test_fail(f'peer MTU is {mtu_b}, expected negotiated MRU {MRU}')

    # ICMP payload larger than the MTU still arrives (IP fragmentation).
    # Only meaningful with namespaces: without them the ping would
    # short-circuit via loopback.
    if IS_LINUX and pair.b.ping(IP_A, count=2, size=1000) < 1:
        test_fail('MTU-exceeding ping failed across the link')
