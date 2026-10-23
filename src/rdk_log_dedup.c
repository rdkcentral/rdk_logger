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

/**
 * @file rdk_log_dedup.c
 * @brief Duplicate log message suppression engine for rdk_logger.
 *
 * Tracks consecutive identical log messages per category and suppresses
 * duplicates, emitting a summary count when the suppression ends.
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "rdk_log_dedup.h"

/** Static dedup configuration. */
static rdk_dedup_config_t g_dedup_config = {
    .enabled       = false,
    .window_sec    = RDK_DEDUP_DEFAULT_WINDOW_SEC,
    .max_suppress  = RDK_DEDUP_DEFAULT_MAX_SUPPRESS
};

/** Static hash table of dedup slots. */
static rdk_dedup_slot_t g_dedup_table[RDK_DEDUP_TABLE_SIZE];

/** FNV-1a constants */
#define FNV1A_OFFSET_BASIS 0x811c9dc5u
#define FNV1A_PRIME        0x01000193u

/** Maximum formatted message length for hashing. */
#define DEDUP_MSG_BUF_SIZE 512

uint32_t rdk_dedup_fnv1a_hash(const char *str)
{
    uint32_t hash = FNV1A_OFFSET_BASIS;
    if (!str)
        return hash;

    while (*str)
    {
        hash ^= (uint32_t)(unsigned char)(*str);
        hash *= FNV1A_PRIME;
        str++;
    }
    return hash;
}

/**
 * Get the table slot index for a category pointer.
 */
static unsigned int dedup_slot_index(const void *category_key)
{
    /* Use pointer value as hash input */
    uintptr_t ptr = (uintptr_t)category_key;
    return (unsigned int)(ptr >> 4) % RDK_DEDUP_TABLE_SIZE;
}

/**
 * Find or allocate a slot for the given category.
 * Returns NULL if no slot available (all occupied by other categories).
 */
static rdk_dedup_slot_t* dedup_find_slot(const void *category_key)
{
    unsigned int idx = dedup_slot_index(category_key);
    unsigned int i;

    /* Linear probing */
    for (i = 0; i < RDK_DEDUP_TABLE_SIZE; i++)
    {
        unsigned int probe = (idx + i) % RDK_DEDUP_TABLE_SIZE;
        if (g_dedup_table[probe].category_key == category_key)
        {
            return &g_dedup_table[probe];
        }
        if (g_dedup_table[probe].category_key == NULL)
        {
            /* Empty slot - allocate it */
            g_dedup_table[probe].category_key = category_key;
            g_dedup_table[probe].last_msg_hash = 0;
            g_dedup_table[probe].repeat_count = 0;
            g_dedup_table[probe].first_seen = 0;
            g_dedup_table[probe].last_level = 0;
            return &g_dedup_table[probe];
        }
    }
    return NULL; /* Table full */
}

void rdk_dedup_init(void)
{
    memset(g_dedup_table, 0, sizeof(g_dedup_table));
    g_dedup_config.enabled = false;
    g_dedup_config.window_sec = RDK_DEDUP_DEFAULT_WINDOW_SEC;
    g_dedup_config.max_suppress = RDK_DEDUP_DEFAULT_MAX_SUPPRESS;
}

void rdk_dedup_set_config(const rdk_dedup_config_t *config)
{
    if (config)
    {
        g_dedup_config.enabled = config->enabled;
        g_dedup_config.window_sec = config->window_sec;
        g_dedup_config.max_suppress = config->max_suppress;
    }
}

const rdk_dedup_config_t* rdk_dedup_get_config(void)
{
    return &g_dedup_config;
}

bool rdk_dedup_check(const void *category_key, int log4c_prio,
                     const char *format, va_list args)
{
    rdk_dedup_slot_t *slot;
    uint32_t msg_hash;
    char msg_buf[DEDUP_MSG_BUF_SIZE];
    time_t now;
    va_list args_copy;

    if (!g_dedup_config.enabled)
        return false; /* Not suppressed */

    if (!category_key || !format)
        return false;

    /* Format the message for hashing */
    va_copy(args_copy, args);
    vsnprintf(msg_buf, sizeof(msg_buf), format, args_copy);
    va_end(args_copy);

    msg_hash = rdk_dedup_fnv1a_hash(msg_buf);

    slot = dedup_find_slot(category_key);
    if (!slot)
        return false; /* Table full, don't suppress */

    now = time(NULL);

    /* Check if this is a duplicate of the previous message */
    if (slot->repeat_count > 0 && slot->last_msg_hash == msg_hash)
    {
        /* Check window expiry */
        if ((now - slot->first_seen) >= (time_t)g_dedup_config.window_sec)
        {
            /* Window expired - flush summary and allow this message */
            fprintf(stderr, "Previous message repeated %u times\n", slot->repeat_count);
            slot->repeat_count = 0;
            slot->first_seen = now;
            slot->last_msg_hash = msg_hash;
            slot->last_level = log4c_prio;
            return false; /* Don't suppress - allow through after flush */
        }

        /* Check max suppression count */
        if (slot->repeat_count >= g_dedup_config.max_suppress)
        {
            /* Max reached - flush and allow */
            fprintf(stderr, "Previous message repeated %u times\n", slot->repeat_count);
            slot->repeat_count = 0;
            slot->first_seen = now;
            slot->last_msg_hash = msg_hash;
            slot->last_level = log4c_prio;
            return false; /* Don't suppress */
        }

        /* Suppress this duplicate */
        slot->repeat_count++;
        return true;
    }
    else
    {
        /* Different message or first message */
        if (slot->repeat_count > 0)
        {
            /* Emit summary for previous suppression run */
            fprintf(stderr, "Previous message repeated %u times\n", slot->repeat_count);
        }

        /* Record new message state */
        slot->last_msg_hash = msg_hash;
        slot->repeat_count = 1;
        slot->first_seen = now;
        slot->last_level = log4c_prio;
        return false; /* Don't suppress first occurrence */
    }
}

void rdk_dedup_flush(const void *category_key)
{
    rdk_dedup_slot_t *slot;
    unsigned int idx;
    unsigned int i;

    if (!category_key)
        return;

    idx = dedup_slot_index(category_key);

    for (i = 0; i < RDK_DEDUP_TABLE_SIZE; i++)
    {
        unsigned int probe = (idx + i) % RDK_DEDUP_TABLE_SIZE;
        if (g_dedup_table[probe].category_key == category_key)
        {
            slot = &g_dedup_table[probe];
            if (slot->repeat_count > 1)
            {
                fprintf(stderr, "Previous message repeated %u times\n",
                        slot->repeat_count - 1);
            }
            slot->repeat_count = 0;
            return;
        }
        if (g_dedup_table[probe].category_key == NULL)
        {
            return; /* Not found */
        }
    }
}

void rdk_dedup_flush_all(void)
{
    unsigned int i;
    for (i = 0; i < RDK_DEDUP_TABLE_SIZE; i++)
    {
        if (g_dedup_table[i].category_key != NULL && g_dedup_table[i].repeat_count > 1)
        {
            fprintf(stderr, "Previous message repeated %u times\n",
                    g_dedup_table[i].repeat_count - 1);
            g_dedup_table[i].repeat_count = 0;
        }
    }
}
