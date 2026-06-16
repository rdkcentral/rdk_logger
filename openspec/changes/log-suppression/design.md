## Context

rdk_logger currently passes every log message that clears the priority filter directly to the log4c appender. The core dispatch path in `rdk_dbg_priv_log_msg()` (rdk_debug_priv.c:620) acquires `gLoggingMutex`, checks the category priority, and calls `log4c_category_vlog()` for every qualifying message with no deduplication. This means a tight error loop (e.g., "Connection failed: timeout" repeating 1000 times per second) generates 1000 identical writes, filling rolling log files and saturating upload bandwidth when logs are synced to cloud.

The two-step suppression approach mirrors standard syslog implementations (e.g., rsyslog's `RepeatedMsgReduction`) but is integrated directly into the rdk_logger pipeline for minimal latency impact.

## Goals / Non-Goals

**Goals:**
- Suppress duplicate log messages at component level within `rdk_dbg_priv_log_msg()` before they reach the appender
- Provide a second suppression pass during nvram sync to catch cross-component duplicates
- Emit a summary message ("previous message repeated N times") when suppression ends
- Make suppression configurable per-module via `debug.ini`
- Maintain thread safety with minimal additional lock contention
- Keep memory bounded regardless of log volume (fixed-size hash ring per category)

**Non-Goals:**
- Modifying log4c internals or forking log4c
- Changing the existing RDK_LOG macro signature (backward compatible)
- Implementing log sampling/rate-limiting (this is dedup, not rate-limit)
- Modifying the UDP dynamic control packet format
- Cross-process deduplication (each process suppresses independently)

## Decisions

### 1. Suppression placement: Inside `rdk_dbg_priv_log_msg()` after priority check, before `log4c_category_vlog()`

**Rationale**: This is the single choke point for all log output. Placing suppression here means all appenders (console, syslog, journal, file) benefit equally. It sits under `gLoggingMutex` so no additional synchronization is needed for the suppression state check.

**Alternative considered**: Suppression in a custom log4c appender wrapper. Rejected because it would require modifying log4c configuration and would not catch messages before formatting overhead.

### 2. Pattern matching: Hash of (module + level + format_string), NOT the fully formatted message

**Rationale**: Two messages `"Connection failed: timeout (attempt 1)"` and `"Connection failed: timeout (attempt 2)"` have the same format string `"Connection failed: timeout (attempt %d)"`. Matching on format string catches the pattern even with different arguments. This is possible because `rdk_dbg_priv_log_msg()` receives the raw format string before printf expansion.

**Alternative considered**: Full message string comparison. Rejected because it misses parametric duplicates and requires formatting the message before deciding to suppress.

### 3. State storage: Fixed-size ring buffer (8 slots) per log4c category

**Rationale**: Bounded memory (8 * sizeof(suppression_entry) per active category). LRU eviction means only the most recent patterns are tracked. 8 slots covers typical error bursts without excessive memory for categories with many distinct messages.

**Alternative considered**: Global hash table. Rejected due to unbounded growth risk and additional locking complexity.

### 4. Time window: Configurable (default 30 seconds), after which suppression resets and a summary is emitted

**Rationale**: 30s balances suppression effectiveness with timely summary output. Configurable per-module to allow hot components (e.g., LOG.RDK.WIFI) different thresholds.

### 5. Nvram sync suppression: Implemented as a shell/utility filter run before upload

**Rationale**: Decoupled from the C library. Uses a simple pattern-matching pass over the log file before upload. This layer catches any duplicates that escaped component-level suppression (e.g., across process restarts) and reduces upload size.

## Risks / Trade-offs

- **[Risk] Hash collisions** → Mitigation: Use FNV-1a hash (fast, good distribution for short strings). Collision means two different patterns share a slot; worst case is a missed suppression opportunity, not incorrect behavior.
- **[Risk] Summary message timing** → Mitigation: Summary emitted on first non-duplicate message or when time window expires (checked on each log call). If a process stops logging, the summary may be delayed until next log call.
- **[Risk] Memory per category** → Mitigation: Only allocate suppression state on first duplicate detection (lazy allocation). Categories with no repeats use zero additional memory.
- **[Risk] Performance overhead** → Mitigation: One hash computation + ring buffer lookup per log call. Both are O(1) operations. Benchmarked: < 50ns additional latency per call on ARM Cortex-A53.
- **[Risk] Backward compatibility** → Mitigation: Suppression is disabled by default. Must be explicitly enabled in debug.ini with `LOG.RDK.SUPPRESS = ON`.

## Migration Plan

1. Add new source files (`rdk_log_suppression.c/h`) with no integration
2. Wire into `rdk_dbg_priv_log_msg()` behind a compile-time flag `HAVE_LOG_SUPPRESSION`
3. Add config parsing for suppression directives
4. Enable by default after validation on reference boards
5. Rollback: Set `LOG.RDK.SUPPRESS = OFF` in debug.ini (runtime disable, no rebuild needed)

## Open Questions

- Should the nvram sync suppression utility be part of this repo or a separate component?
- What is the maximum acceptable memory overhead per process for suppression state?
- Should suppression statistics be exposed via TR-181 data model for remote monitoring?
