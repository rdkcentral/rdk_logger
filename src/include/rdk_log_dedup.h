/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
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

#if !defined(_RDK_LOG_DEDUP_H)
#define _RDK_LOG_DEDUP_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** Maximum number of tracked category slots for dedup state. */
#define RDK_DEDUP_TABLE_SIZE 256

/** Default dedup window in seconds. */
#define RDK_DEDUP_DEFAULT_WINDOW_SEC 30

/** Default max suppression count before forced flush. */
#define RDK_DEDUP_DEFAULT_MAX_SUPPRESS 1000

/**
 * Configuration for the dedup engine.
 * Parsed from debug.ini keys:
 *   LOG.RDK.DEDUP_ENABLED = 0|1
 *   LOG.RDK.DEDUP_WINDOW_SEC = <seconds>
 *   LOG.RDK.DEDUP_MAX_SUPPRESS = <count>
 */
typedef struct {
    bool     enabled;          /**< Whether dedup is active */
    uint32_t window_sec;       /**< Time window before auto-flush of summary */
    uint32_t max_suppress;     /**< Max consecutive duplicates before forced flush */
} rdk_dedup_config_t;

/**
 * Per-category dedup state slot.
 */
typedef struct {
    const void *category_key;  /**< log4c category pointer (used as hash key) */
    uint32_t    last_msg_hash; /**< FNV-1a hash of last formatted message */
    uint32_t    repeat_count;  /**< Number of suppressed duplicates */
    time_t      first_seen;    /**< Timestamp when first duplicate was seen */
    int         last_level;    /**< Log level of suppressed message (log4c priority) */
} rdk_dedup_slot_t;

/**
 * Initialize the dedup engine with default configuration.
 * Must be called once during logger init (from rdk_dbg_priv_init).
 */
void rdk_dedup_init(void);

/**
 * Update dedup configuration. Called when debug.ini is parsed.
 * @param config  New configuration values.
 */
void rdk_dedup_set_config(const rdk_dedup_config_t *config);

/**
 * Get current dedup configuration (read-only).
 */
const rdk_dedup_config_t* rdk_dedup_get_config(void);

/**
 * Check if a message should be suppressed.
 *
 * MUST be called under gLoggingMutex.
 *
 * @param category_key  Opaque pointer identifying the category (log4c_category_t*).
 * @param log4c_prio    The log4c priority level of the message.
 * @param format        The format string of the log message.
 * @param args          The va_list arguments (used for hashing the formatted message).
 * @return true if the message should be suppressed (duplicate), false if it should be logged.
 *
 * Side effects: If returning false after a suppression run, emits a summary
 *               message via the provided category before returning.
 */
bool rdk_dedup_check(const void *category_key, int log4c_prio,
                     const char *format, va_list args);

/**
 * Flush any pending dedup summary for a category (e.g., on window expiry).
 * MUST be called under gLoggingMutex.
 * @param category_key  The category to flush.
 */
void rdk_dedup_flush(const void *category_key);

/**
 * Flush all pending dedup summaries. Called during deinit.
 * MUST be called under gLoggingMutex.
 */
void rdk_dedup_flush_all(void);

/**
 * Compute FNV-1a 32-bit hash of a string.
 * @param str  Null-terminated string to hash.
 * @return 32-bit hash value.
 */
uint32_t rdk_dedup_fnv1a_hash(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* _RDK_LOG_DEDUP_H */
