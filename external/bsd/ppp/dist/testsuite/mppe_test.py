#!/usr/bin/env python3
# MPPE strength negotiation: require-mppe must allow both 40-bit and
# 128-bit encryption (so two require-mppe peers settle on 128-bit and a
# require-mppe peer can talk to a 128-bit-only one), require-mppe-40 still
# limits the link to 40-bit, and "require-mppe nomppe-40" must not let the
# link come up without MPPE.

import os
import subprocess

from pppfns import (
    IS_LINUX, PPPD, PPPD_PEER, PppPair, require_link_env, sudo_prefix,
    test_fail, test_skipped,
)

require_link_env()

for binary in {PPPD, PPPD_PEER}:
    with open(binary, 'rb') as f:
        if b'require-mppe' not in f.read():
            test_skipped(f'{binary} is built without MPPE support')
if IS_LINUX:
    subprocess.run(sudo_prefix() + ['modprobe', 'ppp_mppe'],
                   capture_output=True)
    if not os.path.exists('/sys/module/ppp_mppe'):
        test_skipped('no kernel MPPE support (ppp_mppe)')

# MPPE keys come from MS-CHAP, so b authenticates a with MS-CHAPv2.
server_kwargs = dict(noauth=False, chap_secrets='cli\tsrv\t"s3cret"\t*\n')
server_options = ['auth', 'require-mschap-v2', 'name', 'srv']
client_options = ['user', 'cli', 'remotename', 'srv', 'password', 's3cret']

CCP_UP = r'\S.* compression enabled'


def pair(name, a_mppe, b_mppe):
    return PppPair(a_options=client_options + a_mppe,
                   b_options=server_options + b_mppe, b_kwargs=server_kwargs,
                   name=name)


def check(name, a_mppe, b_mppe, expect):
    print(f'{" ".join(a_mppe)} vs {" ".join(b_mppe)}:')
    with pair(name, a_mppe, b_mppe) as p:
        p.up()
        for peer in (p.a, p.b):
            got = peer.wait_for_text(CCP_UP).group(0)
            print(f'  {peer.name}: {got}')
            if got != f'MPPE {expect} stateless compression enabled':
                test_fail(f'pppd {peer.name}: expected MPPE {expect}, '
                          f'got "{got}"')
        p.down()


check('both', ['require-mppe'], ['require-mppe'], '128-bit')
check('plus-128', ['+mppe'], ['require-mppe-128'], '128-bit')
check('no128', ['require-mppe', 'nomppe-128'], ['require-mppe'], '40-bit')
check('only40', ['require-mppe-40'], ['require-mppe-40'], '40-bit')

print('require-mppe nomppe-40 vs no MPPE options:')
with pair('refuse', ['require-mppe', 'nomppe-40'], []) as p:
    p.start()
    got = p.a.wait_for_text(CCP_UP + '|MPPE required but peer '
                            'negotiation failed').group(0)
    print(f'  a: {got}')
    if 'MPPE required' not in got:
        test_fail(f'pppd a: link came up without MPPE ("{got}")')
