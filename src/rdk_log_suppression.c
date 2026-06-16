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

#include "rdk_log_suppression.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

/* Maximum number of categories tracked for suppression */
#define MAX_SUPPRESS_CATEGORIES 64

/* FNV-1a hash parameters (32-bit) */
#define FNV_OFFSET_BASIS 0x811c9dc5u
#define FNV_PRIME        0x01000193u

/** Single entry in the per-category ring buffer */
typedef struct {
    uint32_t pattern_hash;       /**< FNV-1a hash of (module + level + fmt) */
    uint32_t repeat_count;       /**< Number of consecutive repeats */
    struct timespec first_seen;  /**< Timestamp of first occurrence */
    struct timespec last_seen;   /**< Timestamp of most recent occurrence */
    bool active;                 /**< Slot is in use */
} suppress_entry_t;

/** Per-category suppression state */
typedef struct {
    char module[128];                              /**< Module/category name */
    suppress_entry_t ring[RDK_SUPPRESS_RING_SIZE]; /**< Ring buffer of tracked patterns */
    uint32_t ring_head;                            /**< Next slot to write */
    bool enabled;                                  /**< Suppression enabled for this category */
    uint32_t window_sec;                           /**< Time window in seconds */
    uint32_t threshold;                            /**< Repeats before suppression */
    uint64_t total_suppressed;                     /**< Lifetime counter */
    uint64_t total_summaries;                      /**< Lifetime summary count */
} suppress_category_t;

/** Global suppression state */
static struct {
    bool initialized;
    bool globally_enabled;
    uint32_t default_window_sec;
    uint32_t default_threshold;
    suppress_category_t categories[MAX_SUPPRESS_CATEGORIES];
    int num_categories;
} g_suppress = {
    .initialized = false,
    .globally_enabled = false,
    .default_window_sec = RDK_SUPPRESS_DEFAULT_WINDOW_SEC,
    .default_threshold = RDK_SUPPRESS_DEFAULT_THRESHOLD,
    .num_categories = 0
};

/**
 * Compute FNV-1a hash of a byte sequence.
 */
static uint32_t fnv1a_hash(const void *data, size_t len)
{
    const unsigned char *p = (const unsigned char *)data;
    uint32_t hash = FNV_OFFSET_BASIS;
    for (size_t i = 0; i < len; i++) {
        hash ^= p[i];
        hash *= FNV_PRIME;
    }
    return hash;
}

/**
 * Compute pattern hash from module + level + format string.
 */
static uint32_t compute_pattern_hash(const char *module, int level, const char *fmt)
{
    uint32_t hash = FNV_OFFSET_BASIS;

    /* Hash module name */
    if (module) {
        const unsigned char *p = (const unsigned char *)module;
        while (*p) {
            hash ^= *p++;
            hash *= FNV_PRIME;
        }
    }

    /* Hash level */
    hash ^= (uint32_t)level;
    hash *= FNV_PRIME;

    /* Hash format string */
    if (fmt) {
        const unsigned char *p = (const unsigned char *)fmt;
        while (*p) {
            hash ^= *p++;
            hash *= FNV_PRIME;
        }
    }

    return hash;
}

/**
 * Get elapsed seconds between two timespecs.
 */
static double timespec_diff_sec(const struct timespec *end, const struct timespec *start)
{
    return (double)(end->tv_sec - start->tv_sec) +
           (double)(end->tv_nsec - start->tv_nsec) / 1e9;
}

/**
 * Find or create a category entry.
 */
static suppress_category_t *find_or_create_category(const char *module)
{
    if (!module) return NULL;

    /* Search existing */
    for (int i = 0; i < g_suppress.num_categories; i++) {
        if (strcmp(g_suppress.categories[i].module, module) == 0) {
            return &g_suppress.categories[i];
        }
    }

    /* Create new if space available */
    if (g_suppress.num_categories >= MAX_SUPPRESS_CATEGORIES) {
        return NULL;
    }

    suppress_category_t *cat = &g_suppress.categories[g_suppress.num_categories];
    memset(cat, 0, sizeof(*cat));
    strncpy(cat->module, module, sizeof(cat->module) - 1);
    cat->module[sizeof(cat->module) - 1] = '\0';
    cat->enabled = true;  /* inherit global setting */
    cat->window_sec = g_suppress.default_window_sec;
    cat->threshold = g_suppress.default_threshold;
    g_suppress.num_categories++;

    return cat;
}

int rdk_log_suppression_init(void)
{
    if (g_suppress.initialized) {
        return 0;
    }

    memset(g_suppress.categories, 0, sizeof(g_suppress.categories));
    g_suppress.num_categories = 0;
    g_suppress.default_window_sec = RDK_SUPPRESS_DEFAULT_WINDOW_SEC;
    g_suppress.default_threshold = RDK_SUPPRESS_DEFAULT_THRESHOLD;
    g_suppress.initialized = true;

    return 0;
}

void rdk_log_suppression_deinit(void)
{
    g_suppress.initialized = false;
    g_suppress.num_categories = 0;
}

rdk_SuppressResult rdk_log_suppression_check(const char *module, int level,
                                              const char *fmt,
                                              uint32_t *repeat_count)
{
    if (!g_suppress.initialized || !g_suppress.globally_enabled) {
        return RDK_SUPPRESS_PASS;
    }

    if (!module || !fmt) {
        return RDK_SUPPRESS_PASS;
    }

    suppress_category_t *cat = find_or_create_category(module);
    if (!cat || !cat->enabled) {
        return RDK_SUPPRESS_PASS;
    }

    uint32_t hash = compute_pattern_hash(module, level, fmt);
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    /* Search ring buffer for matching pattern */
    for (int i = 0; i < RDK_SUPPRESS_RING_SIZE; i++) {
        suppress_entry_t *entry = &cat->ring[i];

        if (!entry->active || entry->pattern_hash != hash) {
            continue;
        }

        /* Found matching pattern - check time window */
        double elapsed = timespec_diff_sec(&now, &entry->first_seen);

        if (elapsed > (double)cat->window_sec) {
            /* Window expired - emit summary if we suppressed, then reset */
            if (entry->repeat_count >= cat->threshold) {
                uint32_t suppressed = entry->repeat_count - cat->threshold + 1;
                if (repeat_count) {
                    *repeat_count = suppressed;
                }
                cat->total_summaries++;
                /* Reset entry for new window */
                entry->first_seen = now;
                entry->last_seen = now;
                entry->repeat_count = 1;
                return RDK_SUPPRESS_EMIT_SUMMARY;
            }
            /* Window expired but below threshold - reset */
            entry->first_seen = now;
            entry->last_seen = now;
            entry->repeat_count = 1;
            return RDK_SUPPRESS_PASS;
        }

        /* Within window - increment count */
        entry->repeat_count++;
        entry->last_seen = now;

        if (entry->repeat_count > cat->threshold) {
            /* Suppress this message */
            cat->total_suppressed++;
            if (repeat_count) {
                *repeat_count = entry->repeat_count - cat->threshold;
            }
            return RDK_SUPPRESS_BLOCK;
        }

        /* Below threshold - let it through */
        return RDK_SUPPRESS_PASS;
    }

    /* Pattern not found - add to ring buffer */
    suppress_entry_t *slot = &cat->ring[cat->ring_head];
    
    /* If overwriting an active entry that was suppressing, emit summary */
    if (slot->active && slot->repeat_count >= cat->threshold) {
        /* We lose this suppression context - acceptable trade-off for bounded memory */
    }

    slot->pattern_hash = hash;
    slot->repeat_count = 1;
    slot->first_seen = now;
    slot->last_seen = now;
    slot->active = true;

    cat->ring_head = (cat->ring_head + 1) % RDK_SUPPRESS_RING_SIZE;

    return RDK_SUPPRESS_PASS;
}

int rdk_log_suppression_configure(const char *module, bool enabled,
                                   uint32_t window_sec, uint32_t threshold)
{
    if (!g_suppress.initialized) {
        return -1;
    }

    if (!module) {
        /* Global configuration */
        g_suppress.globally_enabled = enabled;
        if (window_sec > 0) {
            g_suppress.default_window_sec = window_sec;
        }
        if (threshold > 0) {
            g_suppress.default_threshold = threshold;
        }
        return 0;
    }

    suppress_category_t *cat = find_or_create_category(module);
    if (!cat) {
        return -1;
    }

    cat->enabled = enabled;
    if (window_sec > 0) {
        cat->window_sec = window_sec;
    }
    if (threshold > 0) {
        cat->threshold = threshold;
    }

    return 0;
}

int rdk_log_suppression_get_stats(const char *module, rdk_SuppressStats *stats)
{
    if (!g_suppress.initialized || !stats) {
        return -1;
    }

    if (!module) {
        /* Aggregate stats */
        memset(stats, 0, sizeof(*stats));
        for (int i = 0; i < g_suppress.num_categories; i++) {
            stats->total_suppressed += g_suppress.categories[i].total_suppressed;
            stats->total_summaries += g_suppress.categories[i].total_summaries;
        }
        return 0;
    }

    for (int i = 0; i < g_suppress.num_categories; i++) {
        if (strcmp(g_suppress.categories[i].module, module) == 0) {
            stats->total_suppressed = g_suppress.categories[i].total_suppressed;
            stats->total_summaries = g_suppress.categories[i].total_summaries;
            /* Find current active repeat count for this module */
            stats->current_repeat_count = 0;
            for (int j = 0; j < RDK_SUPPRESS_RING_SIZE; j++) {
                if (g_suppress.categories[i].ring[j].active &&
                    g_suppress.categories[i].ring[j].repeat_count > stats->current_repeat_count) {
                    stats->current_repeat_count = g_suppress.categories[i].ring[j].repeat_count;
                }
            }
            return 0;
        }
    }

    return -1;
}

void rdk_log_suppression_reset(const char *module)
{
    if (!g_suppress.initialized) {
        return;
    }

    if (!module) {
        /* Reset all */
        for (int i = 0; i < g_suppress.num_categories; i++) {
            memset(g_suppress.categories[i].ring, 0,
                   sizeof(g_suppress.categories[i].ring));
            g_suppress.categories[i].ring_head = 0;
            g_suppress.categories[i].total_suppressed = 0;
            g_suppress.categories[i].total_summaries = 0;
        }
        return;
    }

    for (int i = 0; i < g_suppress.num_categories; i++) {
        if (strcmp(g_suppress.categories[i].module, module) == 0) {
            memset(g_suppress.categories[i].ring, 0,
                   sizeof(g_suppress.categories[i].ring));
            g_suppress.categories[i].ring_head = 0;
            g_suppress.categories[i].total_suppressed = 0;
            g_suppress.categories[i].total_summaries = 0;
            return;
        }
    }
}

bool rdk_log_suppression_is_enabled(void)
{
    return g_suppress.initialized && g_suppress.globally_enabled;
}
