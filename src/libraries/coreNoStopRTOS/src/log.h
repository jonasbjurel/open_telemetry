/* ************************************************************************************************* *
 * (c) Copyright 2019 Jonas Bjurel (jonasbjurel@hotmail.com)                                         *
 *                                                                                                   *
 * Licensed under the Apache License, Version 2.0 (the "License");                                   *
 * you may not use this file except in compliance with the License.                                  *
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0                *
 *                                                                                                   *
 * Unless required by applicable law or agreed to in writing, software distributed under the License *
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express- *
 * or implied.                                                                                       *
 * See the License for the specific language governing permissions and limitations under the         *
 * licence.                                                                                          *
 *                                                                                                   *
 * ************************************************************************************************* */

 /* ************************************************************************************************ *
  * Project:		    coreNoStopRTOS/www....    ...                                                *
  * Name:				init.h                                                                       *
  * Created:			2019-09-27                                                                   *
  * Originator:			Jonas Bjurel                                                                 *
  * Short description:	Provides a coreNoStopRTOS logging service.                                   *
  *                     Usage: Call macro logdAssert(severity, ...)                                  *
  *							   where severity is one of [_DEBUG_ | _INFO_ | _WARN_ | _ERR_ |  |      *
  *							                             _CRITICAL_ | _PANIC_]                       *
  *					                                     _VARARGS_ sprintf formated message          *
  *																									 *
  * Verbosity			Configurations (Not yet implemented):				    					 *
  *						- Compile configurations                                                     *
							#define _LOG_VERBOSITY_ _DEBUG_ to enable deep verbosity                 *
  *					        #define _LOG_VERBOSITY_ _INFO_  to enable high verbosity                 *
  * 					- Define verbosity level for global scope                                    *
  *						- Define verbosity level per function scope                                  *
  *                                                                                                  *
  * Log Receivers (Not yet implemented):		                                        			 *
  *						- file/sdCard (var/log/syslog                                                *
  *						- serial (standard TTY)                                                      *
  *						- rSyslog (IPv4, port)                                                       *
  *                                                                                                  *
  * Resources:           github, WIKI .....                                                          *
  * ************************************************************************************************ */

#ifndef arduino_HEADER_INCLUDED
	#define arduino_HEADER_INCLUDED
	#include <Arduino.h>
#endif

#ifndef log_HEADER_INCLUDED
	#define log_HEADER_INCLUDED
	
// Debug compile level definintitons
	#define _NORMAL_ 0
	#define _INFO_ 1
	#define _DEBUG_ 2

// Compile level debug level 
	#define _LOG_VERBOSITY_ _INFO_ //[_NORMAL_|_INFO_|_DEBUG_]

	#define __SHORT_FILE__ \
			(strrchr(__FILE__,'\') \
			? strrchr(__FILE__,'\')+1 \
			: __FILE__ \
			)

	#define logdAssert(severity, ...) {sprintf (logstr, "%u %u in function %s, line %u: ", xTaskGetTickCount(), severity,__FUNCTION__, __LINE__);\
                                       Serial.print(logstr);\
                                       sprintf (logstr,__VA_ARGS__);\
                                       Serial.print(logstr);\
									   Serial.print("\n");}

	#define _DEBUG_ 255
    #define _PANIC_ 4
	#define _CRITICAL_ 3
	#define _ERR_ 2
	#define _WARN_ 1
	#define _INFO_ 0

//	void logAssertFunc(TickType_t TickCount, const char* functionName, const char* file, const int line, uint8_t severity, ...);

#endif