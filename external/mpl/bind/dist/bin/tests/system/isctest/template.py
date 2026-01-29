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

from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, Optional, Union

import jinja2

from .log import debug
from .vars import ALL


class TemplateEngine:
    """
    Engine for rendering jinja2 templates in system test directories.
    """

    def __init__(self, directory: Union[str, Path], env_vars=ALL):
        """
        Initialize the template engine for `directory`, optionally overriding
        the `env_vars` that will be used when rendering the templates (defaults
        to the environment variables set by the pytest runner).
        """
        self.directory = Path(directory)
        self.env_vars = dict(env_vars)
        self.j2env = jinja2.Environment(
            loader=jinja2.FileSystemLoader(str(self.directory)),
            undefined=jinja2.StrictUndefined,
            variable_start_string="@",
            variable_end_string="@",
        )

    def render(
        self,
        output: str,
        data: Optional[Dict[str, Any]] = None,
        template: Optional[str] = None,
    ) -> None:
        """
        Render `output` file from jinja `template` and fill in the `data`. The
        `template` defaults to *.j2.manual or *.j2 file. The environment
        variables which the engine was initialized with are also filled in. In
        case of a variable name clash, `data` has precedence.
        """
        if template is None:
            template = f"{output}.j2.manual"
            if not Path(template).is_file():
                template = f"{output}.j2"
        if not Path(template).is_file():
            raise RuntimeError('No jinja2 template found for "{output}"')

        if data is None:
            data = self.env_vars
        else:
            data = {**self.env_vars, **data}

        debug("rendering template `%s` to file `%s`", template, output)
        stream = self.j2env.get_template(template).stream(data)
        stream.dump(output, encoding="utf-8")

    def render_auto(self, data: Optional[Dict[str, Any]] = None):
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
    ip: str


@dataclass
class Zone:
    name: str
    filename: str
    ns: Nameserver
    type: str = "primary"


@dataclass
class TrustAnchor:
    domain: str
    type: str
    contents: str
