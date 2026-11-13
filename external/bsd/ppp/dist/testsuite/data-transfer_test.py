#!/usr/bin/env python3
# Bulk data integrity across the link: push random data over a TCP
# connection that traverses the ppp link and compare checksums. Catches
# HDLC framing/escaping corruption. Run twice: with the default asyncmap
# (all control characters escaped) and with asyncmap 0.

import hashlib
import os
import shutil
import subprocess

from pppfns import (
    IP_A, IP_B, SCRATCHDIR, PppPair, require_link_env, test_fail, test_skipped,
)

require_link_env(routing=True)
if shutil.which('python3') is None:
    test_skipped('python3 not found')

PORT = 7771
SIZE = 2 * 1024 * 1024

# ip, port and file are passed as argv so paths never need quoting inside
# the generated source.
RECEIVER = """
import socket, sys
ip, port, path = sys.argv[1], int(sys.argv[2]), sys.argv[3]
s = socket.socket()
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind((ip, port))
s.listen(1)
sys.stdout.write('listening\\n')
sys.stdout.flush()
c, _ = s.accept()
n = 0
with open(path, 'wb') as f:
    while True:
        d = c.recv(65536)
        if not d:
            break
        f.write(d)
        n += len(d)
sys.stdout.write('received %d\\n' % n)
"""

SENDER = """
import socket, sys
ip, port, path = sys.argv[1], int(sys.argv[2]), sys.argv[3]
s = socket.create_connection((ip, port), timeout=30)
with open(path, 'rb') as f:
    s.sendfile(f)
s.close()
"""


def transfer(pair, label):
    data = os.urandom(SIZE)
    srcfile = SCRATCHDIR / f'{label}-src.bin'
    dstfile = SCRATCHDIR / f'{label}-dst.bin'
    srcfile.write_bytes(data)

    recv = pair.a.popen_in_ns(
        ['python3', '-u', '-c', RECEIVER, IP_A, str(PORT), str(dstfile)],
        stdout=subprocess.PIPE, text=True)
    try:
        if recv.stdout.readline().strip() != 'listening':
            test_fail(f'{label}: receiver did not start listening')
        send = pair.b.run_in_ns(
            ['python3', '-c', SENDER, IP_A, str(PORT), str(srcfile)],
            check=False, timeout=120)
        if send.returncode != 0:
            test_fail(f'{label}: sender failed:\n{send.stdout}\n{send.stderr}')
        if recv.wait(timeout=60) != 0:
            test_fail(f'{label}: receiver failed')
    finally:
        if recv.poll() is None:
            recv.kill()

    got = hashlib.sha256(dstfile.read_bytes()).hexdigest()
    want = hashlib.sha256(data).hexdigest()
    if got != want:
        test_fail(f'{label}: data corrupted across the link '
                  f'({dstfile.stat().st_size} of {SIZE} bytes, checksum mismatch)')
    print(f'{label}: {SIZE} bytes transferred intact')


# Default asyncmap: every control character escaped on the wire.
with PppPair(name='escmap') as pair:
    pair.up()
    transfer(pair, 'escmap')
    pair.down()

# asyncmap 0: raw control characters allowed through.
with PppPair(['asyncmap', '0'], ['asyncmap', '0'], name='rawmap') as pair:
    pair.up()
    transfer(pair, 'rawmap')
    pair.down()
