## Tasks

### Phase 1: Core Suppression Engine

- [ ] Create `include/rdk_log_suppression.h` with suppression API declarations
  - `rdk_log_suppression_init()` / `rdk_log_suppression_deinit()`
  - `rdk_log_suppression_check(module, level, fmt)` → returns SUPPRESS/PASS/EMIT_SUMMARY
  - `rdk_log_suppression_configure(module, enabled, window_sec, max_repeats)`
  - `rdk_log_suppression_stats(module)` → returns suppression counters

- [ ] Create `src/rdk_log_suppression.c` implementing the suppression engine
  - FNV-1a hash function for pattern matching
  - Per-category ring buffer (8 slots) with lazy allocation
  - Time window tracking using `CLOCK_MONOTONIC`
  - Summary message generation ("previous message repeated N times")

- [ ] Integrate suppression check into `src/rdk_debug_priv.c:rdk_dbg_priv_log_msg()`
  - Add suppression check after priority filter (line ~647), before `log4c_category_vlog()` (line ~649)
  - Guard with `#ifdef HAVE_LOG_SUPPRESSION`
  - On SUPPRESS: increment counter, release mutex, return
  - On EMIT_SUMMARY: log summary line, then log current message

### Phase 2: Configuration Support

- [ ] Extend `rdk_logger_parse_config()` in `src/rdk_debug_priv.c` to parse suppression directives
  - `LOG.RDK.SUPPRESS = ON|OFF` (global enable/disable)
  - `LOG.RDK.SUPPRESS.WINDOW = 30` (time window in seconds)
  - `LOG.RDK.SUPPRESS.THRESHOLD = 3` (messages before suppression kicks in)
  - `LOG.RDK.<MODULE>.SUPPRESS = ON|OFF` (per-module override)

- [ ] Add suppression init call to `rdk_logger_init.c` after `rdk_dbg_priv_config()`

- [ ] Add suppression deinit call to `rdk_logger_deinit()`

### Phase 3: Nvram Sync Suppression

- [ ] Create `utils/rdk_log_dedup_sync.c` - standalone utility for pre-upload dedup
  - Read log file, identify repeated patterns within configurable window
  - Write deduplicated output with summary counts
  - Exit codes: 0 = success, 1 = error, 2 = no changes needed

- [ ] Create `scripts/logDedup.sh` - shell wrapper for the dedup sync utility
  - Integrate with existing log upload pipeline
  - Process all log files in `/rdklogs/logs/` before upload

### Phase 4: Build System & Tests

- [ ] Update `configure.ac` to add `--enable-log-suppression` flag (default: yes)
  - Define `HAVE_LOG_SUPPRESSION` when enabled

- [ ] Update `src/Makefile.am` to include `rdk_log_suppression.c`

- [ ] Update `utils/Makefile.am` to build `rdk_log_dedup_sync`

- [ ] Create `unittests/rdk_log_suppression_test.cpp` with test cases:
  - Test: identical messages are suppressed after threshold
  - Test: different messages are not suppressed
  - Test: summary message emitted after suppression window
  - Test: per-module configuration override
  - Test: suppression disabled globally
  - Test: thread safety under concurrent logging
  - Test: ring buffer eviction with many patterns
  - Test: time window expiry resets suppression

### Phase 5: Documentation

- [ ] Update `include/rdk_logger.h` with new public API declarations and documentation comments

- [ ] Add suppression section to existing debug.ini examples
