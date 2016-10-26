#objdump: --section-headers
# Minimum code alignment should be set.

.*: +file format.*aarch64.*

Sections:
Idx Name          Size      VMA               LMA               File off  Algn
  0 \.text         .*  .*  .*  .*  2\*\*2
                  .*CODE.*
  1 \.data         .*  .*  .* .*  2\*\*0
                  .*DATA.*
  2 \.bss          .*  .*  .*  .*  2\*\*0
.*
