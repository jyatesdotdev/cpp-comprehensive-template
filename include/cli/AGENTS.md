# cli — CLI11 helpers & terminal output formatting

Scope: `include/cli/`. Namespaces `cli` and `cli::fmt`. CMake target **`cli` (INTERFACE /
header-only)**, links **CLI11** (fetched via FetchContent). Parent: [include/AGENTS.md](../AGENTS.md).
Design guide: [docs/cli.md](../../docs/cli.md).

## Business role
Reusable command-line plumbing: environment/config/flag merging on top of **CLI11**, plus
terminal presentation helpers (ANSI color, aligned tables, progress bars). The worked example
`examples/cli_demo.cpp` wires these into subcommands and also shows a Boost.ProgramOptions
alternative under `HAS_BOOST`.

## Files
- `cli_helpers.h` (namespace `cli`) — `env_fallback(opt, ENV)`, `parse_config_file(path)`,
  `apply_config_defaults(app, cfg)`, and validators `PortRange`, `FileExists`, `NonEmpty`.
- `output_format.h` (namespace `cli::fmt`) — `color::*` ANSI constants, `colorize`,
  `visible_length` (ANSI-aware), `Table`, `ProgressBar`.

## Invariants & business rules (MUST hold)
1. **Config precedence is a product decision:** CLI flag > env var > config file > default.
   `cli_demo.cpp` implements it (CLI11 `->envname(...)`, then config as lowest-priority fallback
   in the subcommand callback). Preserve that ordering when touching merge logic;
   `apply_config_defaults` only fills options the user did **not** set (`opt->empty()`).
2. **`parse_config_file` format:** `key=value`, `#` comments, blank lines skipped, keys/values
   whitespace-trimmed. Keep it forgiving and side-effect-free (returns a map).
3. **Validators return an empty string on success, an error message on failure** (CLI11's
   convention). `PortRange` = 1–65535; `NonEmpty` rejects empty; `FileExists` wraps
   `CLI::ExistingFile`. Add new validators the same way.
4. **`cli::fmt` table/width math is ANSI-aware.** Column widths use `visible_length`, which
   **skips ANSI escape sequences** so colored cells still align. If you add styling, route it
   through `colorize`/`visible_length` — never count raw `.size()` for layout.
   `colorize` is a no-op when `NO_COLOR` is set (non-empty); `Table`/`ProgressBar` honor it by
   not emitting ANSI of their own.
5. **Stream discipline:** `ProgressBar` draws to **stderr** (so piped stdout stays clean);
   `Table` prints to a caller-supplied `std::ostream` (default `std::cout`). Keep progress/UI
   chrome off stdout so machine-readable output isn't corrupted.

## C++ best practices for this module
- Header-only: keep helpers `inline`; validators are `inline` `CLI::Validator` objects.
- Prefer `std::filesystem::path` for paths (already used) and `std::string_view` for read-only text.
- Don't hard-code colors into data — colorize at the presentation edge only.
- Boost.ProgramOptions code stays under `#ifdef HAS_BOOST`; the CLI11 path is the default and
  must always compile.

## When editing
- Update `tests/cli_tests.cpp` (tag `[cli]`) and `examples/cli_demo.cpp`.
- New subcommand pattern → prefer demonstrating it in `cli_demo.cpp`; put genuinely reusable
  logic in `cli_helpers.h`.

## Neighbors
CLI11 is pulled in by the root `CMakeLists.txt` via FetchContent (v2.4.2). Terminal output is
otherwise self-contained. Full CLI patterns: [docs/cli.md](../../docs/cli.md).
