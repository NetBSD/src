#!/usr/bin/env python3
# Foundational smoke test: pppd --version works for both binaries under
# test. Run via sudo: a non-root pppd refuses to start if the compiled-in
# options file is absent, before --version is even processed.

import re
import subprocess

from pppfns import PPPD, PPPD_PEER, sudo_prefix, test_fail

for binary in dict.fromkeys([PPPD, PPPD_PEER]):
    r = subprocess.run(sudo_prefix() + [binary, '--version'],
                       capture_output=True, text=True)
    out = r.stdout + r.stderr
    print(out.strip())
    m = re.search(r'pppd version (\d+)\.(\d+)', out)
    if not m:
        test_fail(f'{binary} --version printed no version (exit {r.returncode}):\n{out}')
