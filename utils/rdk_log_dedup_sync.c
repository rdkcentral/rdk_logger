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

/**
 * @file rdk_log_dedup_sync.c
 * @brief Pre-upload log deduplication utility.
 *
 * Reads a log file, identifies consecutive duplicate lines,
 * and writes a deduplicated version with "repeated N times" annotations.
 * Intended to be run before log files are uploaded to reduce bandwidth.
 *
 * Usage: rdk_log_dedup_sync <input_file> <output_file> [threshold]
 * Exit codes: 0 = success, 1 = error, 2 = no changes needed

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_LINE_LEN 4096
#define DEFAULT_THRESHOLD 2

static void print_usage(const char *progname)
{
    fprintf(stderr, "Usage: %s <input_file> <output_file> [threshold]\n", progname);
    fprintf(stderr, "  input_file:  Path to log file to deduplicate\n");
    fprintf(stderr, "  output_file: Path to write deduplicated output\n");
    fprintf(stderr, "  threshold:   Minimum repeats before suppression (default: %d)\n",
            DEFAULT_THRESHOLD);
}

int main(int argc, char *argv[])
{
    if (argc < 3 || argc > 4) {
        print_usage(argv[0]);
        return 1;
    }

    const char *input_path = argv[1];
    const char *output_path = argv[2];
    int threshold = DEFAULT_THRESHOLD;

    if (strcmp(input_path, output_path) == 0) {
        fprintf(stderr, "Error: input_file and output_file must be different paths\n");
        return 1;
    }

    if (argc == 4) {
        threshold = atoi(argv[3]);
        if (threshold < 1) {
            fprintf(stderr, "Error: threshold must be >= 1\n");
            return 1;
        }
    }

    FILE *fin = fopen(input_path, "r");
    if (!fin) {
        fprintf(stderr, "Error: Cannot open input file: %s\n", input_path);
        return 1;
    }

    struct stat in_st;
    mode_t out_mode = S_IRUSR | S_IWUSR;
    if (stat(input_path, &in_st) == 0) {
        out_mode = in_st.st_mode & 0777;
    }

    int fd_out = open(output_path, O_WRONLY | O_CREAT | O_TRUNC, out_mode);
    if (fd_out < 0) {
        fprintf(stderr, "Error: Cannot open output file: %s\n", output_path);
        fclose(fin);
        return 1;
    }

    FILE *fout = fdopen(fd_out, "w");
    if (!fout) {
        fprintf(stderr, "Error: Cannot open output file stream: %s\n", output_path);
        close(fd_out);
        fclose(fin);
        return 1;
    }

    char current_line[MAX_LINE_LEN];
    char prev_line[MAX_LINE_LEN] = {0};
    int repeat_count = 0;
    bool has_prev = false;
    bool any_dedup = false;

    while (fgets(current_line, sizeof(current_line), fin)) {
        if (has_prev && strcmp(current_line, prev_line) == 0) {
            /* Duplicate line */
            repeat_count++;
        } else {
            /* Different line - flush previous */
            if (has_prev) {
                fputs(prev_line, fout);
                if (repeat_count >= threshold) {
                    fprintf(fout, "--- Previous message repeated %d times ---\n",
                            repeat_count);
                    any_dedup = true;
                } else {
                    /* Below threshold: write out all repeats */
                    for (int i = 0; i < repeat_count; i++) {
                        fputs(prev_line, fout);
                    }
                }
            }
            strncpy(prev_line, current_line, sizeof(prev_line) - 1);
            prev_line[sizeof(prev_line) - 1] = '\0';
            repeat_count = 0;
            has_prev = true;
        }
    }

    /* Flush last line */
    if (has_prev) {
        fputs(prev_line, fout);
        if (repeat_count >= threshold) {
            fprintf(fout, "--- Previous message repeated %d times ---\n",
                    repeat_count);
            any_dedup = true;
        } else {
            for (int i = 0; i < repeat_count; i++) {
                fputs(prev_line, fout);
            }
        }
    }

    fclose(fin);
    fclose(fout);

    if (!any_dedup) {
        /* No deduplication was needed - remove output and signal caller */
        if (remove(output_path) != 0) {
            fprintf(stderr, "Error: Failed to remove unchanged output file: %s\n", output_path);
            return 1;
        }
        return 2;
    }

    return 0;
}
