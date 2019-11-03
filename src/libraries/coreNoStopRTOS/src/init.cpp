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

/* ************************************************************************************************* *
 * Project:		        coreNoStopRTOS/www.......                                                    *
 * Name:				init.cpp                                                                     *
 * Created:			    2019-09-27                                                                   *
 * Originator:			Jonas Bjurel                                                                 *
 * Short description:	Provides a FreeRtos shim-layer for nearly non-stop real time systems with    *
 *                      the prime use-case for model air-craft on-board systems.                     *
 * Resources:           github, WIKI .....                                                           *
 * ************************************************************************************************* */

#include "init.h"
#include "initfunc.h"

/* ********************************************************************************************* */
/* Init static definitions                                                                       */

tasks initd;
bool tasks::tasksObjExist = false; //TODO Test that tasks cant be instantiated twisse
bool tasks::schedulerRunning = false;
//static void* initFunc[MAX_NO_OF_INIT_FUNCTIONS] ???? DOES THIS NEED TO BE HERE OR IN THE CLASS?
uint8_t tasks::noOfTasks = 0;
task_desc_t* tasks::classTasktable[MAX_NO_OF_STATIC_TASKS];
SemaphoreHandle_t tasks::globalInitMutex;
TimerHandle_t tasks::watchDogTimer;
uint32_t tasks::watchdogOverruns = 0; 
uint32_t tasks::watchdogTaskCnt = 0;
uint32_t tasks::hunderedMsCnt = 0;
uint32_t tasks::secCnt = 0;
uint32_t tasks::tenSecCnt = 0;
uint32_t tasks::minCnt = 0;
uint32_t tasks::hourCnt = 0;
SemaphoreHandle_t tasks::watchdogMutex;
char logstr[320]; //TODO: move to log.c and log.h

/* ********************************************************************************************* */
/* Init system construction, initiation and dsestruction                                         */
/* Description: tasks constructor	                                                             */
/*                                                                                               */
tasks::tasks() {
	if (tasksObjExist) {
		logdAssert(_PANIC_, "Someone tried to create a second tasks class object - RESTARTING THE SYSTEM...");
		ESP.restart();
	}
	tasksObjExist = true;
}

/* ********************************************************************************************* */
/* Init system initialization                                                                    */
/* Description: Initialize all variables, resources and calls user init functions registered in  */
/* initfunc.h                                                                                    */
/*                                                                                               */
void tasks::startInit(void) {
	globalInitMutex = xSemaphoreCreateRecursiveMutex();
	logdAssert(_INFO_, "System is being started, initializing initd...");
	//logdAssert(_INFO_, "Initiating user init tasks...");
	//logdAssert(_INFO_, "Size of init tasks is %u", sizeof((void*)initFuncs) / sizeof((void*)initFuncs[0]));
	for (uint8_t i = 0; i < sizeof((void*)initFuncs)/ sizeof((void*)initFuncs[0]); i++) {
		//logdAssert(_DEBUG_, "Initfunction i %u will be ran", i);
		initFuncs[i]();
		//logdAssert(_DEBUG_, "Initfunction i %u ran...", i);
	}
	logdAssert(_INFO_, "All user init functions ran...");
	if (uint8_t res = initd.startAllTasks(false, true)) { //start all registered static tasks without incrementing restartCnt, but inititiating dynamic statistics data.
		logdAssert(_PANIC_, "Could not start all registered static tasks, cause %u  - RESTARTING THE SYSTEM...", res);
		ESP.restart();
	}
	logdAssert(_INFO_, "All registered static tasks have been started");
	watchdogMutex = xSemaphoreCreateMutex();
	watchDogTimer =  xTimerCreate("watchDogTimer", pdMS_TO_TICKS(_TASK_WATCHDOG_MS_), pdTRUE, (void*)0, runTaskWatchdogs); //Change back to pdTRUE
	if (watchDogTimer == NULL) {
		logdAssert(_PANIC_, "Could not get a initd watchdog timer - RESTARTING THE SYSTEM...");
		ESP.restart();
	}
	schedulerRunning = true;
	logdAssert(_INFO_, "initd scheduler has started");

	if (xTimerStart(watchDogTimer, 0) != pdPASS) {
		ESP.restart();
	}
	logdAssert(_INFO_, "Watchdog started");
	//Assert System OK indication callback
}

/* ********************************************************************************************* */
/* Init system destructor                                                                        */
/* Description: Not supported, will cause system panic and restart                               */
/*                                                                                               */
tasks::~tasks() {
	logdAssert(_PANIC_, "Someone is trying to destruct initd - RESTARTING THE SYSTEM...");
	ESP.restart();
}

/* ****************************************** Task control ***************************************** */
/* Init system task controling methods.                                                              */
/* Description: start/stop tasks with various properties and task pointing methods.	                 */
/*                                                                                                   */
/* Public:                                                                                           */
/* uint8_t startStaticTask(......) - Start or register a static task monitored by init               */
/* TaskHandle_t tasks::startDynamicTask(...) - Start a dynamic task not at all monitored by init     */
/*                                                                                                   */
/* Private:                                                                                          */
/* uint8_t startAllTasks(...) - start all static tasks previously registered with startStaticTask    */ 
/* uint8_t stopAllTasks() - stop all static tasks previously registered with startStaticTask         */
/* uint8_t startTaskByName(...) - start a static task by name registered with startStaticTask        */
/* uint8_t stopTaskByName(...) - stop a static task by name registered with startStaticTask          */
/* uint8_t startTaskByTid(...) - start a static task by its task ID                                  */
/* uint8_t stopTaskByTid(...) - stop a static task by its task ID                                    */
/* uint8_t startTaskByTaskDesc(...) - start a static task by task descriptor prev. registerd         */
/* uint8_t stopTaskByTaskDesc(...) - stop a static task by task descriptor prev. registerd           */
/* ------------------------------------------------------------------------------------------------- */


/* ------------------------------------- PUBLIC:startStaticTask -------------------------------------*/
/* uint8_t startStaticTask(TaskFunction_t taskFunc, char taskName[20], void* taskParams,             */
/*                         uint32_t stackDepth, UBaseType_t taskPrio, BaseType_t taskPinning,        */
/*                         uint32_t watchdogTimeout, uint8_t watchdogWarnPerc,                       */
/*                         uint8_t stackWarnPerc, uint32_t heapWarnKB, uint8_t restartEscalationCnt) */
/*	Description: This method registers and starts a static task. If the this method is called before */
/*               the init scheduler has been started it will register the task and start it when     */
/*               once the init scheduler is started. If the scheduler is already running it will     */
/*               register the task and immediately start it.                                         */
/*               The task must never return, but it might kill it self usning vTaskDelete(NULL)      */
/*               The task may be monitored by init using watchdog.                                   */
/*               Dynamic resources allocated by the task such as memory (taskMalloc/taskMfree)       */
/*               such that these resources are tidied up when a task has crashed or being deleted.   */
/*               malloc/free should not be used. Neither should other methods creating dynamic       */
/*               resources                                                                           */
/*  Properties:  Public method.                                                                      */
/*               This method is thread safe.                                                         */
/*  Return codes:[_TASKS_OK_= 0 | _TASKS_RESOURCES_EXHAUSTED | _TASKS_API_PARAMETER_ERROR_ |         */
/*               | _TASKS_GENERAL_ERROR_, _TASKS_NOT_FOUND_ | _TASKS_MUTEX_TIMEOUT_]                 */
/*  Parameters:  TaskFunction_t taskFunc - The task function to be called                            */
/*               char taskName[20] - A textual name of the task                                      */
/*               void* taskParams - Parameters to be passed to the task function                     */
/*               uint32_t stackDepth - The stack depth provided to the task                          */
/*               UBaseType_t taskPrio - Priority - higher numbers - higher prio.                     */
/*               BaseType_t taskPinning - pinning to a core [0|1|tskNO_AFFINITY]                     */
/*               uint32_t watchdogTimeout - Number of watchdog runs (without watchdog kicking)       */
/*                                          before the init watchdog restarts the task               */
/*                                          in ms: _TASK_WATCHDOG_MS_ * watchdogTimeout              */
/*                                          In case set to zero, the task will not be monitored      */
/*               uint8_t watchdogWarnPerc - Warning level for the watchdog [%]                       */
/*               uint8_t stackWarnPerc - Stack warning level [%]                                     */
/*               uint32_t heapWarnKB - Warning level for heap usage [KB]                             */
/*               uint8_t restartEscalationCnt - Task restarts leading up to a system panic/restart   */
/*                                              The restartEscalationCnt will be decreased every     */
/*                                              minute. If set to zero, task restart never escalates */
/*                                              to a system panic/restart                            */
/*                                                                                                   */

uint8_t tasks::startStaticTask(TaskFunction_t taskFunc, const char* taskName, void* taskParams, uint32_t stackDepth, UBaseType_t taskPrio,
	                    BaseType_t taskPinning, uint32_t watchdogTimeout, uint8_t watchdogWarnPerc, uint8_t stackWarnPerc,
	                    uint32_t heapWarnKB, uint8_t restartEscalationCnt) {

	globalInitMutexTake();
	classTasktable[noOfTasks] = (task_desc_t*)malloc(sizeof(task_desc_t));
	classTasktable[noOfTasks]->pvTaskCode = taskFunc;
	classTasktable[noOfTasks]->pcName = (char*) malloc(20);
	strcpy(classTasktable[noOfTasks]->pcName, taskName);
	classTasktable[noOfTasks]->pvParameters = taskParams;
	classTasktable[noOfTasks]->usStackDepth = stackDepth;
	classTasktable[noOfTasks]->uxPriority = taskPrio;
	classTasktable[noOfTasks]->xCoreID = taskPinning;
	classTasktable[noOfTasks]->WatchdogTimeout = watchdogTimeout;
	classTasktable[noOfTasks]->watchdogHighWatermarkWarnPerc = watchdogWarnPerc;
	classTasktable[noOfTasks]->stackHighWatermarkWarnPerc = stackWarnPerc;
	classTasktable[noOfTasks]->heapHighWatermarkKBWarn = heapWarnKB;
	classTasktable[noOfTasks]->escalationRestartCnt = restartEscalationCnt;
	classTasktable[noOfTasks]->pvCreatedTask = NULL;
	classTasktable[noOfTasks]->taskWatchDogMutex = xSemaphoreCreateRecursiveMutex();
	classTasktable[noOfTasks]->dynMemObjects_p = NULL;
	
	if (schedulerRunning) {
		if (uint8_t res = startTaskByTaskDesc(classTasktable[noOfTasks], false, true)) { //Start a static task while the scheduler is running
			logdAssert(_ERR_, "Tasks name: %s, tid: %d didn't start, cause: %u", classTasktable[noOfTasks]->pcName, noOfTasks, res);
			globalInitMutexGive();
			return(res);
		}
		logdAssert(_INFO_, "Static task  %s, tid: %u has been started", classTasktable[noOfTasks]->pcName, noOfTasks);
	}
	noOfTasks++;
	globalInitMutexGive();
	return(_TASKS_OK_);
}
/* -------------------------------------- END:startStaticTask ---------------------------------------*/

/* ------------------------------------ PUBLIC:startDynamicTask -------------------------------------*/
/* TaskHandle_t tasks::startDynamicTask(TaskFunction_t taskFunc, char taskName[20], void* taskParams,*/
/*                                      uint32_t stackDepth, UBaseType_t taskPrio,                   */
/*                                      BaseType_t taskPinning                                       */
/* Description:  This method starts a dynamic task which is not tracked or monitored by the init     */
/*               system in any way. If the this method is called before the init scheduler has been  */
/*               started the task will be started once the init scheduler is started.                */
/*               The task must never return, but it might kill it self usning vTaskDelete(NULL)      */
/*               Since dynamic tasks are not monitored, and any resources created inside are not     */
/*               not tracked - it is a good practice to allocate dynamic resources with dynamic tasks*/
/*  Properties:  Public method.                                                                      */
/*               This method is thread safe.                                                         */
/*  Return:      Returns the task handle [ TaskHandle_t]                                             */
/*  Parameters:  TaskFunction_t taskFunc - The task function to be called                            */
/*               char taskName[20] - A textual name of the task                                      */
/*               void* taskParams - Parameters to be passed to the task function                     */
/*               uint32_t stackDepth - The stack depth provided to the task                          */
/*               UBaseType_t taskPrio - Priority - higher numbers - higher prio.                     */
/*               BaseType_t taskPinning - pinning to a core [0|1|tskNO_AFFINITY]                     */
/*                                                                                                   */
TaskHandle_t tasks::startDynamicTask(TaskFunction_t taskFunc, char taskName[20], void* taskParams, uint32_t stackDepth, UBaseType_t taskPrio,
	BaseType_t taskPinning) {
	TaskHandle_t dynamicTaskHandle;

	globalInitMutexTake();
	if (xTaskCreatePinnedToCore(taskFunc, taskName, stackDepth, taskParams, taskPrio, &dynamicTaskHandle, taskPinning) != pdPASS) {
		logdAssert(_ERR_, "General error, the dynamic task %s task was probably not started", taskName);
		globalInitMutexGive();
		return(NULL);
	}
	//logdAssert(_DEBUG_, "Dynamic task %s task has been started", taskName);
	globalInitMutexGive();
	return(dynamicTaskHandle);
}
/* --------------------------------------- END:startDynamicTask -------------------------------------*/

/* -------------------------------------- PRIVATE:startAllTasks -------------------------------------*/
/* uint8_t tasks::startAllTasks(bool incrementRestartCnt, bool initiateStat)                         */
/* Description:  Start all static tasks that previously registered with                              */
/*               uint8_t startStaticTask(...)                                                        */
/*  Properties:  Privat method.                                                                      */
/*               This method is thread safe.                                                         */
/*  Return codes:[_TASKS_OK_= 0 | _TASKS_RESOURCES_EXHAUSTED | _TASKS_API_PARAMETER_ERROR_ |         */
/*               | _TASKS_GENERAL_ERROR_, _TASKS_NOT_FOUND_ | _TASKS_MUTEX_TIMEOUT_]                 */
/*  Parameters:  bool incrementRestartCnt - if true, each tasks restart cnt is incremented           */
/*               bool initiateStat - if true each tasks stats are reset                              */
/*                                                                                                   */
uint8_t tasks::startAllTasks(bool incrementRestartCnt, bool initiateStat) {
	globalInitMutexTake();
	logdAssert(_INFO_, "%u static tasks has been registered, and will be started/restarted", noOfTasks);
	for (uint8_t i = 0; i < noOfTasks; i++) {
		//logdAssert(_INFO_, "Tasks name: %s, tid: %u is being initiated", classTasktable[i]->pcName, i);
		//logdAssert(_DEBUG_, "Tasks name: %s, tid: %u is being initiated with param %u", classTasktable[i]->pcName, i, *((uint8_t*)(classTasktable[i]->pvParameters)));
		if (uint8_t res = startTaskByTaskDesc(classTasktable[i], incrementRestartCnt, initiateStat)) {
			globalInitMutexGive();
			logdAssert(_ERR_, "Tasks name: %s, tid: %d didn't start, cause: %u", classTasktable[i]->pcName, i, res);
			return(_TASKS_GENERAL_ERROR_);
			break;
		}
		//logdAssert(_DEBUG_, "Tasks name: %s, tid: %u was successfully started/restarted", classTasktable[i]->pcName, i);
	}
	globalInitMutexGive();
	return(_TASKS_OK_);
}
/* ---------------------------------------- END:startAllTasks ---------------------------------------*/

/* -------------------------------------- PRIVATE:stopAllTasks --------------------------------------*/
/* uint8_t tasks::stopAllTasks(bool incrementRestartCnt, bool initiateStat)                          */
/* Description:  Stop all static tasks that previously registered with                               */
/*               uint8_t startStaticTask(...)                                                        */
/*  Properties:  Privat method.                                                                      */
/*               This method is thread safe.                                                         */
/*  Return codes:[_TASKS_OK_= 0 | _TASKS_RESOURCES_EXHAUSTED | _TASKS_API_PARAMETER_ERROR_ |         */
/*               | _TASKS_GENERAL_ERROR_, _TASKS_NOT_FOUND_ | _TASKS_MUTEX_TIMEOUT_]                 */
/*  Parameters:  -                                                                                   */
/*                                                                                                   */
uint8_t tasks::stopAllTasks() {
	globalInitMutexTake();
	//logdAssert(_DEBUG_, "Stoping all tasks");
	for (uint8_t i = 0; i < noOfTasks; i++) {
		//logdAssert(_DEBUG_, "Trying to stop taskid: %d, with task name: %c", i, classTasktable[i]->pcName);
		if (uint8_t res = stopTaskByTid(i)) {
			logdAssert(_WARN_, "Stoping taskid: %d, with task name: %c failed with cause: %d", i, classTasktable[i]->pcName, res);
			globalInitMutexGive();
			return(res);
			break;
		}
		//logdAssert(_DEBUG_, "Taskid: %d, with task name: %c stopped", i, classTasktable[i]->pcName);
	}
	globalInitMutexGive();
	return(_TASKS_OK_);
}
/* ---------------------------------------- END:stopAllTasks ----------------------------------------*/

/* ------------------------------------ PRIVATE:startTaskByName -------------------------------------*/
/* uint8_t startTaskByName(char* pcName, bool incrementRestartCnt, bool initiateStat)                */
/* Description:  start a static task by its name previously registered with                          */
/*               uint8_t startStaticTask(...)                                                        */
/*  Properties:  Privat method.                                                                      */
/*               This method is thread safe.                                                         */
/*  Return codes:[_TASKS_OK_= 0 | _TASKS_RESOURCES_EXHAUSTED | _TASKS_API_PARAMETER_ERROR_ |         */
/*               | _TASKS_GENERAL_ERROR_, _TASKS_NOT_FOUND_ | _TASKS_MUTEX_TIMEOUT_]                 */
/*  Parameters:  char* pcName - the textual name of the static function                              */
/*               bool incrementRestartCnt - if true, each tasks restart cnt is incremented           */
/*               bool initiateStat - if true each tasks stats are reset                              */
/*                                                                                                   */

uint8_t tasks::startTaskByName(char* pcName, bool incrementRestartCnt, bool initiateStat) {
	task_desc_t* task_p;
	globalInitMutexTake();
	//logdAssert(_DEBUG_, "Trying to start/restart taskname: %c", pcName);
	if (uint8_t res = getTaskDescByName(pcName, &task_p)) {
		logdAssert(_ERR_, "Failed to get task descriptor by name: %s, Cause: %d", pcName, res);
		globalInitMutexGive();
		return(res);
	}
	
	if(uint8_t res = startTaskByTaskDesc(task_p, incrementRestartCnt, initiateStat)) {
		logdAssert(_ERR_, "Failed to start task by name: %s, Cause: %d", pcName, res);
		globalInitMutexGive();
		return(res);
	}
	globalInitMutexGive();
	//logdAssert(_INFO_, "Task: %s started/restarted", pcName);
	return(_TASKS_OK_);
}
/* -------------------------------------- END:startTaskByName ---------------------------------------*/

/* ------------------------------------- PRIVATE:stopTaskByName -------------------------------------*/
/* uint8_t tasks::stopTaskByName(char* pcName)                                                       */
/* Description:  stop a static task by its name previously registered with                           */
/*               uint8_t startStaticTask(...)                                                        */
/*  Properties:  Privat method.                                                                      */
/*               This method is thread safe.                                                         */
/*  Return codes:[_TASKS_OK_= 0 | _TASKS_RESOURCES_EXHAUSTED | _TASKS_API_PARAMETER_ERROR_ |         */
/*               | _TASKS_GENERAL_ERROR_, _TASKS_NOT_FOUND_ | _TASKS_MUTEX_TIMEOUT_]                 */
/*  Parameters:  char* pcName - the textual name of the static function                              */
/*                                                                                                   */
uint8_t tasks::stopTaskByName(char* pcName) {
	task_desc_t* task_p;
	//logdAssert(_INFO_, "Trying to stop task name: %s", task->pcName);
	globalInitMutexTake();
	if (uint8_t res = getTaskDescByName(pcName, &task_p)) {
		globalInitMutexGive();
		logdAssert(_ERR_, "Could not find a taske with name: %s", task_p->pcName);
		return(res);
	}
	if (uint8_t res = stopTaskByTaskDesc(task_p)) {
		globalInitMutexGive();
		logdAssert(_ERR_, "Could not stop task: %s", task_p->pcName);
		return(res);
	}
	else {
		globalInitMutexGive();
		//logdAssert(_INFO_, "Task name: %s stopped", task_p->pcName);
		return(_TASKS_OK_);
	}
}
/* --------------------------------------- END:stopTaskByName ---------------------------------------*/

/* ------------------------------------- PRIVATE:startTaskByTid -------------------------------------*/
/* uint8_t startTaskByTid(uint8_t tid, bool incrementRestartCnt, bool initiateStat)                  */
/* Description:  start a static task by its task id previously registered with                       */
/*               uint8_t startStaticTask(...)                                                        */
/*  Properties:  Privat method.                                                                      */
/*               This method is thread safe.                                                         */
/*  Return codes:[_TASKS_OK_= 0 | _TASKS_RESOURCES_EXHAUSTED | _TASKS_API_PARAMETER_ERROR_ |         */
/*               | _TASKS_GENERAL_ERROR_, _TASKS_NOT_FOUND_ | _TASKS_MUTEX_TIMEOUT_]                 */
/*  Parameters:  uint8_t tid - the task ID                                                           */
/*               bool incrementRestartCnt - if true, each tasks restart cnt is incremented           */
/*               bool initiateStat - if true each tasks stats are reset                              */
/*                                                                                                   */
uint8_t tasks::startTaskByTid(uint8_t tid, bool incrementRestartCnt, bool initiateStat) {
	//logdAssert(_INFO_, "Trying to start/restart task tid: %u", tid);
	if (tid >= noOfTasks) {
		logdAssert(_ERR_, "Parameter error, tid: %u", tid);
		return(_TASKS_API_PARAMETER_ERROR_);
	}
	taskInitMutexTake(classTasktable[tid]);
	if (uint8_t res = startTaskByTaskDesc(classTasktable[tid], incrementRestartCnt, initiateStat)) {
		logdAssert(_ERR_, "Could not start task by descriptor, tid: %u, cause: %u", tid, res);
		return (res);
	}
	taskInitMutexGive(classTasktable[tid]);
	//logdAssert(_INFO_, "Task tid: %u started/restarted", tid);
	return(_TASKS_OK_);
}
/* --------------------------------------- END:startTaskByTid ---------------------------------------*/

/* -------------------------------------- PRIVATE:stopTaskByTid -------------------------------------*/
/* uint8_t stopTaskByTid(uint8_t tid)                                                                */
/* Description:  stops a static task by its task id previously registered with                       */
/*               uint8_t startStaticTask(...)                                                        */
/*  Properties:  Privat method.                                                                      */
/*               This method is thread safe.                                                         */
/*  Return codes:[_TASKS_OK_= 0 | _TASKS_RESOURCES_EXHAUSTED | _TASKS_API_PARAMETER_ERROR_ |         */
/*               | _TASKS_GENERAL_ERROR_, _TASKS_NOT_FOUND_ | _TASKS_MUTEX_TIMEOUT_]                 */
/*  Parameters:  uint8_t tid - the task ID                                                           */
/*                                                                                                   */
uint8_t tasks::stopTaskByTid(uint8_t tid) {
	//logdAssert(_INFO_, "Trying to stop task tid: %u", tid);
	if (tid >= noOfTasks) {
		logdAssert(_ERR_, "Parameter error, tid: %u", tid);
		return(_TASKS_API_PARAMETER_ERROR_);
	}
	taskInitMutexTake(classTasktable[tid]);
	if (uint8_t res = stopTaskByTaskDesc(classTasktable[tid])) {
		taskInitMutexGive(classTasktable[tid]);
		logdAssert(_ERR_, "Could not stop task tid: %u", tid);
		return(res);
	}
	else {
		taskInitMutexGive(classTasktable[tid]);
		//logdAssert(_INFO_, "Task tid %u stopped", tid);
		return(_TASKS_OK_);
	}
}
/* ---------------------------------------- END:stopTaskByTid ---------------------------------------*/

/* ----------------------------------- PRIVATE:startTaskByTaskDec -----------------------------------*/
/* uint8_t startTaskByTaskDesc(task_desc_t* task_p, bool incrementRestartCnt,                        */
/*                             bool initiateStat)                                                    */
/* Description:  starts a static task by its task descriptor previously registered with              */
/*               uint8_t startStaticTask(...)                                                        */
/*               This is the prime method to start a dynamic task, all other methods are in the end  */
/*               using this method to start a static task                                            */
/*  Properties:  Privat method.                                                                      */
/*               This method is thread safe.                                                         */
/*  Return codes:[_TASKS_OK_= 0 | _TASKS_RESOURCES_EXHAUSTED | _TASKS_API_PARAMETER_ERROR_ |         */
/*               | _TASKS_GENERAL_ERROR_, _TASKS_NOT_FOUND_ | _TASKS_MUTEX_TIMEOUT_]                 */
/*  Parameters:  task_desc_t* task_p - task descriptor                                               */
/*               bool incrementRestartCnt - if true, each tasks restart cnt is incremented           */
/*               bool initiateStat - if true each tasks stats are reset                              */
/*                                                                                                   */
uint8_t tasks::startTaskByTaskDesc(task_desc_t* task_p, bool incrementRestartCnt, bool initiateStat) {
	//logdAssert(_DEBUG_, "Start task: %s by descriptor %p", task_p->pcName, task_p);
	if (task_p == NULL) {
		logdAssert(_ERR_, "Parameter error - task_p=NULL");
		return(_TASKS_API_PARAMETER_ERROR_);
	}
	taskInitMutexTake(task_p);
	if (!checkIfTaskAlive(task_p)) {
		if (uint8_t res = stopTaskByTaskDesc(task_p)) {
			logdAssert(_ERR_, "Could not stop task: %s", task_p->pcName);
			return(_TASKS_GENERAL_ERROR_);
		}
	}

	//logdAssert(_DEBUG_, "Check if incrementing restart cnt");
	if (incrementRestartCnt) {
		if (task_p->escalationRestartCnt) {
			//logdAssert(_DEBUG_, "Incrementing restart cnt");
			++(task_p->restartCnt);
			if (task_p->escalationRestartCnt) {
				if (task_p->restartCnt >= task_p->escalationRestartCnt) {
					logdAssert(_PANIC_, "Task: %s has frequently restarted, escalating to system restart...", task_p->pcName);
					ESP.restart();
				}
			}
			else {
				//logdAssert(_DEBUG_, "Reset restart cnt");
				task_p->restartCnt = 0;
			}
			//logdAssert(_DEBUG_, "Incremented restart cnt for task %s now %u", task_p->pcName, task_p->restartCnt);
		}
	}
	else {
		//logdAssert(_DEBUG_, "Reset restart cnt for task %s", task_p->pcName);
		task_p->restartCnt = 0;
	}
	if (initiateStat) {
		clearStats(task_p);
	}
	//logdAssert(_DEBUG_, "*task_p %p taskhandle %p", task_p, task_p->pvCreatedTask);
	//logdAssert(_DEBUG_, "Resetting watchdog timer for Task: %s", task_p->pcName);
	task_p->watchDogTimer = 0;

	//logdAssert(_DEBUG_, "Watchdog cleared");
	//logdAssert(_DEBUG_, "Params before starting task %u", *((uint8_t*)(task_p->pvParameters)));
	//logdAssert(_DEBUG_, "Starting task with function pointer %p named %s", task_p->pvTaskCode, task_p->pcName);

	if (xTaskCreatePinnedToCore(
		task_p->pvTaskCode,
		task_p->pcName,
		task_p->usStackDepth,
		task_p,
		task_p->uxPriority,
		&(task_p->pvCreatedTask),
		task_p->xCoreID) != pdPASS) {
		taskInitMutexGive(task_p);
		logdAssert(_ERR_, "General error, the task was probably not started");
		return(_TASKS_GENERAL_ERROR_);
	}
	//logdAssert(_DEBUG_, "Giving back the Mutex");
	taskInitMutexGive(task_p);
	//logdAssert(_DEBUG_, "The Mutex is given back");
	//logdAssert(_INFO_, "Task: %s was started/restarted", task_p->pcName);
	//logdAssert(_DEBUG_, "Leaving starttaskbytaskdesc");

	return(_TASKS_OK_);
}
/* ------------------------------------- END:startTaskByTaskDec -------------------------------------*/

/* ------------------------------------ PRIVATE:stopTaskByTaskDec -----------------------------------*/
/* uint8_t tasks::stopTaskByTaskDesc(task_desc_t* task_p)                                            */
/* Description:  stops a static task by its task descriptor previously registered with               */
/*               uint8_t startStaticTask(...)                                                        */
/*               This is the prime method to start a staticc task, all other methods are in the end  */
/*               using this method to stop a static task                                             */
/*  Properties:  Privat method.                                                                      */
/*               This method is thread safe.                                                         */
/*  Return codes:[_TASKS_OK_= 0 | _TASKS_RESOURCES_EXHAUSTED | _TASKS_API_PARAMETER_ERROR_ |         */
/*               | _TASKS_GENERAL_ERROR_, _TASKS_NOT_FOUND_ | _TASKS_MUTEX_TIMEOUT_]                 */
/*  Parameters:  task_desc_t* task_p - task descriptor                                               */
/*                                                                                                   */
uint8_t tasks::stopTaskByTaskDesc(task_desc_t* task_p) {
	//logdAssert(_INFO_, "Trying to stop task: %s", task->pcName);
	taskInitMutexTake(task_p);
	if (!checkIfTaskAlive(task_p)) {
		//logdAssert(_INFO_, "Task %s is running and will be stopped", task->pcName);
		vTaskDelete(task_p->pvCreatedTask);
		initd.taskMfreeAll(task_p);
		taskInitMutexGive(task_p);
		//logdAssert(_INFO_, "Task: %s stopped", task_p->pcName);
		return(_TASKS_OK_);
	}
	else {
		taskInitMutexGive(task_p);
		logdAssert(_ERR_, "Task: %s is not running and cannot be stoped, trying to perform task garbage collection", task_p->pcName);
		//initd.taskMfreeAll(task);???????
		return(_TASKS_GENERAL_ERROR_);
	}
}
/* -------------------------------------- END:stopTaskByTaskDec -------------------------------------*/
/* **************************************** END Task control *************************************** */


/* ********************************************************************************************* */
/* Various Init system system helper functions                                                   */
/* Description:                                                                                  */
/*                                                                                               */
/* Public:                                                                                       */
/* uint8_t getTidByTaskDesc(uint8_t* tid, task_desc_t* task)                                     */
/* */

uint8_t tasks::getTidByTaskDesc(uint8_t* tid, task_desc_t* task) {

	if (task == NULL) {
		return(_TASKS_API_PARAMETER_ERROR_);
	}

	for (uint8_t i = 0; i < noOfTasks; i++) {
		if (classTasktable[i] == task) {
			*tid = i;
			return(_TASKS_OK_);
			break;
		}
	}
	return(_TASKS_NOT_FOUND_);
}

uint8_t tasks::getTaskDescByName(char* pcName, task_desc_t** task_pp) {
	//logdAssert(_DEBUG_, "Trying to get task by name: %c", pcName);
	globalInitMutexTake();
	for (uint8_t i = 0; i < noOfTasks; i++) {
		if (uint8_t res = getTaskDescByTID(i, task_pp)) {
			globalInitMutexGive();
			logdAssert(_ERR_, "Could not get task tid: %u, Cause: %u", i, res);
			return(res);
		}
		if (!strcmp((*task_pp)->pcName, pcName)) {
			globalInitMutexGive();
			//logdAssert(_INFO_, "Got the task by name: %s, tid: %u" , pcName, i);
			return(_TASKS_OK_);
			break;
		}
	}
	globalInitMutexGive();
	//logdAssert(_INFO_, "Didn't find any task by name: %s", pcName);
	return(_TASKS_NOT_FOUND_);
}

uint8_t tasks::getTaskDescByTID(uint8_t tid, task_desc_t** task_pp) {
	//logdAssert(_DEBUG_, "Trying to get task by tid: %i", tid);
	if (tid >= noOfTasks) {
		logdAssert(_ERR_, "Parameter error, tid: %u", tid);
		return(_TASKS_API_PARAMETER_ERROR_);
	}

	if ((*task_pp = classTasktable[tid]) == NULL) { //Assumed atomic not needing any lock
		logdAssert(_ERR_, "General error in getting descriptor by tid: %u", tid);
		return(_TASKS_GENERAL_ERROR_);
	}
	//logdAssert(_DEBUG_, "Task(%p) Task Pointer: %p", task_pp, *task_pp);
	//logdAssert(_DEBUG_, "Got task descriptor by tid: %i, taskName %s", tid, (*task_pp)->pcName);
	//logdAssert(_DEBUG_, "**task: %p *task: %p", task_pp, *task_pp);
	return(_TASKS_OK_);
}

uint8_t tasks::checkIfTaskAlive(task_desc_t* task_p) {
	taskInitMutexTake(task_p);
	//logdAssert(_DEBUG_, "Mutex taken");
	//logdAssert(_DEBUG_, "*task %p taskhandle %p", task, task->pvCreatedTask);
	if (task_p->pvCreatedTask == NULL) {
		//logdAssert(_INFO_, "Task does not exist, handle = NULL");
		taskInitMutexGive(task_p);
		return (_TASK_DELETED_);
	}
	if (eTaskGetState(task_p->pvCreatedTask) == 1 /*eDeleted*/) { //eDeleted doesn't work
		//logdAssert(_INFO_, "Task deleted");
		taskInitMutexGive(task_p);
		return (_TASK_DELETED_);
	}
	else {
		//logdAssert(_INFO_, "Task %s exists with state %u, edeleted %u", task_p->pcName, eTaskGetState(task_p->pvCreatedTask), eDeleted);
		taskInitMutexGive(task_p);
		return (_TASK_EXIST_);
	}
}

uint8_t tasks::clearStats(task_desc_t* task_p) {
	//logdAssert(_INFO_, "Clearing task stats");
	if (task_p == NULL) {
		logdAssert(_ERR_, "Parameter error");
		return(_TASKS_API_PARAMETER_ERROR_);
	}

	taskInitMutexTake(task_p);
	task_p->watchDogTimer = 0;
	task_p->watchdogWarn = false;
	task_p->watchdogHighWatermarkAbs = 0;
	task_p->watchdogHighWatermarkWarnAbs = (task_p->watchdogHighWatermarkWarnPerc) * (task_p->WatchdogTimeout) / 100;
	task_p->heapCurr = 0;
	task_p->heapHighWatermarkAbs = 0;
	task_p->heapHighWatermarkWarnAbs = (task_p->heapHighWatermarkKBWarn) * 1024;
	task_p->heapWarn = false;
	task_p->stackHighWatermarkAbs = 0;
	task_p->stackHighWatermarkWarnAbs = (task_p->stackHighWatermarkWarnPerc) * (task_p->usStackDepth) / 100;
	task_p->restartCnt = 0;

	taskInitMutexGive(task_p);
	return(_TASKS_OK_);
}

uint8_t tasks::getStalledTasks(uint8_t* tid, task_desc_t* task_p) {
	TaskStatus_t* pxTaskStatus;
	globalInitMutexTake();
	//logdAssert(_INFO_, "Scanning for stalled tasks starting from tid: %u", *tid);
	for (uint8_t i = *tid; i < noOfTasks; i++) {
		task_p = classTasktable[i];
		if (task_p != NULL) {
			if (pxTaskStatus == NULL || eTaskGetState(task_p->pvCreatedTask) == 1 /*eDeleted*/) { //eDeleted doesn't work
				*tid = i;
				globalInitMutexGive();
				logdAssert(_INFO_, "Found a stalled task tid: %u", i);
				return(_TASKS_OK_);
				break;
			}
		}
	}
	task_p = NULL;
	globalInitMutexGive();
	//logdAssert(_INFO_, "Found no more stalled tasks");
	return(_TASKS_NOT_FOUND_);
}

/* ************************** Task mutex/critical section handling  ******************************** */
/* These methods intend to provide secure pass through critical sections avoiding                    */
/* interfearence from parallel tasks/threads manipulating the datastructures being processed.        */
/* Two access methods are defined: tasks:globalInitMutexTake/(Give) locking the entire tasks data-   */
/* structure and:                                                                                    */
/* tasks:taskInitMutexTake/(Give) [not implemented - translated to tasks:globalInitMutexTake/(Give)] */
/* intended to implement a atomic method where global mutexes blocks any task data structure         */
/* modifications, ...                                                                                */
/* while task                                                                                        */
/* blocks global-, this task                                                                         */
/* ------------------------------------------------------------------------------------------------- */
/* Private methods:                                                                                  */
/* tasks::globalInitMutexTake(TickType_t timeout)*/
uint8_t tasks::globalInitMutexTake(TickType_t timeout) {
	if (timeout != portMAX_DELAY) {
		xSemaphoreTakeRecursive(globalInitMutex, pdMS_TO_TICKS(timeout));
		if (xSemaphoreGetMutexHolder(globalInitMutex) != xTaskGetCurrentTaskHandle()) {
			return(_TASKS_MUTEX_TIMEOUT_);
		}
		else {
			return(_TASKS_OK_);
		}
	}
	else {
		return(_TASKS_OK_);
	}
}

uint8_t tasks::globalInitMutexGive() {
	xSemaphoreGiveRecursive(globalInitMutex);
	return(_TASKS_OK_);
}

uint8_t tasks::taskInitMutexTake(task_desc_t* task_p, TickType_t timeout) { //Fine grained task locks not yet implemented
	return(globalInitMutexTake(timeout));
}

uint8_t tasks::taskInitMutexGive(task_desc_t* task_p) { //Fine grained task locks not yet implemente
	return(globalInitMutexGive());
}

/* *********************************** Task resources management *********************************** */
/* Task resource management internal task class functions meant to provide general mechanisms for    */
/* various resource management, such as memory-, message passing-, queues- management or any other   */
/* arbitrary resources which the task class need to be aware of in order to be able to destruct      */
/* in case there are premature task terminations.                                                    */
/* Using these class primitives enables destruction of resources if a task is terminated             */
/* ------------------------------------------------------------------------------------------------- */
/* No pubblic methods are exposed, but-                                                              */
/* Following private primitives are provided:                                                        */
/* taskResourceLink(...)                                                                             */
/* taskResourceUnLink(...)                                                                           */
/* findResourceDescByResourceObj(...)                                                                */
/*                                                                                                   */
/* --------------------------------------------------------------------------------------------------*/

/* ----------------------------------- PRIVATE:taskResourceLink() -----------------------------------*/
/* uint8_t taskResourceLink(task_resource_t** taskResourceRoot_pp, void* taskResourceObj_p)          */
/*	Description:  This method links a new resource descriptor with an object pointer to a list       */
/*                of descriptors starting at ask_resource_t** taskResourceRoot_pp                    */
/*  Properties:   Private method.                                                                    */
/*                This method is thread unsafe, it is unsafe to use this method while the root or    */
/*                task object is being modified by any other parallel thread.                        */
/*  Return codes: [_TASKS_OK_= 0 | _TASKS_RESOURCES_EXHAUSTED | _TASKS_API_PARAMETER_ERROR_ |        */
/*                 | _TASKS_GENERAL_ERROR_, _TASKS_NOT_FOUND_ | _TASKS_MUTEX_TIMEOUT_]               */
/*  Parameters:   task_resource_t** taskResourceRoot_pp: Address of the root pointer for the-        */
/*                resource list                                                                      */
/*                void* taskResourceObj_p: An arbitrary resource object pointer                      */
/*                                                                                                   */

uint8_t tasks::taskResourceLink(task_resource_t** taskResourceRoot_pp, void* taskResourceObj_p) {
	void* taskResourceDesc_p;
	
	if (taskResourceRoot_pp == NULL || taskResourceObj_p == NULL) {
		logdAssert(_INFO_, "API error, one of task_resource_t** taskResourceRoot_pp, void* taskResourceObj_p is NULL");
		return(_TASKS_API_PARAMETER_ERROR_);
	}
	//logdAssert(_DEBUG_, "Trying to link in taskResourceObj_p %p to taskResourceRoot_pp %p, which is pointing to %p", taskResourceObj_p, taskResourceRoot_pp, *taskResourceRoot_pp);
	if ((taskResourceDesc_p = malloc(sizeof(task_resource_t))) == NULL) {
		logdAssert(_ERR_, "Could not allocate memory for resource descriptor");
		return(_TASKS_RESOURCES_EXHAUSTED_);
	}
	
	//logdAssert(_DEBUG_, "taskResourceDesc_p %p allocated, linking it to beginning of root_pp %p", taskResourceDesc_p, taskResourceRoot_pp);
	((task_resource_t*)taskResourceDesc_p)->prevResource_p = NULL;
	((task_resource_t*)taskResourceDesc_p)->taskResource_p = taskResourceObj_p;

	if (*taskResourceRoot_pp == NULL) {
		//logdAssert(_DEBUG_, "This is the first object");
		*taskResourceRoot_pp = (task_resource_t*)taskResourceDesc_p;
		((task_resource_t*)taskResourceDesc_p)->nextResource_p = NULL;
	}
	else {
		//logdAssert(_DEBUG_, "There exists more resource objects, linking it in as the first object");
		(*taskResourceRoot_pp)->prevResource_p = (task_resource_t*)taskResourceDesc_p;
		((task_resource_t*)taskResourceDesc_p)->nextResource_p = *taskResourceRoot_pp;
		*taskResourceRoot_pp = (task_resource_t*)taskResourceDesc_p;
		//logdAssert(_DEBUG_, "root pointer is now (*taskResourceRoot_pp) is now %p", *taskResourceRoot_pp);
		//logdAssert(_DEBUG_, "the created object descriptor address (taskResourceDesc_p) is %p", taskResourceDesc_p);
		//logdAssert(_DEBUG_, "and its content is: prev %p, object %p, next %p", ((task_resource_t*)taskResourceDesc_p)->prevResource_p, ((task_resource_t*)taskResourceDesc_p)->taskResource_p, ((task_resource_t*)taskResourceDesc_p)->nextResource_p);
		//logdAssert(_DEBUG_, "and its successor content is: prev %p, object %p, next %p", ((task_resource_t*)taskResourceDesc_p)->nextResource_p->prevResource_p, ((task_resource_t*)taskResourceDesc_p)->nextResource_p->taskResource_p, ((task_resource_t*)taskResourceDesc_p)->nextResource_p->nextResource_p);

	}
	//logdAssert(_DEBUG_, "Resource descriptor successfully linked-in");
	return(_TASKS_OK_);
}
/* --------------------------------- END PRIVATE:taskResourceLink() ---------------------------------*/

/* ---------------------------------- PRIVATE:taskResourceUnLink() ----------------------------------*/
/* uint8_t tasks::taskResourceUnLink(task_resource_t** taskResourceRoot_pp, void* taskResourceObj_p) */
/* Description: This method un-links and destroys a resource descriptor, but does destroy the        */
/*               actual resource object.                                                             */
/*               The execution time is almost linear with the amount of linked task resource         */
/*               desctiptors                                                                         */
/* Properties:   Private method.                                                                     */
/*               This method is thread unsafe, it is unsafe to use this method while the root- or    */
/*               task object is being modified by any other parallel thread.                         */
/*               The execution time is almost linear with the amount of linked task resource         */
/*               desctiptors                                                                         */
/* Return codes: [_TASKS_OK_= 0 | _TASKS_RESOURCES_EXHAUSTED | _TASKS_API_PARAMETER_ERROR_ |         */
/*               | _TASKS_GENERAL_ERROR_, _TASKS_NOT_FOUND_ | _TASKS_MUTEX_TIMEOUT_]                 */
/* Parameters:   task_resource_t** taskResourceRoot_pp: Address of the root pointer for the-         */
/*               resource list                                                                       */
/*               void* taskResourceObj_p: An arbitrary resource object pointer for which it's        */
/*               resource descriptor should be removed                                               */
/*                                                                                                   */


uint8_t tasks::taskResourceUnLink(task_resource_t** taskResourceRoot_pp, void* taskResourceObj_p) {
	task_resource_t* taskResourceDesc_p;
	task_resource_t** taskResourceDesc_pp = &taskResourceDesc_p;

	if (taskResourceRoot_pp == NULL || taskResourceObj_p == NULL) {
		logdAssert(_ERR_, "API error, one of task_resource_t** taskResourceRoot_pp, void* taskResourceObj_p are NULL");
		return(_TASKS_API_PARAMETER_ERROR_);
	}
	//logdAssert(_DEBUG_, "Trying to unlink  taskResourceObj_p %p from taskResourceRoot_pp %p pointing at %p", taskResourceObj_p, taskResourceRoot_pp, *taskResourceRoot_pp);
	//logdAssert(_DEBUG_, "Trying to find taskResourceDesc_p by taskResourceObj_p %p from the list pointed to by taskResourceRoot_pp %p pointing at *taskResourceRoot_pp", taskResourceObj_p, taskResourceRoot_pp, *taskResourceRoot_pp);
	if (uint8_t res = initd.findResourceDescByResourceObj(taskResourceObj_p, *taskResourceRoot_pp, taskResourceDesc_pp)) {
		logdAssert(_WARN_, "resourceObj_p %p could not be found in the list pointed to by *taskResourceRoot_pp %p", taskResourceObj_p, *taskResourceDesc_pp);
		return(_TASKS_NOT_FOUND_);
	}
	//logdAssert(_DEBUG_, "Found taskResourceDesc_p by taskResourceObj_p %p", taskResourceObj_p);

	if ((*taskResourceDesc_pp)->nextResource_p == NULL) {
		//logdAssert(_DEBUG_, "This *taskResourceDesc_pp %p is the last in the list", *taskResourceDesc_pp);
		if ((*taskResourceDesc_pp)->prevResource_p == NULL) {
			*taskResourceRoot_pp = NULL;
			//logdAssert(_DEBUG_, "This *taskResourceDesc_pp %p was the only one and has been un-linked, *taskResourceRoot_pp in now %p", *taskResourceDesc_pp, *taskResourceRoot_pp);
		}
		else {
			((*taskResourceDesc_pp)->prevResource_p)->nextResource_p = NULL;
			//logdAssert(_DEBUG_, "This taskResourceDesc_p %p were at the end of many, and has been un-linked, *taskResourceRoot_pp is now %p", *taskResourceDesc_pp, *taskResourceRoot_pp);
		}
	}
	else {
		if ((*taskResourceDesc_pp)->prevResource_p == NULL) {
			*taskResourceRoot_pp = (*taskResourceDesc_pp)->nextResource_p;
			(*taskResourceDesc_pp)->nextResource_p->prevResource_p = NULL;
			//logdAssert(_DEBUG_, "There were several taskResourceDesc in the list, but this *taskResourceDesc_pp %p was the first one and has been unlinked, *taskResourceRoot_pp is now %p", *taskResourceDesc_pp, *taskResourceRoot_pp);
		}
		else {
			(*taskResourceDesc_pp)->nextResource_p->prevResource_p = (*taskResourceDesc_pp)->prevResource_p;
			(*taskResourceDesc_pp)->prevResource_p->nextResource_p = (*taskResourceDesc_pp)->nextResource_p;
			//logdAssert(_DEBUG_, "This *taskResourceDesc_pp %p  was in the mid of the list and has been un-linked, *taskResourceRoot_pp is now %p", *taskResourceDesc_pp, *taskResourceRoot_pp);
			//logdAssert(_DEBUG_, "The previous *taskResourceDesc_pp was %p and it's next is %p", (*taskResourceDesc_pp)->prevResource_p, (*taskResourceDesc_pp)->nextResource_p);
			//logdAssert(_DEBUG_, "The Next *taskResourceDesc_pp was %p and it's next is %p", (*taskResourceDesc_pp)->prevResource_p, (*taskResourceDesc_pp)->nextResource_p);
		}
	}
	free(*taskResourceDesc_pp);
	return(_TASKS_OK_);
}
/* -------------------------------- END PRIVATE:taskResourceUnLink() --------------------------------*/

/* ------------------------------ PRIVATE:findResourceDescByResourceObj -----------------------------*/
/* uint8_t findResourceDescByResourceObj(void* resourceObj_p, task_resource_t* taskResourceRoot_p,   */
/*                                       task_resource_t** taskResourceDesc_pp)                      */
/*	Description: This method tries to find the a task_resource_t** taskResourceDesc_pp task resource */
/*               descriptor holding an void* resourceObj_p resource object.                          */
/*  Properties:  Private method.                                                                     */
/*               This method is thread unsafe, it is unsafe to use this method while the root- or    */
/*               task object is being modified by any other parallel thread.                         */
/*               The execution time is almost linear with the amount of linked task resource         */
 /*              desctiptors                                                                         */
/*  Return codes: [_TASKS_OK_= 0 | _TASKS_RESOURCES_EXHAUSTED | _TASKS_API_PARAMETER_ERROR_ |        */
/*                | _TASKS_GENERAL_ERROR_, _TASKS_NOT_FOUND_ | _TASKS_MUTEX_TIMEOUT_]                */
/*  Parameters:   void* resourceObj_p: The arbitrary object being searche for.                       */
/*                task_resource_t** taskResourceRoot_pp: Address of the root pointer for the -       */
/*                                                       resource list                               */
/*                task_resource_t** taskResourceDesc_pp: The address of the pointer which this       */
/*														 this function return - the pointer to the   */
/*                                                       descriptor holding the resource object      */

uint8_t tasks::findResourceDescByResourceObj(void* taskResourceObj_p, task_resource_t* taskResourceRoot_p, task_resource_t** taskResourceDesc_pp) {

	if (taskResourceObj_p == NULL || taskResourceRoot_p == NULL) {
		logdAssert(_ERR_, "API error, one or both of: void* taskResourceObj_p, task_resource_t* taskResourceRoot_p is NULL");
		return(_TASKS_API_PARAMETER_ERROR_);
	}
	//logdAssert(_DEBUG_, "Finding *taskResourceDesc_pp from &taskResourceRoot_p %p, taskResourceRoot_p %p", &taskResourceRoot_p, taskResourceRoot_p);

	*taskResourceDesc_pp = taskResourceRoot_p;
	while ((*taskResourceDesc_pp)->nextResource_p != NULL && (*taskResourceDesc_pp)->taskResource_p != taskResourceObj_p) { 
		(*taskResourceDesc_pp) = (*taskResourceDesc_pp)->nextResource_p;
		//logdAssert(_DEBUG_, "Itterating to next *taskResourceDesc_pp %p", *taskResourceDesc_pp);
	}
	//logdAssert(_DEBUG_, "Itterating END");
	if ((*taskResourceDesc_pp)->taskResource_p == taskResourceObj_p) {
		//logdAssert(_DEBUG_, "Found *taskResourceDesc_pp %p by taskResourceObj_p %p", *taskResourceDesc_pp, taskResourceObj_p);
		return(_TASKS_OK_);
	}
	else {
		*taskResourceDesc_pp = NULL;
		logdAssert(_INFO_, "Could not find taskResourceObj_p by resource object: %p", taskResourceObj_p);
		return(_TASKS_NOT_FOUND_);
	}
}
/* --------------------------- END PRIVATE:findResourceDescByResourceObj ----------------------------*/

/* ********************************* END Task resources management ********************************* */


/* ********************************** Memory resources management ********************************** */
/* Task memory resource management makes sure that memory allocated by Tasks can be released         */
/* after a task crash or whenever the task manager kills a tak due to watchdog timeouts, etc         */
/* ------------------------------------------------------------------------------------------------- */
/* Pubblic methods:                                                                                  */
/* void* taskMalloc(task_desc_t* task_p, int size);                                                  */
/* uint8_t taskMfree(task_desc_t* task_p, void* mem_p)                                               */
/*                                                                                                   */
/* Private methods:                                                                                  */
/* uint8_t tasks::taskMfreeAll(task_desc_t* task_p)                                                  */
/* --------------------------------------------------------------------------------------------------*/

/* --------------------------------------- PUBLIC:taskMalloc ----------------------------------------*/
/* void* tasks::taskMalloc(task_desc_t* task_p, int size)                                            */
/* Description:   This method allocates memory of size "size" and register it with the task manager  */
/*                such that it will beleted, should the task crash or otherwise terminated.          */
/* Properties:    Public method.                                                                     */
/*                This method is thread SAFE, it is safe to use this method while the root- or       */
/*                task object is trying to be modified.                                              */
/*                The execution time is almost linear with the amount of linked task memory resource */
/*                desctiptors.                                                                       */
/* Return codes:  pointer to/handle to the beginning of the memory chunk allocated                   */
/* Parameters:    vtask_desc_t* task_p: The task decriptor pointer.                                  */
/*                int size: size of the memory chunk to be allocated.                                */
/*                                                                                                   */
void* tasks::taskMalloc(task_desc_t* task_p, int size) {
	void* mem_p;

	if (size == 0 || task_p == NULL) {
		logdAssert(_ERR_, "API error, size=0 or task_p=NULL");
		return(NULL);
	}
	//logdAssert(_DEBUG_, "Allocating memory of size: %u: requested from task: %s", size, task_p->pcName)
	taskInitMutexTake(task_p);
	if ((mem_p = malloc(size + sizeof(int))) == NULL) {
		logdAssert(_WARN_, "Could not allocate memory");
		taskInitMutexGive(task_p);
		return(NULL);
	}
	//logdAssert(_DEBUG_, "base mem_p is %p size is %u", mem_p, size + sizeof(int));
	*((int*)mem_p) = size + sizeof(int);

	if (uint8_t res = taskResourceLink(&(task_p->dynMemObjects_p) , (char*)mem_p + sizeof(int))) {
		free(mem_p);
		logdAssert(_ERR_, "Register/link dynamic memory resource with return code: %u", res);
		taskInitMutexGive(task_p);
		return(NULL);
	}
	//logdAssert(_DEBUG_, "Memory allocated at base adrress: %p, Size: %u", mem_p, size);
	incHeapStats(task_p, size + sizeof(int));
	taskInitMutexGive(task_p);
	return((char*)mem_p + sizeof(int));
}
/* -------------------------------------- END PUBLIC:taskMalloc -------------------------------------*/

/* ---------------------------------------- PUBLIC:taskMfree ----------------------------------------*/
/* uint8_t tasks::taskMfree(task_desc_t* task_p, void* mem_p)                                        */
/* Description:   This method frees memory previously allocated with taskMalloc and unregisters it   */
/*                with the task manager memory allocation tracking                                   */
/* Properties:    Public method.                                                                     */
/*                This method is thread SAFE, it is safe to use this method while the root- or       */
/*                task object is trying to be modified.                                              */
/*                The execution time is almost linear with the amount of linked task memory resource */
/*                desctiptors.                                                                       */
/* Return codes:  [_TASKS_OK_= 0 | _TASKS_RESOURCES_EXHAUSTED | _TASKS_API_PARAMETER_ERROR_ |        */
/*                | _TASKS_GENERAL_ERROR_, _TASKS_NOT_FOUND_ | _TASKS_MUTEX_TIMEOUT_]                */
/* Parameters:    vtask_desc_t* task_p: The task decriptor pointer.                                  */
/*                void* mem_p the memory chunk handle to be freed/starting pointer to the memory     */
/*                chunk.                                                                             */
/*                                                                                                   */
uint8_t tasks::taskMfree(task_desc_t* task_p, void* mem_p) {
	task_resource_t** taskResourceDesc_pp = &(task_p->dynMemObjects_p);
	int size;

	//logdAssert(_DEBUG_, "taskResourceDesc_pp %p", taskResourceDesc_pp);
	if (task_p == NULL || mem_p == NULL) {
		logdAssert(_ERR_, "API error, one of ask_desc_t* task_p, void* mem_p is NULL");
		return(_TASKS_API_PARAMETER_ERROR_);
	}
	taskInitMutexTake(task_p);
	//logdAssert(_DEBUG_, "Trying to free heap memory %p allocated by task %s", mem_p, task_p->pcName);
	size = *((int*)((char*)mem_p - sizeof(int)));
	//logdAssert(_DEBUG_, "base mem_p is %p size is %u", (char*)mem_p - sizeof(int), size);
	free((char*)mem_p - sizeof(int));
	decHeapStats(task_p, size);
	//logdAssert(_DEBUG_, "Task heap memory freed: %p", mem_p);
	
	//logdAssert(_DEBUG_, "Trying to un-link memory resource %p from root (&(task_p->dynMemObjects_p) %p", mem_p, &(task_p->dynMemObjects_p));
	if (uint8_t res = taskResourceUnLink(&(task_p->dynMemObjects_p), mem_p)) {
		logdAssert(_ERR_, "un-linking memory resource failed - resason: %u", res);
		taskInitMutexGive(task_p);
		return(res);
	}
	//logdAssert(_DEBUG_, "un-link memory resource %p from root (&(task_p->dynMemObjects_p) %p successfull", mem_p, &(task_p->dynMemObjects_p));
	taskInitMutexGive(task_p);
	return(_TASKS_OK_);
}
/* -------------------------------------- END PUBLIC:taskMfree --------------------------------------*/

/* -------------------------------------- PRIVATE:taskMfreeAll --------------------------------------*/
/* uint8_t tasks::taskMfreeAll(task_desc_t* task_p)                                                  */
/* Description:   This is a private method which frees all memory previously allocated by a task     */
/*                with taskMalloc method and unregisters it                                          */
/* Properties:    Private method.                                                                    */
/*                This method is thread SAFE, it is safe to use this method while the root- or       */
/*                task object is trying to be modified.                                              */
/*                The execution time is almost linear with the amount of linked task memory resource */
/*                desctiptors.                                                                       */
/* Return codes:  [_TASKS_OK_= 0 | _TASKS_RESOURCES_EXHAUSTED | _TASKS_API_PARAMETER_ERROR_ |        */
/*                | _TASKS_GENERAL_ERROR_, _TASKS_NOT_FOUND_ | _TASKS_MUTEX_TIMEOUT_]                */
/* Parameters:    vtask_desc_t* task_p: The task decriptor pointer.                                  */
/*                                                                                                   */
uint8_t tasks::taskMfreeAll(task_desc_t* task_p) {
	
	if (task_p == NULL) {
		logdAssert(_ERR_, "API ERROR, task_p = NULL");
		return(_TASKS_API_PARAMETER_ERROR_);
	}
	logdAssert(_DEBUG_, "Freing all heap memory allocated by task: %s", task_p->pcName);
	taskInitMutexTake(task_p);
	//#ifdef debug

	if (task_p->dynMemObjects_p == NULL) {
		//logdAssert(_DEBUG_, "No heap memory to free for task %s", task_p->pcName);
	}
	else {
		//logdAssert(_DEBUG_, "There are heap memory objects for task %s, freeng those", task_p->pcName);
	}
	while (task_p->dynMemObjects_p != NULL) {
		//logdAssert(_DEBUG_, "Freing Memory resource descriptor %p alocated by task %s", task_p->dynMemObjects_p, task_p->pcName);
		if (uint8_t res = taskMfree(task_p, (task_p->dynMemObjects_p->taskResource_p))) {
			logdAssert(_INFO_, "Could not free memory, result code: %u", res);
			taskInitMutexGive(task_p);
			return(res);
		}
	} 
	taskInitMutexTake(task_p);
	return(_TASKS_OK_);
}
/* ------------------------------------ END PRIVATE:taskMfreeAll ------------------------------------*/

/* -------------------------------------- PRIVATE:incHeapStats --------------------------------------*/
/* void tasks::incHeapStats(task_desc_t* task_p, int size)                                           */ 
void tasks::incHeapStats(task_desc_t* task_p, int size) {
	task_p->heapCurr += size;
	//logdAssert(_DEBUG_, "Task %s is currently using %u of the heap, the warn level is set to %u and heapwarn is %u", task_p->pcName, task_p->heapCurr, task_p->heapHighWatermarkWarnAbs, task_p->heapWarn);
	if (task_p->heapCurr >= task_p->heapHighWatermarkAbs) {
		task_p->heapHighWatermarkAbs = task_p->heapCurr;
	}
	if (task_p->heapCurr >= task_p->heapHighWatermarkWarnAbs && task_p->heapHighWatermarkKBWarn &&!task_p->heapWarn) {
		task_p->heapWarn = true;
		logdAssert(_WARN_, "Task %s has reached warning level %u, High water mark is %u", task_p->pcName, task_p->heapHighWatermarkWarnAbs, task_p->heapHighWatermarkAbs);
	}
}
/* ---------------------------------------- END:incHeapStats ----------------------------------------*/

/* -------------------------------------- PRIVATE:decHeapStats --------------------------------------*/
void tasks::decHeapStats(task_desc_t* task_p, int size) {
	task_p->heapCurr -= size;
	if (task_p->heapCurr < task_p->heapHighWatermarkWarnAbs && task_p->heapHighWatermarkKBWarn && task_p->heapWarn) {
		task_p->heapWarn = false;
		logdAssert(_INFO_, "Task %s has no longer exceeded the heap warning watermark", task_p->pcName);
	}
}
/* ---------------------------------------- END:decHeapStats ----------------------------------------*/

/* ******************************** END Memory resources management ******************************** */

/* *************************** Task Management/watchdog core tasks  ******************************** */
/* These tasks are periodic core init OS, monitoring spawned tasks, restarting those ig crashed      */
/* or if watchdog expired, collecting and logging system and task statistics, and restarting the     */
/* system if there is no other way out, or if these services are not responding - monitored by the   */
/* Arduino main loop                                                                                 */
/* ------------------------------------------------------------------------------------------------- */
/* Public methods:                                                                                   */
/* void tasks::kickTaskWatchdogs(...);                                                               */
/*      tasks method to kick the watchdog in case task_dec_t.WatchdogTimeout != 0 [??? 10 ms ???]    */
/* void tasks::taskPanic(...);                                                                       */
/*      Initiates a graceful task panic exception handling, task will be terminated and it's         */
/*      resources will be collected and cleaned.  The task might be restarted depending on           */
/*      task_dec_t.WatchdogTimeout (if 0 the task will not be restarted, otherwise it will....       */
/*                                                                                                   */
/* Private methods:                                                                                  */
/* void tasks:: runTaskWatchdogs(...)                                                                */
/*         A core tasks timer method to run periodic maintenance tasks, checking watchdogs, task-    */
/*         health, collecting and reporting statistics, etc.                                         */
/*                                                                                                   */
/* --------------------------------------------------------------------------------------------------*/

/* --------------------------------- - Public:kickTaskWatchdogs()- --------------------------------- */
/* void tasks::kickTaskWatchdogs(task_desc_t* task_p)                                                */
/*	Description:  This method kicks the watchdog for task: task_p and needs to be called frequently  */
/*                with a periodicy less than defined in task_desc_t.WatchdogTimeout                  */
/*  Properties:   Public method. This method is thread safe.                                         */
/*  Return codes: void                                                                               */
void tasks::kickTaskWatchdogs(task_desc_t* task_p) {
	task_p->watchDogTimer = 0; //atomic
	return;
}
/* ---------------------------------- - END:kickTaskWatchdogs()- ----------------------------------- */

/* ------------------------------------- - Public:taskPanic()- ------------------------------------- */
/* void tasks::taskPanic(task_desc_t* task_p)                                                        */
/*	Description:  Initiates a graceful task panic exception handling, task will be terminated and    */
/*                it's resources will be collected and cleaned.  The task might be restarted         */
/*                depending on task_dec_t.WatchdogTimeout (if 0 the task will not be restarted,      */
/*                otherwise it will....
/*  Properties:   Public method. This method is thread safe.                                         */
/*  Return codes: void                                                                               */
void tasks::taskPanic(task_desc_t* task_p) {
	initd.taskMfreeAll(task_p);
	//release any other resources
	vTaskDelete(NULL);
}
/* -------------------------------------- - END:taskPanic() - -------------------------------------- */

/* ---------------------------------- - Public:taskExceptions()- ----------------------------------- */

// Stack-overflow management void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char* pcTaskName );
// Out of space handling void vApplicationMallocFailedHook( void );
/* ------------------------------------ - END:taskExceptions()- ------------------------------------ */


/* --------------------------------- - Private:runTaskWatchdogs() - -------------------------------- */
/* void tasks::runTaskWatchdogs(TimerHandle_t xTimer)                                                */
/*         A core tasks timer method to run periodic maintenance tasks, checking watchdogs, task-    */
/*         health, collecting and reporting statistics, etc.                                         */
/*         The periodicy of this task is set by: */
/*                                                                                                   */
void tasks::runTaskWatchdogs(TimerHandle_t xTimer) {
	task_desc_t* task_p;
	static int nexthunderedMSWatchdogTaskCnt = (100 / _TASK_WATCHDOG_MS_) + (100 % _TASK_WATCHDOG_MS_);
	static int nextSecWatchdogTaskCnt = (1000 / _TASK_WATCHDOG_MS_) + (1000 % _TASK_WATCHDOG_MS_);
	static int nextTenSecWatchdogTaskCnt = ((10 * 1000) / _TASK_WATCHDOG_MS_) + ((10 * 1000) % _TASK_WATCHDOG_MS_);
	static int nextMinWatchdogTaskCnt = ((60 * 1000) / _TASK_WATCHDOG_MS_) + ((60 * 1000) % _TASK_WATCHDOG_MS_);
	static int nextHourWatchdogTaskCnt = ((3600 * 1000) / _TASK_WATCHDOG_MS_) + ((3600 * 1000) % _TASK_WATCHDOG_MS_);
	bool decreaseEscalationCnt = false;
	static uint8_t minStatTaskWalk = 255;

	if (xSemaphoreTake(watchdogMutex, 0) == pdFALSE) {
		watchdogOverruns++;
		//logdAssert(_DEBUG_, "Watchdog overrun, skipping this watchdog run");
		return;
	}
	//	logdAssert(_DEBUG_, "Running watchdogs nexthunderedMSWatchdogTaskCnt %u", nexthunderedMSWatchdogTaskCnt); 
	//	logdAssert(_DEBUG_, "Running watchdogs initd.watchdogTaskCnt %u initd.hunderedMsCnt %u", initd.watchdogTaskCnt, initd.hunderedMsCnt);
	initd.watchdogTaskCnt++; //make sure it is initiated somewhere

	//Every 100 ms tasks
	if (initd.watchdogTaskCnt >= nexthunderedMSWatchdogTaskCnt) {
		initd.hunderedMsCnt++;
		//logdAssert(_DEBUG_, "Running recurring watchdog maintenance tasks [100 ms]");
		nexthunderedMSWatchdogTaskCnt = (initd.hunderedMsCnt + 1) * ((100 / _TASK_WATCHDOG_MS_) + (100 % _TASK_WATCHDOG_MS_));
		// do short 1 second stuff here, long run tasks must be deferred evenly across the watchdog runs - 
		//  any mutexes must be local in this "if second" context handled in here.

		//Distributing minute statistics reporting work to 100 ms chunks.
		if (minStatTaskWalk < noOfTasks) {
			initd.getTaskDescByTID(minStatTaskWalk, &task_p);
			logdAssert(_INFO_, "Heap size for Task %s is currently %u, with a high watermark of %u", task_p->pcName,
				task_p->heapCurr, task_p->heapHighWatermarkAbs);
            // If the task isn't running we cannot request the task watermark!!!!!!!!!!!
			//logdAssert(_INFO_, "Stack use for Task %s is currently [currently unsupported], with a high watermark of %u",
			//  	       task_p->pcName, uxTaskGetStackHighWaterMark(task_p->pvCreatedTask));
			logdAssert(_INFO_, "CPU use for Task %s is currently [currently unsupported]", task_p->pcName);
			logdAssert(_INFO_, "Watchdog statistics for Task %s, High water mark %u, Timout set to %u", task_p->pcName,
				task_p->watchdogHighWatermarkAbs, task_p->WatchdogTimeout);
			minStatTaskWalk++;
		}
	}

	//*** Every 1 second tasks
	if (initd.watchdogTaskCnt >= nextSecWatchdogTaskCnt) {
		initd.secCnt++;
		//logdAssert(_DEBUG_, "Running recurring watchdog maintenance tasks [second] - %u hours %u minutes %u seconds", initd.hourCnt, initd.minCnt%60, initd.secCnt%60);
		nextSecWatchdogTaskCnt = (initd.secCnt + 1) * ((1000 / _TASK_WATCHDOG_MS_) + (1000 % _TASK_WATCHDOG_MS_));
		// do short 1 second stuff here, long run tasks must be deferred evenly across the watchdog runs - 
		//  any mutexes must be local in this "if second" context handled in here.
	}

	//*** Every 10 second tasks
	if (initd.watchdogTaskCnt >= nextTenSecWatchdogTaskCnt) {
		initd.tenSecCnt++;
		logdAssert(_DEBUG_, "Running recurring watchdog maintenance tasks [10 seconds] - %u hours %u minutes %u seconds", initd.hourCnt, initd.minCnt % 60, initd.secCnt % 60);
		nextTenSecWatchdogTaskCnt = (initd.tenSecCnt + 1) * (((10 * 1000) / _TASK_WATCHDOG_MS_) + ((10 * 1000) % _TASK_WATCHDOG_MS_));
		//  do short 10 second stuff here, long run tasks must be deferred evenly across the watchdog runs - 
		//  any mutexes must be local in this "if tensecond" context handled in here.
	}

	//*** Every minute tasks
	if (initd.watchdogTaskCnt >= nextMinWatchdogTaskCnt) {
		initd.minCnt++;
		logdAssert(_DEBUG_, "Running recurring watchdog maintenance tasks [minute]- %u hours %u minutes %u seconds", initd.hourCnt, initd.minCnt % 60, initd.secCnt % 60);
		logdAssert(_INFO_, "Watchdog overruns: %u", watchdogOverruns);
		nextMinWatchdogTaskCnt = (initd.minCnt + 1) * (((60 * 1000) / _TASK_WATCHDOG_MS_) + ((60 * 1000) % _TASK_WATCHDOG_MS_));
		//  do short minute stuff here, long run tasks must be deferred evenly across the watchdog runs - 
		//  any mutexes must be local in this "if minute" context handled in here.
		decreaseEscalationCnt = true;
		minStatTaskWalk = 0;
	}

	//*** Houerly tasks
	if (initd.watchdogTaskCnt >= nextHourWatchdogTaskCnt) {
		initd.hourCnt++;
		logdAssert(_DEBUG_, "Running recurring watchdog maintenance tasks [hour]- %u hours %u minutes %u seconds", initd.hourCnt, initd.minCnt % 60, initd.secCnt % 60);
		nextHourWatchdogTaskCnt = (initd.hourCnt + 1) * (((3600 * 1000) / _TASK_WATCHDOG_MS_) + ((3600 * 1000) % _TASK_WATCHDOG_MS_));
		//  do short hour stuff here, long run tasks must be deferred evenly across the watchdog runs - 
		//  any mutexes must be local in this "if hour" context handled in here.

	}

	initd.globalInitMutexTake();

	for (uint8_t i = 0; i < noOfTasks; i++) {
		//	logdAssert(_DEBUG_, "**classTasktable[%u] %p classTasktable[%u] %p", i, &classTasktable[i], i, classTasktable[i]);
		//	logdAssert(_DEBUG_, "**task %p *task %p", &task_p, task_p);
		initd.getTaskDescByTID(i, &task_p);
		//	logdAssert(_DEBUG_, "**task %p *task %p", &task_p, task_p);
		if (task_p == NULL) {
			logdAssert(_PANIC_, "Could not find task by id: %u", i);
			ESP.restart();
		}
		if (decreaseEscalationCnt && task_p->restartCnt) {
			task_p->restartCnt--;
		}

		// if static task not alive - no matter what -> initd.taskMfreeAll(task_p); TODO

		if (task_p->WatchdogTimeout && !(task_p->watchDogTimer == _WATCHDOG_DISABLE_)) {
			if (initd.checkIfTaskAlive(task_p)) {
				task_p->watchDogTimer = _WATCHDOG_DISABLE_;
				logdAssert(_ERR_, "Task: %s  has stalled", task_p->pcName);
				initd.taskMfreeAll(task_p);
				// Free any other static task resources
				initd.startTaskByTid(i, true, false);
			}
			if (++(task_p->watchDogTimer) >= task_p->WatchdogTimeout) {
				task_p->watchDogTimer = _WATCHDOG_DISABLE_;
				logdAssert(_ERR_, "Watchdog timeout for Task: %s", task_p->pcName);
				initd.taskMfreeAll(task_p);
				// Free any other static task resources
				initd.startTaskByTid(i, true, false);
			}
			else {
				if (task_p->watchDogTimer > task_p->watchdogHighWatermarkAbs) {
					task_p->watchdogHighWatermarkAbs = task_p->watchDogTimer;
					if (task_p->watchdogHighWatermarkWarnPerc && task_p->watchdogHighWatermarkWarnAbs <= task_p->watchdogHighWatermarkAbs) {
						//The logging below is likely causing stack overflow - need to increase configMINIMAL_STACK_SIZE
						//logdAssert(_WARN_, "Watchdog count for Task: %s reached Warn level of: %u  percent, watchdog counter: %u" ,task_p->pcName, task_p->watchdogHighWatermarkWarnPerc, task_p->watchDogTimer);
					}
				}
			}
		}
		//logdAssert(_DEBUG_, "Breakpoint 8 Task: %s, restartcnt %u", task_p->pcName, task_p->restartCnt);
		if (task_p->escalationRestartCnt && task_p->restartCnt >= task_p->escalationRestartCnt) {
			logdAssert(_PANIC_, "Task: %s has restarted exessively, escalating to system restart", task_p->pcName);
			ESP.restart();
		}
		if (task_p->watchdogHighWatermarkAbs >= task_p->watchdogHighWatermarkWarnAbs && task_p->WatchdogTimeout && !task_p->watchdogWarn) {
			task_p->watchdogWarn = true;
			logdAssert(_WARN_, "Watchdog for Task %s, has reached warning level %u, High water mark is %u, Timout set to %u", task_p->pcName, task_p->watchdogHighWatermarkWarnAbs, task_p->watchdogHighWatermarkAbs, task_p->WatchdogTimeout);
		}
	}
	initd.globalInitMutexGive();
	xSemaphoreGive(watchdogMutex);
	return;
}
