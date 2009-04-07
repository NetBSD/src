;; -*- lisp-interaction -*-
;; -*- emacs-lisp -*-
;;
;; Set emacs up for editing code using CVS indentation conventions.
;; See HACKING for more on what those conventions are.
;; To use, put in your .emacs:
;;   (load "c-mode")
;;   (load "cvs-format.el")
;; You need to load c-mode first or else when c-mode autoloads it will
;; clobber the settings from cvs-format.el.  Using c-mode-hook perhaps would
;; be a cleaner way to handle that.  Or see below about (set-c-style "BSD").
;;
;; Credits: Originally from the personal .emacs file of Rich Pixley,
;;      then rich@cygnus.com, circa 1992.  He sez "feel free to copy."
;;
;; If you have an Emacs that does not have "c-mode", but does have
;; "cc-mode" then put this into your .emacs:
;;   (require 'cc-mode)
;;   (load "cvs-format.el")
;;   (add-hook 'c-mode-hook '(lambda () (c-set-style "cvshome")))
;;
;; Credit: From the personal .emacs file of Mark D. Baushke
;  circa 2005. Feel free to do anything you want with it.

;;
;;
;;	This section sets constants used by c-mode for formating
;;
;;

;;  If `c-auto-newline' is non-`nil', newlines are inserted both
;;before and after braces that you insert, and after colons and semicolons.
;;Correct C indentation is done on all the lines that are made this way.

(if (boundp 'c-auto-newline)
    (setq c-auto-newline nil))


;;*Non-nil means TAB in C mode should always reindent the current line,
;;regardless of where in the line point is when the TAB command is used.
;;It might be desirable to set this to nil for CVS, since unlike GNU
;; CVS often uses comments over to the right separated by TABs.
;; Depends some on whether you're in the habit of using TAB to
;; reindent.
;(setq c-tab-always-indent nil)

;;; It seems to me that 
;;;    `M-x set-c-style BSD RET'
;;; or
;;;    (set-c-style "BSD")
;;; takes care of the indentation parameters correctly.


;;  C does not have anything analogous to particular function names for which
;;special forms of indentation are desirable.  However, it has a different
;;need for customization facilities: many different styles of C indentation
;;are in common use.
;;
;;  There are six variables you can set to control the style that Emacs C
;;mode will use.
;;
;;`c-indent-level'     
;;     Indentation of C statements within surrounding block.  The surrounding
;;     block's indentation is the indentation of the line on which the
;;     open-brace appears.

(if (boundp 'c-indent-level)
    (setq c-indent-level 4))

;;`c-continued-statement-offset'     
;;     Extra indentation given to a substatement, such as the then-clause of
;;     an if or body of a while.

(if (boundp 'c-continued-statement-offset)
    (setq c-continued-statement-offset 4))

;;`c-brace-offset'     
;;     Extra indentation for line if it starts with an open brace.

(if (boundp 'c-brace-offset)
    (setq c-brace-offset -4))

;;`c-brace-imaginary-offset'     
;;     An open brace following other text is treated as if it were this far
;;     to the right of the start of its line.

(if (boundp 'c-brace-imaginary-offset)
    (setq c-brace-imaginary-offset 0))

;;`c-argdecl-indent'     
;;     Indentation level of declarations of C function arguments.

(if (boundp 'c-argdecl-indent)
    (setq c-argdecl-indent 4))

;;`c-label-offset'     
;;     Extra indentation for line that is a label, or case or default.
;;  This doesn't quite do the right thing for CVS switches, which use the
;;    switch (foo)
;;    {
;;        case 0:
;;            break;
;;  style.  But if one manually aligns the first case, then the rest
;;  should work OK.
(if (boundp 'c-label-offset)
    (setq c-label-offset -2))

;;
;;
;;	This section sets constants used by cc-mode for formating
;;
;;

;; Folks that are using cc-mode in the more modern version of Emacs
;; will likely find this useful

(if (and (fboundp 'featurep)
	 (featurep 'cc-styles)
	 (fboundp 'c-add-style))
    (c-add-style "cvshome"
		 '((c-brace-offset . -4)
		   (c-basic-offset . 4)
		   (c-continued-statement-offset . (4 . 4))
		   (c-offsets-alist
		    . ((statement-block-intro . +)
		       (knr-argdecl-intro . 4)
		       (substatement-open . 0)
		       (label . 2)
		       (case-label . 2)
		       (statement-case-open . +)
		       (statement-cont . +)
		       (arglist-intro . c-lineup-arglist-intro-after-paren)
		       (arglist-close . c-lineup-arglist)
		       (inline-open . 0)
		       (brace-list-open . 0)))
		   (c-special-indent-hook . c-gnu-impose-minimum)
		   (c-block-comment-prefix . ""))))

;; You may now use the following when you wish to make use of the style:
;;     `M-x c-set-style RET cvshome RET'
;; or
;;     (c-set-style "cvshome")
;; to take care of things.

;;;; eof
