;;; po-mode.el -- for helping GNU gettext lovers to edit PO files.
;;; Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
;;; François Pinard <pinard@iro.umontreal.ca>, 1995.
;;; Helped by Greg McGary <gkm@magilla.cichlid.com>.

;; This file is part of GNU gettext.

;; GNU gettext is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 2, or (at your option)
;; any later version.

;; GNU gettext is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with GNU Emacs; see the file COPYING.  If not, write to the
;; Free Software Foundation, 59 Temple Place - Suite 330, Boston,
;; MA 02111-1307, USA.

;;; This package provides the tools meant to help editing PO files,
;;; as documented in the GNU gettext user's manual.  See this manual
;;; for user documentation, which is not repeated here.

;;; To install, merely put this file somewhere GNU Emacs will find it,
;;; then add the following lines to your .emacs file:
;;;
;;;   (autoload 'po-mode "po-mode")
;;;   (setq auto-mode-alist (cons '("\\.po[tx]?\\'\\|\\.po\\." . po-mode)
;;;				  auto-mode-alist))
;;;
;;; To automatically use proper fonts under Emacs 20, also add:
;;;
;;;   (autoload 'po-find-file-coding-system "po-mode")
;;;   (modify-coding-system-alist 'file "\\.po[tx]?\\'\\|\\.po\\."
;;;				  'po-find-file-coding-system)
;;;
;;; You may also adjust some variables, below, by defining them in your
;;; `.emacs' file, either directly or through command `M-x customize'.

;;; Emacs portability matters.

;;; Most portability matters are addressed in this page.  All other cases
;;; involve one of `eval-and-compile' or `fboundp', just search for these.

;; Identify which Emacs variety is being used.
(eval-and-compile
  (cond ((string-match "Lucid\\|XEmacs" emacs-version)
	 (setq po-EMACS20 nil po-XEMACS t))
	((and (string-lessp "19" emacs-version) (featurep 'faces))
	 (setq po-EMACS20 t po-XEMACS nil))
	(t (setq po-EMACS20 nil po-XEMACS nil))))

;; Experiment with Emacs LISP message internationalisation.
(eval-and-compile
  (or (fboundp 'set-translation-domain)
      (defsubst set-translation-domain (string) nil))
  (or (fboundp 'translate-string)
      (defsubst translate-string (string) string)))
(defsubst _ (string) (translate-string string))
(defsubst N_ (string) string)

;; Handle missing `customs' package.
(eval-and-compile
  (condition-case ()
      (require 'custom)
    (error nil))
  (if (and (featurep 'custom) (fboundp 'custom-declare-variable))
      nil
    (defmacro defgroup (&rest args)
      nil)
    (defmacro defcustom (var value doc &rest args)
      (` (defvar (, var) (, value) (, doc))))))

;; Protect string comparisons from text properties.
(eval-and-compile
  (fset 'po-buffer-substring
	(symbol-function (if (fboundp 'buffer-substring-no-properties)
			     'buffer-substring-no-properties
			   'buffer-substring))))

;; Handle missing `with-temp-buffer' function.
(eval-and-compile
  (if nil ; FIXME: just testing...  (fboundp 'with-temp-buffer)

      (fset 'po-with-temp-buffer (symbol-function 'with-temp-buffer))

    (defmacro po-with-temp-buffer (&rest forms)
      "Create a temporary buffer, and evaluate FORMS there like `progn'."
      (let ((curr-buffer (make-symbol "curr-buffer"))
	    (temp-buffer (make-symbol "temp-buffer")))
	`(let ((,curr-buffer (current-buffer))
	       (,temp-buffer (get-buffer-create
			      (generate-new-buffer-name " *po-temp*"))))
	   (unwind-protect
	       (progn
		 (set-buffer ,temp-buffer)
		 ,@forms)
	     (set-buffer ,curr-buffer)
	     (and (buffer-name ,temp-buffer)
		  (kill-buffer ,temp-buffer))))))))

;; Handle missing `kill-new' function.
(eval-and-compile
  (if (fboundp 'kill-new)

      (fset 'po-kill-new (symbol-function 'kill-new))

    (defun po-kill-new (string)
      "Push STRING onto the kill ring, for Emacs 18 where kill-new is missing."
      (po-with-temp-buffer
	(insert string)
	(kill-region (point-min) (point-max))))))

;; Handle missing `read-event' function.
(eval-and-compile
  (fset 'po-read-event
	(cond ((fboundp 'read-event)
	       ;; GNU Emacs.
	       'read-event)
	      ((fboundp 'next-command-event)
	       ;; XEmacs.
	       'next-command-event)
	      (t
	       ;; Older Emacses.
	       'read-char))))

;; Handle missing `force-mode-line-update' function.
(eval-and-compile
  (if (fboundp 'force-mode-line-update)

      (fset 'po-force-mode-line-update
	    (symbol-function 'force-mode-line-update))

    (defun po-force-mode-line-update ()
      "Force the mode-line of the current buffer to be redisplayed."
      (set-buffer-modified-p (buffer-modified-p)))))

;; Handle portable highlighting.  Code has been adapted (OK... stolen! :-)
;; from `ispell.el'.
(eval-and-compile
  (cond
   (po-EMACS20

    (defun po-create-overlay ()
      "Create and return a deleted overlay structure.
The variable `po-highlight-face' selects the face to use for highlighting."
      (let ((overlay (make-overlay (point) (point))))
	(overlay-put overlay 'face po-highlight-face)
	;; The fun thing is that a deleted overlay retains its face, and is
	;; movable.
	(delete-overlay overlay)
	overlay))

    (defun po-highlight (overlay start end &optional buffer)
      "Use OVERLAY to highlight the string from START to END.
If limits are not relative to the current buffer, use optional BUFFER."
      (move-overlay overlay start end (or buffer (current-buffer))))

    (defun po-rehighlight (overlay)
      "Ensure OVERLAY is highlighted."
      ;; There is nothing to do, as GNU Emacs allows multiple highlights.
      nil)

    (defun po-dehighlight (overlay)
      "Display normally the last string which OVERLAY highlighted.
The current buffer should be in PO mode, when this function is called."
      (delete-overlay overlay)))

   (po-XEMACS

    (defun po-create-overlay ()
      "Create and return a deleted overlay structure."
      (cons (make-marker) (make-marker)))

    (defun po-highlight (overlay start end &optional buffer)
      "Use OVERLAY to highlight the string from START to END.
If limits are not relative to the current buffer, use optional BUFFER."
      (if buffer
	  (save-excursion
	    (set-buffer buffer)
	    (isearch-highlight start end))
	(isearch-highlight start end))
      (set-marker (car overlay) start (or buffer (current-buffer)))
      (set-marker (cdr overlay) end (or buffer (current-buffer))))

    (defun po-rehighlight (overlay)
      "Ensure OVERLAY is highlighted."
      (let ((buffer (marker-buffer (car overlay)))
	    (start (marker-position (car overlay)))
	    (end (marker-position (cdr overlay))))
	(and buffer
	     (name-buffer buffer)
	     (po-highlight overlay start end buffer))))

    (defun po-dehighlight (overlay)
      "Display normally the last string which OVERLAY highlighted."
      (isearch-dehighlight t)
      (setcar overlay (make-marker))
      (setcdr overlay (make-marker))))

   (t

    (defun po-create-overlay ()
      "Create and return a deleted overlay structure."
      (cons (make-marker) (make-marker)))

    (defun po-highlight (overlay start end &optional buffer)
      "Use OVERLAY to highlight the string from START to END.
If limits are not relative to the current buffer, use optional BUFFER.
No doubt that highlighting, when Emacs does not allow it, is a kludge."
      (save-excursion
	(and buffer (set-buffer buffer))
	(let ((modified (buffer-modified-p))
	      (buffer-read-only nil)
	      (inhibit-quit t)
	      (buffer-undo-list t)
	      (text (buffer-substring start end)))
	  (goto-char start)
	  (delete-region start end)
	  (insert-char ?  (- end start))
	  (sit-for 0)
	  (setq inverse-video (not inverse-video))
	  (delete-region start end)
	  (insert text)
	  (sit-for 0)
	  (setq inverse-video (not inverse-video))
	  (set-buffer-modified-p modified)))
      (set-marker (car overlay) start (or buffer (current-buffer)))
      (set-marker (cdr overlay) end (or buffer (current-buffer))))

    (defun po-rehighlight (overlay)
      "Ensure OVERLAY is highlighted."
      (let ((buffer (marker-buffer (car overlay)))
	    (start (marker-position (car overlay)))
	    (end (marker-position (cdr overlay))))
	(and buffer
	     (name-buffer buffer)
	     (po-highlight overlay start end buffer))))

    (defun po-dehighlight (overlay)
      "Display normally the last string which OVERLAY highlighted."
      (let ((buffer (marker-buffer (car overlay)))
	    (start (marker-position (car overlay)))
	    (end (marker-position (cdr overlay))))
	(if buffer
	    (save-excursion
	      (set-buffer buffer)
	      (let ((modified (buffer-modified-p))
		    (buffer-read-only nil)
		    (inhibit-quit t)
		    (buffer-undo-list t))
		(let ((text (buffer-substring start end)))
		  (goto-char start)
		  (delete-region start end)
		  (insert-char ?  (- end start))
		  (sit-for 0)
		  (delete-region start end)
		  (insert text)
		  (sit-for 0)
		  (set-buffer-modified-p modified)))))
	(setcar overlay (make-marker))
	(setcdr overlay (make-marker))))

    )))

;;; Customisation.

(defgroup po nil
  "Major mode for editing PO files"
  :group 'i18n)

(defcustom po-auto-edit-with-msgid nil
  "*Automatically use msgid when editing untranslated entries."
  :type 'boolean
  :group 'po)

(defcustom po-auto-fuzzy-on-edit nil
  "*Automatically mark entries fuzzy when being edited."
  :type 'boolean
  :group 'po)

(defcustom po-auto-select-on-unfuzzy nil
  "*Automatically select some new entry while making an entry not fuzzy."
  :type 'boolean
  :group 'po)

(defcustom po-auto-replace-revision-date 'ask
  "*Automatically revise date in headers.  Value is nil, t, or ask."
  :type '(choice (const nil)
		 (const t)
		 (const ask))
  :group 'po)

(defcustom po-default-file-header "\
# SOME DESCRIPTIVE TITLE.
# Copyright (C) YEAR Free Software Foundation, Inc.
# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.
#
#, fuzzy
msgid \"\"
msgstr \"\"
\"Project-Id-Version: PACKAGE VERSION\\n\"
\"PO-Revision-Date: YEAR-MO-DA HO:MI +ZONE\\n\"
\"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n\"
\"Language-Team: LANGUAGE <LL@li.org>\\n\"
\"MIME-Version: 1.0\\n\"
\"Content-Type: text/plain; charset=CHARSET\\n\"
\"Content-Transfer-Encoding: ENCODING\\n\"
"
  "*Default PO file header."
  :type 'string
  :group 'po)

(defcustom po-highlighting (or po-EMACS20 po-XEMACS)
  "*Highlight text whenever appropriate, when non-nil.  However, on older
Emacses, a yet unexplained highlighting bug causes files to get mangled."
  :type 'boolean
  :group 'po)

(defcustom po-highlight-face 'highlight
  "*The face used for PO mode highlighting.  For Emacses with overlays.
Possible values are `highlight', `modeline', `secondary-selection',
`region', and `underline'.
This variable can be set by the user to whatever face they desire.
It's most convenient if the cursor color and highlight color are
slightly different."
  :type 'face
  :group 'po)

(defcustom po-gzip-uuencode-command "gzip -9 | uuencode -m"
  "*The filter to use for preparing a mail invoice of the PO file.
Normally \"gzip -9 | uuencode -m\", remove the -9 for lesser compression,
or remove the -m if you are not using the GNU version of `uuencode'."
  :type 'string
  :group 'po)

;;; The following block of declarations has the main purpose of avoiding
;;; byte compiler warnings.  It also introduces some documentation for
;;; each of these variables, all meant to be local to PO mode buffers.

;; Flag telling that MODE-LINE-STRING should be displayed.  See `Window'
;; page below.  Exceptionally, this variable is local to *all* buffers.

(defvar po-mode-flag)

;; PO buffers are kept read-only to prevent random modifications.  READ-ONLY
;; holds the value of the read-only flag before PO mode was entered.

(defvar po-read-only)

;; The current entry extends from START-OF-ENTRY to END-OF-ENTRY, it
;; includes preceding whitespace and excludes following whitespace.  The
;; start of keyword lines are START-OF-MSGID and START-OF-MSGSTR.
;; ENTRY-TYPE classifies the entry.

(defvar po-start-of-entry)
(defvar po-start-of-msgid)
(defvar po-start-of-msgstr)
(defvar po-end-of-entry)
(defvar po-entry-type)

;; A few counters are usefully shown in the Emacs mode line.

(defvar po-translated-counter)
(defvar po-fuzzy-counter)
(defvar po-untranslated-counter)
(defvar po-obsolete-counter)
(defvar po-mode-line-string)

;; PO mode keeps track of fields being edited, for one given field should
;; have one editing buffer at most, and for exiting a PO buffer properly
;; should offer to close all pending edits.  Variable EDITED-FIELDS holds an
;; an list of "slots" of the form: (ENTRY-MARKER EDIT-BUFFER OVERLAY-INFO).
;; To allow simultaneous edition of the comment and the msgstr of an entry,
;; ENTRY-MARKER points to the msgid line if a comment is being edited, or to
;; the msgstr line if the msgstr is being edited.  EDIT-BUFFER is the
;; temporary Emacs buffer used to edit the string.  OVERLAY-INFO, when not
;; nil, holds an overlay (or if overlays are not supported, a cons of two
;; markers) for this msgid string which became highlighted for the edit.

(defvar po-edited-fields)

;; We maintain a set of movable pointers for returning to entries.

(defvar po-marker-stack)

;; SEARCH path contains a list of directories where files may be found,
;; in a format suitable for read completion.  Each directory includes
;; its trailing slash.  PO mode starts with "./" and "../".

(defvar po-search-path)

;; The following variables are meaningful only when REFERENCE-CHECK
;; is identical to START-OF-ENTRY, else they should be recomputed.
;; REFERENCE-ALIST contains all known references for the current
;; entry, each list element is (PROMPT FILE LINE), where PROMPT may
;; be used for completing read, FILE is a string and LINE is a number.
;; REFERENCE-CURSOR is a cycling cursor into REFERENCE-ALIST.

(defvar po-reference-alist)
(defvar po-reference-cursor)
(defvar po-reference-check)

;; The following variables are for marking translatable strings in program
;; sources.  KEYWORDS is the list of keywords for marking translatable
;; strings, kept in a format suitable for reading with completion.
;; NEXT-FILE-LIST is the list of source files to visit, gotten from the tags
;; table.  STRING-START is the position for the beginning of the last found
;; string, or nil if the string is invalidated.  STRING-END is the position
;; for the end of the string and indicates where the search should be
;; resumed, or nil for the beginning of the current file.  MARKING-OVERLAY,
;; if not `nil', holds the overlay which highlight the last found string;
;; for older Emacses, it holds the cons of two markers around the
;; highlighted region.

(defvar po-keywords)
(defvar po-next-file-list)
(defvar po-string-start)
(defvar po-string-end)
(defvar po-marking-overlay)

;;; PO mode variables and constants (usually not to customize).

;; The textdomain should really be "gettext", only trying it for now.
;; All this requires more thinking, we cannot just do this like that.
(set-translation-domain "po-mode")

(defun po-mode-version ()
  "Show Emacs PO mode version."
  (interactive)
  (message (_"Emacs PO mode, version %s") (substring "Revision: 1.42" 11 -2)))

(defconst po-help-display-string
  (_"\
PO Mode Summary           Next Previous            Miscellaneous
*: Later, /: Docum        n    p    Any type       .     Redisplay
                          /t   /M-t Translated     /v    Version info
Moving around             f    M-f  Fuzzy          ?, h  This help
<    First if any         o    M-o  Obsolete       =     Current index
>    Last if any          u    M-u  Untranslated   O     Other window
/SPC Auto select                                   V     Validate
                        Msgstr Comments            M     Mail officially
Modifying entries         RET  #    Call editor    U     Undo
TAB   Remove fuzzy mark   k    K    Kill to        E     Edit out full
/DEL  Fuzzy or fade out   w    W    Copy to        Q     Forceful quit
LFD   Init with msgid     y    Y    Yank from      q     Confirm and quit

gettext Keyword Marking                            Position Stack
,    Find next string     Compendiums              m  Mark and push current
M-,  Mark translatable    *c    To compendium      r  Pop and return
M-.  Change mark, mark    *M-C  Select, save       x  Exchange current/top

Program Sources           Auxiliary Files          Lexicography
s    Cycle reference      a    Cycle file          *l    Lookup translation
M-s  Select reference     M-a  Select file         *M-l  Add/edit translation
S    Consider path        A    Consider PO file    *L    Consider lexicon
M-S  Ignore path          M-A  Ignore PO file      *M-L  Ignore lexicon
")
  "Help page for PO mode.")

(defconst po-mode-menu-layout
  '("PO"
    ("Moving around"
     ["Auto select" po-auto-select-entry t]
     "---"
     "Forward"
     ["Any next" po-next-entry t]
     ["Next translated" po-next-translated-entry t]
     ["Next fuzzy" po-next-fuzzy-entry t]
     ["Next obsolete" po-next-obsolete-entry t]
     ["Next untranslated" po-next-untranslated-entry t]
     ["Last file entry" po-last-entry t]
     "---"
     "Backward"
     ["Any previous" po-previous-entry t]
     ["Previous translated" po-previous-translated-entry t]
     ["Previous fuzzy" po-previous-fuzzy-entry t]
     ["Previous obsolete" po-previous-obsolete-entry t]
     ["Previous untranslated" po-previous-untranslated-entry t]
     ["First file entry" po-first-entry t]
     "---"
     "Position stack"
     ["Mark and push current" po-push-location t]
     ["Pop and return" po-pop-location t]
     ["Exchange current/top" po-exchange-location t]
     "---"
     ["Redisplay" po-current-entry t]
     ["Current index" po-statistics t])
    ("Modifying entries"
     ["Undo" po-undo t]
     "---"
     "Msgstr"
     ["Edit msgstr" po-edit-msgstr t]
     ["Kill msgstr" po-kill-msgstr t]
     ["Save msgstr" po-kill-ring-save-msgstr t]
     ["Yank msgstr" po-yank-msgstr t]
     "---"
     "Comments"
     ["Edit comment" po-edit-comment t]
     ["Kill comment" po-kill-comment t]
     ["Save comment" po-kill-ring-save-comment t]
     ["Yank comment" po-yank-comment t]
     "---"
     ["Remove fuzzy mark" po-unfuzzy t]
     ["Fuzzy or fade out" po-fade-out-entry t]
     ["Init with msgid" po-msgid-to-msgstr t])
    ("Other files"
     ["Other window" po-other-window t]
     "---"
     "Program sources"
     ["Cycle reference" po-cycle-source-reference t]
     ["Select reference" po-select-source-reference t]
     ["Consider path" po-consider-source-path t]
     ["Ignore path" po-ignore-source-path t]
     "---"
     "Compendiums"
     ["To compendium" po-save-entry nil]
     ["Select, save" po-select-and-save-entry nil]
     "---"
     "Auxiliary files"
     ["Cycle file" po-cycle-auxiliary nil]
     ["Select file" po-select-auxiliary nil]
     ["Consider file" po-consider-as-auxiliary nil]
     ["Ignore file" po-ignore-as-auxiliary nil]
     "---"
     "Lexicography"
     ["Lookup translation" po-lookup-lexicons nil]
     ["Add/edit translation" po-edit-lexicon-entry nil]
     ["Consider lexicon" po-consider-lexicon-file nil]
     ["Ignore lexicon" po-ignore-lexicon-file nil])
    "---"
    "Source marking"
    ["Find first string" (po-tags-search '(nil)) t]
    ["Prefer keyword" (po-select-mark-and-mark '(nil)) t]
    ["Find next string" po-tags-search t]
    ["Mark preferred" po-mark-translatable t]
    ["Mark with keyword" po-select-mark-and-mark t]
    "---"
    ["Version info" po-mode-version t]
    ["Help page" po-help t]
    ["Validate" po-validate t]
    ["Mail officially" po-send-mail t]
    ["Edit out full" po-edit-out-full t]
    "---"
    ["Forceful quit" po-quit t]
    ["Soft quit" po-confirm-and-quit t])
  "Menu layout for PO mode.")

(defconst po-subedit-message
  (_"Type `C-c C-c' once done, or `C-c C-k' to abort edit")
  "Message to post in the minibuffer when an edit buffer is displayed.")

(defconst po-content-type-charset-alist
  '((euc . japanese-iso-8bit))
  "How to convert Content-Type into a Mule coding system.")

(defvar po-auxiliary-list nil
  "List of auxiliary PO files, in completing read format.")

(defvar po-auxiliary-cursor nil
  "Cursor into the `po-auxiliary-list'.")

(defvar po-translation-project-address
  "translation@iro.umontreal.ca"
  "Electronic mail address of the Translation Project.")

(defvar po-compose-mail-function
  (let ((functions '(compose-mail-other-window
		     message-mail-other-window
		     compose-mail
		     message-mail))
	result)
    (while (and (not result) functions)
      (if (fboundp (car functions))
	  (setq result (car functions))
	(setq functions (cdr functions))))
    (cond (result)
	  ((fboundp 'mail-other-window)
	   (function (lambda (to subject)
		       (mail-other-window nil to subject))))
	  ((fboundp 'mail)
	   (function (lambda (to subject)
		       (mail nil to subject))))
	  (t (function (lambda (to subject)
			 (error (_"I do not know how to mail to `%s'") to))))))
  "Function to start composing an electronic message.")

(defvar po-any-msgid-regexp
  "^\\(#~?[ \t]*\\)?msgid.*\n\\(\\(#~?[ \t]*\\)?\".*\n\\)*"
  "Regexp matching a whole msgid field, whether obsolete or not.")

(defvar po-any-msgstr-regexp
  "^\\(#~?[ \t]*\\)?msgstr.*\n\\(\\(#~?[ \t]*\\)?\".*\n\\)*"
  "Regexp matching a whole msgstr field, whether obsolete or not.")

(defvar po-msgfmt-program "msgfmt"
  "Path to msgfmt program from GNU gettext package.")

;; Font lock based highlighting code.
(defconst po-font-lock-keywords
  '(
    ("^\\(msgid \\|msgstr \\)?\"\\|\"$" . font-lock-keyword-face)
    ("\\\\.\\|%[-.0-9ul]*[a-zA-Z]" . font-lock-variable-name-face)
    ("^# .*\\|^#[:,]?" . font-lock-comment-face)
    ("^#:\\(.*\\)" 1 font-lock-reference-face)
    ;; The following line does not work, and I wonder why.
    ;;("^#,\\(.*\\)" 1 font-function-name-reference-face)
    )
  "Additional expressions to highlight in PO mode.")

;; Old activator for `font lock'.  Is it still useful?  I don't think so.
;;
;;(if (boundp 'font-lock-keywords)
;;    (put 'po-mode 'font-lock-keywords 'po-font-lock-keywords))

;; `hilit19' based highlighting code has been disabled, as most probably
;; nobody really needs it (it also generates ugly byte-compiler warnings).
;;
;;(if (fboundp 'hilit-set-mode-patterns)
;;    (hilit-set-mode-patterns 'po-mode
;;			     '(("^# .*\\|^#$" nil comment)
;;			       ("^#[.,:].*" nil include)
;;			       ("^\\(msgid\\|msgstr\\) *\"" nil keyword)
;;			       ("^\"\\|\"$" nil keyword))))

;;; Mode activation.

(eval-and-compile
  (if po-EMACS20

      (defun po-find-file-coding-system (arg-list)
	"Return a Mule (DECODING . ENCODING) pair, according to PO file charset.
Called through file-coding-system-alist, before the file is visited for real."
	(and (eq (car arg-list) 'insert-file-contents)
	     (with-temp-buffer
	       (let ((coding-system-for-read 'no-conversion))
		 ;; Is 4096 enough?  FIXME: See archives to decide!  Some
		 ;; translators insert looong comments for the header entry.
		 (insert-file-contents (nth 1 arg-list) nil 0 4096)
		 (if (re-search-forward
		      "^\"Content-Type: text/plain;[ \t]*charset=\\([^\\]+\\)"
		      nil t)
		     (let* ((charset (intern (downcase (buffer-substring
							(match-beginning 1)
							(match-end 1)))))
			    (slot (assq charset
					po-content-type-charset-alist)))
		       (list (cond (slot (cdr slot))
				   ((memq charset (coding-system-list)) charset)
				   (t 'no-conversion))))
		   '(no-conversion))))))

    ))

(defvar po-mode-map nil
  "Keymap for PO mode.")
(if po-mode-map
    ()
  ;; The following line because (make-sparse-keymap) does not work on Demacs.
  (setq po-mode-map (make-keymap))
  (suppress-keymap po-mode-map)
  (define-key po-mode-map "\C-i" 'po-unfuzzy)
  (define-key po-mode-map "\C-j" 'po-msgid-to-msgstr)
  (define-key po-mode-map "\C-m" 'po-edit-msgstr)
  (define-key po-mode-map " " 'po-auto-select-entry)
  (define-key po-mode-map "?" 'po-help)
  (define-key po-mode-map "#" 'po-edit-comment)
  (define-key po-mode-map "," 'po-tags-search)
  (define-key po-mode-map "." 'po-current-entry)
  (define-key po-mode-map "<" 'po-first-entry)
  (define-key po-mode-map "=" 'po-statistics)
  (define-key po-mode-map ">" 'po-last-entry)
  (define-key po-mode-map "a" 'po-cycle-auxiliary)
;;;;  (define-key po-mode-map "c" 'po-save-entry)
  (define-key po-mode-map "f" 'po-next-fuzzy-entry)
  (define-key po-mode-map "h" 'po-help)
  (define-key po-mode-map "k" 'po-kill-msgstr)
;;;;  (define-key po-mode-map "l" 'po-lookup-lexicons)
  (define-key po-mode-map "m" 'po-push-location)
  (define-key po-mode-map "n" 'po-next-entry)
  (define-key po-mode-map "o" 'po-next-obsolete-entry)
  (define-key po-mode-map "p" 'po-previous-entry)
  (define-key po-mode-map "q" 'po-confirm-and-quit)
  (define-key po-mode-map "r" 'po-pop-location)
  (define-key po-mode-map "s" 'po-cycle-source-reference)
  (define-key po-mode-map "t" 'po-next-translated-entry)
  (define-key po-mode-map "u" 'po-next-untranslated-entry)
  (define-key po-mode-map "v" 'po-mode-version)
  (define-key po-mode-map "w" 'po-kill-ring-save-msgstr)
  (define-key po-mode-map "x" 'po-exchange-location)
  (define-key po-mode-map "y" 'po-yank-msgstr)
  (define-key po-mode-map "A" 'po-consider-as-auxiliary)
  (define-key po-mode-map "E" 'po-edit-out-full)
  (define-key po-mode-map "K" 'po-kill-comment)
;;;;  (define-key po-mode-map "L" 'po-consider-lexicon-file)
  (define-key po-mode-map "M" 'po-send-mail)
  (define-key po-mode-map "O" 'po-other-window)
  (define-key po-mode-map "Q" 'po-quit)
  (define-key po-mode-map "S" 'po-consider-source-path)
  (define-key po-mode-map "U" 'po-undo)
  (define-key po-mode-map "V" 'po-validate)
  (define-key po-mode-map "W" 'po-kill-ring-save-comment)
  (define-key po-mode-map "Y" 'po-yank-comment)
  (define-key po-mode-map "\177" 'po-fade-out-entry)
  (define-key po-mode-map "\M-," 'po-mark-translatable)
  (define-key po-mode-map "\M-." 'po-select-mark-and-mark)
  (define-key po-mode-map "\M-a" 'po-select-auxiliary)
;;;;  (define-key po-mode-map "\M-c" 'po-select-and-save-entry)
  (define-key po-mode-map "\M-f" 'po-previous-fuzzy-entry)
;;;;  (define-key po-mode-map "\M-l" 'po-edit-lexicon-entry)
  (define-key po-mode-map "\M-o" 'po-previous-obsolete-entry)
  (define-key po-mode-map "\M-t" 'po-previous-translated-entry)
  (define-key po-mode-map "\M-u" 'po-previous-untranslated-entry)
  (define-key po-mode-map "\M-s" 'po-select-source-reference)
  (define-key po-mode-map "\M-A" 'po-ignore-as-auxiliary)
;;;;  (define-key po-mode-map "\M-L" 'po-ignore-lexicon-file)
  (define-key po-mode-map "\M-S" 'po-ignore-source-path)
  )

(defun po-mode ()
  "Major mode for translators when they edit PO files.
Special commands:\\{po-mode-map}
Turning on PO mode calls the value of the variable `po-mode-hook',
if that value is non-nil.  Behaviour may be adjusted through some variables,
all reachable through `M-x customize', in group `Emacs.Editing.I18n.Po'."

  (interactive)
  (kill-all-local-variables)
  (setq major-mode 'po-mode
	mode-name "PO")
  (use-local-map po-mode-map)
  (if (fboundp 'easy-menu-define)
      (easy-menu-define po-mode-menu po-mode-map "" po-mode-menu-layout))
  (make-local-variable 'font-lock-defaults)
  (setq font-lock-defaults '(po-font-lock-keywords t))

  (make-local-variable 'po-read-only)
  (setq po-read-only buffer-read-only
	buffer-read-only t)

  (make-local-variable 'po-start-of-entry)
  (make-local-variable 'po-start-of-msgid)
  (make-local-variable 'po-start-of-msgstr)
  (make-local-variable 'po-end-of-entry)
  (make-local-variable 'po-entry-type)

  (make-local-variable 'po-translated-counter)
  (make-local-variable 'po-fuzzy-counter)
  (make-local-variable 'po-untranslated-counter)
  (make-local-variable 'po-obsolete-counter)
  (make-local-variable 'po-mode-line-string)

  (setq po-mode-flag t)

  (po-check-file-header)
  (po-compute-counters nil)

  (make-local-variable 'po-edited-fields)
  (setq po-edited-fields nil)

  (make-local-variable 'po-marker-stack)
  (setq po-marker-stack nil)

  (make-local-variable 'po-search-path)
  (setq po-search-path '(("./") ("../")))

  (make-local-variable 'po-reference-alist)
  (make-local-variable 'po-reference-cursor)
  (make-local-variable 'po-reference-check)
  (setq po-reference-alist nil
	po-reference-cursor nil
	po-reference-check 0)

  (make-local-variable 'po-keywords)
  (make-local-variable 'po-next-file-list)
  (make-local-variable 'po-string-start)
  (make-local-variable 'po-string-end)
  (make-local-variable 'po-marking-overlay)
  (setq po-keywords '(("gettext") ("gettext_noop") ("_") ("N_"))
	po-next-file-list nil
	po-string-start nil
	po-string-end nil
	po-marking-overlay (po-create-overlay))

  (message (_"You may type `h' or `?' for a short PO mode reminder."))
  (run-hooks 'po-mode-hook))

;;; Window management.

(make-variable-buffer-local 'po-mode-flag)

(defvar po-mode-line-entry '(po-mode-flag ("  " po-mode-line-string))
  "Mode line format entry displaying MODE-LINE-STRING.")

;; Insert MODE-LINE-ENTRY in mode line, but on first load only.
(or (member po-mode-line-entry mode-line-format)
    (let ((entry (member 'global-mode-string mode-line-format)))
      (setcdr entry (cons po-mode-line-entry (cdr entry)))))

(defun po-update-mode-line-string ()
  "Compute a new statistics string to display in mode line."
  (setq po-mode-line-string
	(concat (format "%dt" po-translated-counter)
		(if (> po-fuzzy-counter 0)
		    (format "+%df" po-fuzzy-counter))
		(if (> po-untranslated-counter 0)
		    (format "+%du" po-untranslated-counter))
		(if (> po-obsolete-counter 0)
		    (format "+%do" po-obsolete-counter))))
  (po-force-mode-line-update))

(defun po-type-counter ()
  "Return the symbol name of the counter appropriate for the current entry."
  (cond ((eq po-entry-type 'obsolete) 'po-obsolete-counter)
	((eq po-entry-type 'fuzzy) 'po-fuzzy-counter)
	((eq po-entry-type 'translated) 'po-translated-counter)
	((eq po-entry-type 'untranslated) 'po-untranslated-counter)
	(t (error (_"Unknown entry type")))))

(defun po-decrease-type-counter ()
  "Decrease the counter corresponding to the nature of the current entry."
  (let ((counter (po-type-counter)))
    (set counter (1- (eval counter)))))

(defun po-increase-type-counter ()
  "Increase the counter corresponding to the nature of the current entry.
Then, update the mode line counters."
  (let ((counter (po-type-counter)))
    (set counter (1+ (eval counter))))
  (po-update-mode-line-string))

;; Avoid byte compiler warnings.
(defvar po-fuzzy-regexp)
(defvar po-untranslated-regexp)

(defun po-compute-counters (flag)
  "Prepare counters for mode line display.  If FLAG, also echo entry position."
  (and flag (po-find-span-of-entry))
  (setq po-translated-counter 0
	po-fuzzy-counter 0
	po-untranslated-counter 0
	po-obsolete-counter 0)
  (let ((position 0) (total 0) here)
    (save-excursion
      (goto-char (point-min))
      (while (re-search-forward po-any-msgstr-regexp nil t)
	(and (= (% total 20) 0)
	     (if flag
		 (message (_"Position %d/%d") position total)
	       (message (_"Position %d") total)))
	(setq here (point))
	(goto-char (match-beginning 0))
	(setq total (1+ total))
	(and flag (eq (point) po-start-of-msgstr) (setq position total))
	(cond ((eq (following-char) ?#)
	       (setq po-obsolete-counter (1+ po-obsolete-counter)))
	      ((looking-at po-untranslated-regexp)
	       (setq po-untranslated-counter (1+ po-untranslated-counter)))
	      (t (setq po-translated-counter (1+ po-translated-counter))))
	(goto-char here))

      ;; Make another pass just for the fuzzy entries, kind of kludgey.
      ;; FIXME: Counts will be wrong if untranslated entries are fuzzy, yet
      ;; this should not normally happen.
      (goto-char (point-min))
      (while (re-search-forward po-fuzzy-regexp nil t)
	(setq po-fuzzy-counter (1+ po-fuzzy-counter)))
      (setq po-translated-counter (- po-translated-counter po-fuzzy-counter)))

    ;; Push the results out.
    (if flag
	(message (_"\
Position %d/%d; %d translated, %d fuzzy, %d untranslated, %d obsolete")
		 position total po-translated-counter po-fuzzy-counter
		 po-untranslated-counter po-obsolete-counter)
      (message "")))
  (po-update-mode-line-string))

(defun po-redisplay ()
  "Redisplay the current entry."
  ;; FIXME: Should try to fit the whole entry on the window.  If this is not
  ;; possible, should try to fit the comment and the msgid.  Otherwise,
  ;; should try to fit the msgid.  Else, the first line of the msgid should
  ;; be at the top of the window.
  (goto-char po-start-of-msgid))

(defun po-other-window ()
  "Get the cursor into another window, out of PO mode."
  (interactive)
  (if (one-window-p t)
      (progn
	(split-window)
	(switch-to-buffer (other-buffer)))
    (other-window 1)))

;;; Processing the PO file header entry.

(defun po-check-file-header ()
  "Create a missing PO mode file header, or replace an oldish one."
  (save-excursion
    (let ((buffer-read-only po-read-only)
	  insert-flag end-of-header)
      (goto-char (point-min))
      (if (re-search-forward po-any-msgstr-regexp nil t)
	  (progn

	    ;; There is at least one entry.
	    (goto-char (match-beginning 0))
	    (previous-line 1)
	    (setq end-of-header (match-end 0))
	    (if (looking-at "msgid \"\"\n")

		;; There is indeed a PO file header.
		(if (re-search-forward "\n\"PO-Revision-Date: "
				       end-of-header t)
		    nil

		  ;; This is an oldish header.  Replace it all.
		  (goto-char end-of-header)
		  (while (> (point) (point-min))
		    (previous-line 1)
		    (insert "#~ ")
		    (beginning-of-line))
		  (beginning-of-line)
		  (setq insert-flag t))

	      ;; The first entry is not a PO file header, insert one.
	      (setq insert-flag t)))

	;; Not a single entry found.
	(setq insert-flag t))

      (goto-char (point-min))
      (and insert-flag (insert po-default-file-header "\n")))))

(defun po-replace-revision-date ()
  "Replace the revision date by current time in the PO file header."
  (if (fboundp 'format-time-string)
      (if (or (eq po-auto-replace-revision-date t)
	      (and (eq po-auto-replace-revision-date 'ask)
		   (y-or-n-p (_"May I set PO-Revision-Date? "))))
	  (save-excursion
	    (goto-char (point-min))
	    (if (re-search-forward "^\"PO-Revision-Date:.*" nil t)
		(let* ((buffer-read-only po-read-only)
		       (time (current-time))
		       (seconds (or (car (current-time-zone time)) 0))
		       (minutes (/ (abs seconds) 60))
		       (zone (format "%c%02d:%02d"
				     (if (< seconds 0) ?- ?+)
				     (/ minutes 60)
				     (% minutes 60))))
		  (replace-match
		       (concat "\"PO-Revision-Date: "
			       (format-time-string "%Y-%m-%d %H:%M" time)
			       zone "\\n\"")
		       t t))))
	(message ""))
    (message (_"PO-Revision-Date should be adjusted..."))))

;;; Handling span of entry, entry type and entry attributes.

(defun po-find-span-of-entry ()
  "Find the extent of the PO file entry where the cursor is.  Set variables
PO-START-OF-ENTRY, PO-START-OF-MSGID, PO-START-OF-MSGSTR, PO-END-OF-ENTRY
and PO-ENTRY-TYPE to meaningful values.  Decreasing priority of type
interpretation is: obsolete, fuzzy, untranslated or translated."
  (let ((here (point)))
    (if (re-search-backward po-any-msgstr-regexp nil t)
	(progn

	  ;; After a backward match, (match-end 0) will not extend
	  ;; beyond point, in case point was *inside* the regexp.  We
	  ;; need a dependable (match-end 0), so we redo the match in
	  ;; the forward direction.
	  (re-search-forward po-any-msgstr-regexp)
	  (if (<= (match-end 0) here)
	      (progn

		;; We most probably found the msgstr of the previous
		;; entry.  The current entry then starts just after
		;; its end, save this information just in case.
		(setq po-start-of-entry (match-end 0))

		;; However, it is also possible that we are located in
		;; the crumb after the last entry in the file.  If
		;; yes, we know the middle and end of last PO entry.
		(setq po-start-of-msgstr (match-beginning 0)
		      po-end-of-entry (match-end 0))

		(if (re-search-forward po-any-msgstr-regexp nil t)
		    (progn

		      ;; We definitely were not in the crumb.
		      (setq po-start-of-msgstr (match-beginning 0)
			    po-end-of-entry (match-end 0)))

		  ;; We were in the crumb.  The start of the last PO
		  ;; file entry is the end of the previous msgstr if
		  ;; any, or else, the beginning of the file.
		  (goto-char po-start-of-msgstr)
		  (setq po-start-of-entry
			(if (re-search-backward po-any-msgstr-regexp nil t)
			    (match-end 0)
			  (point-min)))))

	    ;; The cursor was inside msgstr of the current entry.
	    (setq po-start-of-msgstr (match-beginning 0)
		  po-end-of-entry (match-end 0))
	    ;; The start of this entry is the end of the previous
	    ;; msgstr if any, or else, the beginning of the file.
	    (goto-char po-start-of-msgstr)
	    (setq po-start-of-entry
		  (if (re-search-backward po-any-msgstr-regexp nil t)
		      (match-end 0)
		    (point-min)))))

      ;; The cursor was before msgstr in the first entry in the file.
      (setq po-start-of-entry (point-min))
      (goto-char po-start-of-entry)
      ;; There is at least the PO file header, so this should match.
      (re-search-forward po-any-msgstr-regexp)
      (setq po-start-of-msgstr (match-beginning 0)
	    po-end-of-entry (match-end 0)))

    ;; Find start of msgid.
    (goto-char po-start-of-entry)
    (re-search-forward po-any-msgid-regexp)
    (setq po-start-of-msgid (match-beginning 0))

    ;; Classify the entry.
    (setq po-entry-type
	  (if (eq (following-char) ?#)
	      'obsolete
	    (goto-char po-start-of-entry)
	    (if (re-search-forward po-fuzzy-regexp po-start-of-msgid t)
		'fuzzy
	      (goto-char po-start-of-msgstr)
	      (if (looking-at po-untranslated-regexp)
		  'untranslated
		'translated))))

    ;; Put the cursor back where it was.
    (goto-char here)))

(defun po-add-attribute (name)
  "Add attribute NAME to the current entry, unless it is already there."
  (save-excursion
    (let ((buffer-read-only po-read-only))
      (goto-char po-start-of-entry)
      (if (re-search-forward "\n#[,!] .*" po-start-of-msgid t)
	  (save-restriction
	    (narrow-to-region (match-beginning 0) (match-end 0))
	    (goto-char (point-min))
	    (if (re-search-forward (concat "\\b" name "\\b") nil t)
		nil
	      (goto-char (point-max))
	      (insert ", " name)))
	(skip-chars-forward "\n")
	(while (eq (following-char) ?#)
	  (next-line 1))
	(insert "#, " name "\n")))))

(defun po-delete-attribute (name)
  "Delete attribute NAME from the current entry, if any."
  (save-excursion
    (let ((buffer-read-only po-read-only))
      (goto-char po-start-of-entry)
      (if (re-search-forward "\n#[,!] .*" po-start-of-msgid t)
	  (save-restriction
	    (narrow-to-region (match-beginning 0) (match-end 0))
	    (goto-char (point-min))
	    (if (re-search-forward
		 (concat "\\(\n#[,!] " name "$\\|, " name "$\\| " name ",\\)")
		 nil t)
		(replace-match "" t t)))))))

;;; Entry positionning.

(defun po-say-location-depth ()
  "Tell how many entries in the entry location stack."
  (let ((depth (length po-marker-stack)))
    (cond ((= depth 0) (message (_"Empty location stack")))
	  ((= depth 1) (message (_"One entry in location stack")))
	  (t (message (_"%d entries in location stack") depth)))))

(defun po-push-location ()
  "Stack the location of the current entry, for later return."
  (interactive)
  (po-find-span-of-entry)
  (save-excursion
    (goto-char po-start-of-msgid)
    (setq po-marker-stack (cons (point-marker) po-marker-stack)))
  (po-say-location-depth))

(defun po-pop-location ()
  "Unstack a saved location, and return to the corresponding entry."
  (interactive)
  (if po-marker-stack
      (progn
	(goto-char (car po-marker-stack))
	(setq po-marker-stack (cdr po-marker-stack))
	(po-current-entry)
	(po-say-location-depth))
    (error (_"The entry location stack is empty"))))

(defun po-exchange-location ()
  "Exchange the location of the current entry with the top of stack."
  (interactive)
  (if po-marker-stack
      (progn
	(po-find-span-of-entry)
	(goto-char po-start-of-msgid)
	(let ((location (point-marker)))
	  (goto-char (car po-marker-stack))
	  (setq po-marker-stack (cons location (cdr po-marker-stack))))
	(po-current-entry)
	(po-say-location-depth))
    (error (_"The entry location stack is empty"))))

(defun po-current-entry ()
  "Display the current entry."
  (interactive)
  (po-find-span-of-entry)
  (po-redisplay))

(defun po-first-entry-with-regexp (regexp)
  "Display the first entry in the file which msgstr matches REGEXP."
  (let ((here (point)))
    (goto-char (point-min))
    (if (re-search-forward regexp nil t)
	(progn
	  (goto-char (match-beginning 0))
	  (po-current-entry))
      (goto-char here)
      (error (_"There is no such entry")))))

(defun po-last-entry-with-regexp (regexp)
  "Display the last entry in the file which msgstr matches REGEXP."
  (let ((here (point)))
    (goto-char (point-max))
    (if (re-search-backward regexp nil t)
	(po-current-entry)
      (goto-char here)
      (error (_"There is no such entry")))))

(defun po-next-entry-with-regexp (regexp wrap)
  "Display the entry following the current entry which msgstr matches REGEXP.
If WRAP is not nil, the search may wrap around the buffer."
  (po-find-span-of-entry)
  (let ((here (point)))
    (goto-char po-end-of-entry)
    (if (re-search-forward regexp nil t)
	(progn
	  (goto-char (match-beginning 0))
	  (po-current-entry))
      (if (and wrap
	       (progn
		 (goto-char (point-min))
		 (re-search-forward regexp po-start-of-entry t)))
	  (progn
	    (goto-char (match-beginning 0))
	    (po-current-entry)
	    (message (_"Wrapping around the buffer")))
	(goto-char here)
	(error (_"There is no such entry"))))))

(defun po-previous-entry-with-regexp (regexp wrap)
  "Redisplay the entry preceding the current entry which msgstr matches REGEXP.
If WRAP is not nil, the search may wrap around the buffer."
  (po-find-span-of-entry)
  (let ((here (point)))
    (goto-char po-start-of-entry)
    (if (re-search-backward regexp nil t)
	(po-current-entry)
      (if (and wrap
	       (progn
		 (goto-char (point-max))
		 (re-search-backward regexp po-end-of-entry t)))
	  (progn
	    (po-current-entry)
	    (message (_"Wrapping around the buffer")))
	(goto-char here)
	(error (_"There is no such entry"))))))

;; Any entries.

(defun po-first-entry ()
  "Display the first entry."
  (interactive)
  (po-first-entry-with-regexp po-any-msgstr-regexp))

(defun po-last-entry ()
  "Display the last entry."
  (interactive)
  (po-last-entry-with-regexp po-any-msgstr-regexp))

(defun po-next-entry ()
  "Display the entry following the current entry."
  (interactive)
  (po-next-entry-with-regexp po-any-msgstr-regexp nil))

(defun po-previous-entry ()
  "Display the entry preceding the current entry."
  (interactive)
  (po-previous-entry-with-regexp po-any-msgstr-regexp nil))

;; Untranslated entries.

(defvar po-after-entry-regexp
  "\\(\\'\\|\\(#[ \t]*\\)?[^\"]\\)"
  "Regexp which should be true after a full msgstr string matched.")

(defvar po-untranslated-regexp
  (concat "^msgstr[ \t]*\"\"\n" po-after-entry-regexp)
  "Regexp matching a whole msgstr field, but only if active and empty.")

(defun po-next-untranslated-entry ()
  "Find the next untranslated entry, wrapping around if necessary."
  (interactive)
  (po-next-entry-with-regexp po-untranslated-regexp t))

(defun po-previous-untranslated-entry ()
  "Find the previous untranslated entry, wrapping around if necessary."
  (interactive)
  (po-previous-entry-with-regexp po-untranslated-regexp t))

(defun po-msgid-to-msgstr ()
  "Use another window to edit msgstr reinitialized with msgid."
  (interactive)
  (po-find-span-of-entry)
  (if (or (eq po-entry-type 'untranslated)
	  (eq po-entry-type 'obsolete)
	  (y-or-n-p (_"Really loose previous translation? ")))
      (po-set-msgstr (po-get-msgid nil)))
  (message ""))

;; Obsolete entries.

(defvar po-obsolete-msgstr-regexp
  "^#~?[ \t]*msgstr.*\n\\(#~?[ \t]*\".*\n\\)*"
  "Regexp matching a whole msgstr field of an obsolete entry.")

(defun po-next-obsolete-entry ()
  "Find the next obsolete entry, wrapping around if necessary."
  (interactive)
  (po-next-entry-with-regexp po-obsolete-msgstr-regexp t))

(defun po-previous-obsolete-entry ()
  "Find the previous obsolete entry, wrapping around if necessary."
  (interactive)
  (po-previous-entry-with-regexp po-obsolete-msgstr-regexp t))

;; Fuzzy entries.

(defvar po-fuzzy-regexp "^#[,!] .*fuzzy"
  "Regexp matching the string inserted by msgmerge for translations
which does not match exactly.")

(defun po-next-fuzzy-entry ()
  "Find the next fuzzy entry, wrapping around if necessary."
  (interactive)
  (po-next-entry-with-regexp po-fuzzy-regexp t))

(defun po-previous-fuzzy-entry ()
  "Find the next fuzzy entry, wrapping around if necessary."
  (interactive)
  (po-previous-entry-with-regexp po-fuzzy-regexp t))

(defun po-unfuzzy ()
  "Remove the fuzzy attribute for the current entry."
  (interactive)
  (po-find-span-of-entry)
  (cond ((eq po-entry-type 'fuzzy)
	 (po-decrease-type-counter)
	 (po-delete-attribute "fuzzy")
	 (po-current-entry)
	 (po-increase-type-counter)))
  (if po-auto-select-on-unfuzzy
      (po-auto-select-entry))
  (po-update-mode-line-string))

;; Translated entries.

(defun po-next-translated-entry ()
  "Find the next untranslated entry, wrapping around if necessary."
  (interactive)
  (if (= po-translated-counter 0)
      (error (_"There is no such entry"))
    (po-next-entry-with-regexp po-untranslated-regexp t)
    (po-find-span-of-entry)
    (while (not (eq po-entry-type 'translated))
      (po-next-entry-with-regexp po-any-msgstr-regexp t)
      (po-find-span-of-entry))))

(defun po-previous-translated-entry ()
  "Find the previous untranslated entry, wrapping around if necessary."
  (interactive)
  (if (= po-translated-counter 0)
      (error (_"There is no such entry"))
    (po-previous-entry-with-regexp po-any-msgstr-regexp t)
    (po-find-span-of-entry)
    (while (not (eq po-entry-type 'translated))
      (po-previous-entry-with-regexp po-untranslated-regexp t)
    (po-find-span-of-entry))))

;; Auto-selection feature.

(defun po-auto-select-entry ()
  "Select the next entry having the same type as the current one.
If none, wrap from the beginning of the buffer with another type,
going from untranslated to fuzzy, and from fuzzy to obsolete.
Plain translated entries are always disregarded unless there are
no entries of the other types."
  (interactive)
  (po-find-span-of-entry)
  (goto-char po-end-of-entry)
  (if (and (= po-untranslated-counter 0)
	   (= po-fuzzy-counter 0)
	   (= po-obsolete-counter 0))

      ;; All entries are plain translated.  Next entry will do, or
      ;; wrap around if there is none.
      (if (re-search-forward po-any-msgstr-regexp nil t)
	  (goto-char (match-beginning 0))
	(goto-char (point-min)))

    ;; If over a translated entry, look for an untranslated one first.
    ;; Else, look for an entry of the same type first.
    (let ((goal (if (eq po-entry-type 'translated)
		    'untranslated
		  po-entry-type)))
      (while goal

	;; Find an untranslated entry, or wrap up for a fuzzy entry.
	(if (eq goal 'untranslated)
	    (if (and (> po-untranslated-counter 0)
		     (re-search-forward po-untranslated-regexp nil t))
		(progn
		  (goto-char (match-beginning 0))
		  (setq goal nil))
	      (goto-char (point-min))
	      (setq goal 'fuzzy)))

	;; Find a fuzzy entry, or wrap up for an obsolete entry.
	(if (eq goal 'fuzzy)
	    (if (and (> po-fuzzy-counter 0)
		     (re-search-forward po-fuzzy-regexp nil t))
		(progn
		  (goto-char (match-beginning 0))
		  (setq goal nil))
	      (goto-char (point-min))
	      (setq goal 'obsolete)))

	;; Find an obsolete entry, or wrap up for an untranslated entry.
	(if (eq goal 'obsolete)
	    (if (and (> po-obsolete-counter 0)
		     (re-search-forward po-obsolete-msgstr-regexp nil t))
		(progn
		  (goto-char (match-beginning 0))
		  (setq goal nil))
	      (goto-char (point-min))
	      (setq goal 'untranslated))))))

  ;; Display this entry nicely.
  (po-current-entry))

;;; Killing and yanking fields.

(defun po-extract-unquoted (buffer start end)
  "Extract and return the unquoted string in BUFFER going from START to END.
Crumb preceding or following the quoted string is ignored."
  (po-with-temp-buffer
    (insert-buffer-substring buffer start end)
    ;; Remove preceding crumb.
    (goto-char (point-min))
    (search-forward "\"")
    (delete-region (point-min) (point))
    ;; Remove following crumb.
    (goto-char (point-max))
    (search-backward "\"")
    (delete-region (point) (point-max))
    ;; Glue concatenated strings.
    (goto-char (point-min))
    (while (re-search-forward "\"[ \t]*\\\\?\n\\(#~?\\)?[ \t]*\"" nil t)
      (replace-match "" t t))
    ;; Remove escaped newlines.
    (goto-char (point-min))
    (while (re-search-forward "\\\\[ \t]*\n" nil t)
      (replace-match "" t t))
    ;; Unquote individual characters.
    (goto-char (point-min))
    (while (re-search-forward "\\\\[\"abfnt\\0-7]" nil t)
      (cond ((eq (preceding-char) ?\") (replace-match "\"" t t))
	    ((eq (preceding-char) ?a) (replace-match "\a" t t))
	    ((eq (preceding-char) ?b) (replace-match "\b" t t))
	    ((eq (preceding-char) ?f) (replace-match "\f" t t))
	    ((eq (preceding-char) ?n) (replace-match "\n" t t))
	    ((eq (preceding-char) ?t) (replace-match "\t" t t))
	    ((eq (preceding-char) ?\\) (replace-match "\\" t t))
	    (t (let ((value (- (preceding-char) ?0)))
		 (replace-match "" t t)
		 (while (looking-at "[0-7]")
		   (setq value (+ (* 8 value) (- (following-char) ?0)))
		   (replace-match "" t t))
		 (insert value)))))
    (buffer-string)))

(defun po-eval-requoted (form prefix obsolete)
  "Eval FORM, which inserts a string, and return the string fully requoted.
If PREFIX, precede the result with its contents.  If OBSOLETE, comment all
generated lines in the returned string.  Evaluating FORM should insert the
wanted string in the buffer which is current at the time of evaluation.
If FORM is itself a string, then this string is used for insertion."
  (po-with-temp-buffer
    (if (stringp form)
	(insert form)
      (push-mark)
      (eval form))
    (goto-char (point-min))
    (let ((multi-line (re-search-forward "[^\n]\n+[^\n]" nil t)))
      (goto-char (point-min))
      (while (re-search-forward "[\"\a\b\f\n\t\\]" nil t)
	(cond ((eq (preceding-char) ?\") (replace-match "\\\"" t t))
	      ((eq (preceding-char) ?\a) (replace-match "\\a" t t))
	      ((eq (preceding-char) ?\b) (replace-match "\\b" t t))
	      ((eq (preceding-char) ?\f) (replace-match "\\f" t t))
	      ((eq (preceding-char) ?\n)
	       (replace-match (if (or (not multi-line) (eobp))
				  "\\n"
				"\\n\"\n\"")
			      t t))
	      ((eq (preceding-char) ?\t) (replace-match "\\t" t t))
	      ((eq (preceding-char) ?\\) (replace-match "\\\\" t t))))
      (goto-char (point-min))
      (if prefix (insert prefix " "))
      (insert (if multi-line "\"\"\n\"" "\""))
      (goto-char (point-max))
      (insert "\"")
      (if prefix (insert "\n"))
      (if obsolete
	  (progn
	    (goto-char (point-min))
	    (while (not (eobp))
	      (or (eq (following-char) ?\n) (insert "#~ "))
	      (search-forward "\n"))))
      (buffer-string))))

(defun po-get-msgid (kill)
  "Extract and return the unquoted msgid string.
If KILL, then add the unquoted string to the kill ring."
  (let ((string (po-extract-unquoted (current-buffer)
				     po-start-of-msgid po-start-of-msgstr)))
    (if kill (po-kill-new string))
    string))

(defun po-get-msgstr (kill)
  "Extract and return the unquoted msgstr string.
If KILL, then add the unquoted string to the kill ring."
  (let ((string (po-extract-unquoted (current-buffer)
				     po-start-of-msgstr po-end-of-entry)))
    (if kill (po-kill-new string))
    string))

(defun po-set-msgid (form)
  "Replace the current msgid, using FORM to get a string.
Evaluating FORM should insert the wanted string in the current buffer.  If
FORM is itself a string, then this string is used for insertion.  The string
is properly requoted before the replacement occurs.

Returns `nil' if the buffer has not been modified, for if the new msgid
described by FORM is merely identical to the msgid already in place."
  (let ((string (po-eval-requoted form "msgid" (eq po-entry-type 'obsolete))))
    (save-excursion
      (goto-char po-start-of-entry)
      (re-search-forward po-any-msgid-regexp po-start-of-msgstr)
      (and (not (string-equal (po-buffer-substring (match-beginning 0)
						   (match-end 0))
			      string))
	   (let ((buffer-read-only po-read-only))
	     (replace-match string t t)
	     (goto-char po-start-of-msgid)
	     (po-find-span-of-entry)
	     t)))))

(defun po-set-msgstr (form)
  "Replace the current msgstr, using FORM to get a string.
Evaluating FORM should insert the wanted string in the current buffer.  If
FORM is itself a string, then this string is used for insertion.  The string
is properly requoted before the replacement occurs.

Returns `nil' if the buffer has not been modified, for if the new msgstr
described by FORM is merely identical to the msgstr already in place."
  (let ((string (po-eval-requoted form "msgstr" (eq po-entry-type 'obsolete))))
    (save-excursion
      (goto-char po-start-of-entry)
      (re-search-forward po-any-msgstr-regexp po-end-of-entry)
      (and (not (string-equal (po-buffer-substring (match-beginning 0)
						   (match-end 0))
			      string))
	   (let ((buffer-read-only po-read-only))
	     (po-decrease-type-counter)
	     (replace-match string t t)
	     (goto-char po-start-of-msgid)
	     (po-find-span-of-entry)
	     (po-increase-type-counter)
	     t)))))

(defun po-kill-ring-save-msgstr ()
  "Push the msgstr string from current entry on the kill ring."
  (interactive)
  (po-find-span-of-entry)
  (po-get-msgstr t))

(defun po-kill-msgstr ()
  "Empty the msgstr string from current entry, pushing it on the kill ring."
  (interactive)
  (po-kill-ring-save-msgstr)
  (po-set-msgstr ""))

(defun po-yank-msgstr ()
  "Replace the current msgstr string by the top of the kill ring."
  (interactive)
  (po-find-span-of-entry)
  (po-set-msgstr (if (eq last-command 'yank) '(yank-pop 1) '(yank)))
  (setq this-command 'yank))

(defun po-fade-out-entry ()
  "Mark an active entry as fuzzy; obsolete a fuzzy or untranslated entry;
or completely delete an obsolete entry, saving its msgstr on the kill ring."
  (interactive)
  (po-find-span-of-entry)

  (cond ((eq po-entry-type 'translated)
	 (po-decrease-type-counter)
	 (po-add-attribute "fuzzy")
	 (po-current-entry)
	 (po-increase-type-counter))

	((or (eq po-entry-type 'fuzzy)
	     (eq po-entry-type 'untranslated))
	 (if (yes-or-no-p (_"Should I really obsolete this entry? "))
	     (progn
	       (po-decrease-type-counter)
	       (save-excursion
		 (save-restriction
		   (narrow-to-region po-start-of-entry po-end-of-entry)
		   (let ((buffer-read-only po-read-only))
		     (goto-char (point-min))
		     (skip-chars-forward "\n")
		     (while (not (eobp))
		       (insert "#~ ")
		       (search-forward "\n")))))
	       (po-current-entry)
	       (po-increase-type-counter)))
	 (message ""))

	((and (eq po-entry-type 'obsolete)
	      (po-check-for-pending-edit po-start-of-msgid)
	      (po-check-for-pending-edit po-start-of-msgstr))
	 (po-decrease-type-counter)
	 (po-update-mode-line-string)
	 (po-get-msgstr t)
	 (let ((buffer-read-only po-read-only))
	   (delete-region po-start-of-entry po-end-of-entry))
	 (goto-char po-start-of-entry)
	 (if (re-search-forward po-any-msgstr-regexp nil t)
	     (goto-char (match-beginning 0))
	   (re-search-backward po-any-msgstr-regexp nil t))
	 (po-current-entry)
	 (message ""))))

;;; Killing and yanking comments.

(defvar po-active-comment-regexp
  "^\\(#\n\\|# .*\n\\)+"
  "Regexp matching the whole editable comment part of an active entry.")

(defvar po-obsolete-comment-regexp
  "^\\(#~? #\n\\|#~? # .*\n\\)+"
  "Regexp matching the whole editable comment part of an obsolete entry.")

(defun po-get-comment (kill-flag)
  "Extract and return the editable comment string, uncommented.
If KILL-FLAG, then add the unquoted comment to the kill ring."
  (let ((buffer (current-buffer))
	(obsolete (eq po-entry-type 'obsolete)))
    (save-excursion
      (goto-char po-start-of-entry)
      (if (re-search-forward (if obsolete po-obsolete-comment-regexp
			         po-active-comment-regexp)
			     po-end-of-entry t)
	  (po-with-temp-buffer
	    (insert-buffer-substring buffer (match-beginning 0) (match-end 0))
	    (goto-char (point-min))
	    (while (not (eobp))
	      (if (looking-at (if obsolete "#~? # ?" "#~? ?"))
		  (replace-match "" t t))
	      (forward-line 1))
	    (and kill-flag (copy-region-as-kill (point-min) (point-max)))
	    (buffer-string))
	""))))

(defun po-set-comment (form)
  "Using FORM to get a string, replace the current editable comment.
Evaluating FORM should insert the wanted string in the current buffer.
If FORM is itself a string, then this string is used for insertion.
The string is properly recommented before the replacement occurs."
  (let ((obsolete (eq po-entry-type 'obsolete))
	string)
    (po-with-temp-buffer
      (if (stringp form)
	  (insert form)
	(push-mark)
	(eval form))
      (if (not (or (bobp) (= (preceding-char) ?\n)))
	  (insert "\n"))
      (goto-char (point-min))
      (while (not (eobp))
	(insert (if (= (following-char) ?\n)
		    (if obsolete "#~ #" "#")
		  (if obsolete "#~ # " "# ")))
	(search-forward "\n"))
      (setq string (buffer-string)))
    (goto-char po-start-of-entry)
    (if (re-search-forward
	 (if obsolete po-obsolete-comment-regexp po-active-comment-regexp)
	 po-end-of-entry t)
	(if (not (string-equal (po-buffer-substring (match-beginning 0)
						    (match-end 0))
			       string))
	    (let ((buffer-read-only po-read-only))
	      (replace-match string t t)))
      (skip-chars-forward " \t\n")
      (let ((buffer-read-only po-read-only))
	(insert string))))
  (po-current-entry))

(defun po-kill-ring-save-comment ()
  "Push the msgstr string from current entry on the kill ring."
  (interactive)
  (po-find-span-of-entry)
  (po-get-comment t))

(defun po-kill-comment ()
  "Empty the msgstr string from current entry, pushing it on the kill ring."
  (interactive)
  (po-kill-ring-save-comment)
  (po-set-comment "")
  (po-redisplay))

(defun po-yank-comment ()
  "Replace the current comment string by the top of the kill ring."
  (interactive)
  (po-find-span-of-entry)
  (po-set-comment (if (eq last-command 'yank) '(yank-pop 1) '(yank)))
  (setq this-command 'yank)
  (po-redisplay))

;;; Editing management and submode.

;; In a string edit buffer, BACK-POINTER points to one of the slots of the
;; list EDITED-FIELDS kept in the PO buffer.  See its description elsewhere.
;; Reminder: slots have the form (ENTRY-MARKER EDIT-BUFFER OVERLAY-INFO).

(defvar po-subedit-back-pointer)

(defun po-clean-out-killed-edits ()
  "From EDITED-FIELDS, clean out any edit having a killed edit buffer."
  (while (and po-edited-fields
	      (null (buffer-name (nth 1 (car po-edited-fields)))))
    (let ((overlay (nth 2 (car po-edited-fields))))
      (and overlay (po-dehighlight overlay)))
    (setq po-edited-fields (cdr po-edited-fields)))
  (let ((cursor po-edited-fields))
    (while cursor
      (let ((slot (car cursor)))
	(setq cursor (cdr cursor))
	(if (buffer-name (nth 1 slot))
	    nil
	  (let ((overlay (nth 2 slot)))
	    (and overlay (po-dehighlight overlay)))
	  (setq po-edited-fields (delete slot po-edited-fields)))))))

(defun po-check-all-pending-edits ()
  "Resume any pending edit.  Return nil if some remains."
  (po-clean-out-killed-edits)
  (or (null po-edited-fields)
      (let ((slot (car po-edited-fields)))
	(goto-char (nth 0 slot))
	(pop-to-buffer (nth 1 slot))
	(let ((overlay (nth 2 slot)))
	  (and overlay (po-rehighlight overlay)))
	(message po-subedit-message)
	nil)))

(defun po-check-for-pending-edit (position)
  "Resume any pending edit at POSITION.  Return nil if such edit exists."
  (po-clean-out-killed-edits)
  (let ((marker (make-marker)))
    (set-marker marker position)
    (let ((slot (assoc marker po-edited-fields)))
      (if slot
	  (progn
	    (goto-char marker)
	    (pop-to-buffer (nth 1 slot))
	    (let ((overlay (nth 2 slot)))
	      (and overlay (po-rehighlight overlay)))
	    (message po-subedit-message)))
      (not slot))))

(defun po-edit-out-full ()
  "Get out of PO mode, leaving PO file buffer in fundamental mode."
  (interactive)
  (if (and (po-check-all-pending-edits)
	   (yes-or-no-p (_"Should I let you edit the whole PO file? ")))
      (progn
	(setq buffer-read-only po-read-only)
	(fundamental-mode)
	(message (_"Type `M-x po-mode RET' once done")))))

(defvar po-subedit-mode-map nil
  "Keymap while editing a PO mode entry (or the full PO file).")
(if po-subedit-mode-map
    ()
  (setq po-subedit-mode-map (make-sparse-keymap))
  (define-key po-subedit-mode-map "\C-c\C-a" 'po-subedit-cycle-auxiliary)
  (define-key po-subedit-mode-map "\C-c\C-c" 'po-subedit-exit)
  (define-key po-subedit-mode-map "\C-c\C-k" 'po-subedit-abort))

(defun po-subedit-abort ()
  "Exit the subedit buffer, merely discarding its contents."
  (interactive)
  (let* ((edit-buffer (current-buffer))
	 (back-pointer po-subedit-back-pointer)
	 (marker (nth 0 back-pointer))
	 (overlay (nth 2 back-pointer))
	 (buffer (marker-buffer marker)))
    (if (null buffer)
	(error (_"Corresponding PO buffer does not exist anymore"))
      (or (one-window-p) (delete-window))
      (switch-to-buffer buffer)
      (goto-char marker)
      (and overlay (po-dehighlight overlay))
      (kill-buffer edit-buffer)
      (setq po-edited-fields (delete back-pointer po-edited-fields)))))

(defun po-subedit-exit ()
  "Exit the subedit buffer, replacing the string in the PO buffer."
  (interactive)
  (goto-char (point-max))
  (skip-chars-backward " \t\n")
  (if (eq (preceding-char) ?<)
      (delete-region (1- (point)) (point-max)))
  (let ((string (buffer-string)))
    (po-subedit-abort)
    (po-find-span-of-entry)
    (cond ((= (point) po-start-of-msgid)
	   (po-set-comment string)
	   (po-redisplay))
	  ((= (point) po-start-of-msgstr)
	   (let ((replaced (po-set-msgstr string)))
	     (if (and replaced
		      po-auto-fuzzy-on-edit
		      (eq po-entry-type 'translated))
		 (progn
		   (po-decrease-type-counter)
		   (po-add-attribute "fuzzy")
		   (po-current-entry)
		   (po-increase-type-counter)))))
	  (t (debug)))))

(defun po-edit-string (string type expand-tabs)
  "Prepare a pop up buffer for editing STRING, which is of a given TYPE.
TYPE may be 'comment or 'msgstr.  If EXPAND-TABS, expand tabs to spaces.
Run functions on po-subedit-mode-hook."
  (let ((marker (make-marker)))
    (set-marker marker (cond ((eq type 'comment) po-start-of-msgid)
			     ((eq type 'msgstr) po-start-of-msgstr)))
    (if (po-check-for-pending-edit marker)
	(let ((edit-buffer (generate-new-buffer
			    (concat "*" (buffer-name) "*")))
	      (buffer (current-buffer))
	      overlay slot)
	  (if (and (eq type 'msgstr) po-highlighting)
	      ;; ;; Try showing all of msgid in the upper window while editing.
	      ;; (goto-char (1- po-start-of-msgstr))
	      ;; (recenter -1)
	      (save-excursion
		(goto-char po-start-of-entry)
		(re-search-forward po-any-msgid-regexp nil t)
		(let ((end (1- (match-end 0))))
		  (goto-char (match-beginning 0))
		  (re-search-forward "msgid +" nil t)
		  (setq overlay (po-create-overlay))
		  (po-highlight overlay (point) end buffer))))
	  (setq slot (list marker edit-buffer overlay)
		po-edited-fields (cons slot po-edited-fields))
	  (pop-to-buffer edit-buffer)
	  (make-local-variable 'po-subedit-back-pointer)
	  (setq po-subedit-back-pointer slot)
	  (erase-buffer)
	  (insert string "<")
	  (goto-char (point-min))
	  (and expand-tabs (setq indent-tabs-mode nil))
	  (use-local-map po-subedit-mode-map)
	  (run-hooks 'po-subedit-mode-hook)
	  (message po-subedit-message)))))

(defun po-edit-comment ()
  "Use another window to edit the current translator comment."
  (interactive)
  (po-find-span-of-entry)
  (po-edit-string (po-get-comment nil) 'comment nil))

(defun po-edit-msgstr ()
  "Use another window to edit the current msgstr."
  (interactive)
  (po-find-span-of-entry)
  (po-edit-string (if (and po-auto-edit-with-msgid
			   (eq po-entry-type 'untranslated))
		      (po-get-msgid nil)
		    (po-get-msgstr nil))
		  'msgstr
		  t))

;;; String normalization and searching.

(defun po-normalize-old-style (explain)
  "Normalize old gettext style fields using K&R C multiline string syntax.
To minibuffer messages sent while normalizing, add the EXPLAIN string."
  (let ((here (point-marker))
	(counter 0)
	(buffer-read-only po-read-only))
    (goto-char (point-min))
    (message (_"Normalizing %d, %s") counter explain)
    (while (re-search-forward
	    "\\(^#?[ \t]*msg\\(id\\|str\\)[ \t]*\"\\|[^\" \t][ \t]*\\)\\\\\n"
	    nil t)
      (if (= (% counter 10) 0)
	  (message (_"Normalizing %d, %s") counter explain))
      (replace-match "\\1\"\n\"" t nil)
      (setq counter (1+ counter)))
    (goto-char here)
    (message (_"Normalizing %d...done") counter)))

(defun po-normalize-field (field explain)
  "Normalize FIELD of all entries.  FIELD is either the symbol msgid or msgstr.
To minibuffer messages sent while normalizing, add the EXPLAIN string."
  (let ((here (point-marker))
	(counter 0))
    (goto-char (point-min))
    (while (re-search-forward po-any-msgstr-regexp nil t)
      (if (= (% counter 10) 0)
	  (message (_"Normalizing %d, %s") counter explain))
      (goto-char (match-beginning 0))
      (po-find-span-of-entry)
      (cond ((eq field 'msgid) (po-set-msgid (po-get-msgid nil)))
	    ((eq field 'msgstr) (po-set-msgstr (po-get-msgstr nil))))
      (goto-char po-end-of-entry)
      (setq counter (1+ counter)))
    (goto-char here)
    (message (_"Normalizing %d...done") counter)))

;; Normalize, but the British way! :-)
(defsubst po-normalise () (po-normalize))

(defun po-normalize ()
  "Normalize all entries in the PO file."
  (interactive)
  (po-normalize-old-style (_"pass 1/3"))
  (po-normalize-field t (_"pass 2/3"))
  (po-normalize-field nil (_"pass 3/3"))
  ;; The last PO file entry has just been processed.
  (if (not (= po-end-of-entry (point-max)))
      (let ((buffer-read-only po-read-only))
	(kill-region po-end-of-entry (point-max))))
  ;; A bizarre format might have fooled the counters, so recompute
  ;; them to make sure their value is dependable.
  (po-compute-counters nil))

;;; Multiple PO files.

(defun po-show-auxiliary-list ()
  "Echo the current auxiliary list in the message area."
  (if po-auxiliary-list
      (let ((cursor po-auxiliary-cursor)
	    string)
	(while cursor
	  (setq string (concat string (if string " ") (car (car cursor)))
		cursor (cdr cursor)))
	(setq cursor po-auxiliary-list)
	(while (not (eq cursor po-auxiliary-cursor))
	  (setq string (concat string (if string " ") (car (car cursor)))
		cursor (cdr cursor)))
	(message string))
    (message (_"No auxiliary files."))))

(defun po-consider-as-auxiliary ()
  "Add the current PO file to the list of auxiliary files."
  (interactive)
  (if (member (list buffer-file-name) po-auxiliary-list)
      nil
    (setq po-auxiliary-list
	  (nconc po-auxiliary-list (list (list buffer-file-name))))
    (or po-auxiliary-cursor
	(setq po-auxiliary-cursor po-auxiliary-list)))
  (po-show-auxiliary-list))

(defun po-ignore-as-auxiliary ()
  "Delete the current PO file from the list of auxiliary files."
  (interactive)
  (setq po-auxiliary-list (delete (list buffer-file-name) po-auxiliary-list)
	po-auxiliary-cursor po-auxiliary-list)
  (po-show-auxiliary-list))

(defun po-seek-equivalent-translation (name string)
  "Search a PO file NAME for a `msgid' STRING having a non-empty `msgstr'.
STRING is the full quoted msgid field, including the `msgid' keyword.  When
found, display the file over the current window, with the `msgstr' field
possibly highlighted, the cursor at start of msgid, then return `t'.
Otherwise, move nothing, and just return `nil'."
  (let ((current (current-buffer))
	(buffer (find-file-noselect name)))
    (set-buffer buffer)
    (let ((start (point))
	  found)
      (goto-char (point-min))
      (while (and (not found) (search-forward string nil t))
	;; Screen out longer `msgid's.
	(if (looking-at "^msgstr ")
	    (progn
	      (po-find-span-of-entry)
	      ;; Ignore an untranslated entry.
	      (or (string-equal
		   (buffer-substring po-start-of-msgstr po-end-of-entry)
		   "msgstr \"\"\n")
		  (setq found t)))))
      (if found
	  (progn
	    (switch-to-buffer buffer)
	    (po-find-span-of-entry)
	    (if po-highlighting
		(progn
		  (goto-char po-start-of-entry)
		  (re-search-forward po-any-msgstr-regexp nil t)
		  (let ((end (1- (match-end 0))))
		    (goto-char (match-beginning 0))
		    (re-search-forward "msgstr +" nil t)
		    ;; FIXME:
		    (po-highlight (point) end))))
	    (goto-char po-start-of-msgid))
	(goto-char start)
	(po-find-span-of-entry)
	(select-buffer current))
      found)))

(defun po-cycle-auxiliary ()
  "Select the next auxiliary file having an entry with same `msgid'."
  (interactive)
  (po-find-span-of-entry)
  (if po-auxiliary-list
      (let ((string (buffer-substring po-start-of-msgid po-start-of-msgstr))
	    (cursor po-auxiliary-cursor)
	    found name)
	(while (and (not found) cursor)
	  (setq name (car (car cursor)))
	  (if (and (not (string-equal buffer-file-name name))
		   (po-seek-equivalent-translation name string))
	      (setq found t
		    po-auxiliary-cursor cursor))
	  (setq cursor (cdr cursor)))
	(setq cursor po-auxiliary-list)
	(while (and (not found) cursor)
	  (setq name (car (car cursor)))
	  (if (and (not (string-equal buffer-file-name name))
		   (po-seek-equivalent-translation name string))
	      (setq found t
		    po-auxiliary-cursor cursor))
	  (setq cursor (cdr cursor)))
	(or found (message (_"No other translation found")))
        found)))

(defun po-subedit-cycle-auxiliary ()
  "Cycle auxiliary file, but from the translation edit buffer."
  (interactive)
  (if po-buffer-of-edited-entry
      (let ((buffer (current-buffer)))
	(pop-to-buffer po-buffer-of-edited-entry)
	(po-cycle-auxiliary)
	(pop-to-buffer buffer))
    (error (_"Not editing a PO file entry"))))

(defun po-select-auxiliary ()
  "Select one of the available auxiliary files and locate an equivalent
entry.  If an entry having the same `msgid' cannot be found, merely select
the file without moving its cursor."
  (interactive)
  (po-find-span-of-entry)
  (if po-auxiliary-list
      (let ((string (buffer-substring po-start-of-msgid po-start-of-msgstr))
	    (name (car (assoc (completing-read (_"Which auxiliary file? ")
					       po-auxiliary-list nil t)
			      po-auxiliary-list))))
	(po-consider-as-auxiliary)
	(or (po-seek-equivalent-translation name string)
	    (find-file name)))))

;;; Original program sources as context.

(defun po-show-source-path ()
  "Echo the current source search path in the message area."
  (if po-search-path
      (let ((cursor po-search-path)
	    string)
	(while cursor
	  (setq string (concat string (if string " ") (car (car cursor)))
		cursor (cdr cursor)))
	(message string))
    (message (_"Empty source path."))))

(defun po-consider-source-path (directory)
  "Add a given DIRECTORY, requested interactively, to the source search path."
  (interactive "DDirectory for search path: ")
  (setq po-search-path (cons (list (if (string-match "/$" directory)
					 directory
				       (concat directory "/")))
			     po-search-path))
  (setq po-reference-check 0)
  (po-show-source-path))

(defun po-ignore-source-path ()
  "Delete a directory, selected with completion, from the source search path."
  (interactive)
  (setq po-search-path
	(delete (list (completing-read (_"Directory to remove? ")
				       po-search-path nil t))
		po-search-path))
  (setq po-reference-check 0)
  (po-show-source-path))

(defun po-ensure-source-references ()
  "Extract all references into a list, with paths resolved, if necessary."
  (po-find-span-of-entry)
  (if (= po-start-of-entry po-reference-check)
      ()
    (setq po-reference-alist nil)
    (save-excursion
      (goto-char po-start-of-entry)
      (if (re-search-forward "^#:" po-start-of-msgid t)
	  (while (looking-at "\\(\n#:\\)? *\\([^: ]+\\):\\([0-9]+\\)")
	    (goto-char (match-end 0))
	    (let* ((name (po-buffer-substring (match-beginning 2)
					      (match-end 2)))
		   (line (po-buffer-substring (match-beginning 3)
					      (match-end 3)))
		   (path po-search-path)
		   file)
	      (while (and (progn (setq file (concat (car (car path)) name))
				 (not (file-exists-p file)))
			  path)
		(setq path (cdr path)))
	      (if path
		  (setq po-reference-alist
			(cons (list (concat file ":" line)
				    file
				    (string-to-int line))
			      po-reference-alist)))))))
    (setq po-reference-alist (nreverse po-reference-alist)
	  po-reference-cursor po-reference-alist
	  po-reference-check po-start-of-entry)))

(defun po-show-source-context (triplet)
  "Show the source context given a TRIPLET which is (PROMPT FILE LINE)."
  (find-file-other-window (car (cdr triplet)))
  (goto-line (car (cdr (cdr triplet))))
  (other-window 1)
  (let ((maximum 0)
	position
	(cursor po-reference-alist))
    (while (not (eq triplet (car cursor)))
      (setq maximum (1+ maximum)
	    cursor (cdr cursor)))
    (setq position (1+ maximum)
	  po-reference-cursor cursor)
    (while cursor
      (setq maximum (1+ maximum)
	    cursor (cdr cursor)))
    (message (_"Displaying %d/%d: \"%s\"") position maximum (car triplet))))

(defun po-cycle-source-reference ()
  "Display some source context for the current entry.
If the command is repeated many times in a row, cycle through contexts."
  (interactive)
  (po-ensure-source-references)
  (if po-reference-cursor
      (po-show-source-context
       (car (if (eq last-command 'po-cycle-source-reference)
		(or (cdr po-reference-cursor) po-reference-alist)
	      po-reference-cursor)))
    (error (_"No resolved source references"))))

(defun po-select-source-reference ()
  "Select one of the available source contexts for the current entry."
  (interactive)
  (po-ensure-source-references)
  (if po-reference-alist
      (po-show-source-context
       (assoc
	(completing-read (_"Which source context? ") po-reference-alist nil t)
	po-reference-alist))
    (error (_"No resolved source references"))))

;;; Program sources strings though tags table.

;;; C mode.

;;; A few long string cases (submitted by Ben Pfaff).

;; #define string "This is a long string " \
;; "that is continued across several lines " \
;; "in a macro in order to test \\ quoting\\" \
;; "\\ with goofy strings.\\"

;; char *x = "This is just an ordinary string "
;; "continued across several lines without needing "
;; "to use \\ characters at end-of-line.";

;; char *y = "Here is a string continued across \
;; several lines in the manner that was sanctioned \
;; in K&R C compilers and still works today, \
;; even though the method used above is more esthetic.";

;;; End of long string cases.

(defun po-find-c-string (keywords)
  "Find the next C string, excluding those marked by any of KEYWORDS.
Returns (START . END) for the found string, or (nil . nil) if none found."
  (let (start end)
    (while (and (not start)
		(re-search-forward "\\([\"']\\|/\\*\\)" nil t))
      (cond ((= (preceding-char) ?*)
	     ;; Disregard comments.
	     (search-forward "*/"))

	    ((= (preceding-char) ?\')
	     ;; Disregard character constants.
	     (forward-char (if (= (following-char) ?\\) 3 2)))

	    ((save-excursion
	       (beginning-of-line)
	       (looking-at "^# *\\(include\\|line\\)"))
	     ;; Disregard lines being #include or #line directives.
	     (end-of-line))

	    ;; Else, find the end of the (possibly concatenated) string.
	    (t (setq start (1- (point))
		     end nil)
	       (while (not end)
		 (cond ((= (following-char) ?\")
			(if (looking-at "\"[ \t\n\\\\]*\"")
			    (goto-char (match-end 0))
			  (forward-char 1)
			  (setq end (point))))
		       ((= (following-char) ?\\) (forward-char 2))
		       (t (skip-chars-forward "^\"\\\\"))))

	       ;; Check before string for keyword and opening parenthesis.
	       (goto-char start)
	       (skip-chars-backward " \n\t")
	       (if (= (preceding-char) ?\()
		   (progn
		     (backward-char 1)
		     (skip-chars-backward " \n\t")
		     (let ((end-keyword (point)))
		       (skip-chars-backward "_A-Za-z0-9")
		       (if (member (list (po-buffer-substring (point)
							      end-keyword))
				   keywords)

			   ;; Disregard already marked strings.
			   (progn
			     (goto-char end)
			     (setq start nil
				   end nil)))))))))

    ;; Return the found string, if any.
    (cons start end)))

(defun po-mark-c-string (start end keyword)
  "Mark the C string, from START to END, with KEYWORD.
Return the adjusted value for END."
  (goto-char end)
  (insert ")")
  (goto-char start)
  (insert keyword)
  (if (not (string-equal keyword "_"))
      (progn (insert " ") (setq end (1+ end))))
  (insert "(")
  (+ end 2 (length keyword)))

;;; Emacs LISP mode.

(defun po-find-emacs-lisp-string (keywords)
  "Find the next Emacs LISP string, excluding those marked by any of KEYWORDS.
Returns (START . END) for the found string, or (nil . nil) if none found."
  (let (start end)
    (while (and (not start)
		(re-search-forward "[;\"?]" nil t))

      (cond ((= (preceding-char) ?\;)
	     ;; Disregard comments.
	     (search-forward "\n"))

	    ((= (preceding-char) ?\?)
	     ;; Disregard character constants.
	     (forward-char (if (= (following-char) ?\\) 2 1)))

	    ;; Else, find the end of the string.
	    (t (setq start (1- (point)))
	       (while (not (= (following-char) ?\"))
		 (skip-chars-forward "^\"\\\\")
		 (if (= (following-char) ?\\) (forward-char 2)))
	       (forward-char 1)
	       (setq end (point))

	       ;; Check before string for keyword and opening parenthesis.
	       (goto-char start)
	       (skip-chars-backward " \n\t")
	       (let ((end-keyword (point)))
		 (skip-chars-backward "-_A-Za-z0-9")
		 (if (and (= (preceding-char) ?\()
			  (member (list (po-buffer-substring (point)
							     end-keyword))
				  keywords))

		     ;; Disregard already marked strings.
		     (progn
		       (goto-char end)
		       (setq start nil
			     end nil)))))))

    ;; Return the found string, if any.
    (cons start end)))

(defun po-mark-emacs-lisp-string (start end keyword)
  "Mark the Emacs LISP string, from START to END, with KEYWORD.
Return the adjusted value for END."
  (goto-char end)
  (insert ")")
  (goto-char start)
  (insert "(" keyword)
  (if (not (string-equal keyword "_"))
      (progn (insert " ") (setq end (1+ end))))
  (+ end 2 (length keyword)))

;;; Processing generic to all programming modes.

(eval-and-compile
  (autoload 'visit-tags-table-buffer "etags"))

(defun po-tags-search (restart)
  "Find an unmarked translatable string through all files in tags table.
Disregard some simple strings which are most probably non-translatable.
With prefix argument, restart search at first file."
  (interactive "P")

  ;; Take care of restarting the search if necessary.
  (if restart (setq po-next-file-list nil))

  ;; Loop doing things until an interesting string is found.
  (let ((keywords po-keywords)
	found buffer start
	(end po-string-end))
    (while (not found)

      ;; Reinitialize the source file list if necessary.
      (if (not po-next-file-list)
	  (progn
	    (setq po-next-file-list
		  (save-excursion
		    (visit-tags-table-buffer)
		    (copy-sequence (tags-table-files))))
	    (or po-next-file-list (error (_"No files to process")))
	    (setq end nil)))

      ;; Try finding a string after resuming the search position.
      (message (_"Scanning %s...") (car po-next-file-list))
      (save-excursion
	(setq buffer (find-file-noselect (car po-next-file-list)))
	(set-buffer buffer)
	(goto-char (or end (point-min)))

	(cond ((string-equal mode-name "C")
	       (let ((pair (po-find-c-string keywords)))
		 (setq start (car pair)
		       end (cdr pair))))
	      ((string-equal mode-name "Emacs-Lisp")
	       (let ((pair (po-find-emacs-lisp-string keywords)))
		 (setq start (car pair)
		       end (cdr pair))))
	      (t (message (_"Unknown source mode for PO mode, skipping..."))
		 (setq start nil
		       end nil))))

      ;; Advance to next file if no string was found.
      (if (not start)
	  (progn
	    (setq po-next-file-list (cdr po-next-file-list))
	    (if po-next-file-list
		(setq end nil)
	      (setq po-string-end nil)
	      (and po-highlighting (po-dehighlight po-marking-overlay))
	      (error (_"All files processed"))))

	;; Push the string just found string into a work buffer for study.
	(po-with-temp-buffer
	 (insert (po-extract-unquoted buffer start end))
	 (goto-char (point-min))

	 ;; Do not disregard if at least three letters in a row.
	 (if (re-search-forward "[A-Za-z][A-Za-z][A-Za-z]" nil t)
	     (setq found t)

	   ;; Disregard if two letters, and more punctuations than letters.
	   (if (re-search-forward "[A-Za-z][A-Za-z]" nil t)
	       (let ((total (buffer-size)))
		 (goto-char (point-min))
		 (while (re-search-forward "[A-Za-z]+" nil t)
		   (replace-match "" t t))
		 (if (< (* 2 (buffer-size)) total)
		     (setq found t))))

	   ;; Disregard if single letters or no letters at all.
	   ))))

    ;; Ensure the string is being displayed.

    (if (one-window-p t) (split-window) (other-window 1))
    (switch-to-buffer buffer)
    (goto-char start)
    (or (pos-visible-in-window-p start) (recenter '(nil)))
    (if (pos-visible-in-window-p end)
	(goto-char end)
      (goto-char end)
      (recenter -1))
    (other-window 1)
    (and po-highlighting (po-highlight po-marking-overlay start end buffer))

    ;; Save the string for later commands.
    (message (_"Scanning %s...done") (car po-next-file-list))
    (setq po-string-start start
	  po-string-end end)))

(defun po-mark-found-string (keyword)
  "Mark last found string in program sources as translatable, using KEYWORD."
  (and po-highlighting (po-dehighlight po-marking-overlay))
  (let ((buffer (find-file-noselect (car po-next-file-list)))
	(start po-string-start)
	(end po-string-end)
	line string)

    ;; Mark string in program sources.
    (setq string (po-extract-unquoted buffer start end))
    (save-excursion
      (set-buffer buffer)
      (setq line (count-lines (point-min) start)
	    end (cond ((string-equal mode-name "C")
		       (po-mark-c-string start end keyword))
		      ((string-equal mode-name "Emacs-Lisp")
		       (po-mark-emacs-lisp-string start end keyword))
		      (t (error (_"Cannot mark in unknown source mode"))))))
    (setq po-string-end end)

    ;; Add PO file entry.
    (let ((buffer-read-only po-read-only))
      (goto-char (point-max))
      (insert "\n" (format "#: %s:%d\n" (car po-next-file-list) line))
      (save-excursion
	(insert (po-eval-requoted string "msgid" nil) "msgstr \"\"\n"))
      (setq po-untranslated-counter (1+ po-untranslated-counter))
      (po-update-mode-line-string))))

(defun po-mark-translatable ()
  "Mark last found string in program sources as translatable, using `_'."
  (interactive)
  (if (and po-string-start po-string-end)
      (progn
	(po-mark-found-string "_")
	(setq po-string-start nil))
    (error (_"No such string"))))

(defun po-select-mark-and-mark (arg)
  "Mark last found string in program sources as translatable, ask for keywoard,
using completion.  With prefix argument, just ask the name of a preferred
keyword for subsequent commands, also added to possible completions."
  (interactive "P")
  (if arg
      (let ((keyword (list (read-from-minibuffer (_"Keyword: ")))))
	(setq po-keywords (cons keyword (delete keyword po-keywords))))
    (if (and po-string-start po-string-end)
	(let* ((default (car (car po-keywords)))
	       (keyword (completing-read (format (_"Mark with keywoard? [%s] ")
						 default)
					 po-keywords nil t )))
	  (if (string-equal keyword "") (setq keyword default))
	  (po-mark-found-string keyword)
	  (setq po-string-start nil))
      (error (_"No such string")))))

;;; Miscellaneous features.

(defun po-help ()
  "Provide an help window for PO mode."
  (interactive)
  (po-with-temp-buffer
   (insert po-help-display-string)
   (goto-char (point-min))
   (save-window-excursion
     (switch-to-buffer (current-buffer))
     (delete-other-windows)
     (message (_"Type any character to continue"))
     (po-read-event))))

(defun po-undo ()
  "Undo the last change to the PO file."
  (interactive)
  (let ((buffer-read-only po-read-only))
    (undo))
  (po-compute-counters nil))

(defun po-statistics ()
  "Say how many entries in each category, and the current position."
  (interactive)
  (po-compute-counters t))

(defun po-validate ()
  "Use `msgfmt' for validating the current PO file contents."
  (interactive)

  ;; If modifications were done already, change the last revision date.
  (if (buffer-modified-p)
      (po-replace-revision-date))

  ;; This `let' is for protecting the previous value of compile-command.
  (let ((compile-command (concat po-msgfmt-program
				 " --statistics -c -v -o /dev/null "
				 buffer-file-name)))
    (compile compile-command)))

(defun po-guess-archive-name ()
  "Return the ideal file name for this PO file in the central archives."
  (let (start-of-header end-of-header package version team)
    (save-excursion
      ;; Find the PO file header entry.
      (goto-char (point-min))
      (re-search-forward po-any-msgstr-regexp)
      (setq start-of-header (match-beginning 0)
	    end-of-header (match-end 0))
      ;; Get the package and version.
      (goto-char start-of-header)
      (if (re-search-forward
	   "\n\"Project-Id-Version:\\( GNU\\)? \\([^\n ]+\\) \\([^\n ]+\\)\\\\n\"$"
	   end-of-header t)
	  (setq package (buffer-substring (match-beginning 2) (match-end 2))
		version (buffer-substring (match-beginning 3) (match-end 3))))
      (if (or (not package) (string-equal package "PACKAGE")
	      (not version) (string-equal version "VERSION"))
	  (error (_"Project-Id-Version field does not have a proper value")))
      ;; Get the team.
      (goto-char start-of-header)
      (if (re-search-forward "\n\"Language-Team:.*<\\(.*\\)@li.org>\\\\n\"$"
			     end-of-header t)
	  (setq team (buffer-substring (match-beginning 1) (match-end 1))))
      (if (or (not team) (string-equal team "LL"))
	  (error (_"Language-Team field does not have a proper value")))
      ;; Compose the name.
      (concat package "-" version "." team ".po"))))

(defun po-guess-team-address ()
  "Return the team address related to this PO file."
  (let (team)
    (save-excursion
      (goto-char (point-min))
      (re-search-forward po-any-msgstr-regexp)
      (goto-char (match-beginning 0))
      (if (re-search-forward
	   "\n\"Language-Team: +\\(.*<\\(.*\\)@li.org>\\)\\\\n\"$"
	   (match-end 0) t)
	  (setq team (buffer-substring (match-beginning 2) (match-end 2))))
      (if (or (not team) (string-equal team "LL"))
	  (error (_"Language-Team field does not have a proper value")))
      (buffer-substring (match-beginning 1) (match-end 1)))))

(defun po-send-mail ()
  "Start composing a letter, possibly including the current PO file."
  (interactive)
  (let* ((team-flag (y-or-n-p
		     (_"\
Write to your team? (`n' means writing to translation project) ")))
	 (address (if team-flag
		      (po-guess-team-address)
		    po-translation-project-address)))
    (if (not (y-or-n-p (_"Include current PO file? ")))
	(apply po-compose-mail-function address
	       (read-string (_"Subject? ")) nil)
      (if (buffer-modified-p)
	  (error (_"The file is not even saved, you did not validate it.")))
      (if (and (y-or-n-p (_"You validated (`V') this file, didn't you? "))
	       (or (zerop po-untranslated-counter)
		   (y-or-n-p
		    (format (_"%d entries are untranslated, include anyway? ")
			    po-untranslated-counter)))
	       (or (zerop po-fuzzy-counter)
		   (y-or-n-p
		    (format (_"%d entries are still fuzzy, include anyway? ")
			    po-fuzzy-counter)))
	       (or (zerop po-obsolete-counter)
		   (y-or-n-p
		    (format (_"%d entries are obsolete, include anyway? ")
			    po-obsolete-counter))))
	  (let ((buffer (current-buffer))
		(name (po-guess-archive-name))
		(transient-mark-mode nil))
	    (apply po-compose-mail-function address
		   (if team-flag
		       (read-string (_"Subject? "))
		     (format "TP-Robot %s" name))
		   nil)
	    (goto-char (point-min))
	    (re-search-forward
	     (concat "^" (regexp-quote mail-header-separator) "\n"))
	    (save-excursion
	      (insert-buffer buffer)
	      (shell-command-on-region
	       (region-beginning) (region-end)
	       (concat po-gzip-uuencode-command " " name ".gz") t))))))
  (message ""))

(defun po-confirm-and-quit ()
  "Confirm if quit should be attempted and then, do it.
This is a failsafe.  Confirmation is asked if only the real quit would not."
  (interactive)
  (if (po-check-all-pending-edits)
      (progn
	(if (or (buffer-modified-p)
		(> po-untranslated-counter 0)
		(> po-fuzzy-counter 0)
		(> po-obsolete-counter 0)
		(y-or-n-p (_"Really quit editing this PO file? ")))
	    (po-quit))
	(message ""))))

(defun po-quit ()
  "Save the PO file and kill buffer.  However, offer validation if
appropriate and ask confirmation if untranslated strings remain."
  (interactive)
  (if (po-check-all-pending-edits)
      (let ((quit t))

	;; Offer validation of newly modified entries.
	(if (and (buffer-modified-p)
		 (not (y-or-n-p
		       (_"File was modified; skip validation step? "))))
	    (progn
	      (message "")
	      (po-validate)
	      ;; If we knew that the validation was all successful, we should
	      ;; just quit.  But since we do not know yet, as the validation
	      ;; might be asynchronous with PO mode commands, the safest is to
	      ;; stay within PO mode, even if this implies that another
	      ;; `po-quit' command will be later required to exit for true.
	      (setq quit nil)))

	;; Offer to work on untranslated entries.
	(if (and quit
		 (or (> po-untranslated-counter 0)
		     (> po-fuzzy-counter 0)
		     (> po-obsolete-counter 0))
		 (not (y-or-n-p
		       (_"Unprocessed entries remain; quit anyway? "))))
	    (progn
	      (setq quit nil)
	      (po-auto-select-entry)))

	;; Clear message area.
	(message "")

	;; Or else, kill buffers and quit for true.
	(if quit
	    (progn
	      (and (buffer-modified-p) (po-replace-revision-date))
	      (save-buffer)
	      (kill-buffer (current-buffer)))))))

;;; po-mode.el ends here
