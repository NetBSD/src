;; Emacs settings.
;; Copyright (C) 2012-2017 Free Software Foundation, Inc.

;; This program is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 3 of the License, or
;; (at your option) any later version.

;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with this program.  If not, see <http://www.gnu.org/licenses/>.

(
 (tcl-mode . ((tcl-indent-level . 4)
	      (tcl-continued-indent-level . 4)
	      (indent-tabs-mode . t)))
 (nil . ((bug-reference-url-format . "http://sourceware.org/bugzilla/show_bug.cgi?id=%s")))
 (c-mode . ((c-file-style . "GNU")
	    (mode . c++)
	    (indent-tabs-mode . t)
	    (tab-width . 8)
	    (c-basic-offset . 2)
	    (eval . (c-set-offset 'innamespace 0))
	    ))
)
