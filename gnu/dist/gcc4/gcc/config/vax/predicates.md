;; Predicate definitions for Motorola 68000.
;; Copyright (C) 2005 Free Software Foundation, Inc.
;;
;; This file is part of GCC.
;;
;; GCC is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 2, or (at your option)
;; any later version.
;;
;; GCC is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with GCC; see the file COPYING.  If not, write to
;; the Free Software Foundation, 51 Franklin Street, Fifth Floor,
;; Boston, MA 02110-1301, USA.

;; Special case of a symbolic operand that's used as a 
;; operand.

(define_predicate "indirect_memory_operand"
   (match_code "mem")
{
  op = XEXP (op, 0);
  if (MEM_P (op))
    return 1;
  if (GET_CODE (op) == PLUS)
    op = XEXP (op, 1);
  return MEM_P (op);
})

(define_predicate "indexed_memory_operand"
   (match_code "mem")
{
  op = XEXP (op, 0);
  return GET_CODE (op) != PRE_DEC && GET_CODE (op) != POST_INC
	 && mode_dependent_address_p (op);
})

(define_predicate "illegal_addsub_di_memory_operand"
   (and (match_code "mem")
	(ior (match_operand 0 "indexed_memory_operand" "")
	     (ior (match_operand 0 "indirect_memory_operand" "")
		  (match_test "GET_CODE (XEXP (op, 0)) == PRE_DEC")))))

(define_predicate "nonimmediate_addsub_di_operand"
   (and (match_code "subreg,reg,mem")
        (and (match_operand:DI 0 "nonimmediate_operand" "")
	     (not (match_operand:DI 0 "illegal_addsub_di_memory_operand")))))

(define_predicate "general_addsub_di_operand"
   (and (match_code "const_int,const_double,subreg,reg,mem")
	(and (match_operand:DI 0 "general_operand" "")
	     (not (match_operand:DI 0 "illegal_addsub_di_memory_operand")))))
