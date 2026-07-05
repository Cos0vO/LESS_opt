# RREF Information-Set Shortcut Experiment

## Goal

Test an experimental RREF side path for LESS_opt: first try the special case where
columns `0..K-1` already form an information set. In that case, row-reducing the
prefix `K x K` block to identity produces the same RREF as the existing generic
`generator_RREF()`. If the prefix block is singular, the wrapper falls back to
the existing implementation.

The experiment does not change keygen, sign, or verify defaults.

## Step Log

### 2026-07-05 - Step 1: Interface and baseline implementation

- Added declarations for:
  - `generator_RREF_prefix_infoset`
  - `generator_RREF_prefix_infoset_or_fallback`
- Added scalar baseline implementations in both the reference RREF module and
  the optimized common RREF module.
- The prefix function only accepts pivot columns `0..K-1`; it returns `0` if any
  prefix pivot is missing.
- The wrapper copies the input before trying the shortcut, so failed shortcut
  attempts cannot corrupt the fallback input.

### 2026-07-05 - Step 2: Tests and benchmark hooks

- Added an equivalence test to `LESS_test`:
  - random matrices,
  - monomial-transformed full-rank matrices,
  - one explicit fallback case where the first `K` columns are singular but the
    full matrix still has rank `K`.
- Extended RREF benchmark output with:
  - prefix shortcut success count,
  - fallback count,
  - wrapper average cycles,
  - wrapper/current RREF cycle ratio.

## Verification Results

### 2026-07-05 - Build and unit tests

- `cmake -S Reference_Implementation -B build-ref-rref-info -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5`
- `cmake --build build-ref-rref-info --target LESS_test_cat_252_192 -j4`
- `./build-ref-rref-info/LESS_test_cat_252_192`
  - Result: pass.
  - New test output: `RREF prefix infoset: ok (random success 32/32, monomial success 32/32)`.
- `cmake -S Optimized_Implementation/neon -B build-neon-rref-info -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5`
- `cmake --build build-neon-rref-info --target LESS_test_cat_252_192 -j4`
- `./build-neon-rref-info/LESS_test_cat_252_192`
  - Result: pass.
  - New test output: `RREF prefix infoset: ok (random success 32/32, monomial success 32/32)`.
- `cmake --build build-neon-rref-info --target LESS_test_cat_252_192 LESS_test_cat_252_68 LESS_test_cat_252_45 LESS_test_cat_400_220 LESS_test_cat_400_102 LESS_test_cat_548_345 LESS_test_cat_548_137 -j4`
- `ctest --test-dir build-neon-rref-info -R '^LESS_test_cat_' --output-on-failure`
  - Result: pass, 7/7 tests passed.

### 2026-07-05 - Benchmark attempt

- Added cycle-counter benchmark hooks to `bench_rref.c`.
- A manual `bench_rref` build for neon started successfully, but this macOS
  host does not currently expose the M1 performance counters to the process:
  `kpc_set_config failed (root?)` and repeated `kpc_get_thread_counters failed`.
- Because those cycle readings are not trustworthy on this host, the run was
  interrupted and not used as evidence.

### 2026-07-05 - Wall-clock fallback benchmark

Temporary wall-clock harness, neon implementation, Release flags, 1024
iterations per parameter set:

| Category/Target | Normal RREF avg | Prefix wrapper avg | Shortcut successes | Factor |
| --- | ---: | ---: | ---: | ---: |
| 252/192 | 258241 ns | 294455 ns | 1022/1024 | 1.140 |
| 400/220 | 745619 ns | 1062720 ns | 1014/1024 | 1.425 |
| 548/345 | 1882146 ns | 2908189 ns | 1018/1024 | 1.545 |

Interpretation: prefix information-set success rate is high for these sampled
monomial-transformed full-rank matrices, but the current scalar shortcut wrapper
is slower than the existing SIMD RREF path. This is a negative performance
result for direct default-path adoption, while still leaving a possible follow-up
for vectorizing the prefix path.

## Notes

- The shortcut is mathematically valid only when the prefix `K x K` block is
  invertible. It is not a replacement for arbitrary information-set selection.
- The optimized implementation currently uses the scalar baseline for the new
  side path. If the shortcut proves useful, a follow-up can vectorize this path.
