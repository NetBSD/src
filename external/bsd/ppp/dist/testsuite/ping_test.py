#!/usr/bin/env python3
# The core IP routing test: each side lives in its own network namespace,
# so the only path between the two addresses is the ppp link. Ping must
# work in both directions across it.

import shutil

from pppfns import IP_A, IP_B, PppPair, require_link_env, test_fail, test_skipped

require_link_env(routing=True)
if shutil.which('ping') is None:
    test_skipped('ping not found')

with PppPair() as pair:
    pair.up()

    for peer, target in ((pair.a, IP_B), (pair.b, IP_A)):
        got = peer.ping(target, count=3)
        print(f'{peer.name}: {got}/3 replies from {target}')
        if got < 1:
            test_fail(f'pppd {peer.name}: no ping replies from {target} '
                      f'across the ppp link')
