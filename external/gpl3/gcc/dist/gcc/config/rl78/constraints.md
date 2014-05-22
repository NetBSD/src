;;  Machine Description for Renesas RL78 processors
;;  Copyright (C) 2011-2013 Free Software Foundation, Inc.
;;  Contributed by Red Hat.

;; This file is part of GCC.

;; GCC is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 3, or (at your option)
;; any later version.

;; GCC is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with GCC; see the file COPYING3.  If not see
;; <http://www.gnu.org/licenses/>.

; Constraints in use:

; core:
; V X g i m n o p r s < >
; 0..9
; I..Q - integers
;   Int8 = 0..255
;   Int3 = 1..7
;   J = -255..0
;   K = 1
;   L = -1
;   M = 0
;   N = 2
;   O = -2
;   P = 1..15

; E..H - float constants

; RL78-specific
; a x b c d e h l w - 8-bit regs
; A B D T S - 16-bit regs
; R = all regular registers (A-L)
; Y - any valid memory
; Wxx - various memory addressing modes
; Qxx - conditionals
; v = virtual registers
; Zxx = specific virtual registers

(define_constraint "Int8"
  "Integer constant in the range 0 @dots{} 255."
  (and (match_code "const_int")
       (match_test "IN_RANGE (ival, 0, 255)")))

(define_constraint "Int3"
  "Integer constant in the range 1 @dots{} 7."
  (and (match_code "const_int")
       (match_test "IN_RANGE (ival, 1, 7)")))

(define_constraint "J"
  "Integer constant in the range -255 @dots{} 0"
  (and (match_code "const_int")
       (match_test "IN_RANGE (ival, -255, 0)")))

(define_constraint "K"
  "Integer constant 1."
  (and (match_code "const_int")
       (match_test "IN_RANGE (ival, 1, 1)")))

(define_constraint "L"
  "Integer constant -1."
  (and (match_code "const_int")
       (match_test "IN_RANGE (ival, -1, -1)")))

(define_constraint "M"
  "Integer constant 0."
  (and (match_code "const_int")
       (match_test "IN_RANGE (ival, 0, 0)")))

(define_constraint "N"
  "Integer constant 2."
  (and (match_code "const_int")
       (match_test "IN_RANGE (ival, 2, 2)")))

(define_constraint "O"
  "Integer constant -2."
  (and (match_code "const_int")
       (match_test "IN_RANGE (ival, -2, -2)")))

(define_constraint "P"
  "Integer constant 1..15"
  (and (match_code "const_int")
       (match_test "IN_RANGE (ival, 1, 15)")))

(define_register_constraint "R" "QI_REGS"
 "@code{A} through @code{L} registers.")

(define_register_constraint "a" "AREG"
 "The @code{A} register.")

(define_register_constraint "x" "XREG"
 "The @code{X} register.")

(define_register_constraint "b" "BREG"
 "The @code{B} register.")

(define_register_constraint "c" "CREG"
 "The @code{C} register.")

(define_register_constraint "d" "DREG"
 "The @code{D} register.")

(define_register_constraint "e" "EREG"
 "The @code{E} register.")

(define_register_constraint "h" "HREG"
 "The @code{H} register.")

(define_register_constraint "l" "LREG"
 "The @code{L} register.")

(define_register_constraint "w" "PSWREG"
 "The @code{PSW} register.")

(define_register_constraint "A" "AXREG"
 "The @code{AX} register.")

(define_register_constraint "B" "BCREG"
 "The @code{BC} register.")

(define_register_constraint "D" "DEREG"
 "The @code{DE} register.")

; because H + L = T, assuming A=1.
(define_register_constraint "T" "HLREG"
 "The @code{HL} register.")

(define_register_constraint "S" "SPREG"
 "The @code{SP} register.")

(define_register_constraint "v" "V_REGS"
 "The virtual registers.")

(define_register_constraint "Z08W" "R8W_REGS"
 "The R8 register, HImode.")

(define_register_constraint "Z10W" "R10W_REGS"
 "The R10 register, HImode.")

(define_register_constraint "Zint" "INT_REGS"
 "The interrupt registers.")

; All the memory addressing schemes the RL78 supports
; of the form W {register} {bytes of offset}
;          or W {register} {register}

; absolute address
(define_memory_constraint "Wab"
  "[addr]"
  (and (match_code "mem")
       (ior (match_test "CONSTANT_P (XEXP (op, 0))")
	    (match_test "GET_CODE (XEXP (op, 0)) == PLUS && GET_CODE (XEXP (XEXP (op, 0), 0)) == SYMBOL_REF"))
	    )
  )

(define_memory_constraint "Wbc"
  "word16[BC]"
  (and (match_code "mem")
       (ior
	(and (match_code "reg" "0")
	     (match_test "REGNO (XEXP (op, 0)) == BC_REG"))
	(and (match_code "plus" "0")
	     (and (and (match_code "reg" "00")
		       (match_test "REGNO (XEXP (XEXP (op, 0), 0)) == BC_REG"))
		       (match_test "uword_operand (XEXP (XEXP (op, 0), 1), VOIDmode)"))))
       )
  )

(define_memory_constraint "Wde"
  "[DE]"
  (and (match_code "mem")
       (and (match_code "reg" "0")
	    (match_test "REGNO (XEXP (op, 0)) == DE_REG")))
  )

(define_memory_constraint "Wca"
  "[AX..HL] for calls"
  (and (match_code "mem")
       (and (match_code "reg" "0")
	    (match_test "REGNO (XEXP (op, 0)) <= HL_REG")))
  )

(define_memory_constraint "Wcv"
  "[AX..HL,r8-r23] for calls"
  (and (match_code "mem")
       (and (match_code "reg" "0")
	    (match_test "REGNO (XEXP (op, 0)) < 24")))
  )

(define_memory_constraint "Wd2"
  "word16[DE]"
  (and (match_code "mem")
       (ior
	(and (match_code "reg" "0")
	     (match_test "REGNO (XEXP (op, 0)) == DE_REG"))
	(and (match_code "plus" "0")
	     (and (and (match_code "reg" "00")
		       (match_test "REGNO (XEXP (XEXP (op, 0), 0)) == DE_REG"))
		       (match_test "uword_operand (XEXP (XEXP (op, 0), 1), VOIDmode)"))))
       )
  )

(define_memory_constraint "Whl"
  "[HL]"
  (and (match_code "mem")
       (and (match_code "reg" "0")
	    (match_test "REGNO (XEXP (op, 0)) == HL_REG")))
  )

(define_memory_constraint "Wh1"
  "byte8[HL]"
  (and (match_code "mem")
       (and (match_code "plus" "0")
	    (and (and (match_code "reg" "00")
		      (match_test "REGNO (XEXP (XEXP (op, 0), 0)) == HL_REG"))
		      (match_test "ubyte_operand (XEXP (XEXP (op, 0), 1), VOIDmode)"))))
  )

(define_memory_constraint "Whb"
  "[HL+B]"
  (and (match_code "mem")
       (match_test "rl78_hl_b_c_addr_p (XEXP (op, 0))"))
  )

(define_memory_constraint "Ws1"
  "word8[SP]"
  (and (match_code "mem")
       (ior
	(and (match_code "reg" "0")
	     (match_test "REGNO (XEXP (op, 0)) == SP_REG"))
	(and (match_code "plus" "0")
	     (and (and (match_code "reg" "00")
		       (match_test "REGNO (XEXP (XEXP (op, 0), 0)) == SP_REG"))
		       (match_test "ubyte_operand (XEXP (XEXP (op, 0), 1), VOIDmode)"))))
       )
  )

(define_memory_constraint "Wfr"
  "ES/CS far pointer"
  (and (match_code "mem")
       (match_test "rl78_far_p (op)"))
  )

(define_memory_constraint "Y"
  "any near legitimate memory access"
  (and (match_code "mem")
       (match_test "!rl78_far_p (op) && rl78_as_legitimate_address (VOIDmode, XEXP (op, 0), true, ADDR_SPACE_GENERIC)"))
)


(define_memory_constraint "Qbi"
  "built-in compare types"
  (match_code "eq,ne,gtu,ltu,geu,leu"))

(define_memory_constraint "Qsc"
  "synthetic compares"
  (match_code "gt,lt,ge,le"))
