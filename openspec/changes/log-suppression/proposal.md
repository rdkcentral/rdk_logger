## Why

Repetitive log flooding (e.g., the same error message repeated thousands of times) causes storage exhaustion on devices, performance degradation from I/O overhead, increased debugging complexity as important messages get buried in noise, and bandwidth waste when logs are uploaded to remote servers. The rdk_logger library currently has no mechanism to detect or suppress duplicate log lines, meaning every `RDK_LOG` call that passes the priority filter results in a full write through the appender, regardless of whether the same message was just written moments ago.

## What Changes

- Add a **component-level log suppression engine** that detects duplicate log messages (same module + level + format pattern) within a configurable time window and suppresses repeated writes, replacing them with a periodic summary line (e.g., "Previous message repeated N times").
- Add a **nvram sync suppression layer** that deduplicates log patterns before logs are synced/uploaded from the device, further reducing upload bandwidth.
- Introduce a new configuration mechanism in `debug.ini` to control suppression behavior per module (enable/disable, time window, max repeats before suppression).
- Expose new public API functions for controlling suppression at runtime.

## Capabilities

### New Capabilities
- `log-dedup-component`: Component-level duplicate log detection and suppression within `rdk_dbg_priv_log_msg()`. Tracks recent log patterns per category, suppresses repeated identical messages within a configurable time window, and emits a summary count when suppression ends.
- `log-dedup-nvram-sync`: Nvram sync-level deduplication that filters redundant log patterns before log files are uploaded/synced to remote servers, reducing bandwidth consumption.
- `log-suppression-config`: Configuration support for suppression parameters in `debug.ini` (per-module enable/disable, time window in seconds, max repeat threshold) and runtime control via the existing UDP dynamic control mechanism.

### Modified Capabilities
<!-- No existing specs to modify - this is a new capability addition to the logging pipeline -->

## Impact

- **Core engine** (`src/rdk_debug_priv.c`): The `rdk_dbg_priv_log_msg()` function path will gain a suppression check stage between the priority filter (stage 8) and log output (stage 9).
- **Public API** (`include/rdk_logger.h`): New functions for suppression control (enable/disable, query stats, reset counters).
- **Config parsing** (`src/rdk_debug_priv.c:rdk_logger_parse_config`): Extended to parse suppression directives from `debug.ini`.
- **New source files**: `src/rdk_log_suppression.c` and `include/rdk_log_suppression.h` for the suppression engine implementation.
- **Thread safety**: Suppression state must be guarded by `gLoggingMutex` or a dedicated mutex to maintain thread safety.
- **Memory**: Small per-category hash table for tracking recent message patterns (bounded memory footprint).
- **Build system**: `configure.ac` and `Makefile.am` updated to include new source files.
- **Unit tests**: New test suite for suppression behavior validation.
