;; Copyright (C) 2010 Embecosm Limited
;;
;; Contributed by Joern Rennecke <joern.rennecke@embecosm.com> in 2010
;;
;; This file is part of GCC.
;;
;; GCC is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 3, or (at your option)
;; any later version.
;;
;; GCC is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with GCC; see the file COPYING3.  If not see
;; <http://www.gnu.org/licenses/>.

(define_constraint "I"
  ""
  (and (match_code "const_int")
       (match_test "ival >= -32768 && ival <= 32767")))

(define_constraint "J"
  ""
  (and (match_code "const_int")
       (match_test "ival == 0")))

(define_constraint "K"
  ""
  (and (match_code "const_int")
       (match_test "ival >= 0 && ival <= 65535")))

(define_constraint "L"
  ""
  (and (match_code "const_int")
       (match_test "ival >= 0 && ival <= 31")))

(define_constraint "M"
  ""
  (and (match_code "const_int")
       (match_test "(ival & 0xffff) == 0")))

(define_constraint "N"
  ""
  (and (match_code "const_int")
       (match_test "ival >= -33554432 && ival <= 33554431")))

(define_constraint "O"
  ""
  (and (match_code "const_int")
       (match_test "ival == 0")))

(define_constraint "C"
  ""
  (match_code "const_double"))

