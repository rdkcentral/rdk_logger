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
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/un.h>

#include "rdk_dynamic_logger.h"
#include "rdk_debug_priv.h"

#define DL_SOCKET_DIR "/tmp/.logger"
#define DL_SIGNATURE "COMC"
#define DL_SIGNATURE_LEN 4

static int g_dl_socket = -1;
static char g_dl_socket_path[sizeof(((struct sockaddr_un *)0)->sun_path)];
static pid_t g_dl_socket_pid = -1;
static pthread_mutex_t g_dl_request_mutex = PTHREAD_MUTEX_INITIALIZER;
extern char *__progname;

static int rdk_dyn_log_make_socket_path(char *path, size_t path_size)
{
    struct stat dir_stat;
    int length;

    if (__progname == NULL || __progname[0] == '\0')
        return -1;

    if (mkdir(DL_SOCKET_DIR, 01777) == 0) {
        /* mkdir honours the caller's umask; force the intended shared-directory mode. */
        if (chmod(DL_SOCKET_DIR, 01777) == -1)
            return -1;
    } else if (errno != EEXIST) {
        return -1;
    }
    if (lstat(DL_SOCKET_DIR, &dir_stat) == -1 || !S_ISDIR(dir_stat.st_mode) ||
            (dir_stat.st_uid != 0 && dir_stat.st_uid != geteuid()) ||
            ((dir_stat.st_mode & (S_IWGRP | S_IWOTH)) && !(dir_stat.st_mode & S_ISVTX)))
        return -1;

    length = snprintf(path, path_size, DL_SOCKET_DIR "/%s.%ld", __progname, (long)getpid());
    if (length < 0 || (size_t)length >= path_size || strchr(__progname, '/') != NULL)
        return -1;

    return 0;
}

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
    if (0 != memcmp(message->signature, DL_SIGNATURE, DL_SIGNATURE_LEN)) {
        return;
    }

    if (__progname == NULL || 0 != strcmp(message->app_name, __progname)) {
        /* The received msg is not intended for this process */
        return;
    }

    rdk_LogLevel loggingLevel = (rdk_LogLevel) message->log_level;

    if((loggingLevel >= RDK_LOG_FATAL) && (loggingLevel <= RDK_LOG_NONE))
    {
	rdk_dbg_priv_log_reconfig(message->component_name, loggingLevel);
        fprintf(stderr, "Log level change request to %s (%u) for the component %s, is success\n", rdk_dyn_log_logLevelToString(loggingLevel), loggingLevel, message->component_name);
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
    struct sockaddr_un sender_addr;
    struct timeval tv;
    int numbytes, ret;
    size_t app_len, comp_len, comp_len_offset;
    socklen_t addr_len;
    fd_set rfds;

    if (pthread_mutex_trylock(&g_dl_request_mutex) != 0)
        return;

    /* After fork/daemonize the inherited socket is named after the parent PID; rebind for ours. */
    if (g_dl_socket != -1 && g_dl_socket_pid != getpid()) {
        close(g_dl_socket);
        g_dl_socket = -1;
        pthread_mutex_unlock(&g_dl_request_mutex);
        rdk_dyn_log_init();
        return;
    }

    if(-1 == g_dl_socket) {
        pthread_mutex_unlock(&g_dl_request_mutex);
        return;
    }
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
            pthread_mutex_unlock(&g_dl_request_mutex);
            return;
        }

        if (numbytes > (int)sizeof(packet) ||
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
	if (comp_len == 0 || comp_len > RDK_DYNAMIC_LOG_COMPONENT_NAME_MAX || comp_len != (size_t)numbytes - comp_len_offset - 1)
            continue;

        memset(&message, 0, sizeof(message));
        memcpy(message.signature, packet, DL_SIGNATURE_LEN);
        message.log_level = packet[DL_SIGNATURE_LEN + 1];
        memcpy(message.app_name, packet + DL_SIGNATURE_LEN + 3, app_len);
        memcpy(message.component_name, packet + comp_len_offset + 1, comp_len);
        rdk_dyn_log_validate_component_name(&message);
    }
    pthread_mutex_unlock(&g_dl_request_mutex);
}

void rdk_dyn_log_init()
{
    struct sockaddr_un my_addr;
    socklen_t addr_len;
    int bind_result;

    pthread_mutex_lock(&g_dl_request_mutex);
    if (rdk_dyn_log_make_socket_path(g_dl_socket_path, sizeof(g_dl_socket_path)) != 0) {
        fprintf(stderr, "%s() invalid or unavailable socket directory/path\n", __func__);
        pthread_mutex_unlock(&g_dl_request_mutex);
        return;
    }

    /* Remove any stale entry; the sticky directory refuses to unlink entries owned by others. */
    if (unlink(g_dl_socket_path) == -1 && errno != ENOENT) {
        fprintf(stderr, "%s() socket path is already in use: %s\n", __func__, strerror(errno));
        pthread_mutex_unlock(&g_dl_request_mutex);
        return;
    }

    if ((g_dl_socket = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
        fprintf(stderr, "%s() Socket creation failed. %s\n",__func__,strerror(errno));
        pthread_mutex_unlock(&g_dl_request_mutex);
        return;
    }

    memset(&my_addr,0,sizeof(my_addr));
    my_addr.sun_family = AF_UNIX;
    memcpy(my_addr.sun_path, g_dl_socket_path, strlen(g_dl_socket_path) + 1);
    addr_len = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + strlen(my_addr.sun_path) + 1);

    bind_result = bind(g_dl_socket, (struct sockaddr *)&my_addr, addr_len);
    if (-1 == bind_result) {
        fprintf(stderr, "%s() bind error %s\n",__func__,strerror(errno));
        close(g_dl_socket);
        g_dl_socket = -1;
        pthread_mutex_unlock(&g_dl_request_mutex);
        return;
    }

    if (chmod(g_dl_socket_path, 0600) == -1) {
        fprintf(stderr, "%s() socket permissions error %s\n",__func__,strerror(errno));
        close(g_dl_socket);
        unlink(g_dl_socket_path);
        g_dl_socket = -1;
        pthread_mutex_unlock(&g_dl_request_mutex);
        return;
    }

    g_dl_socket_pid = getpid();
    pthread_mutex_unlock(&g_dl_request_mutex);
}

void rdk_dyn_log_deinit()
{
    pthread_mutex_lock(&g_dl_request_mutex);
    if (g_dl_socket != -1)
    {
        close(g_dl_socket);
        g_dl_socket = -1;
        /* A forked child must not remove the parent's live socket. */
        if (g_dl_socket_pid == getpid())
            unlink(g_dl_socket_path);
        g_dl_socket_path[0] = '\0';
    }
    pthread_mutex_unlock(&g_dl_request_mutex);
}
