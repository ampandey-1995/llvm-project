"""
Test basic std::shared_ptr functionality.
"""

from lldbsuite.test.decorators import *
from lldbsuite.test.lldbtest import *
from lldbsuite.test import lldbutil


class TestSharedPtr(TestBase):
    @add_test_categories(["libc++"])
    @skipIf(compiler=no_match("clang"))
    @skipIf(compiler="clang", compiler_version=["<", "17.0"])
    @skipIf(macos_version=["<", "15.0"])
    @skipUnlessDarwin
    def test(self):
        self.build()

        lldbutil.run_to_source_breakpoint(
            self, "// Set break point at this line.", lldb.SBFileSpec("main.cpp")
        )

        self.runCmd("settings set target.import-std-module true")

        self.expect_expr(
            "s",
            result_type="std::shared_ptr<int>",
            result_summary="3 strong=1 weak=0",
            result_children=[ValueCheck(name="pointer")],
        )
        self.expect_expr("*s", result_type="element_type", result_value="3")
        self.expect_expr("*s = 5", result_type="element_type", result_value="5")
        self.expect_expr("*s", result_type="element_type", result_value="5")
        self.expect_expr("(bool)s", result_type="bool", result_value="true")
        self.expect("expr s.reset()")
        self.expect_expr("(bool)s", result_type="bool", result_value="false")
