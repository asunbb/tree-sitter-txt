/// <reference types="tree-sitter-cli/dsl" />

module.exports = grammar({
  name: "litetxt",

  externals: ($) => [$._newline, $._indent, $._dedent],

  rules: {
    document: ($) => repeat($.segment),

    segment: ($) =>
      seq(
        $.content,
        optional(
          seq($._indent, $.segment, repeat(seq($._newline, $.segment)), $._dedent)
        )
      ),

    content: ($) => /[^\n]+/,
  },
});
