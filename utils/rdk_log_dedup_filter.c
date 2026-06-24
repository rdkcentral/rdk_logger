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
 * @file rdk_log_dedup_filter.c
 * @brief Standalone utility to filter duplicate log lines from log files.
 *
 * Used during nvram sync or log upload to suppress consecutive duplicate
 * log messages, reducing storage and bandwidth usage.
 *
 * Usage:
 *   rdk_log_dedup_filter [--in-place] [input-file]
 *
 * If no input file is specified, reads from stdin.
 * Output is written to stdout unless --in-place is used.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_LINE_LEN 4096
#define TIMESTAMP_PREFIX_LEN 22  /* "YYMMDD-HH:MM:SS.uuuuuu " */

/** FNV-1a constants */
#define FNV1A_OFFSET_BASIS 0x811c9dc5u
#define FNV1A_PRIME        0x01000193u

static uint32_t fnv1a_hash(const char *str)
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
 * Strip the timestamp prefix from a log line for comparison purposes.
 * RDK log format: "YYMMDD-HH:MM:SS.uuuuuu LEVEL ..."
 * Returns pointer to the content after timestamp, or the original line
 * if no timestamp pattern is detected.
 */
static const char* strip_timestamp(const char *line)
{
    /* Check for pattern: YYMMDD-HH:MM:SS */
    if (strlen(line) < 15)
        return line;

    /* Check digits and separators: DDDDDD-DD:DD:DD */
    if (line[6] == '-' && line[9] == ':' && line[12] == ':')
    {
        /* Skip past timestamp and any trailing whitespace/microseconds */
        const char *p = line + 13; /* past HH:MM:SS */
        /* Skip optional microseconds (.uuuuuu) */
        if (*p == '.')
        {
            p++;
            while (*p >= '0' && *p <= '9')
                p++;
        }
        /* Skip whitespace */
        while (*p == ' ')
            p++;
        return p;
    }

    return line;
}

static void print_usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [--in-place] [input-file]\n", prog);
    fprintf(stderr, "  Filters consecutive duplicate log lines.\n");
    fprintf(stderr, "  If no input file, reads from stdin.\n");
    fprintf(stderr, "  Output goes to stdout unless --in-place is used.\n");
}

int main(int argc, char *argv[])
{
    FILE *input = stdin;
    FILE *output = stdout;
    FILE *tmp_output = NULL;
    bool in_place = false;
    const char *input_path = NULL;
    char *tmp_path = NULL;

    char line[MAX_LINE_LEN];
    char first_line[MAX_LINE_LEN];
    uint32_t prev_hash = 0;
    uint32_t repeat_count = 0;
    bool has_prev = false;

    /* Parse arguments */
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--in-place") == 0)
        {
            in_place = true;
        }
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            print_usage(argv[0]);
            return 0;
        }
        else
        {
            input_path = argv[i];
        }
    }

    if (in_place && !input_path)
    {
        fprintf(stderr, "Error: --in-place requires an input file path.\n");
        return 1;
    }

    if (input_path)
    {
        input = fopen(input_path, "r");
        if (!input)
        {
            fprintf(stderr, "Error: Cannot open input file: %s\n", input_path);
            return 1;
        }
    }

    if (in_place)
    {
        size_t tmp_path_len = strlen(input_path) + 8;
        tmp_path = malloc(tmp_path_len);
        if (!tmp_path)
        {
            fprintf(stderr, "Error: Memory allocation failed\n");
            fclose(input);
            return 1;
        }
        snprintf(tmp_path, tmp_path_len, "%s.dedup", input_path);
        tmp_output = fopen(tmp_path, "w");
        if (!tmp_output)
        {
            fprintf(stderr, "Error: Cannot create temp file: %s\n", tmp_path);
            free(tmp_path);
            fclose(input);
            return 1;
        }
        output = tmp_output;
    }

    /* Process lines */
    while (fgets(line, sizeof(line), input) != NULL)
    {
        const char *content = strip_timestamp(line);
        uint32_t hash = fnv1a_hash(content);

        if (has_prev && hash == prev_hash)
        {
            /* Duplicate line - suppress */
            repeat_count++;
        }
        else
        {
            /* Different line - flush any pending duplicates */
            if (has_prev)
            {
                fputs(first_line, output);
                if (repeat_count > 0)
                {
                    fprintf(output, "... repeated %u times\n", repeat_count);
                }
            }

            /* Store this as the new reference line */
            strncpy(first_line, line, sizeof(first_line) - 1);
            first_line[sizeof(first_line) - 1] = '\0';
            prev_hash = hash;
            repeat_count = 0;
            has_prev = true;
        }
    }

    /* Flush final pending line */
    if (has_prev)
    {
        fputs(first_line, output);
        if (repeat_count > 0)
        {
            fprintf(output, "... repeated %u times\n", repeat_count);
        }
    }

    if (input_path)
        fclose(input);

    if (in_place)
    {
        fclose(tmp_output);
        /* Replace original with filtered version */
        if (rename(tmp_path, input_path) != 0)
        {
            fprintf(stderr, "Error: Failed to replace original file\n");
            free(tmp_path);
            return 1;
        }
        free(tmp_path);
    }

    return 0;
}
