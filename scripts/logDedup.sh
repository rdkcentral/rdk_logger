#!/bin/sh
##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2026 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################

# logDedup.sh - Deduplicate log files before upload to reduce bandwidth
# Usage: logDedup.sh [log_directory] [threshold]
#
# Processes all .log and .txt files in the specified directory,
# replacing them with deduplicated versions.

LOG_DIR="${1:-/rdklogs/logs}"
THRESHOLD="${2:-2}"
DEDUP_BIN="/usr/bin/rdkLogDedupSync"
DEDUP_COUNT=0

if [ ! -d "$LOG_DIR" ]; then
    echo "Error: Log directory not found: $LOG_DIR"
    exit 1
fi

if [ ! -x "$DEDUP_BIN" ]; then
    echo "Error: Dedup binary not found or not executable: $DEDUP_BIN"
    exit 1
fi

for logfile in "$LOG_DIR"/*.log "$LOG_DIR"/*.txt.*; do
    [ -f "$logfile" ] || continue

    tmpfile="${logfile}.dedup.tmp"

    "$DEDUP_BIN" "$logfile" "$tmpfile" "$THRESHOLD"
    ret=$?

    if [ $ret -eq 0 ]; then
        # Dedup was applied - replace original
        mv "$tmpfile" "$logfile"
        DEDUP_COUNT=$((DEDUP_COUNT + 1))
    elif [ $ret -eq 2 ]; then
        # No dedup needed - tmp already removed by utility
        :
    else
        # Error
        rm -f "$tmpfile" 2>/dev/null
        echo "Warning: Dedup failed for $logfile"
    fi
done

echo "Log dedup complete: $DEDUP_COUNT files deduplicated in $LOG_DIR"
exit 0
