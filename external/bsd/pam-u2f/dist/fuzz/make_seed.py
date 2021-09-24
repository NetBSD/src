#!/usr/bin/env python3

# append the 1 byte the fuzzer needs to decide which format to choose,
# 1 byte for debug or not and then 4 bytes for the srandom() seed.
# run this script and then merge the generated seeds to the corpus.
# Note: this has already been done with the existing corpus. This
# script is included in case new credentials tests are added
# ./make_seed.py
# ./fuzz_format_parsers corpus seed -merge=1

import os

if not os.path.exists("./seed"):
    os.mkdir("./seed")

with os.scandir("../tests/credentials") as entries:
    for entry in entries:
        if not entry.is_file():
            continue
        print(entry.name)
        with open("./seed/{}".format(entry.name), "wb") as w:
            w.write(bytes([1,1,1,1,1,1]))
            with open("../tests/credentials/{}".format(entry.name), "rb") as r:
                w.write(r.read())
        with open("./seed/{}.ssh".format(entry.name), "wb") as w:
            w.write(bytes([0,1,1,1,1,1]))
            with open("../tests/credentials/{}".format(entry.name), "rb") as r:
                w.write(r.read())
