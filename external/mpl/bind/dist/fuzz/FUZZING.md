<!--
Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.
-->

= Fuzzing

The tests in this directory can be operated in three modes:

* non-fuzzing - the test just runs over all input located in `<test_name>.in/`
  directory by compiling with mock main.c that walks through the directory and
  runs `LLVMFuzzerTestOneInput()` over the input files
* AFL - `./configure --with-fuzzing=afl` will either feed the stdin to
  `LLVMFuzzerTestOneInput()` or run the `__AFL_LOOP(10000)` if compiled with
  `afl-clang-fast`
* LibFuzzer - `./configure --with-fuzzing=libfuzzer` will disable `main.c`
  completely and it uses the standard LibFuzzer mechanims to feed
  `LLVMFuzzerTestOneInput` with the fuzzer

== Test Cases

Each test case should be called descriptively and the executable target must
link `testcase.o` and `main.o` and the `test_case.c` must have a function
`LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)`.

== Adding more fuzzers

To add a different fuzzer, `main.c` must be modified to include `main()` function
for a specific fuzzer (or no function as is case with LibFuzzer).
