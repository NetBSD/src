#!/usr/bin/python3

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

from dataclasses import dataclass, field
from pathlib import Path
from re import compile as Re
from typing import Any

import re

import jinja2

from .log import debug
from .vars import ALL

NS_DIR_RE = Re(r"^(a?ns([0-9]+))/")


class TemplateEngine:
    """
    Engine for rendering jinja2 templates in system test directories.
    """

    def __init__(self, directory: str | Path, env_vars=ALL):
        """
        Initialize the template engine for `directory`, optionally overriding
        the `env_vars` that will be used when rendering the templates (defaults
        to the environment variables set by the pytest runner).
        """
        self.directory = Path(directory)
        self.env_vars = dict(env_vars)
        self.j2env = jinja2.Environment(
            loader=jinja2.ChoiceLoader(
                [
                    jinja2.FileSystemLoader(self.directory),
                    jinja2.PrefixLoader(
                        {
                            "_common": jinja2.FileSystemLoader(
                                Path(ALL["srcdir"]) / "_common"
                            ),
                        }
                    ),
                ]
            ),
            undefined=jinja2.StrictUndefined,
            variable_start_string="@",
            variable_end_string="@",
            trim_blocks=True,
            keep_trailing_newline=True,
        )
        # allow instantiating the template dataclasses in jinja2 templates when
        # using {% set %}
        self.j2env.globals["Nameserver"] = Nameserver
        self.j2env.globals["TrustAnchor"] = TrustAnchor
        self.j2env.globals["Zone"] = Zone

    def render(
        self,
        output: str,
        data: dict[str, Any] | None = None,
        template: str | None = None,
    ) -> None:
        """
        Render `output` file from jinja `template` and fill in the `data`. The
        `template` defaults to *.j2.manual or *.j2 file. The environment
        variables which the engine was initialized with are also filled in. In
        case of a variable name clash, `data` has precedence.
        """
        available = self.j2env.list_templates()
        if template is None:
            template = f"{output}.j2.manual"
            if template not in available:
                template = f"{output}.j2"
        if template not in available:
            raise RuntimeError(f'No jinja2 template found for "{output}"')

        if data is None:
            data = {**self.env_vars}
        else:
            data = {**self.env_vars, **data}

        # directory-specific "ns" var
        assert "ns" not in data, '"ns" variable is reserved for nameserver data'
        match = NS_DIR_RE.search(output)
        if match:
            data["ns"] = Nameserver(match.group(1))

        debug("rendering template `%s` to file `%s`", template, output)
        stream = self.j2env.get_template(template).stream(data)
        stream.dump(output, encoding="utf-8")

    def render_auto(self, data: dict[str, Any] | None = None):
        """
        Render all *.j2 templates with default (and optionally the provided)
        values and write the output to files without the .j2 extensions.
        """
        templates = [
            str(filepath.relative_to(self.directory))
            for filepath in self.directory.rglob("*.j2")
        ]
        for template in templates:
            self.render(template[:-3], data)


@dataclass
class Nameserver:

    name: str
    num: int | None = None
    ip: str | None = None
    ip6: str | None = None

    def __post_init__(self):
        if self.num is None:
            match = re.search(r"\d+", self.name)
            assert match
            self.num = int(match.group(0))
        if self.ip is None:
            self.ip = f"10.53.0.{self.num}"
        if self.ip6 is None:
            self.ip6 = f"fd92:7065:b8e:ffff::{self.num}"


NS1 = Nameserver("ns1")
NS2 = Nameserver("ns2")
NS3 = Nameserver("ns3")
NS4 = Nameserver("ns4")
NS5 = Nameserver("ns5")
NS6 = Nameserver("ns6")
NS7 = Nameserver("ns7")
NS8 = Nameserver("ns8")
NS9 = Nameserver("ns9")
NS10 = Nameserver("ns10")
NS11 = Nameserver("ns11")


@dataclass
class Zone:

    name: str
    ns: Nameserver
    type: str = "primary"
    filepath: Path | None = field(default=None)

    def __post_init__(self) -> None:
        if self.filepath is None:
            base = "root" if self.name == "." else self.name
            self.filepath = Path(f"zones/{base}.db")


@dataclass
class TrustAnchor:
    domain: str
    type: str
    contents: str
