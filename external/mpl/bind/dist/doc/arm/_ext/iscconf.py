############################################################################
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.
############################################################################

"""
Sphinx domains for ISC configuration files.

Use setup() to install new Sphinx domains for ISC configuration files.

This extension is based on combination of two Sphinx extension tutorials:
https://www.sphinx-doc.org/en/master/development/tutorials/todo.html
https://www.sphinx-doc.org/en/master/development/tutorials/recipe.html
"""

from collections import namedtuple

from docutils import nodes
from docutils.parsers.rst import Directive, directives
from sphinx import addnodes
from sphinx.directives import ObjectDescription
from sphinx.domains import Domain
from sphinx.roles import XRefRole
from sphinx.util import logging
from sphinx.util.nodes import make_refnode

import checkgrammar

logger = logging.getLogger(__name__)


def split_csv(argument, required):
    argument = argument or ""
    outlist = list(filter(len, (s.strip() for s in argument.split(","))))
    if required and not outlist:
        raise ValueError(
            "a non-empty list required; provide at least one value or remove"
            " this option"
        )
    if not len(outlist) == len(set(outlist)):
        raise ValueError("duplicate value detected")
    return outlist


def domain_factory(domainname, domainlabel, todolist, grammar):
    """
    Return parametrized Sphinx domain object.
    @param domainname Name used when referencing domain in .rst: e.g. namedconf
    @param confname Humand-readable name for texts, e.g. named.conf
    @param todolist A placeholder object which must be pickable.
                    See StatementListDirective.
    """

    class StatementListDirective(Directive):
        """A custom directive to generate list of statements.
        It only installs placeholder which is later replaced by
        process_statementlist_nodes() callback.
        """

        option_spec = {
            "filter_blocks": lambda arg: split_csv(arg, required=True),
            "filter_tags": lambda arg: split_csv(arg, required=True),
        }

        def run(self):
            placeholder = todolist("")
            placeholder["isc_filter_tags"] = self.options.get("filter_tags", [])
            placeholder["isc_filter_blocks"] = self.options.get("filter_blocks", [])
            return [placeholder]

    class ISCConfDomain(Domain):
        """
        Custom Sphinx domain for ISC config.
        Provides .. statement:: directive to define config statement and
        .. statementlist:: to generate summary tables.
        :ref:`statementname` works as usual.

        See https://www.sphinx-doc.org/en/master/extdev/domainapi.html
        """

        class StatementDirective(ObjectDescription):
            """
            A custom directive that describes a statement,
            e.g. max-cache-size.
            """

            has_content = True
            required_arguments = 1
            option_spec = {
                "tags": lambda arg: split_csv(arg, required=False),
                # one-sentece description for use in summary tables
                "short": directives.unchanged_required,
                "suppress_grammar": directives.flag,
            }

            @property
            def isc_name(self):
                names = self.get_signatures()
                if len(names) != 1:
                    raise NotImplementedError(
                        "statements with more than one name are not supported", names
                    )
                return names[0]

            def handle_signature(self, sig, signode):
                signode += addnodes.desc_name(text=sig)
                return sig

            def add_target_and_index(self, _name_cls, sig, signode):
                signode["ids"].append(domainname + "-statement-" + sig)

                iscconf = self.env.get_domain(domainname)
                iscconf.add_statement(
                    sig, self.isc_tags, self.isc_short, self.isc_short_node, self.lineno
                )

            @property
            def isc_tags(self):
                return self.options.get("tags", [])

            @property
            def isc_short(self):
                return self.options.get("short", "")

            @property
            def isc_short_node(self):
                """Short description parsed from rst to docutils node"""
                return self.parse_nested_str(self.isc_short)

            def format_path(self, path):
                assert path[0] == "_top"
                if len(path) == 1:
                    return "topmost"
                return ".".join(path[1:])

            def format_paths(self, paths):
                zone_types = set()
                nozone_paths = []
                for path in paths:
                    try:
                        zone_idx = path.index("zone")
                        zone_type_txt = path[zone_idx + 1]
                        if zone_type_txt.startswith("type "):
                            zone_types.add(zone_type_txt[len("type ") :])
                        else:
                            assert zone_type_txt == "in-view"
                            zone_types.add(zone_type_txt)
                    except (ValueError, IndexError):
                        nozone_paths.append(path)
                condensed_paths = nozone_paths[:]
                if zone_types:
                    condensed_paths.append(
                        ("_top", "zone (" + ", ".join(sorted(zone_types)) + ")")
                    )
                condensed_paths = sorted(condensed_paths, key=len)
                return list(self.format_path(path) for path in condensed_paths)

            def format_blocks(self, grammar_blocks):
                """Generate node with list of all allowed blocks"""
                blocks = nodes.paragraph()
                blocks += nodes.strong(text="Blocks: ")
                blocks += nodes.Text(", ".join(self.format_paths(grammar_blocks)))
                return blocks

            def format_grammar(self, list_blocks, grammar_grp):
                """
                Generate grammar description node, optionally with list of
                blocks accepting this particular grammar.
                Example: Grammar (block1, block2): grammar;
                """
                grammarnode = nodes.paragraph()
                if list_blocks:
                    separator = " "
                    paths = ", ".join(
                        self.format_paths(variant.path for variant in grammar_grp)
                    )
                else:
                    separator = ""
                    paths = ""
                subgrammar = grammar_grp[0].subgrammar
                subgrammar_txt = checkgrammar.pformat_grammar(subgrammar).strip()
                grammar_txt = subgrammar.get("_pprint_name", self.isc_name)
                if subgrammar_txt != ";":
                    grammar_txt += " "
                grammar_txt += subgrammar_txt
                if "\n" in grammar_txt.strip():
                    nodetype = nodes.literal_block
                else:
                    nodetype = nodes.literal
                grammarnode += nodes.strong(text=f"Grammar{separator}{paths}: ")
                grammarnode += nodetype(text=grammar_txt)
                return grammarnode

            def format_warnings(self, flags):
                """Return node with a warning box about deprecated and
                experimental options"""
                warn = nodes.warning()
                if "deprecated" in flags:
                    warn += nodes.paragraph(
                        text=(
                            "This option is deprecated and will be removed in a future"
                            " version of BIND."
                        )
                    )
                if "experimental" in flags:
                    warn += nodes.paragraph(
                        text="This option is experimental and subject to change."
                    )
                return warn

            def parse_nested_str(self, instr):
                """Parse string as nested rst syntax and produce a node"""
                raw = nodes.paragraph(text=instr)
                parsed = nodes.paragraph()
                self.state.nested_parse(raw, self.content_offset, parsed)
                return parsed

            def transform_content(self, content_node: addnodes.desc_content) -> None:
                """autogenerate content from structured data"""
                self.workaround_transform_content = True
                if self.isc_short:
                    content_node.insert(0, self.isc_short_node)
                if self.isc_tags:
                    tags = nodes.paragraph()
                    tags += nodes.strong(text="Tags: ")
                    tags += nodes.Text(", ".join(self.isc_tags))
                    content_node.insert(0, tags)

                iscconf = self.env.get_domain(domainname)

                name = self.isc_name
                if name not in iscconf.statement_blocks:
                    return  # not defined in grammar, nothing to render

                blocks = self.format_blocks(iscconf.statement_blocks[name])
                content_node.insert(0, blocks)

                grammars = iscconf.statement_grammar_groups[name]
                multi_grammar = len(grammars) > 1
                union_flags = set()
                for grammar_grp in grammars:
                    for one_grammar_dict in grammar_grp:
                        union_flags = union_flags.union(
                            set(one_grammar_dict.subgrammar.get("_flags", []))
                        )
                    if "suppress_grammar" in self.options:
                        continue
                    grammarnode = self.format_grammar(multi_grammar, grammar_grp)
                    content_node.insert(0, grammarnode)

                warn = self.format_warnings(union_flags)
                if len(warn):
                    content_node.insert(0, warn)

            def __init__(self, *args, **kwargs):
                """Compability with Sphinx < 3.0.0"""
                self.workaround_transform_content = False
                super().__init__(*args, **kwargs)

            def run(self):
                """Compability with Sphinx < 3.0.0"""
                nodelist = super().run()
                if not self.workaround_transform_content:
                    # get access to "contentnode" created inside super.run()
                    self.transform_content(nodelist[1][-1])
                return nodelist

        name = domainname
        label = domainlabel

        directives = {
            "statement": StatementDirective,
            "statementlist": StatementListDirective,
        }

        roles = {"ref": XRefRole(warn_dangling=True)}
        initial_data = {
            # name -> {"tags": [list of tags], ...}; see add_statement()
            "statements": {},
        }

        indices = {}  # no custom indicies

        def __init__(self, *args, **kwargs):
            super().__init__(*args, **kwargs)
            self.grammar = grammar
            self.statement_blocks = checkgrammar.statement2block(grammar, ["_top"])
            self.statement_grammar_groups = checkgrammar.diff_statements(
                self.grammar, self.statement_blocks
            )

        def get_objects(self):
            """
            Sphinx API:
            Iterable of Sphinx object descriptions (tuples defined in the API).
            """
            for obj in self.data["statements"].values():
                yield tuple(
                    obj[key]
                    for key in [
                        "fullname",
                        "signature",
                        "label",
                        "docname",
                        "anchor",
                        "priority",
                    ]
                )

        def resolve_xref(self, env, fromdocname, builder, typ, target, node, contnode):
            """
            Sphinx API:
            Resolve the pending_xref *node* with the given typ and target.
            """
            try:
                obj = self.data["statements"][self.get_statement_name(target)]
            except KeyError:
                return None

            refnode = make_refnode(
                builder,
                fromdocname,
                obj["docname"],
                obj["anchor"],
                contnode,
                obj["anchor"],
            )
            return refnode

        def resolve_any_xref(self, env, fromdocname, builder, target, node, contnode):
            """
            Sphinx API:
            Raising NotImplementedError uses fall-back bassed on resolve_xref.
            """
            raise NotImplementedError

        @staticmethod
        def log_statement_overlap(new, old):
            assert new["fullname"] == old["fullname"]
            logger.warning(
                "duplicite detected! %s previously defined at %s:%d",
                new["fullname"],
                old["filename"],
                old["lineno"],
                location=(new["docname"], new["lineno"]),
            )

        def get_statement_name(self, signature):
            return f"{domainname}.statement.{signature}"

        def add_statement(self, signature, tags, short, short_node, lineno):
            """
            Add a new statement to the domain data structures.
            No visible effect.
            """
            name = self.get_statement_name(signature)
            anchor = f"{domainname}-statement-{signature}"

            new = {
                "tags": tags,
                "short": short,
                "short_node": short_node,
                "filename": self.env.doc2path(self.env.docname),
                "lineno": lineno,
                # Sphinx API
                "fullname": name,  # internal name
                "signature": signature,  # display name
                "label": domainlabel + " statement",  # description for index
                "docname": self.env.docname,
                "anchor": anchor,
                "priority": 1,  # search priority
            }

            if name in self.data["statements"]:
                self.log_statement_overlap(new, self.data["statements"][name])
            self.data["statements"][name] = new

        def clear_doc(self, docname):
            """
            Sphinx API: like env-purge-doc event, but in a domain.

            Remove traces of a document in the domain-specific inventories.
            """
            self.data["statements"] = dict(
                {
                    key: obj
                    for key, obj in self.data["statements"].items()
                    if obj["docname"] != docname
                }
            )

        def merge_domaindata(self, docnames, otherdata):
            """Sphinx API: Merge in data regarding *docnames* from a different
            domaindata inventory (coming from a subprocess in parallel builds).

            @param otherdata is self.data equivalent from another process
            """
            old = self.data["statements"]
            new = otherdata["statements"]
            for name in set(old).intersection(set(new)):
                self.log_statement_overlap(new[name], old[name])
            old.update(new)

        def check_consistency(self):
            """Sphinx API"""
            defined_statements = set(
                obj["signature"] for obj in self.data["statements"].values()
            )
            statements_in_grammar = set(self.statement_blocks)
            missing_statement_sigs = statements_in_grammar.difference(
                defined_statements
            )
            for missing in missing_statement_sigs:
                grammars = self.statement_grammar_groups[missing]
                if len(grammars) == 1:
                    flags = grammars[0][0].subgrammar.get("_flags", [])
                    if ("obsolete" in flags) or ("test only" in flags):
                        continue

                logger.warning(
                    "statement %s is defined in %s grammar but is not described"
                    " using .. statement:: directive",
                    missing,
                    domainlabel,
                )

            extra_statement_sigs = defined_statements.difference(statements_in_grammar)
            for extra in extra_statement_sigs:
                fullname = self.get_statement_name(extra)
                desc = self.data["statements"][fullname]
                logger.warning(
                    ".. statement:: %s found but matching definition in %s grammar is"
                    " missing",
                    extra,
                    domainlabel,
                    location=(desc["docname"], desc["lineno"]),
                )

        @classmethod
        def process_statementlist_nodes(cls, app, doctree):
            """
            Replace todolist objects (placed into document using
            .. statementlist::) with automatically generated table
            of statements.
            """

            def gen_replacement_table(acceptable_blocks, acceptable_tags):
                table_header = [
                    TableColumn("ref", "Statement"),
                    TableColumn("short_node", "Description"),
                ]
                tag_header = []
                if len(acceptable_tags) != 1:
                    # tags column only if tag filter is not applied
                    tag_header = [
                        TableColumn("tags_txt", "Tags"),
                    ]

                table_b = DictToDocutilsTableBuilder(table_header + tag_header)
                table_b.append_iterable(
                    sorted(
                        filter(
                            lambda item: (
                                (
                                    not acceptable_tags
                                    or set(item["tags"]).intersection(acceptable_tags)
                                )
                                and (
                                    not acceptable_blocks
                                    or set(item["block_names"]).intersection(
                                        acceptable_blocks
                                    )
                                )
                            ),
                            iscconf.list_all(),
                        ),
                        key=lambda x: x["fullname"],
                    )
                )
                return table_b.get_docutils()

            env = app.builder.env
            iscconf = env.get_domain(cls.name)

            for node in doctree.traverse(todolist):
                acceptable_tags = node["isc_filter_tags"]
                acceptable_blocks = node["isc_filter_blocks"]
                node.replace_self(
                    gen_replacement_table(acceptable_blocks, acceptable_tags)
                )

        def list_all(self):
            for statement in self.data["statements"].values():
                sig = statement["signature"]
                block_names = set(
                    path[-1] for path in self.statement_blocks.get(sig, [])
                )
                tags_txt = ", ".join(statement["tags"])

                refpara = nodes.inline()
                refnode = addnodes.pending_xref(
                    sig,
                    reftype="statement",
                    refdomain=domainname,
                    reftarget=sig,
                    refwarn=True,
                )
                refnode += nodes.Text(sig)
                refpara += refnode

                copy = statement.copy()
                copy["block_names"] = block_names
                copy["ref"] = refpara
                copy["tags_txt"] = tags_txt
                yield copy

    return ISCConfDomain


# source dict key: human description
TableColumn = namedtuple("TableColumn", ["dictkey", "description"])


class DictToDocutilsTableBuilder:
    """generate docutils table"""

    def __init__(self, header):
        """@param header: [ordered list of TableColumn]s"""
        self.header = header
        self.table = nodes.table()
        self.table["classes"] += ["colwidths-auto"]
        self.returned = False
        # inner nodes of the table
        self.tgroup = nodes.tgroup(cols=len(self.header))
        for _ in range(len(self.header)):
            # ignored because of colwidths-auto, but must be present
            colspec = nodes.colspec(colwidth=1)
            self.tgroup.append(colspec)
        self.table += self.tgroup
        self._gen_header()

        self.tbody = nodes.tbody()
        self.tgroup += self.tbody

    def _gen_header(self):
        thead = nodes.thead()

        row = nodes.row()
        for column in self.header:
            entry = nodes.entry()
            entry += nodes.paragraph(text=column.description)
            row += entry

        thead.append(row)
        self.tgroup += thead

    def append_iterable(self, objects):
        """Append rows for each object (dict), ir order.
        Extract column values from keys listed in self.header."""
        for obj in objects:
            row = nodes.row()
            for column in self.header:
                entry = nodes.entry()
                value = obj[column.dictkey]
                if isinstance(value, str):
                    value = nodes.paragraph(text=value)
                else:
                    value = value.deepcopy()
                entry += value
                row += entry
            self.tbody.append(row)

    def get_docutils(self):
        # guard against table reuse - that's most likely an error
        assert not self.returned
        self.returned = True
        return self.table


def setup(app, domainname, confname, docutilsplaceholder, grammar):
    """
    Install new parametrized Sphinx domain.
    """

    Conf = domain_factory(domainname, confname, docutilsplaceholder, grammar)
    app.add_domain(Conf)
    app.connect("doctree-read", Conf.process_statementlist_nodes)

    return {
        "version": "0.1",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
