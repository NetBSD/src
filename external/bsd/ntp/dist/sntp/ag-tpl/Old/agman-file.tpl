[+: -*- Mode: nroff -*-

  AutoGen5 template man

## agman-file.tpl -- Template for file man pages
##
## Time-stamp:      "2011-11-18 07:48:17 bkorb"
##
##  This file is part of AutoOpts, a companion to AutoGen.
##  AutoOpts is free software.
##  Copyright (c) 1992-2012 Bruce Korb - all rights reserved
##
##  AutoOpts is available under any one of two licenses.  The license
##  in use must be one of these two and the choice is under the control
##  of the user of the license.
##
##   The GNU Lesser General Public License, version 3 or later
##      See the files "COPYING.lgplv3" and "COPYING.gplv3"
##
##   The Modified Berkeley Software Distribution License
##      See the file "COPYING.mbsd"
##
##  These files have the following md5sums:
##
##  43b91e8ca915626ed3818ffb1b71248b COPYING.gplv3
##  06a1a2e4760c90ea5e1dad8dfaac4d39 COPYING.lgplv3
##  66a5cedaf62c4b2637025f049f9b826f COPYING.mbsd

# Produce a man page for section 1, 5 or 8 commands.
# Which is selected via:  -DMAN_SECTION=n
# passed to the autogen invocation.  "n" may have a suffix, if desired.
#
:+][+:

(define head-line (lambda()
        (sprintf ".TH %s %s \"%s\" \"%s\" \"%s\"\n.\\\"\n"
                (get "prog-name") man-sect
        (shell "date '+%d %b %Y'") package-text section-name) ))

(define man-page #t)

:+][+:

INCLUDE "cmd-doc.tlib"

:+]
.\"
.SH NAME
[+: prog-name :+] \- [+: prog-title :+]
[+:

(out-push-new)            :+][+:

INVOKE build-doc          :+][+:

  (shell (string-append
    "fn='" (find-file "mdoc2man") "'\n"
    "test -f ${fn} || die mdoc2man not found from $PWD\n"
    "${fn} <<\\_EndOfMdoc_ || die ${fn} failed in $PWD\n"
    (out-pop #t)
    "\n_EndOfMdoc_" ))

:+][+:

(out-move (string-append (get "prog-name") "."
          man-sect))      :+][+: #

.\" = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
.\"  S Y N O P S I S
.\" = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = :+][+:

DEFINE mk-synopsis                          :+][+:
  (out-push-new file-name)                 \:+]
.SH SYNOPSIS
.B [+: file-path :+]
.PP [+:

(if (exist? "explain")
    (string-append "\n.PP\n"
      (join "\n.PP\n" (stack "explain"))) ) :+][+:

(out-pop)                                   :+][+:

ENDDEF mk-synopsis

agman-file.tpl ends here   :+]
