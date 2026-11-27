# tools

Helper build for triage. `ini2json` links `src/ini.c` only, so a report can be
reproduced without libFuzzer and compared between a fixed and an unfixed build:

```sh
clang -fsanitize=address,undefined -g -Isrc src/ini.c tools/ini2json.c -o ini2json
./ini2json crash-<hash> | diff - <(./ini2json.fixed crash-<hash>)
```

It prints every stored entry as JSON and exits `2` when two entries end up under the same
`(section, key)` pair, naming the shadowed index and the index that wins. That is the
signal for the truncation collision in issue #1: distinct 70 byte section names are
stored under one 63 byte prefix, so a later section overrides an earlier one and the
override is invisible in the output.

Ordinary use:

```sh
./ini2json corpus/basic.ini
```
