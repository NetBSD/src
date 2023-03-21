#name: Discarded dynamic relocation section
#ld: -shared -T reloc-discard.ld
#readelf: -r --use-dynamic
#target: [check_shared_lib_support]
#skip: powerpc64*-*-*
#source: reloc-discard.s
#xfail: tic6x-*-*
# Need to figure out how to pass `-fpic' for NDS32 or `-mpic -mpid=near'
# for TI C6X targets to GAS for this test.
# PowerPC64 warns when discarding dynamic relocs, which is generally
# a good thing.  See reloc-discard-warn.d test variant.

There are no dynamic relocations in this file\.
