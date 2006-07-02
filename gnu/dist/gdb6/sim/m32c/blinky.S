;;; blinky.S --- sample program to blink LED's on M32C simulator
;;; 
;;; Copyright (C) 2005 Free Software Foundation, Inc.
;;; Contributed by Red Hat, Inc.
;;;
;;; This file is part of the GNU simulators.
;;;
;;; The GNU simulators are free software; you can redistribute them
;;; and/or modify them under the terms of the GNU General Public
;;; License as published by the Free Software Foundation; either
;;; version 2 of the License, or (at your option) any later version.
;;;
;;; The GNU simulators are distributed in the hope that they will be
;;; useful, but WITHOUT ANY WARRANTY; without even the implied
;;; warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
;;; PURPOSE. See the GNU General Public License for more details.
;;;
;;; You should have received a copy of the GNU General Public License
;;; along with the GNU simulators; if not, write to the Free Software
;;; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
;;; 02110-1301, USA

	.text

	.global _start
_start:
	mov.w	#0xe1,a0
top:
	sub.w	#1,r0
	mov.b	r0h,[a0]

	mov.w	#1000,r1
loop:
	adjnz.w	#-1,r1,loop

	jmp.w	top
