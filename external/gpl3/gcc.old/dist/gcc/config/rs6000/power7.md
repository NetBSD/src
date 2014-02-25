;; Scheduling description for IBM POWER7 processor.
;; Copyright (C) 2009 Free Software Foundation, Inc.
;;
;; Contributed by Pat Haugen (pthaugen@us.ibm.com).

;; This file is part of GCC.
;;
;; GCC is free software; you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published
;; by the Free Software Foundation; either version 3, or (at your
;; option) any later version.
;;
;; GCC is distributed in the hope that it will be useful, but WITHOUT
;; ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
;; or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
;; License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with GCC; see the file COPYING3.  If not see
;; <http://www.gnu.org/licenses/>.

(define_automaton "power7iu,power7lsu,power7vsu,power7misc")

(define_cpu_unit "iu1_power7,iu2_power7" "power7iu")
(define_cpu_unit "lsu1_power7,lsu2_power7" "power7lsu")
(define_cpu_unit "vsu1_power7,vsu2_power7" "power7vsu")
(define_cpu_unit "bpu_power7,cru_power7" "power7misc")
(define_cpu_unit "du1_power7,du2_power7,du3_power7,du4_power7,du5_power7"
                 "power7misc")


(define_reservation "DU_power7"
		    "du1_power7|du2_power7|du3_power7|du4_power7")

(define_reservation "DU2F_power7"
		    "du1_power7+du2_power7")

(define_reservation "DU4_power7"
		    "du1_power7+du2_power7+du3_power7+du4_power7")

(define_reservation "FXU_power7"
                    "iu1_power7|iu2_power7")

(define_reservation "VSU_power7"
                    "vsu1_power7|vsu2_power7")

(define_reservation "LSU_power7"
                    "lsu1_power7|lsu2_power7")


; Dispatch slots are allocated in order conforming to program order.
(absence_set "du1_power7" "du2_power7,du3_power7,du4_power7,du5_power7")
(absence_set "du2_power7" "du3_power7,du4_power7,du5_power7")
(absence_set "du3_power7" "du4_power7,du5_power7")
(absence_set "du4_power7" "du5_power7")


; LS Unit
(define_insn_reservation "power7-load" 2
  (and (eq_attr "type" "load")
       (eq_attr "cpu" "power7"))
  "DU_power7,LSU_power7")

(define_insn_reservation "power7-load-ext" 3
  (and (eq_attr "type" "load_ext")
       (eq_attr "cpu" "power7"))
  "DU2F_power7,LSU_power7,FXU_power7")

(define_insn_reservation "power7-load-update" 2
  (and (eq_attr "type" "load_u")
       (eq_attr "cpu" "power7"))
  "DU2F_power7,LSU_power7+FXU_power7")

(define_insn_reservation "power7-load-update-indexed" 3
  (and (eq_attr "type" "load_ux")
       (eq_attr "cpu" "power7"))
  "DU4_power7,FXU_power7,LSU_power7+FXU_power7")

(define_insn_reservation "power7-load-ext-update" 4
  (and (eq_attr "type" "load_ext_u")
       (eq_attr "cpu" "power7"))
  "DU2F_power7,LSU_power7+FXU_power7,FXU_power7")

(define_insn_reservation "power7-load-ext-update-indexed" 4
  (and (eq_attr "type" "load_ext_ux")
       (eq_attr "cpu" "power7"))
  "DU4_power7,FXU_power7,LSU_power7+FXU_power7,FXU_power7")

(define_insn_reservation "power7-fpload" 3
  (and (eq_attr "type" "fpload")
       (eq_attr "cpu" "power7"))
  "DU_power7,LSU_power7")

(define_insn_reservation "power7-fpload-update" 3
  (and (eq_attr "type" "fpload_u,fpload_ux")
       (eq_attr "cpu" "power7"))
  "DU2F_power7,LSU_power7+FXU_power7")

(define_insn_reservation "power7-store" 6 ; store-forwarding latency
  (and (eq_attr "type" "store")
       (eq_attr "cpu" "power7"))
  "DU_power7,LSU_power7+FXU_power7")

(define_insn_reservation "power7-store-update" 6
  (and (eq_attr "type" "store_u")
       (eq_attr "cpu" "power7"))
  "DU2F_power7,LSU_power7+FXU_power7,FXU_power7")

(define_insn_reservation "power7-store-update-indexed" 6
  (and (eq_attr "type" "store_ux")
       (eq_attr "cpu" "power7"))
  "DU4_power7,LSU_power7+FXU_power7,FXU_power7")

(define_insn_reservation "power7-fpstore" 6
  (and (eq_attr "type" "fpstore")
       (eq_attr "cpu" "power7"))
  "DU_power7,LSU_power7+VSU_power7")

(define_insn_reservation "power7-fpstore-update" 6
  (and (eq_attr "type" "fpstore_u,fpstore_ux")
       (eq_attr "cpu" "power7"))
  "DU_power7,LSU_power7+VSU_power7+FXU_power7")

(define_insn_reservation "power7-larx" 3
  (and (eq_attr "type" "load_l")
       (eq_attr "cpu" "power7"))
  "DU4_power7,LSU_power7")

(define_insn_reservation "power7-stcx" 10
  (and (eq_attr "type" "store_c")
       (eq_attr "cpu" "power7"))
  "DU4_power7,LSU_power7")

(define_insn_reservation "power7-vecload" 3
  (and (eq_attr "type" "vecload")
       (eq_attr "cpu" "power7"))
  "DU_power7,LSU_power7")

(define_insn_reservation "power7-vecstore" 6
  (and (eq_attr "type" "vecstore")
       (eq_attr "cpu" "power7"))
  "DU_power7,LSU_power7+VSU_power7")

(define_insn_reservation "power7-sync" 11
  (and (eq_attr "type" "sync")
       (eq_attr "cpu" "power7"))
  "DU4_power7,LSU_power7")


; FX Unit
(define_insn_reservation "power7-integer" 1
  (and (eq_attr "type" "integer,insert_word,insert_dword,shift,trap,\
                        var_shift_rotate,exts,isel")
       (eq_attr "cpu" "power7"))
  "DU_power7,FXU_power7")

(define_insn_reservation "power7-cntlz" 2
  (and (eq_attr "type" "cntlz")
       (eq_attr "cpu" "power7"))
  "DU_power7,FXU_power7")

(define_insn_reservation "power7-two" 2
  (and (eq_attr "type" "two")
       (eq_attr "cpu" "power7"))
  "DU_power7+DU_power7,FXU_power7,FXU_power7")

(define_insn_reservation "power7-three" 3
  (and (eq_attr "type" "three")
       (eq_attr "cpu" "power7"))
  "DU_power7+DU_power7+DU_power7,FXU_power7,FXU_power7,FXU_power7")

(define_insn_reservation "power7-cmp" 1
  (and (eq_attr "type" "cmp,fast_compare")
       (eq_attr "cpu" "power7"))
  "DU_power7,FXU_power7")

(define_insn_reservation "power7-compare" 2
  (and (eq_attr "type" "compare,delayed_compare,var_delayed_compare")
       (eq_attr "cpu" "power7"))
  "DU2F_power7,FXU_power7,FXU_power7")

(define_bypass 3 "power7-cmp,power7-compare" "power7-crlogical,power7-delayedcr")

(define_insn_reservation "power7-mul" 4
  (and (eq_attr "type" "imul,imul2,imul3,lmul")
       (eq_attr "cpu" "power7"))
  "DU_power7,FXU_power7")

(define_insn_reservation "power7-mul-compare" 5
  (and (eq_attr "type" "imul_compare,lmul_compare")
       (eq_attr "cpu" "power7"))
  "DU2F_power7,FXU_power7,nothing*3,FXU_power7")

(define_insn_reservation "power7-idiv" 36
  (and (eq_attr "type" "idiv")
       (eq_attr "cpu" "power7"))
  "DU2F_power7,iu1_power7*36|iu2_power7*36")

(define_insn_reservation "power7-ldiv" 68
  (and (eq_attr "type" "ldiv")
       (eq_attr "cpu" "power7"))
  "DU2F_power7,iu1_power7*68|iu2_power7*68")

(define_insn_reservation "power7-isync" 1 ;
  (and (eq_attr "type" "isync")
       (eq_attr "cpu" "power7"))
  "DU4_power7,FXU_power7")


; CR Unit
(define_insn_reservation "power7-mtjmpr" 4
  (and (eq_attr "type" "mtjmpr")
       (eq_attr "cpu" "power7"))
  "du1_power7,FXU_power7")

(define_insn_reservation "power7-mfjmpr" 5
  (and (eq_attr "type" "mfjmpr")
       (eq_attr "cpu" "power7"))
  "du1_power7,cru_power7+FXU_power7")

(define_insn_reservation "power7-crlogical" 3
  (and (eq_attr "type" "cr_logical")
       (eq_attr "cpu" "power7"))
  "du1_power7,cru_power7")

(define_insn_reservation "power7-delayedcr" 3
  (and (eq_attr "type" "delayed_cr")
       (eq_attr "cpu" "power7"))
  "du1_power7,cru_power7")

(define_insn_reservation "power7-mfcr" 6
  (and (eq_attr "type" "mfcr")
       (eq_attr "cpu" "power7"))
  "du1_power7,cru_power7")

(define_insn_reservation "power7-mfcrf" 3
  (and (eq_attr "type" "mfcrf")
       (eq_attr "cpu" "power7"))
  "du1_power7,cru_power7")

(define_insn_reservation "power7-mtcr" 3
  (and (eq_attr "type" "mtcr")
       (eq_attr "cpu" "power7"))
  "DU4_power7,cru_power7+FXU_power7")


; BR Unit
; Branches take dispatch Slot 4.  The presence_sets prevent other insn from
; grabbing previous dispatch slots once this is assigned.
(define_insn_reservation "power7-branch" 3
  (and (eq_attr "type" "jmpreg,branch")
       (eq_attr "cpu" "power7"))
  "(du5_power7\
   |du4_power7+du5_power7\
   |du3_power7+du4_power7+du5_power7\
   |du2_power7+du3_power7+du4_power7+du5_power7\
   |du1_power7+du2_power7+du3_power7+du4_power7+du5_power7),bpu_power7")


; VS Unit (includes FP/VSX/VMX/DFP)
(define_insn_reservation "power7-fp" 6
  (and (eq_attr "type" "fp,dmul")
       (eq_attr "cpu" "power7"))
  "DU_power7,VSU_power7")

(define_bypass 8 "power7-fp" "power7-branch")

(define_insn_reservation "power7-fpcompare" 4
  (and (eq_attr "type" "fpcompare")
       (eq_attr "cpu" "power7"))
  "DU_power7,VSU_power7")

(define_insn_reservation "power7-sdiv" 26
  (and (eq_attr "type" "sdiv")
       (eq_attr "cpu" "power7"))
  "DU_power7,VSU_power7")

(define_insn_reservation "power7-ddiv" 32
  (and (eq_attr "type" "ddiv")
       (eq_attr "cpu" "power7"))
  "DU_power7,VSU_power7")

(define_insn_reservation "power7-sqrt" 31
  (and (eq_attr "type" "ssqrt")
       (eq_attr "cpu" "power7"))
  "DU_power7,VSU_power7")

(define_insn_reservation "power7-dsqrt" 43
  (and (eq_attr "type" "dsqrt")
       (eq_attr "cpu" "power7"))
  "DU_power7,VSU_power7")

(define_insn_reservation "power7-vecsimple" 2
  (and (eq_attr "type" "vecsimple")
       (eq_attr "cpu" "power7"))
  "du1_power7,VSU_power7")

(define_insn_reservation "power7-veccmp" 7
  (and (eq_attr "type" "veccmp")
       (eq_attr "cpu" "power7"))
  "du1_power7,VSU_power7")

(define_insn_reservation "power7-vecfloat" 7
  (and (eq_attr "type" "vecfloat")
       (eq_attr "cpu" "power7"))
  "du1_power7,VSU_power7")

(define_bypass 6 "power7-vecfloat" "power7-vecfloat")

(define_insn_reservation "power7-veccomplex" 7
  (and (eq_attr "type" "veccomplex")
       (eq_attr "cpu" "power7"))
  "du1_power7,VSU_power7")

(define_insn_reservation "power7-vecperm" 3
  (and (eq_attr "type" "vecperm")
       (eq_attr "cpu" "power7"))
  "du2_power7,VSU_power7")
