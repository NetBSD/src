<!--
SPDX-FileCopyrightText: 2023 EfficiOS Inc.

SPDX-License-Identifier: CC-BY-4.0
-->

# Userspace-RCU ABI definitions

This directory contains the serialized ABI definitions for a typical build of
the liburcu libraries. This information is extracted using
[libabigail](https://sourceware.org/libabigail/).

The artefacts used to generate these were built with **CFLAGS="-O0 -ggdb"** on
an Ubuntu 18.04 x86_64 system.

You can compare the serialized ABI with a shared object to check for breaking
changes. For example, here we compare an in-tree built version of
**liburcu-memb.so** with the serialized ABI of stable-0.13 :

````
abidiff \
  extras/abi/0.13/x86_64-pc-linux-gnu/liburcu-memb.so.8.xml \
  src/.libs/liburcu-memb.so
````
