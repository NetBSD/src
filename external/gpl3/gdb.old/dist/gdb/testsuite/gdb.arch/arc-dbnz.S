; This testcase is part of GDB, the GNU debugger.

; Copyright 2024 Free Software Foundation, Inc.

; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program. If not, see <http://www.gnu.org/licenses/>.

	.section .text
	.align 4
	.global main
	.type main, @function

main:
	mov r0,1
dbnz1:
	; r0 == 0 after decrementing. dbnz doesn't do branch.
	dbnz r0,@end

	mov r0,5
dbnz2:
	; r0 == 3 after decrementing and delay slot. dbnz does branch.
	dbnz.d r0,@dbnz3
	sub r0,r0,1

dbnz4:
	; r0 == 1 after decrementing. dbnz does branch.
	dbnz r0,@end

dbnz3:
	; r0 == 2 after decrementing. dbnz does branch.
	dbnz r0,@dbnz4

end:
	mov r0,0
	j [blink]

	.section .note.GNU-stack,"",@progbits
