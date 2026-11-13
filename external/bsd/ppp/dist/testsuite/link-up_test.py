#!/usr/bin/env python3
# Two pppd instances negotiate a link over a socketpair: IPCP must assign
# the expected addresses on both sides, the ppp interfaces must show the
# right local/peer addresses, and SIGTERM must produce a clean shutdown.

from pppfns import (
    EXIT_HANGUP, EXIT_OK, EXIT_PEER_DEAD, EXIT_USER_REQUEST, IS_LINUX,
    IP_A, IP_B, PppPair, require_link_env, test_fail,
)

require_link_env()

with PppPair() as pair:
    pair.up()

    for peer, lip, rip in ((pair.a, IP_A, IP_B), (pair.b, IP_B, IP_A)):
        addr = peer.ip_addr()
        print(f'{peer.name}: {addr.strip()}')
        if lip not in addr or rip not in addr:
            test_fail(f'pppd {peer.name}: expected {lip} peer {rip} on '
                      f'{peer.ifname}, got:\n{addr}')

    # Clean shutdown: SIGTERM the b side; it exits with EXIT_USER_REQUEST
    # after an LCP terminate handshake, and the a side sees the link drop.
    pair.b.stop()
    if pair.b.exitcode != EXIT_USER_REQUEST:
        test_fail(f'pppd b exited {pair.b.exitcode}, '
                  f'expected {EXIT_USER_REQUEST} (user request)')
    if 'Connection terminated' not in pair.b.log_text():
        test_fail('pppd b log lacks "Connection terminated"')
    # The a side sees the link drop when b goes away. On Linux b's socket
    # EOFs as it exits, so a reports a hangup/dead peer; on Solaris the LCP
    # terminate handshake completes cleanly first and a exits EXIT_OK.
    accepted = {EXIT_USER_REQUEST, EXIT_PEER_DEAD, EXIT_HANGUP}
    if not IS_LINUX:
        accepted.add(EXIT_OK)
    pair.a.expect_exit(accepted)
