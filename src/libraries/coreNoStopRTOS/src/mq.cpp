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
  * Name:				 mq.cpp                                                                      *
  * Created:			 2019-09-27                                                                  *
  * Originator:		     Jonas Bjurel                                                                *
  * Short description:   Provides a very basic FreeRTOS message queue based on FreeRtos queues       *
  * Resources:           github, WIKI .....                                                          *
  * ************************************************************************************************ */

#include "init.h"
#include "log.h"

static tasks initd;
bool tasks::tasksObjExist;
task_desc_t* tasks::classTasktable[_NO_OF_TASKS_];
SemaphoreHandle_t tasks::globalInitMutex;
TimerHandle_t tasks::watchDogTimer;
int tasks::tenMsCnt;

char logstr[60];

tasks::tasks() {}

void tasks::startInit(void) {
	logdAssert(_INFO_, "Initializing initd...");
	sleep(1);
	vTaskStartScheduler();
	globalInitMutex = xSemaphoreCreateRecursiveMutex();
	for (uint8_t i = 0; i < _NO_OF_TASKS_; i++) {
		classTasktable[i] = (task_desc_t*)malloc(sizeof(task_desc_t));
		memcpy(classTasktable[i], &taskTable[i], sizeof(task_desc_t));
		classTasktable[i]->taskWatchDogMutex = xSemaphoreCreateRecursiveMutex(); //taskInitMutex.. not yet implemented
		//logdAssert(_DEBUG_, "classTasktable [%u](%p) pointer assigned: %p", i, &classTasktable[i], classTasktable[i]); 
		//logdAssert(_INFO_, "classTasktable[%u] initiated, TaskName: %s, Stack: %u, Prio: %u", i, classTasktable[i]->pcName, classTasktable[i]->usStackDepth, classTasktable[i]->uxPriority);
	}
	initd.startAllTasks(false, true);
	watchDogTimer =  xTimerCreate("watchDogTimer", pdMS_TO_TICKS(1000), pdTRUE, (void*)0, runTaskWatchdogs); //Change back to pdTRUE
	if (watchDogTimer == NULL) {
		logdAssert(_PANIC_, "Could not get a initd watchdog timer");
		ESP.restart();
	}
	if (xTimerStart(watchDogTimer, 0) != pdPASS) {
		logdAssert(_PANIC_, "Could not start the initd watchdog timer");
		ESP.restart();
	}
	logdAssert(_INFO_, "initd scheduler has started");
	//logdAssert(_DEBUG_, "classTasktable[%u](%p) Task Pointer: %p", 0, &classTasktable[0], classTasktable[0]);
	//logdAssert(_DEBUG_, "classTasktable[%u](%p) Task Pointer: %p", 1, &classTasktable[1], classTasktable[1]);
}

tasks::~tasks() {
	logdAssert(_PANIC_, "Someone is trying to destruct initd - rebooting...");
	ESP.restart();
}

void tasks::taskPanic(task_desc_t* task) {
	//Overall Delete task method 
}

uint8_t tasks::getTidByTaskDesc(uint8_t* tid, task_desc_t* task) {
	
	if (task == NULL) {
		return(_TASKS_API_PARAMETER_ERROR_);
	}

	for (uint8_t i = 0; i < _NO_OF_TASKS_; i++) {
		if (classTasktable[i] == task) {
			*tid = i;
			return(_TASKS_OK_);
			break;
		}
	}
	return(_TASKS_NOT_FOUND_);
}

uint8_t tasks::startAllTasks(bool incrementRestartCnt, bool initiateStat) {
	globalInitMutexTake();
	logdAssert(_INFO_, "All tasks will be started/restarted");
	for (uint8_t i = 0; i < _NO_OF_TASKS_; i++) {
		logdAssert(_INFO_, "Tasks name: %s, tid: %u is being initiated", classTasktable[i]->pcName, i);
		if (uint8_t res = startTaskByTaskDesc(classTasktable[i], incrementRestartCnt, initiateStat)) {
			globalInitMutexGive();
			logdAssert(_ERR_, "Tasks name: %s, tid: %d didn't start, cause: %u", classTasktable[i]->pcName, i, res);
			return(_TASKS_GENERAL_ERROR_);
			break;
		}
		logdAssert(_INFO_, "Tasks name: %s, tid: %u was successfully started/restarted", classTasktable[i]->pcName, i);
	}
	globalInitMutexGive();
	return(_TASKS_OK_);
}

uint8_t tasks::stopAllTasks() {
	globalInitMutexTake();
	logdAssert(_INFO_, "Stoping all tasks");
	for (uint8_t i = 0; i < _NO_OF_TASKS_; i++) {
		logdAssert(_INFO_, "Trying to stop taskid: %d, with task name: %c", i, classTasktable[i]->pcName);
		if (uint8_t res = stopTaskByTid(i)) {
			logdAssert(_ERR_, "Stoping taskid: %d, with task name: %c failed with cause: %d", i, classTasktable[i]->pcName, res);
			globalInitMutexGive();
			return(res);
			break;
		}
		logdAssert(_INFO_, "Taskid: %d, with task name: %c stopped", i, classTasktable[i]->pcName);
	}
	globalInitMutexGive();
	return(_TASKS_OK_);
}

uint8_t tasks::startTaskByName(char* pcName, bool incrementRestartCnt, bool initiateStat) {
	task_desc_t* task;
	globalInitMutexTake();
	logdAssert(_INFO_, "Trying to start/restart taskname: %c", pcName);
	if (uint8_t res = getTaskDescByName(pcName, &task)) {
		logdAssert(_ERR_, "Failed to get task descriptor by name: %s, Cause: %d", pcName, res);
		globalInitMutexGive();
		return(res);
	}
	
	if(uint8_t res = startTaskByTaskDesc(task, incrementRestartCnt, initiateStat)) {
		logdAssert(_ERR_, "Failed to start task by name: %s, Cause: %d", pcName, res);
		globalInitMutexGive();
		return(res);
	}
	globalInitMutexGive();
	logdAssert(_INFO_, "Task: %s started/restarted", pcName);
	return(_TASKS_OK_);
}

uint8_t tasks::startTaskByTid(uint8_t tid, bool incrementRestartCnt, bool initiateStat) {
	logdAssert(_INFO_, "Trying to start/restart task tid: %u", tid);
	if (tid >= _NO_OF_TASKS_) {
		logdAssert(_ERR_, "Parameter error, tid: %u", tid);
		return(_TASKS_API_PARAMETER_ERROR_);
	}
	taskInitMutexTake(classTasktable[tid]);
	if (uint8_t res = startTaskByTaskDesc(classTasktable[tid], incrementRestartCnt, initiateStat)) {
		logdAssert(_ERR_, "Could not start task by descriptor, tid: %u, cause: %u", tid, res);
		return (res);
	}
	taskInitMutexGive(classTasktable[tid]);
	logdAssert(_INFO_, "Task tid: %u started/restarted", tid);
	return(_TASKS_OK_);
}

uint8_t tasks::stopTaskByName(char* pcName) {
	task_desc_t* task;
	logdAssert(_INFO_, "Trying to stop task name: %s", task->pcName);
	globalInitMutexTake();
	if (uint8_t res = getTaskDescByName(pcName, &task)) {
		globalInitMutexGive();
		logdAssert(_ERR_, "Could not find a taske with name: %s", task->pcName);
		return(res);
	}
	if (uint8_t res = stopTaskByTaskDesc(task)) {
		globalInitMutexGive();
		logdAssert(_ERR_, "Could not stop task: %s", task->pcName);
		return(res);
	}
	else {
		globalInitMutexGive();
		logdAssert(_INFO_, "Task name: %s stopped", task->pcName);
		return(_TASKS_OK_);
	}
}

uint8_t tasks::stopTaskByTid(uint8_t tid) {
	logdAssert(_INFO_, "Trying to stop task tid: %u", tid);
	if (tid >= _NO_OF_TASKS_) {
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
		logdAssert(_INFO_, "Task tid %u stopped", tid);
		return(_TASKS_OK_);
	}
}

uint8_t tasks::getTaskDescByName(char* pcName, task_desc_t** task) {
	//logdAssert(_DEBUG_, "Trying to get task by name: %c", pcName);
	globalInitMutexTake();
	for (uint8_t i = 0; i < _NO_OF_TASKS_; i++) {
		if (uint8_t res = getTaskDescByTID(i, task)) {
			globalInitMutexGive();
			logdAssert(_ERR_, "Could not get task tid: %u, Cause: %u",i ,res);
			return(res);
		}
		if (!strcmp((*task)->pcName, pcName)) {
			globalInitMutexGive();
			logdAssert(_INFO_, "Got the task by name: %s, tid: %u" , pcName, i);
			return(_TASKS_OK_);
			break;
		}
	}
	globalInitMutexGive();
	logdAssert(_INFO_, "Didn't find any task by name: %s", pcName);
	return(_TASKS_NOT_FOUND_);
}

uint8_t tasks::getTaskDescByTID(uint8_t tid, task_desc_t **task) {
	//logdAssert(_DEBUG_, "Trying to get task by tid: %i", tid);
	if (tid >= _NO_OF_TASKS_) {
		logdAssert(_ERR_, "Parameter error, tid: %u", tid);
		return(_TASKS_API_PARAMETER_ERROR_);
	}
	
	if ((*task = classTasktable[tid]) == NULL) { //Assumed atomic not needing any lock
		logdAssert(_ERR_, "General error in getting descriptor by tid: %u", tid);
		return(_TASKS_GENERAL_ERROR_);
	}
	//logdAssert(_DEBUG_, "Task(%p) Task Pointer: %p", task, *task);
	//logdAssert(_DEBUG_, "Got task descriptor by tid: %i, taskName %s", tid, (*task)->pcName);
	//logdAssert(_DEBUG_, "**task: %p *task: %p", task, *task);
	return(_TASKS_OK_);
}

uint8_t tasks::getStalledTasks(uint8_t* tid, task_desc_t* task) {
	TaskStatus_t* pxTaskStatus;
	globalInitMutexTake();
	logdAssert(_INFO_, "Scanning for stalled tasks starting from tid: %u", *tid);
	for (uint8_t i = *tid; i < _NO_OF_TASKS_; i++) {
		task = classTasktable[i];
		if (task != NULL) {
			eTaskGetState(task->pvCreatedTask); //TODO handle return value
			if (pxTaskStatus == NULL || pxTaskStatus->eCurrentState == eDeleted) {
				*tid = i;
				globalInitMutexGive();
				logdAssert(_INFO_, "Found a stalled task tid: %u", i);
				return(_TASKS_OK_);
				break;
			}
		}
	}
	task = NULL;
	globalInitMutexGive();
	logdAssert(_INFO_, "Found no more stalled tasks");
	return(_TASKS_NOT_FOUND_);
}

uint8_t tasks::kickTaskWatchdogs(task_desc_t* task) {
	task->watchDogTimer = 0; //atomic
	return(_TASKS_OK_);
}

uint8_t tasks::clearStats(task_desc_t* task) {
	logdAssert(_INFO_, "Clearing task stats");
	if (task == NULL) {
		logdAssert(_ERR_, "Parameter error");
		return(_TASKS_API_PARAMETER_ERROR_);
	}
	
	taskInitMutexTake(task);
	task->watchdogHighWatermarkWarnAbs = (task->watchdogHighWatermarkWarnPerc) * (task->WatchdogTimeout) / 100;
	task->watchdogHighWatermarkAbs = 0;
	task->stackHighWatermarkWarnAbs = (task->stackHighWatermarkWarnPerc) * (task->usStackDepth) / 100;
	task->stackHighWatermarkAbs = 0;
	taskInitMutexGive(task);
	return(_TASKS_OK_);
}

uint8_t tasks::startTaskByTaskDesc(task_desc_t* task, bool incrementRestartCnt, bool initiateStat) {
	logdAssert(_INFO_, "Start task by descriptor");
	if (task == NULL) {
		logdAssert(_ERR_, "Parameter error - task=NULL");
		return(_TASKS_API_PARAMETER_ERROR_);
	}
	taskInitMutexTake(task);
	if (!checkIfTaskAlive(task)) {
		if (uint8_t res = stopTaskByTaskDesc(task)) {
			logdAssert(_ERR_, "Could not stop task: %s", task->pcName);
			return(_TASKS_GENERAL_ERROR_);
		}
	}
	//logdAssert(_DEBUG_, "Check if incrementing restart cnt");
	if (incrementRestartCnt) {
		if (task->escalationRestartCnt) {
			//logdAssert(_DEBUG_, "Incrementing restart cnt");
			++(task->restartCnt);
			if (task->escalationRestartCnt) {
				if (task->restartCnt >= task->escalationRestartCnt) {
					logdAssert(_PANIC_, "Task: %s has frequently restarted, escalating to system restart...", task->pcName);
					ESP.restart();
				}
			}
			else {
				//logdAssert(_DEBUG_, "Reset restart cnt");
				task->restartCnt = 0;
			}
			//logdAssert(_DEBUG_, "Incremented restart cnt, now %u", task->restartCnt);
		}
	}
	if (initiateStat) {
		clearStats(task);
	}
	//logdAssert(_DEBUG_, "*task %p taskhandle %p", task, task->pvCreatedTask);
	//(_DEBUG_, "Resetting watchdog timer for Task: %s", task->pcName);
	task->watchDogTimer = 0;
	//logdAssert(_DEBUG_, "Watchdog cleared");

	//logdAssert(_DEBUG_, "Starting task");
	if (xTaskCreatePinnedToCore(
		task->pvTaskCode,
		task->pcName,
		task->usStackDepth,
		task,
		task->uxPriority,
		&(task->pvCreatedTask),
		task->xCoreID) != pdPASS) {
		taskInitMutexGive(task);
		logdAssert(_ERR_, "General error, the task was probably not started");
		return(_TASKS_GENERAL_ERROR_);
	}
	//logdAssert(_DEBUG_, "Giving back the Mutex");
	taskInitMutexGive(task);
	//logdAssert(_DEBUG_, "The Mutex is given back");
	logdAssert(_INFO_, "Task: %s was started/restarted", task->pcName);
	return(_TASKS_OK_);
}

uint8_t tasks::stopTaskByTaskDesc(task_desc_t* task) {
	logdAssert(_INFO_, "Trying to stop task");
	taskInitMutexTake(task);
	if (!checkIfTaskAlive(task)) {
		//logdAssert(_DEBUG_, "I shouldnt be here");
		vTaskDelete(task->pvCreatedTask);
		taskInitMutexGive(task);
		logdAssert(_INFO_, "Task: %s stopped", task->pcName);
		return(_TASKS_OK_);
	}
	else {
		taskInitMutexGive(task);
		logdAssert(_ERR_, "Could not stop task: %s", task->pcName);
		return(_TASKS_GENERAL_ERROR_);
	}
}

uint8_t tasks::checkIfTaskAlive(task_desc_t* task) {
	taskInitMutexTake(task);
	//logdAssert(_DEBUG_, "Mutex taken");
	//logdAssert(_DEBUG_, "*task %p taskhandle %p", task, task->pvCreatedTask);
	if (task->pvCreatedTask == NULL) {
		logdAssert(_INFO_, "Task does not exist, handle = NULL");
		taskInitMutexGive(task);
return (_TASK_DELETED_);
	}
	if (eTaskGetState(task->pvCreatedTask) == eDeleted) {
		logdAssert(_INFO_, "Task deleted");
		taskInitMutexGive(task);
		return (_TASK_DELETED_);
	}
	else {
		//logdAssert(_DEBUG_, "Task exists");
		taskInitMutexGive(task);
		return (_TASK_EXIST_);
	}
}

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

uint8_t tasks::taskInitMutexTake(task_desc_t* task, TickType_t timeout) { //Fine grained task locks not yet implemente
	return(globalInitMutexTake(timeout));
}

uint8_t tasks::taskInitMutexGive(task_desc_t* task) { //Fine grained task locks not yet implemente
	return(globalInitMutexGive());
}

void tasks::runTaskWatchdogs(TimerHandle_t xTimer) {
	task_desc_t* task;
	bool decEsc = false;

	//logdAssert(_DEBUG_, "Running watchdogs");
	initd.globalInitMutexTake();
	if (++initd.tenMsCnt >= 6000) {
		initd.tenMsCnt = 0;
		decEsc = true;
	}
	for (uint8_t i = 0; i < _NO_OF_TASKS_; i++) {
		//	logdAssert(_DEBUG_, "**classTasktable[%u] %p classTasktable[%u] %p", i, &classTasktable[i], i, classTasktable[i]);
		//	logdAssert(_DEBUG_, "**task %p *task %p", &task, task);
		initd.getTaskDescByTID(i, &task);
		//	logdAssert(_DEBUG_, "**task %p *task %p", &task, task);
		if (task == NULL) {
			//		logdAssert(_PANIC_, "Could not find task by id: %u", i);
			ESP.restart();
		}
		//	logdAssert(_DEBUG_, "Breakpoint 1");
		if (decEsc && task->restartCnt) {
			//		logdAssert(_DEBUG_, "Breakpoint 2");
			task->restartCnt--;
		}
		//logdAssert(_DEBUG_, "Breakpoint 3, watchdog timer %u", task->watchDogTimer);
		if (task->WatchdogTimeout && task->watchDogTimer != _WATCHDOG_DISABLE_) {
			//	logdAssert(_DEBUG_, "Breakpoint 4");
			if (initd.checkIfTaskAlive(task)) {
				logdAssert(_ERR_, "Task: %s  has stalled", task->pcName);
				initd.startTaskByTid(i, true, false);
			}
			//logdAssert(_DEBUG_, "Breakpoint 5");
			if (++(task->watchDogTimer) >= task->WatchdogTimeout) {
				logdAssert(_ERR_, "Watchdog timeout for Task: %s, watchdog counter: %u", task->pcName, task->watchDogTimer);
				initd.startTaskByTid(i, true, false);
			}
			else {
				//logdAssert(_DEBUG_, "Breakpoint 6");
				if (task->watchDogTimer > task->watchdogHighWatermarkAbs) {
					//logdAssert(_DEBUG_, "Breakpoint 7");
					task->watchdogHighWatermarkAbs = task->watchDogTimer;
					if (task->watchdogHighWatermarkWarnPerc && task->watchdogHighWatermarkWarnAbs <= task->watchdogHighWatermarkAbs) {
						//The logging below is likely causing stack overflow - need to increase configMINIMAL_STACK_SIZE
						//logdAssert(_WARN_, "Watchdog count for Task: %s reached Warn level of: %u  percent, watchdog counter: %u" ,task->pcName, task->watchdogHighWatermarkWarnPerc, task->watchDogTimer);
					}
				}
			}
		}
		//logdAssert(_DEBUG_, "Breakpoint 8 Task: %s, restartcnt %u", task->pcName, task->restartCnt);
		if (task->escalationRestartCnt && task->restartCnt >= task->escalationRestartCnt) {
			logdAssert(_PANIC_, "Task: %s has restarted exessively, escalating to system restart", task->pcName);
			ESP.restart();
		}
	}
	initd.globalInitMutexGive();
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
	void* search_p = *taskResourceRoot_pp;
	
	if (taskResourceRoot_pp == NULL || taskResourceObj_p == NULL) {
		//logdAssert(_DEBUG_, "API error, one of task_resource_t** taskResourceRoot_pp, void* taskResourceObj_p is NULL");
		return(_TASKS_API_PARAMETER_ERROR_);
	}
	//logdAssert(_DEBUG_, Linking a resource to resource root pointer: %p", *taskResourceRoot_pp);
	if ((taskResourceDesc_p = malloc(sizeof(task_resource_t))) == NULL) {
		//logdAssert(_DEBUG_, "Could not allocate memory for resource descriptor");
		return(_TASKS_RESOURCES_EXHAUSTED_);
	}
	if (*taskResourceRoot_pp == NULL) {
		*taskResourceRoot_pp = (task_resource_t*)taskResourceDesc_p;
		((task_resource_t*)taskResourceDesc_p)->prevResource_p = NULL;
	}
	else {
		while (((task_resource_t*)search_p)->nextResource_p != NULL) {
			search_p = ((task_resource_t*)search_p)->nextResource_p;
		}
		((task_resource_t*)search_p)->nextResource_p = (task_resource_t*)taskResourceDesc_p;
		((task_resource_t*)taskResourceDesc_p)->prevResource_p = (task_resource_t*)search_p;
	}
	((task_resource_t*)taskResourceDesc_p)->taskResource_p = taskResourceObj_p;
	((task_resource_t*)taskResourceDesc_p)->nextResource_p = NULL;
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
	if (taskResourceRoot_pp == NULL || taskResourceDesc_p == NULL) {
		logdAssert(_ERR_, "API error, one of task_resource_t** taskResourceRoot_pp, task_resource_t* taskResourceDesc_p is NULL");
		return(_TASKS_API_PARAMETER_ERROR_);
	}
	//logdAssert(_DEBUG_, "Un-register/Un-linking task resource: %p", taskResourceDesc_p);
	if (uint8_t res = findResourceDescByResourceObj(taskResourceObj_p, *taskResourceRoot_pp, &taskResourceDesc_p)) {
		logdAssert(_ERR_, "Could not find Resource Desc By ResourceObj, return code: %u", res);
		return(res);
	}
	if (taskResourceDesc_p->nextResource_p == NULL) {
		if (taskResourceDesc_p->prevResource_p == NULL) {
			*taskResourceRoot_pp = NULL;
		}
		else {
			(taskResourceDesc_p->prevResource_p)->nextResource_p = NULL;
		}
	}
	else {
		if (taskResourceDesc_p->prevResource_p == NULL) {
			*taskResourceRoot_pp = taskResourceDesc_p->nextResource_p;
			(taskResourceDesc_p->nextResource_p)->prevResource_p = NULL;
		}
		else {
			(taskResourceDesc_p->nextResource_p)->prevResource_p = taskResourceDesc_p->prevResource_p;
			(taskResourceDesc_p->prevResource_p)->nextResource_p = taskResourceDesc_p->nextResource_p;
		}
	}
	free(taskResourceDesc_p);
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

uint8_t tasks::findResourceDescByResourceObj(void* resourceObj_p, task_resource_t* taskResourceRoot_p, task_resource_t** taskResourceDesc_pp) {

	if (resourceObj_p == NULL || taskResourceRoot_p == NULL) {
		logdAssert(_ERR_, "API error, one or both of: void* resourceObj_p, task_resource_t* taskResourceRoot_p is NULL");
		return(_TASKS_API_PARAMETER_ERROR_);
	}
	//logdAssert(_DEBUG_, "finding Task resource descriptor by resource object: %p", resourceObj_p);
	*taskResourceDesc_pp = taskResourceRoot_p;
	while ((*taskResourceDesc_pp)->nextResource_p != NULL && (*taskResourceDesc_pp)->taskResource_p != resourceObj_p) {
		(*taskResourceDesc_pp) = (*taskResourceDesc_pp)->nextResource_p;
	}
	if ((*taskResourceDesc_pp)->taskResource_p == resourceObj_p) {
		return(_TASKS_OK_);
	}
	else {
		*taskResourceDesc_pp = NULL;
		logdAssert(_INFO_, "Could not find Task resource descriptor by resource object: %p", resourceObj_p);
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

	//take mem lock per task_p
	if (size == 0 || task_p == NULL) {
		logdAssert(_ERR_, "API error, size=0 or task_p=NULL");
		//give mem lock per task_p
		return(NULL);
	}
	//logdAssert(_DEBUG_, "Allocating memory of size: %u: requested from task: %s", size, task_p->pcName)
	if ((mem_p = malloc(size)) == NULL) {
		logdAssert(_ERR_, "Could not allocate memory");
		//give mem lock per task_p
		return(NULL);
	}
	if (uint8_t res = taskResourceLink(&(task_p->dynMemObjects_p) , mem_p)) {
		free(mem_p);
		logdAssert(_ERR_, "Register/link dynamic memory resource with return code: %u", res);
		//give mem lock per task_p
		return(NULL);
	}
	//logdAssert(_DEBUG_, "Memory allocated at base adrress: %p, Size: %u", mem_p, size);
	//give mem lock per task_p
	return(mem_p);
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
	task_resource_t** taskResourceDesc_pp;

	//take mem lock per task_p
	if (task_p == NULL || mem_p == NULL) {
		logdAssert(_ERR_, "API error, one of ask_desc_t* task_p, void* mem_p is NULL");
		return(_TASKS_API_PARAMETER_ERROR_);
		//give mem lock per task_p
	}
	//logdAssert(_DEBUG_, "Task memory free of mem_p: %p", mem_p);
	free(mem_p);
	if (uint8_t res = findResourceDescByResourceObj(mem_p, task_p->dynMemObjects_p, taskResourceDesc_pp)) {
		logdAssert(_ERR_, "Memory resource object - mem_p: %p not found, memory may not have been successfully freed", mem_p);
		//Give mem lock per task_p
		return(res);
	}
	if (uint8_t res = taskResourceUnLink(&(task_p->dynMemObjects_p), *taskResourceDesc_pp)) {
		logdAssert(_ERR_, "Un-Register/un-linking memory resource failed - resason: %u", res);
		//Give mem lock per task_p
		return(res);
	}
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
	logdAssert(_INFO_, "freing all memory allocated by task: %s", task_p->pcName);
	//take mem lock per task_p
	while (task_p->dynMemObjects_p != NULL) {
		if (uint8_t res = taskMfree(task_p, (task_p->dynMemObjects_p->taskResource_p))) {
			logdAssert(_INFO_, "Could not free memory, result code: %u", res);
			//Give mem lock per task_p
			return(res);
		}
	} 
	//´Give mem lock per task_p
	return(_TASKS_OK_);
}
/* ------------------------------------ END PRIVATE:taskMfreeAll ------------------------------------*/

/* ******************************** END Memory resources management ******************************** */


/*void init(void* pvParameters) {
	tasks initTasks;

	if (uint8_t ret = initTasks.startAllTasks(_TASKS_DONOT_INITIATE_STATS_)) {
		//analyse start error - probably Panic;
	}
	
	while (true) {
		// Kick init watchdog
		// release Task memory when task is terminated
		// Check task Stats
		// depend on log level write to Serial, and to ...
	}
}
*/