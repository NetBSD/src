# $NetBSD: powerpc.s,v 1.1 2011/11/12 01:18:41 jmmv Exp $
#
# Copyright (c) 2011 The NetBSD Foundation, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

# ------------------------------------------------------------------------

.section ".note.netbsd.ident", "a"
	# This ELF section is used by the kernel to determine, among other
	# things, the system call interface used by the binary.
	#
	# See http://www.netbsd.org/docs/kernel/elf-notes.html for more
	# details.

	.int 7			# Length of the OS name field below.
	.int 4			# Length of the description field below.
	.int 0x01		# The type of the note: NetBSD OS Version.
	.ascii	"NetBSD\0\0"	# The OS name, padded to 8 bytes.
	.int 0x23b419a0		# The description value; 5.99.56.

# ------------------------------------------------------------------------

.section ".data"

message:
	.ascii "Hello, world!\n"
	.set MESSAGE_SIZE, . - message

# ------------------------------------------------------------------------

.section ".text"

	.balign 4

	.globl _start
	.type _start, @function
_start:
	# write(STDOUT_FILENO, message, MESSAGE_SIZE)
	li	%r0, 4			# r0: write(2) syscall number.
	li	%r3, 1			# r3: first argument.
	addis	%r4, %r0, message@h	# r4: second argument.
	ori	%r4, %r4, message@l
	li	%r5, MESSAGE_SIZE	# r5: third argument.
	sc

	# exit(EXIT_SUCCESS)
	li	%r0, 1			# r0: exit(2) syscall number.
	li	%r3, 0			# r3: first argument.
	sc

    .size _start, . - _start
