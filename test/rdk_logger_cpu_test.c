/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h> 
#include "rdk_logger.h"

typedef struct {
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;
    unsigned long long guest;
    unsigned long long guest_nice;
} CpuInfo;

void getCPUStat(CpuInfo *times) {
    FILE* file = fopen("/proc/stat", "r");
    if (file == NULL) {
        perror("Error opening /proc/stat");
        exit(EXIT_FAILURE);
    }
    char line[256];
    memset(times, 0, sizeof(*times));
    if (fgets(line, sizeof(line), file) != NULL) {
        int parsed = sscanf(line, "cpu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
            &times->user, &times->nice, &times->system, &times->idle,
            &times->iowait, &times->irq, &times->softirq, &times->steal,
            &times->guest, &times->guest_nice);
        if (parsed != 10) {
            fprintf(stderr, "Failed to parse CPU statistics from /proc/stat (parsed %d fields)\n", parsed);
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }
    fclose(file);
}

void PrintCPUMetric(CpuInfo cpu_before, CpuInfo cpu_after, const char* pBuf)
{
    // Calculate the difference in time for each state
    unsigned long long prev_idle = cpu_before.idle + cpu_before.iowait;
    unsigned long long prev_total = prev_idle + cpu_before.user + cpu_before.nice + cpu_before.system +
                                    cpu_before.irq + cpu_before.softirq + cpu_before.steal;

    unsigned long long now_idle = cpu_after.idle + cpu_after.iowait;
    unsigned long long now_total = now_idle + cpu_after.user + cpu_after.nice + cpu_after.system +
                                   cpu_after.irq + cpu_after.softirq + cpu_after.steal;

    unsigned long long diff_idle = now_idle - prev_idle;
    unsigned long long diff_total = now_total - prev_total;

    /* Lets calculate CPU usage percentage. The usage is, 100.0 - (percentage of idle time) */
    float cpu_usage = 0.0f;
    if (diff_total > 0) {
        cpu_usage = (float)(100.0 * (double)(diff_total - diff_idle) / (double)diff_total);
    }

    FILE* file = fopen("cpu.txt", "a+");
    if (file == NULL) {
        perror("Error opening cpu.txt");
        exit(EXIT_FAILURE);
    }
    if (pBuf)
    {
        fprintf(file, "%16s : \t", pBuf);
    }
    
    fprintf(file, "Current CPU usage: %.2f%%\n", cpu_usage);
    fclose(file);

    return;
}

int testLogging()
{
    CpuInfo cpu_before = {0}, cpu_after = {0};
    int i, j;

    char buffer[250] = {0};
    for(j=0; j<249; j++) buffer[j] = 'A' + (j % 26);

    getCPUStat(&cpu_before);
    rdk_logger_ext_config_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.pModuleName = "LOG.RDK.FOO";
    cfg.loglevel = RDK_LOG_INFO;
    cfg.output = RDKLOG_OUTPUT_SYSLOG;
    cfg.format = RDKLOG_FORMAT_WITH_TS_TID;
    cfg.pFilePolicy = NULL;
    rdk_logger_ext_init(&cfg);

    getCPUStat(&cpu_after);
    PrintCPUMetric(cpu_before, cpu_after, "Ext Init");
    usleep(10000);

    // Test 1: Print DEBUG where DEBUG is not enabled.
    // CPU taken to drop the log message
    getCPUStat(&cpu_before);
    for (i = 0; i < 15000; i++)
    {
            rdk_logger_msg_printf(RDK_LOG_DEBUG, "LOG.RDK.FOO", "%s : %d\n", buffer, i);
    }
    getCPUStat(&cpu_after);
    PrintCPUMetric(cpu_before, cpu_after, "Syslog - Debug");
    usleep(10000);

    // Test 2: Print INFO where INFO is enabled.
    // CPU taken to print the log message
    getCPUStat(&cpu_before);
    for (i = 0; i < 15000; i++)
    {
            rdk_logger_msg_printf(RDK_LOG_INFO, "LOG.RDK.FOO", "%s : %d\n", buffer, i);
    }
    getCPUStat(&cpu_after);
    PrintCPUMetric(cpu_before, cpu_after, "Syslog - Info");
    usleep(10000);

    getCPUStat(&cpu_before);
    memset(&cfg, 0, sizeof(cfg));
    cfg.pModuleName = "LOG.RDK.FOO";
    cfg.loglevel = RDK_LOG_INFO;
    cfg.output = RDKLOG_OUTPUT_CONSOLE;
    cfg.format = RDKLOG_FORMAT_WITH_TID;
    cfg.pFilePolicy = NULL;
    rdk_logger_ext_init(&cfg);
    getCPUStat(&cpu_after);
    PrintCPUMetric(cpu_before, cpu_after, "Ext Init");
    usleep(10000);

    // Test 3: Print DEBUG where DEBUG is not enabled.
    // CPU taken to drop the log message
    getCPUStat(&cpu_before);
    for (i = 0; i < 15000; i++)
    {
        rdk_logger_msg_printf(RDK_LOG_DEBUG, "LOG.RDK.FOO", "%s : %d\n", buffer, i);
    }
    getCPUStat(&cpu_after);
    PrintCPUMetric(cpu_before, cpu_after, "Console - Debug");
    usleep(10000);

    // Test 4: Print INFO where INFO is enabled.
    // CPU taken to print the log message
    getCPUStat(&cpu_before);
    for (i = 0; i < 15000; i++)
    {
        rdk_logger_msg_printf(RDK_LOG_INFO, "LOG.RDK.FOO", "%s : %d\n", buffer, i);
    }
    getCPUStat(&cpu_after);
    PrintCPUMetric(cpu_before, cpu_after, "Console - Info");
    return 0;
}

int main ()
{
    CpuInfo cpu_before = {0}, cpu_after = {0};

    // Begin
    getCPUStat(&cpu_before);
    rdk_logger_init(NULL);
    // End
    getCPUStat(&cpu_after);

    /* Print CPU */
    PrintCPUMetric(cpu_before, cpu_after, "Init");

    for (int i = 0; i < 5; i ++)
    {
        testLogging();
        sleep(3);
    }
    return 0;
}
