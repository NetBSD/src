# name: Armv8.1-M Mainline vcvt instruction in it block (with MVE)
# as: -march=armv8.1-m.main+mve.fp+fp.dp
# objdump: -dr --prefix-addresses --show-raw-insn -marmv8.1-m.main

.*: +file format .*arm.*

Disassembly of section .text:
^[^>]*> bf18[ 	]+it[ 	]+ne
^[^>]*> eefd 6bc8[ 	]+vcvtne.s32.f64[ 	]+s13, d8