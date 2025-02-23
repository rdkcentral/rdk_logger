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
#ifdef FEATURE_SUPPORT_ONBOARD_LOGGING
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "rdk_debug.h"

#define MAX_BUF_SIZE 1024

int main( int argc, char **argv)
{
    char msg[MAX_BUF_SIZE];
    if(argc > 2 && (NULL != argv[1]) && (NULL != argv[2]))
    {
        snprintf(msg,sizeof(msg), "%s\n", argv[2]);
        rdk_logger_log_onboard(argv[1], msg);
    }
    else if(argc > 1 && (NULL != argv[1]))
    {
        snprintf(msg,sizeof(msg), "%s\n", argv[1]);
        rdk_logger_log_onboard(NULL, msg);
    }

    return 0;
}
#endif
