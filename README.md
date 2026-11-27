# ini-fuzz

A libFuzzer harness around `src/ini.c`, a ~120 line INI config parser written the way
most service-side parsers are: a 64 byte section buffer, a 64 byte key buffer, a 256 byte
value buffer, and truncation instead of allocation on every field. The harness exists to
answer one question cheaply: **which inputs make this parser store an entry it will hand
back wrong?**

## Why

Config parsers sit on the trust boundary of nearly every daemon. They are fed by
provisioning systems, CI variables and mounted secrets, and their output picks up ports,
paths and feature flags. Bugs that survive review here are usually not memory
corruption but *state confusion*: two different inputs mapping onto one stored key, so a
later section silently overrides an earlier one. That is a privilege question before it
is a correctness question.

## Threat model

- Attacker controls config file bytes, not the parser call sites.
- Truncation at `INI_MAX_SECTION` / `INI_MAX_KEY` is deliberate: a truncated name can
  collide with a shorter, distinct name.
- Duplicate keys are legal and last-write-wins, so ordering is attacker-influenced.
- No escape processing and no includes, so the attack surface is the line splitter,
  the `[` / `=` scanners and the fixed buffers.
- Out of scope: file I/O, symlink races, and anything that needs a filesystem.

## Build

```sh
make            # clang -fsanitize=fuzzer,address, needs clang 12+
make run        # 60 second run over corpus/
```

## Usage

```sh
./ini-fuzz -max_total_time=120 corpus
./ini-fuzz crash-<hash>              # replay one input
```

Interesting inputs belong in `corpus/`; crashes are minimized with
`clang -fsanitize=fuzzer,address src/ini.c src/ini_fuzz.c -o ini-fuzz` first, so the
report carries the smallest reproducer.

## Repro

Known truncation collision, one command:

```sh
printf '[a%.0s]\n' {1..70}; printf 'port=1\n' > /tmp/c1.ini
printf '[%s]\nport=2\n' "$(printf 'a%.0s' {1..70})" >> /tmp/c1.ini
./ini-fuzz /tmp/c1.ini   # no crash, but ini_get("aaa...", "port") sees only 2
```

The 70 byte section name is stored as the same 63 byte prefix in both entries, so the
first `port=1` is unreachable through the public lookup. Details in issue #1.

## Mitigation

- Reject (or hash) section and key names longer than the buffer instead of truncating.
- Treat a duplicate `(section, key)` pair as a hard config error rather than last-wins.
- Assert the round-trip invariant in tests: every accepted entry must be findable.

## Layout

```
src/ini.h        public API and the three fixed capacities
src/ini.c        line splitter, section/key/value scanners, lookup
src/ini_fuzz.c   libFuzzer entry point and invariant checks
corpus/          seed inputs
```

The parser is intentionally dependency-free; `make` needs nothing but a clang with
libFuzzer and ASan.

## License

MIT, see `LICENSE`.
