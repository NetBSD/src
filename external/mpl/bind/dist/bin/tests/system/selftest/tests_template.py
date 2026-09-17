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
"""
isctest.template self-test
Check the {% include_indented %} tag of the TemplateEngine.
"""

import jinja2
import pytest


def render(templates, system_test_dir, name, source, data=None):
    (system_test_dir / f"{name}.j2").write_text(source)
    templates.render(name, data)
    return (system_test_dir / name).read_text()


def test_include_indented(templates, system_test_dir):
    (system_test_dir / "inc.conf.j2").write_text("first @word@;\nsecond;\n")
    output = render(
        templates,
        system_test_dir,
        "main.conf",
        'block {\n\t{% include_indented "inc.conf.j2" %}\n};\n',
        {"word": "value"},
    )
    assert output == "block {\n\tfirst value;\n\tsecond;\n};\n"


def test_include_indented_depth_follows_tag(templates, system_test_dir):
    (system_test_dir / "inc.conf.j2").write_text("a;\nb;\n")
    output = render(
        templates,
        system_test_dir,
        "nested.conf",
        'one {\n\ttwo {\n\t\t{% include_indented "inc.conf.j2" %}\n\t};\n};\n',
    )
    assert output == "one {\n\ttwo {\n\t\ta;\n\t\tb;\n\t};\n};\n"


def test_include_indented_keeps_blank_lines_blank(templates, system_test_dir):
    (system_test_dir / "inc.conf.j2").write_text("a;\n\nb;\n")
    output = render(
        templates,
        system_test_dir,
        "blank.conf",
        '\t{% include_indented "inc.conf.j2" %}\n',
    )
    assert output == "\ta;\n\n\tb;\n"


def test_include_indented_must_follow_indentation_only(templates, system_test_dir):
    (system_test_dir / "inc.conf.j2").write_text("a;\n")
    with pytest.raises(jinja2.TemplateSyntaxError, match="indentation only"):
        render(
            templates,
            system_test_dir,
            "inline.conf",
            'block { {% include_indented "inc.conf.j2" %}\n};\n',
        )


def test_include_indented_requires_loaded_template(templates):
    with pytest.raises(jinja2.TemplateSyntaxError, match="loader-backed"):
        templates.j2env.from_string('\t{% include_indented "inc.conf.j2" %}\n')


def test_include_indented_common_prefix(templates, system_test_dir):
    output = render(
        templates,
        system_test_dir,
        "hint.conf",
        'view v {\n\t{% include_indented "_common/root.hint.conf" %}\n};\n',
    )
    assert output == (
        "view v {\n"
        '\tzone "." {\n'
        "\t\ttype hint;\n"
        '\t\tfile "../../_common/root.hint";\n'
        "\t};\n"
        "};\n"
    )
