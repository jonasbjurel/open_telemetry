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
  * Project:			 coreNoStopRTOS/www.......                                                   *
  * Name:				 initfunc.h                                                                  *
  * Created:			 2019-10-18                                                                  *
  * Originator:		     Jonas Bjurel                                                                *
  * Short description:   Defines system init functions to be called at system restart, before        *
  *                      coreNoStopRTOS scheduling has started. These functions will run in to       *
  *                      completion, in series, without any parallelism                              *
  * Resources:           github, WIKI .....                                                          *
  * ************************************************************************************************ */



#ifndef initFunc_HEADER_INCLUDED
	#define initFunc_HEADER_INCLUDED
	#include "init.h"
	#include "../ci/myTestTask.h"

    // Define the list of init functions to be called at system restart
    void (*initFuncs[])() = { myTestTaskInit };

#endif