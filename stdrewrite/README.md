# stdrewrite

`stdrewrite/` is the built-in WRS rule library for the early WIL pipeline.

Rule sets:

- `arithmetic.wrs` - neutral-element, annihilator, and simple arithmetic
  normalization rules.
- `logic.wrs` - boolean identities, idempotence, and `select` simplifications.
- `canonical.wrs` - shape-normalization rules that collapse equivalent graphs
  toward a single canonical form.

The Stage 6 library is intended to load through
`wilRewriteSystemAddAllBuiltinRules()` and to be safe for repeated fixpoint
normalization with the current WRS engine.

- `mal_peephole.wrs` - MAL instruction peephole rules consumed by the Stage 7 MRS loader.
