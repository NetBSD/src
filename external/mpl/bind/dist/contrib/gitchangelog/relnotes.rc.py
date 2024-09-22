ignore_regexps = [
    r"^$",  ## ignore commits with empty messages
]

section_regexps = [
    (
        "Security Fixes",
        [
            r"^(\[9\.[0-9]{2}(-S)?\])?\s*(\[[^]]*\]\s*)?sec:\s*(usr|pkg)\s*:\s*([^\n]*)$",
        ],
    ),
    (
        "New Features",
        [
            r"^(\[9\.[0-9]{2}(-S)?\])?\s*(\[[^]]*\]\s*)?new:\s*(usr|pkg)\s*:\s*([^\n]*)$",
        ],
    ),
    (
        "Removed Features",
        [
            r"^(\[9\.[0-9]{2}(-S)?\])?\s*(\[[^]]*\]\s*)?rem:\s*(usr|pkg)\s*:\s*([^\n]*)$",
        ],
    ),
    (
        "Feature Changes",
        [
            r"^(\[9\.[0-9]{2}(-S)?\])?\s*(\[[^]]*\]\s*)?chg:\s*(usr|pkg)\s*:\s*([^\n]*)$",
        ],
    ),
    (
        "Bug Fixes",
        [
            r"^(\[9\.[0-9]{2}(-S)?\])?\s*(\[[^]]*\]\s*)?fix:\s*(usr|pkg)\s*:\s*([^\n]*)$",
        ],
    ),
]

body_process = (
    ReSub(r"\n*See merge request isc-private/bind9!\d+", r"")
    | ReSub(r"https://gitlab.isc.org/isc-projects/bind9/-/issues/", r"#")
    | ReSub(r"https://gitlab.isc.org/isc-projects/bind9/-/merge_requests/", r"!")
    | ReSub(r"\n*Backport of [^\n]+", r"")
    | ReSub(r"\n*(Replaces|Supercedes)[^\n]+", r"")
    | ReSub(
        r"\n*(Closes|Fixes|Related|See):?\s*(isc-projects/bind9)?((#|!)\d+)",
        r" :gl:`\3`",
    )
    | ReSub(r"\n*Merge branch '[^']+' into [^\n]+", r"")
    | ReSub(r"\n*See merge request isc-projects/bind9(!\d+)", r"")
    | Wrap(regexp="\n\n", separator="\n\n")
    | strip
)

subject_process = (
    strip
    | ReSub(
        r"^(\[9\.[0-9]{2}(-S)?\])?\s*(\[[^]]*\]\s*)?(chg|fix|new|rem|sec):\s*((usr|pkg)\s*:\s*)?([^\n]*)$",
        r"\3\7",
    )
    | SetIfEmpty("No commit message.")
    | ucfirst
    | final_dot
)

tag_filter_regexp = r"^v9\.[0-9]+\.[0-9]+(-S[0-9]+)?$"

unreleased_version_label = "(-dev)"

include_commit_sha = False

output_engine = rest_py

include_merge = True

publish = stdout

revs = []
