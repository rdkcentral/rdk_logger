/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef RDK_LOG_SUPPRESSION_H
#define RDK_LOG_SUPPRESSION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/** Maximum number of tracked patterns per category */
#define RDK_SUPPRESS_RING_SIZE 8

/** Default suppression time window in seconds */
#define RDK_SUPPRESS_DEFAULT_WINDOW_SEC 30

/** Default number of repeats before suppression activates */
#define RDK_SUPPRESS_DEFAULT_THRESHOLD 3

/** Result of a suppression check */
typedef enum {
    RDK_SUPPRESS_PASS = 0,         /**< Message should be logged normally */
    RDK_SUPPRESS_BLOCK = 1,        /**< Message is suppressed (duplicate) */
    RDK_SUPPRESS_EMIT_SUMMARY = 2  /**< Emit summary then log current message */
} rdk_SuppressResult;

/** Suppression statistics for a module */
typedef struct {
    uint64_t total_suppressed;     /**< Total messages suppressed */
    uint64_t total_summaries;      /**< Total summary messages emitted */
    uint32_t current_repeat_count; /**< Current consecutive repeat count */
} rdk_SuppressStats;

/**
 * Initialize the log suppression subsystem.
 * Must be called after rdk_dbg_priv_config() during logger initialization.
 * @return 0 on success, -1 on failure
 */
int rdk_log_suppression_init(void);

/**
 * Deinitialize the log suppression subsystem.
 * Frees all allocated suppression state.
 */
void rdk_log_suppression_deinit(void);

/**
 * Check if a log message should be suppressed.
 * Called from rdk_dbg_priv_log_msg() after priority check.
 *
 * @param module   The log category name (e.g., "LOG.RDK.TR69")
 * @param level    The log level of the message
 * @param fmt      The format string (before printf expansion)
 * @param[out] repeat_count  If result is EMIT_SUMMARY, set to number of suppressed messages
 * @return RDK_SUPPRESS_PASS, RDK_SUPPRESS_BLOCK, or RDK_SUPPRESS_EMIT_SUMMARY
 */
rdk_SuppressResult rdk_log_suppression_check(const char *module, int level,
                                              const char *fmt,
                                              uint32_t *repeat_count);

/**
 * Configure suppression for a specific module.
 *
 * @param module       Module name (NULL for global default)
 * @param enabled      true to enable, false to disable
 * @param window_sec   Time window in seconds (0 = use default)
 * @param threshold    Messages before suppression kicks in (0 = use default)
 * @return 0 on success, -1 on failure
 */
int rdk_log_suppression_configure(const char *module, bool enabled,
                                   uint32_t window_sec, uint32_t threshold);

/**
 * Get suppression statistics for a module.
 *
 * @param module  Module name (NULL for global stats)
 * @param[out] stats  Filled with current statistics
 * @return 0 on success, -1 if module not found
 */
int rdk_log_suppression_get_stats(const char *module, rdk_SuppressStats *stats);

/**
 * Reset suppression state for a module (or all if module is NULL).
 * Useful after configuration changes or for testing.
 *
 * @param module  Module name (NULL to reset all)
 */
void rdk_log_suppression_reset(const char *module);

/**
 * Check if suppression is globally enabled.
 * @return true if suppression is active
 */
bool rdk_log_suppression_is_enabled(void);

#ifdef __cplusplus
}
#endif

#endif /* RDK_LOG_SUPPRESSION_H */
