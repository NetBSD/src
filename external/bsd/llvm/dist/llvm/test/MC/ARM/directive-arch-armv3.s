@ Test the .arch directive for armv3

@ This test case will check the default .ARM.attributes value for the
@ armv3 architecture.

@ RUN: llvm-mc < %s -triple=arm-linux-gnueabi -filetype=asm \
@ RUN:   | FileCheck %s --check-prefix=CHECK-ASM
@ RUN: llvm-mc < %s -triple=arm-linux-gnueabi -filetype=obj \
@ RUN:   | llvm-readobj -s -sd | FileCheck %s --check-prefix=CHECK-OBJ

	.syntax	unified
	.arch	armv3

@ CHECK-ASM: 	.arch	armv3

@ CHECK-OBJ:    Name: .ARM.attributes
@ CHECK-OBJ:    Type: SHT_ARM_ATTRIBUTES (0x70000003)
@ CHECK-OBJ:    Flags [ (0x0)
@ CHECK-OBJ:    ]
@ CHECK-OBJ:    Address: 0x0
@ CHECK-OBJ:    Offset: 0x34
@ CHECK-OBJ:    Size: 23
@ CHECK-OBJ:    Link: 0
@ CHECK-OBJ:    Info: 0
@ CHECK-OBJ:    AddressAlignment: 1
@ CHECK-OBJ:    EntrySize: 0
@ CHECK-OBJ:    SectionData (
@ CHECK-OBJ:      0000: 41160000 00616561 62690001 0C000000  |A....aeabi......|
@ CHECK-OBJ:      0010: 05330006 010801                      |.3.....|
@ CHECK-OBJ:    )
