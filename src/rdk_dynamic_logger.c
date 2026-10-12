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

/**
 * @file rdk_dynamic_logger.c
 */

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>

#include "rdk_dynamic_logger.h"
#include "rdk_debug_priv.h"

#define DL_PORT 12035
#define DL_SIGNATURE "COMC"
#define DL_SIGNATURE_LEN 4

static int g_dl_socket = -1;
extern char *__progname;

static char * rdk_dyn_log_logLevelToString(rdk_LogLevel log_level)
{
    switch(log_level){
        case RDK_LOG_FATAL:  return "FATAL";
        case RDK_LOG_ERROR:  return "ERROR";
        case RDK_LOG_WARN:   return "WARNING";
        case RDK_LOG_NOTICE: return "NOTICE";
        case RDK_LOG_INFO:   return "INFO";
        case RDK_LOG_DEBUG:  return "DEBUG";
        case RDK_LOG_TRACE:  return "TRACE";
        case RDK_LOG_NONE:   return "NONE";
    }
    return "NONE";
}

static void rdk_dyn_log_validate_component_name(const rdk_dynamic_log_message_t *message)
{
    size_t app_len = message->app_length;
    size_t comp_len = message->component_length;
    char comp_name[RDK_DYNAMIC_LOG_COMPONENT_NAME_MAX] = {0};

    if (0 != memcmp(message->signature, DL_SIGNATURE, DL_SIGNATURE_LEN)) {
        return;
    }

    if (app_len == 0 || app_len > sizeof(message->app_name) ||
            __progname == NULL || app_len != strlen(__progname) ||
            0 != memcmp(message->app_name, __progname, app_len)) {
        /* The received msg is not intended for this process */
        return;
    }

    if (comp_len >= sizeof(comp_name))
    {
       fprintf(stderr, "Error: component name too long\n");
       return;
    }

    rdk_LogLevel loggingLevel = (rdk_LogLevel) message->log_level;

    if((loggingLevel >= RDK_LOG_FATAL) && (loggingLevel <= RDK_LOG_NONE))
    {
        memcpy(comp_name, message->component_name, comp_len);
        rdk_dbg_priv_log_reconfig(comp_name, loggingLevel);
        fprintf(stderr, "Log level change request to %s (%u) for the component %s, is success\n", rdk_dyn_log_logLevelToString(loggingLevel), loggingLevel, comp_name);
    }
    else
    {
        fprintf(stderr, "Log level change request with Invalid input (%u)\n", loggingLevel);
    }

    return;
}

void rdk_dyn_log_process_pending_request()
{
    unsigned char packet[RDK_DYNAMIC_LOG_PACKET_MAX];
    rdk_dynamic_log_message_t message = {0};
    struct sockaddr_in sender_addr;
    struct timeval tv;
    int numbytes, ret;
    size_t app_len, comp_len, comp_len_offset;
    socklen_t addr_len;
    fd_set rfds;

    if(-1 == g_dl_socket)
        return;
    memset(&sender_addr,0,sizeof(sender_addr));
    while(1) {
        FD_ZERO(&rfds);
        FD_SET(g_dl_socket, &rfds);
        addr_len = sizeof(sender_addr);

        tv.tv_sec = 0;
        tv.tv_usec = 0;
        ret = select(g_dl_socket+1,&rfds,NULL,NULL,&tv);
        if(ret <= 0)
            break;

        numbytes = recvfrom(g_dl_socket, packet, sizeof(packet),
                    MSG_DONTWAIT | MSG_TRUNC,
                    (struct sockaddr *)&sender_addr, &addr_len);
        if (numbytes == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            fprintf(stderr,"%s recvfrom failed %s\n",__func__,strerror(errno));
            return;
        }

        if (strcmp("127.0.0.1", inet_ntoa(sender_addr.sin_addr)) != 0 ||
                numbytes > (int)sizeof(packet) ||
                numbytes < DL_SIGNATURE_LEN + 4 ||
                (size_t)numbytes != (size_t)packet[DL_SIGNATURE_LEN] + DL_SIGNATURE_LEN + 1 ||
                memcmp(packet, DL_SIGNATURE, DL_SIGNATURE_LEN) != 0)
            continue;

        app_len = packet[DL_SIGNATURE_LEN + 2];
        if (app_len == 0 || app_len > RDK_DYNAMIC_LOG_APP_NAME_MAX ||
                app_len > (size_t)numbytes - 8)
            continue;

        comp_len_offset = DL_SIGNATURE_LEN + 3 + app_len;
        if (comp_len_offset >= (size_t)numbytes)
            continue;
        comp_len = packet[comp_len_offset];
        if (comp_len > RDK_DYNAMIC_LOG_COMPONENT_NAME_MAX ||
                comp_len != (size_t)numbytes - comp_len_offset - 1)
            continue;

        memset(&message, 0, sizeof(message));
        memcpy(message.signature, packet, DL_SIGNATURE_LEN);
        message.message_length = packet[DL_SIGNATURE_LEN];
        message.log_level = packet[DL_SIGNATURE_LEN + 1];
        message.app_length = (uint8_t)app_len;
        memcpy(message.app_name, packet + DL_SIGNATURE_LEN + 3, app_len);
        message.component_length = (uint8_t)comp_len;
        memcpy(message.component_name, packet + comp_len_offset + 1, comp_len);
        rdk_dyn_log_validate_component_name(&message);
    }
}

void rdk_dyn_log_init()
{
    struct sockaddr_in my_addr;
    int opt = 1;

    if ((g_dl_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        fprintf(stderr, "%s() Socket creation failed. %s\n",__func__,strerror(errno));
        return;
    }

    memset(&my_addr,0,sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(DL_PORT);
    my_addr.sin_addr.s_addr = inet_addr("127.255.255.255");

    if (-1 == setsockopt(g_dl_socket,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(int))) {
        fprintf(stderr, "%s() setsockopt error %s\n",__func__,strerror(errno));
        close(g_dl_socket);
        g_dl_socket = -1;
        return;
    }

    if (-1 == bind(g_dl_socket, (struct sockaddr *)&my_addr, sizeof(struct sockaddr))) {
        fprintf(stderr, "%s() bind error %s\n",__func__,strerror(errno));
        close(g_dl_socket);
        g_dl_socket = -1;
        return;
    }

    //fprintf(stderr, "%sg_dl_socket = %d __progname = %s \n",__func__,g_dl_socket,__progname);
}

void rdk_dyn_log_deinit()
{
    if (g_dl_socket != -1)
    {
        close(g_dl_socket);
        g_dl_socket = -1;
    }
}
