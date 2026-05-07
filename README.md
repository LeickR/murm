# murm

<!-- TODO: project description -->

## Build and run

The example program:

```bash
cd examples/example1
make            # release build
make debug      # debug build
make test       # build + run example1
```

The test suite:

```bash
cd test
make            # default target: build + run all tests
```

## Editor / IDE setup (clangd)

This repo ships a `compile_flags.txt` at the root that tells clangd-aware
editors (Cursor, VSCode + clangd extension, neovim, ...) about the
project's portable compile options: include paths, `-std=c++17`,
`-fopenmp`, and the warning set used by the build.

That file alone is enough on systems where clangd can locate the C++
standard library on its own. On systems where it can't (most commonly:
Linux with `clangd` installed but no `clang++`), you'll see errors like
`'iostream' file not found` and Go to Definition won't work. In that
case, drop a personal `.clangd` file at the repo root with the missing
bits. `.clangd` is gitignored precisely so each developer can tailor it.

### Recipe: Ubuntu / Debian + gcc, no clang++

This is the common case for Linux developers. clangd ships in the
`clangd` package, but Ubuntu doesn't pull in `clang++` with it, so
clangd has nothing to query for the libstdc++ paths. Find them with:

```bash
g++ -E -x c++ -v - </dev/null 2>&1 | sed -n '/#include <\.\.\.> search/,/End of search/p'
```

then translate each path into an `-isystem` entry. For Ubuntu 24.04
with gcc 13 the resulting `.clangd` looks like:

```yaml
CompileFlags:
  Add:
    - -isystem
    - /usr/include/c++/13
    - -isystem
    - /usr/include/x86_64-linux-gnu/c++/13
    - -isystem
    - /usr/include/c++/13/backward
    - -isystem
    - /usr/lib/gcc/x86_64-linux-gnu/13/include
    # clang 18 rejects the 2-arg form of gcc-13's
    # __attribute__((__malloc__(deallocator, idx))).  Strip the macro form
    # so attribute parsing succeeds; standalone __malloc__ still works.
    - -D__malloc__(...)=
```

Adjust the `13` to match your gcc version (`g++ -dumpversion`).

### Recipe: macOS or any system with clang++ installed

You generally need nothing. clang++ knows where its own standard
library lives, and clangd auto-discovers those paths. If Go to
Definition doesn't work, double-check that the `clangd` extension /
binary is wired up in your editor and that clang++ is on PATH.

### Recipe: any system, one-liner via `--query-driver`

Instead of hard-coding `-isystem` paths, you can launch clangd with
`--query-driver=<path-to-compiler>`. clangd then asks the compiler for
its system include paths every time. This is the most portable option
and avoids per-version churn.

For Cursor / VSCode + clangd extension, in your user `settings.json`:

```jsonc
{
  "clangd.arguments": [
    "--query-driver=/usr/bin/g++",
    "--query-driver=/usr/bin/clang++"
  ]
}
```

After saving, restart the language server (command palette →
"clangd: Restart language server").

### Troubleshooting

- After editing `compile_flags.txt` or `.clangd`, restart clangd
  (command palette → "clangd: Restart language server"); changes are
  picked up on next reindex.
- To see exactly what clangd is doing, run from the repo root:

  ```bash
  clangd --check=include/SimOptionHandler.hpp
  ```

  The `Generic fallback command is:` line shows the full compile
  command clangd is using, and any real diagnostics show up as `E[...]`.
