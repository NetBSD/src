# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

from pylint.checkers import BaseChecker

import astroid


class DnsExplicitImportsChecker(BaseChecker):
    name = "dns-explicit-imports"

    msgs = {
        "W9001": (
            "Bare 'import dns' is discouraged; import required submodules explicitly",
            "dns-bare-import",
            "Emitted when the package root 'dns' is imported directly.",
        ),
        "W9002": (
            "Missing explicit import for '%s' (add `import %s`)",
            "dns-missing-submodule-import",
            "Emitted when code references dns.<...> but the corresponding module prefix "
            "was not imported with `import dns.<...>`.",
        ),
        "W9003": (
            "Unused explicit import for '%s' (remove `import %s`)",
            "dns-unused-submodule-import",
            "Emitted when a dns.<...> module is imported explicitly but not used.",
        ),
    }

    def __init__(self, linter=None):
        super().__init__(linter)
        self._imported = {}
        self._imported_aliases = set()
        self._required = {}

    def visit_module(self, node):  # pylint: disable=unused-argument
        self._imported = {}
        self._imported_aliases = set()
        self._required = {}

    def leave_module(self, node):  # pylint: disable=unused-argument
        for mod, use_node in sorted(self._required.items()):
            if mod in self._imported:
                continue
            prefix = mod + "."
            if any(name.startswith(prefix) for name in self._imported):
                continue
            self.add_message(
                "dns-missing-submodule-import",
                node=use_node,
                args=(mod, mod),
            )
        for mod, import_node in sorted(self._imported.items()):
            if mod in self._imported_aliases:
                continue
            if any(
                name == mod or name.startswith(mod + ".") for name in self._required
            ):
                continue
            self.add_message(
                "dns-unused-submodule-import",
                node=import_node,
                args=(mod, mod),
            )

    def visit_import(self, node):
        for name, _asname in node.names:
            if name == "dns":
                self.add_message("dns-bare-import", node=node)
                continue
            if name.startswith("dns."):
                self._imported.setdefault(name, node)
                if _asname:
                    self._imported_aliases.add(name)

    def visit_importfrom(self, node):  # pylint: disable=unused-argument
        return

    def visit_attribute(self, node):
        parent = node.parent
        # For `dns.a.b.c`, astroid visits intermediate attributes too.
        # Process only the rightmost node to avoid duplicate bookkeeping.
        if isinstance(parent, astroid.nodes.Attribute) and parent.expr is node:
            return

        mod = self._dns_module_for_attribute(node)
        if mod is None:
            return

        self._required.setdefault(mod, node)

    @staticmethod
    def _dns_attribute_nodes(node):
        """
        Return the chain of Attribute nodes as a list.

        For `dns.a.b.c`, return the list of Attribute nodes for `dns.a`, `dns.a.b`, and `dns.a.b.c`.

        Return None if the chain is not rooted in `dns`.
        """

        if not isinstance(node, astroid.nodes.Attribute):
            return None

        nodes = []
        expr = node
        while isinstance(expr, astroid.nodes.Attribute):
            nodes.append(expr)
            expr = expr.expr

        if not isinstance(expr, astroid.nodes.Name) or expr.name != "dns":
            return None

        return list(reversed(nodes))

    @classmethod
    def _dns_module_for_attribute(cls, node):
        """
        For dns.a.b.c, return the longest dns.a.b... prefix that is likely to be a module,
        or None if the chain is not rooted in dns.
        """
        last_module = None
        chain_nodes = cls._dns_attribute_nodes(node)
        if chain_nodes is None:
            return None

        full = "dns." + ".".join(part.attrname for part in chain_nodes)
        # Prefer inferred module names to avoid treating classes/constants as
        # modules (e.g. `dns.name.NameRelation` should resolve to `dns.name`).
        for chain_node in chain_nodes:
            inferred = cls._infer_module_name(chain_node)
            if inferred is not None and full.startswith(inferred):
                last_module = inferred
        if last_module is not None:
            return last_module

        # Fallback when inference is unavailable: assume the terminal segment
        # is not a module symbol and require the parent path.
        parts = full.split(".")
        if len(parts) <= 2:
            return full
        return ".".join(parts[:-1])

    @staticmethod
    def _infer_module_name(node):
        """Infer `dns.<module>` for a node; return None if inference is unsure."""
        try:
            for inferred in node.infer():
                if inferred is astroid.util.Uninferable:
                    continue
                # Inference can return either a Module node directly or another
                # symbol rooted in a module; normalize both to module name.
                module = (
                    inferred
                    if isinstance(inferred, astroid.nodes.Module)
                    else inferred.root()
                )
                name = module.name
                if name.startswith("dns."):
                    return name
        # Inference can fail for dynamic/partial code; fall back gracefully.
        except astroid.AstroidError:
            pass
        return None


def register(linter):
    linter.register_checker(DnsExplicitImportsChecker(linter))
