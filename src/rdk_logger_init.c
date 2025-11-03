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
 * @file rdk_logger_init.c
 * This source file contains the APIs for RDK logger initializer.
 */

/**
* @defgroup rdk_logger
* @{
* @defgroup src
* @{
**/


#include <sys/socket.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "rdk_logger.h"
#include "rdk_error.h"
#include "rdk_debug_priv.h"
#include "rdk_dynamic_logger.h"
#include "rdk_utils.h"

#define BUF_LEN 256
static int isLogInited = 0;

/**
 * @brief Sets the default appender type for the given appender.
 *
 * This function maps the provided `rdk_LogAppenderType` to a corresponding
 * appender type string (e.g., "rollingfile", "stream_env") and sets it
 * for the given appender. The appender type determines how log messages
 * are handled (e.g., written to a file, stdout, syslog, etc.).
 *
 * @param[in] app The log4c appender for which the type is to be set.
 * @param[in] appender_type The appender type (e.g., Stdout, FileOutput, Syslog).
 */
void set_default_appender_type(log4c_appender_t* app, rdk_LogAppenderType appender_type)
{
    const char* type_str = NULL;
    switch(appender_type) {
        case Stdout:    type_str = "stream_env"; break;
        case FileOutput:type_str = "rollingfile"; break;
        case Syslog:    type_str = "syslog"; break;
        case Journald:  type_str = "journald"; break;
        default:        type_str = "stream_env"; break;
    }
    const log4c_appender_type_t* type = log4c_appender_type_get(type_str);
    if (app && type) {
        log4c_appender_set_type(app, type);
    }
}

/**
 * @brief Sets the default layout for the given appender.
 *
 * This function maps the provided `rdk_LogLayout` to a corresponding
 * layout string (e.g., "basic", "dated", "comcast_dated") and sets it
 * for the given appender. The layout determines the format of log messages
 * (e.g., timestamp, log level, message content).
 *
 * @param[in] app The log4c appender for which the layout is to be set.
 * @param[in] layout The layout type (e.g., LAYOUT_BASIC, LAYOUT_DATED).
 */
void set_default_layout(log4c_appender_t* app, rdk_LogLayout layout)
{
    const char* layout_str = NULL;
    switch(layout) {
        case LAYOUT_BASIC:         layout_str = "basic"; break;
        case LAYOUT_DATED:         layout_str = "dated"; break;
        case LAYOUT_COMCAST_DATED: layout_str = "comcast_dated"; break;
        default:                   layout_str = "basic"; break;
    }
    log4c_layout_t* layout_obj = log4c_layout_get(layout_str);
    if (layout_obj && app) {
        log4c_appender_set_layout(app, layout_obj);
    }
}

/**
 * @brief Sets the default log level for the given category.
 *
 * This function maps the provided `rdk_LogLevel` to a corresponding
 * log4c priority and sets it for the given category. The log level
 * determines the severity of messages that will be logged (e.g., ERROR, DEBUG).
 * If the category does not exist, it will be created.
 *
 * @param[in] category_name The name of the log4c category.
 * @param[in] log_level The log level to set (e.g., RDK_LOG_ERROR, RDK_LOG_DEBUG).
 */
void set_default_log_level(const char* category_name, rdk_LogLevel log_level)
{
    log4c_category_t* cat = log4c_category_get(category_name);
    if (!cat) {
        cat = log4c_category_new(category_name);
    }
    if (cat) {
        int log4c_prio = rdk_logLevel_to_log4c_priority(log_level);
        log4c_category_set_priority(cat, log4c_prio);
    }
}

/**
 * @brief Initialize the logger. Sets up the environment variable storage by parsing
 * debug configuration file then Initialize the debug support to the underlying platform.
 *
 * @note  Requests not to send SIGPIPE on errors on stream oriented sockets
 * when the other end breaks the connection. The EPIPE error is still returned.
 *
 * @param[in] debugConfigFile The character pointer variable of debug configuration file.
 *
 * @return Returns 0 if initialization of RDK logger module is successful, else it returns -1.
 */
rdk_Error rdk_logger_init(const char* debugConfigFile)
{
    rdk_Error ret;

    if (0 == isLogInited)
    {
        if (NULL == debugConfigFile)
        {
            debugConfigFile = DEBUG_CONF_FILE;
        }

        /* Read the config file & populate pre-configured log levels */
        ret = rdk_logger_parse_config(debugConfigFile);
        if ( RDK_SUCCESS != ret)
        {
            printf("%s:%d Adding debug config file %s failed\n", __FUNCTION__, __LINE__, debugConfigFile);
            return ret;
        }
        /* Perform Logger Internal Init */
        rdk_dbg_init();

        /* Perform Dynamin Logger Internal Init */
        rdk_dyn_log_init();

        /**
         * Requests not to send SIGPIPE on errors on stream oriented
         * sockets when the other end breaks the connection. The EPIPE
         * error is still returned.
         */
        signal(SIGPIPE, SIG_IGN);
        isLogInited = 1;
    }
    return RDK_SUCCESS;
}

rdk_Error rdk_logger_ext_init(const rdk_logger_ext_config_t* config)
 {
    rdk_Error ret;
    ret = RDK_LOGGER_INIT();
    if (ret == RDK_SUCCESS)
    {
        rdk_dbg_priv_ext_init(config->logdir, config->fileName, config->maxCount, config->maxSize);
    }
    return ret;
 }

/**
 * @brief Cleanup the logger instantiation.
 *
 * @return Returns 0 if the call is successful else return -1.
 */
rdk_Error rdk_logger_deinit()
{
    if(isLogInited)
    {
        rdk_dyn_log_deinit();
        rdk_logger_release_config();
        //isLogInited = 0;
    }

    return RDK_SUCCESS;
}
