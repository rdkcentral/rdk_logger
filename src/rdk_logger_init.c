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
#include <stdatomic.h>
#include <unistd.h>
#include "rdk_logger.h"
#include "rdk_debug_priv.h"
#include "rdk_dynamic_logger.h"

atomic_bool isLogInited = false;

#define DEBUG_1_OVERRIDE_PATH "/opt/debug.ini"
#define DEBUG_2_OVERRIDE_PATH "/nvram/debug.ini"

static void __attribute__((constructor)) _rdk_logger_init (void)
{
    if (!isLogInited)
    {
        char* pConfPath = DEBUG_INI_NAME;

        /* Default Path */
        if (0 == access(DEBUG_1_OVERRIDE_PATH, F_OK))
            pConfPath = DEBUG_1_OVERRIDE_PATH;
        else if (0 == access(DEBUG_2_OVERRIDE_PATH, F_OK))
            pConfPath = DEBUG_2_OVERRIDE_PATH;

        if (RDK_SUCCESS == rdk_logger_init(pConfPath))
            isLogInited = true;
    }

    return;
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
    if (!isLogInited)
    {
        if (NULL == debugConfigFile)
        {
            debugConfigFile = DEBUG_CONF_FILE;
        }

        /* Perform Logger Internal Init */
        rdk_dbg_priv_init(debugConfigFile);

        /* Perform Dynamin Logger Internal Init */
        rdk_dyn_log_init();

        /**
         * Requests not to send SIGPIPE on errors on stream oriented
         * sockets when the other end breaks the connection. The EPIPE
         * error is still returned.
         */
        signal(SIGPIPE, SIG_IGN);
        isLogInited = true;
    }
    return RDK_SUCCESS;
}

rdk_Error rdk_logger_ext_init(const rdk_logger_ext_config_t* config)
{
    _rdk_logger_init();
    if (!isLogInited)
    {
        return RDK_FAILURE;
    }
    else
        return rdk_dbg_priv_ext_init(config);
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
    }

    return RDK_SUCCESS;
}
