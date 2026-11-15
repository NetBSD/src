#!/usr/bin/env python3
"""Regression tests for the opt-in KNF clang-format checker."""

import importlib.util
import io
from contextlib import redirect_stdout, redirect_stderr
from pathlib import Path
import tempfile
import unittest

SCRIPT = Path(__file__).with_name("knf-format-check.py")
spec = importlib.util.spec_from_file_location("knf_format_check", SCRIPT)
checker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(checker)


class FormatCheckerTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        directory = Path(self.temp.name)
        self.source = directory / "example.c"
        self.source.write_text("int main(void) { return 0; }\n", encoding="utf-8")
        self.formatter = directory / "fake-clang-format"
        self.formatter.write_text(
            "#!/usr/bin/env python3\n"
            "import pathlib, sys\n"
            "text = pathlib.Path(sys.argv[-1]).read_bytes()\n"
            "sys.stdout.buffer.write(text)\n",
            encoding="utf-8",
        )
        self.formatter.chmod(0o755)

    def test_matching_format_is_success(self):
        with redirect_stdout(io.StringIO()):
            self.assertEqual(checker.main([
                "--clang-format", str(self.formatter), str(self.source)
            ]), 0)

    def test_difference_produces_diff_and_nonzero_exit(self):
        self.formatter.write_text(
            "#!/usr/bin/env python3\n"
            "print('int main(void) {\\n    return 0;\\n}')\n",
            encoding="utf-8",
        )
        output = io.StringIO()
        with redirect_stdout(output):
            status = checker.main([
                "--clang-format", str(self.formatter), str(self.source)
            ])
        self.assertEqual(status, 1)
        self.assertIn("--- ", output.getvalue())
        self.assertIn("+++ ", output.getvalue())
        self.assertEqual(self.source.read_text(), "int main(void) { return 0; }\n")

    def test_missing_file_fails(self):
        errors = io.StringIO()
        with redirect_stderr(errors):
            status = checker.main([
                "--clang-format", str(self.formatter),
                str(self.source.parent / "missing.c")
            ])
        self.assertEqual(status, 1)
        self.assertIn("not a regular file", errors.getvalue())

    def test_formatter_failure_fails(self):
        self.formatter.write_text(
            "#!/usr/bin/env python3\n"
            "import sys\n"
            "sys.stderr.write('bad configuration\\n')\n"
            "sys.exit(2)\n",
            encoding="utf-8",
        )
        errors = io.StringIO()
        with redirect_stderr(errors):
            status = checker.main([
                "--clang-format", str(self.formatter), str(self.source)
            ])
        self.assertEqual(status, 1)
        self.assertIn("bad configuration", errors.getvalue())


if __name__ == "__main__":
    unittest.main()
