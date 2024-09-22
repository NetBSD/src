##
## Format
##
##   ACTION: [AUDIENCE:] COMMIT_MSG
##
## Description
##
##   ACTION is one of 'chg', 'fix', 'new', 'rem', 'sec'
##
##       Is WHAT the change is about.
##
##       'chg' is for refactor, small improvement, cosmetic changes...
##       'fix' is for bug fixes
##       'new' is for new features, big improvement
##       'rem' is for removed features
##       'sec' is for security fixes
##
##   Each MR title should have exactly one ACTION, and no non-merge commits should have these.
##
##   AUDIENCE is optional and can be 'dev', 'usr', 'pkg', 'test', 'doc'
##
##       Is WHO is concerned by the change.
##
##       Included in release notes and changelog:
##       'usr'  is for final users
##       'pkg'  is for packagers
##
##       Included in changelog:
##       'dev'  is for developers
##
##       Omitted from changelog:
##       'test' is for test-only changes
##       'doc'  is for doc-only changes
##
##   COMMIT_MSG is ... well ... the commit message itself.
##
##
## Examples:
##
##   If you want to add a release note, mark it for usr or pkg audience:
##
##       sec: usr: Fix CVE-YYYY-NNNN
##       rem: usr: Deprecate feature xyz
##       new: pkg: libngtcp2 is now required
##
##   If you want to mention it just in the changelog, add dev audience:
##
##       chg: dev: Refactor xyz
##       fix: dev: Fix a rare edge case of xyz
##       new: dev: Add new QTYPE
##
##   If you don't specify audience context, or use a custom one, it won't end
##   up in the changelog:
##
##       chg: doc: Tidy up the docs
##       fix: test: Fix a broken test
##       chg: Do some cleanup
##       sec: test: Add a test case for CVE-YYYY-NNNN
##
##   Please note that multi-line commit messages are supported; only the first
##   line will be considered as the "summary" of the commit message.  So tags
##   and other rules only apply to the summary.  The body of the commit
##   message will be displayed in the changelog without reformatting.


##
## ``ignore_regexps`` is a line of regexps
##
## Any commit having its full commit message matching any regexp listed here
## will be ignored and won't be reported in the changelog.
##
ignore_regexps = [
    r"^$",  ## ignore commits with empty messages
]


## ``section_regexps`` is a list of 2-tuples associating a string label and a
## list of regexp
##
## Commit messages will be classified in sections thanks to this. Section
## titles are the label, and a commit is classified under this section if any
## of the regexps associated is matching.
##
## Please note that ``section_regexps`` will only classify commits and won't
## make any changes to the contents. So you'll probably want to go check
## ``subject_process`` (or ``body_process``) to do some changes to the subject,
## whenever you are tweaking this variable.
##
section_regexps = [
    (
        "Security Fixes",
        [
            r"^(\[9\.[0-9]{2}(-S)?\])?\s*(\[[^]]*\]\s*)?sec:\s*(dev|usr|pkg)\s*:\s*([^\n]*)$",
        ],
    ),
    (
        "New Features",
        [
            r"^(\[9\.[0-9]{2}(-S)?\])?\s*(\[[^]]*\]\s*)?new:\s*(dev|usr|pkg)\s*:\s*([^\n]*)$",
        ],
    ),
    (
        "Removed Features",
        [
            r"^(\[9\.[0-9]{2}(-S)?\])?\s*(\[[^]]*\]\s*)?rem:\s*(dev|usr|pkg)\s*:\s*([^\n]*)$",
        ],
    ),
    (
        "Feature Changes",
        [
            r"^(\[9\.[0-9]{2}(-S)?\])?\s*(\[[^]]*\]\s*)?chg:\s*(dev|usr|pkg)\s*:\s*([^\n]*)$",
        ],
    ),
    (
        "Bug Fixes",
        [
            r"^(\[9\.[0-9]{2}(-S)?\])?\s*(\[[^]]*\]\s*)?fix:\s*(dev|usr|pkg)\s*:\s*([^\n]*)$",
        ],
    ),
]


## ``body_process`` is a callable
##
## This callable will be given the original body and result will
## be used in the changelog.
##
## Available constructs are:
##
##   - any python callable that take one txt argument and return txt argument.
##
##   - ReSub(pattern, replacement): will apply regexp substitution.
##
##   - Indent(chars="  "): will indent the text with the prefix
##     Please remember that template engines gets also to modify the text and
##     will usually indent themselves the text if needed.
##
##   - Wrap(regexp=r"\n\n"): re-wrap text in separate paragraph to fill 80-Columns
##
##   - noop: do nothing
##
##   - ucfirst: ensure the first letter is uppercase.
##     (usually used in the ``subject_process`` pipeline)
##
##   - final_dot: ensure text finishes with a dot
##     (usually used in the ``subject_process`` pipeline)
##
##   - strip: remove any spaces before or after the content of the string
##
##   - SetIfEmpty(msg="No commit message."): will set the text to
##     whatever given ``msg`` if the current text is empty.
##
## Additionally, you can `pipe` the provided filters, for instance:
# body_process = Wrap(regexp=r'\n(?=\w+\s*:)') | Indent(chars="  ")
# body_process = Wrap(regexp=r'\n(?=\w+\s*:)')
# body_process = noop
body_process = (
    ReSub(r"\n*See merge request isc-private/bind9!\d+", r"")
    | ReSub(r"https://gitlab.isc.org/isc-projects/bind9/-/issues/", r"#")
    | ReSub(r"https://gitlab.isc.org/isc-projects/bind9/-/merge_requests/", r"!")
    | ReSub(r"\n*Backport of [^\n]+", r"")
    | ReSub(r"\n*(Replaces|Supersedes)[^\n]+", r"")
    | ReSub(
        r"\n*(Closes|Fixes|Related|See):?\s*(isc-projects/bind9)?((#|!)\d+)",
        r" :gl:`\3`",
    )
    | ReSub(r"\n*Merge branch '[^']+' into [^\n]+", r"")
    | ReSub(r"\n*See merge request isc-projects/bind9(!\d+)", r" :gl:`\1`")
    | Wrap(regexp="\n\n", separator="\n\n")
    | strip
)

## ``subject_process`` is a callable
##
## This callable will be given the original subject and result will
## be used in the changelog.
##
## Available constructs are those listed in ``body_process`` doc.
subject_process = (
    strip
    | ReSub(
        r"^(\[9\.[0-9]{2}(-S)?\])?\s*(\[[^]]*\]\s*)?(chg|fix|new|rem|sec):\s*((dev|usr|pkg)\s*:\s*)?([^\n]*)$",
        r"\3\7",
    )
    | SetIfEmpty("No commit message.")
    | ucfirst
    | final_dot
)

## ``tag_filter_regexp`` is a regexp
##
## Tags that will be used for the changelog must match this regexp.
##
tag_filter_regexp = r"^v9\.[0-9]+\.[0-9]+(-S[0-9]+)?$"


## ``unreleased_version_label`` is a string or a callable that outputs a string
##
## This label will be used as the changelog Title of the last set of changes
## between last valid tag and HEAD if any.
unreleased_version_label = "(-dev)"


## ``include_commit_sha`` is a boolean to indicate whether the sha of the
## commit should be included in the change
include_commit_sha = True


## ``output_engine`` is a callable
##
## This will change the output format of the generated changelog file
##
## Available choices are:
##
##   - rest_py
##
##        Legacy pure python engine, outputs ReSTructured text.
##        This is the default.
##
##   - mustache(<template_name>)
##
##        Template name could be any of the available templates in
##        ``templates/mustache/*.tpl``.
##        Requires python package ``pystache``.
##        Examples:
##           - mustache("markdown")
##           - mustache("restructuredtext")
##
##   - makotemplate(<template_name>)
##
##        Template name could be any of the available templates in
##        ``templates/mako/*.tpl``.
##        Requires python package ``mako``.
##        Examples:
##           - makotemplate("restructuredtext")
##
output_engine = rest_py
# output_engine = mustache("restructuredtext")
# output_engine = mustache("markdown")
# output_engine = makotemplate("restructuredtext")


## ``include_merge`` is a boolean
##
## This option tells git-log whether to include merge commits in the log.
## The default is to include them.
include_merge = True


## ``log_encoding`` is a string identifier
##
## This option tells gitchangelog what encoding is outputed by ``git log``.
## The default is to be clever about it: it checks ``git config`` for
## ``i18n.logOutputEncoding``, and if not found will default to git's own
## default: ``utf-8``.
# log_encoding = 'utf-8'


## ``publish`` is a callable
##
## Sets what ``gitchangelog`` should do with the output generated by
## the output engine. ``publish`` is a callable taking one argument
## that is an interator on lines from the output engine.
##
## Some helper callable are provided:
##
## Available choices are:
##
##   - stdout
##
##        Outputs directly to standard output
##        (This is the default)
##
##   - FileInsertAtFirstRegexMatch(file, pattern, idx=lamda m: m.start())
##
##        Creates a callable that will parse given file for the given
##        regex pattern and will insert the output in the file.
##        ``idx`` is a callable that receive the matching object and
##        must return a integer index point where to insert the
##        the output in the file. Default is to return the position of
##        the start of the matched string.
##
##   - FileRegexSubst(file, pattern, replace, flags)
##
##        Apply a replace inplace in the given file. Your regex pattern must
##        take care of everything and might be more complex. Check the README
##        for a complete copy-pastable example.
##
# publish = FileInsertIntoFirstRegexMatch(
#     "CHANGELOG.rst",
#     r'/(?P<rev>[0-9]+\.[0-9]+(\.[0-9]+)?)\s+\([0-9]+-[0-9]{2}-[0-9]{2}\)\n--+\n/',
#     idx=lambda m: m.start(1)
# )
# publish = stdout
publish = FileInsertAtFirstRegexMatch(
    "doc/arm/changelog.rst",
    r"for changes relevant to them.\n\n",
    idx=lambda m: m.end(0),
)


## ``revs`` is a list of callable or a list of string
##
## callable will be called to resolve as strings and allow dynamical
## computation of these. The result will be used as revisions for
## gitchangelog (as if directly stated on the command line). This allows
## to filter exaclty which commits will be read by gitchangelog.
##
## To get a full documentation on the format of these strings, please
## refer to the ``git rev-list`` arguments. There are many examples.
##
## Using callables is especially useful, for instance, if you
## are using gitchangelog to generate incrementally your changelog.
##
## Some helpers are provided, you can use them::
##
##   - FileFirstRegexMatch(file, pattern): will return a callable that will
##     return the first string match for the given pattern in the given file.
##     If you use named sub-patterns in your regex pattern, it'll output only
##     the string matching the regex pattern named "rev".
##
##   - Caret(rev): will return the rev prefixed by a "^", which is a
##     way to remove the given revision and all its ancestor.
##
## Please note that if you provide a rev-list on the command line, it'll
## replace this value (which will then be ignored).
##
## If empty, then ``gitchangelog`` will act as it had to generate a full
## changelog.
##
## The default is to use all commits to make the changelog.
# revs = ["^1.0.3", ]
# revs = [
#    Caret(
#        FileFirstRegexMatch(
#            "CHANGELOG.rst",
#            r"(?P<rev>[0-9]+\.[0-9]+(\.[0-9]+)?)\s+\([0-9]+-[0-9]{2}-[0-9]{2}\)\n--+\n")),
#    "HEAD"
# ]
revs = []
