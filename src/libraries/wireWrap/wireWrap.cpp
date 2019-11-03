#include "wireWrap.h" 


// Wire/I2c Channel assignment 
//static uint8_t chStatus a descriptor of active I2C channels [0..7] constructs pointing to constructs of [SDA,SCL,*users], where users is a linked list: [*prev, *current, *next]
// pointing at these constructs [wireWrap objHandle]
 
//Transaction queues
//7 prio queues with a [0..7] constructs for every prio, every queue has a linked [*prev, *current, *next] construct which points to constructs of work orders [Order status........]
// of every queue have a linked list of items in queue order: transactio....





//Begin of CPP file
//#include <wireWrap.h>

/* ***********************************************************************************************
   initialization of static wireWrap class static data                                           */
ch wireWrap::channels[4] = { {_UNINIT, 0, 0, NULL}, {_UNINIT, 0, 0, NULL}, {_UNINIT, 0, 0, NULL}, {_UNINIT, 0, 0, NULL} };
transact* wireWrap::transactQueues[4][4] = { {NULL, NULL, NULL, NULL}, {NULL, NULL, NULL, NULL}, {NULL, NULL, NULL, NULL}, {NULL, NULL, NULL, NULL} };
uint8_t _maxPrioQueueDepth = 20;
uint8_t channelStatus = _UNINIT
/* ********************************************************************************************* */


/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*                  ***BEGIN***Class management functions***BEGIN***                             */
/* ********************************************************************************************* */

/* ********************************************************************************************* */
/* wireWrap class object constructor                                                             */
wireWrap::wireWrap(){ 
}

/* ********************************************************************************************* */
/* wireWrap class object destructor                                                              */
// Todo
wireWrap::~wireWrap() {
}
/* ********************************************************************************************* */
/*                      ***END***Class MANAGEMENT functions***END**                              */ 
/* ********************************************************************************************* */
/* ********************************************************************************************* */




/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*                       ***BEGIN***Class user functions***BEGIN***                              */
/* ********************************************************************************************* */

/* ********************************************************************************************* */
/* wireWrap request channel init function                                                        */
/* Type: User                                                                                    */
/* Description: TODO                                                                             */
uint8_t wireWrap::chInit(uint8_t sda,  uint8_t scl){
	if (uint8_t res = createChannel(sda, scl)){
		return(res);
	}
	_status = _INIT;
	return(_WW_OK);
 }

/* ********************************************************************************************* */
/* wireWrap request transaction function                                                         */
/* Type: User                                                                                    */
/* Description: TODO                                                                             */
uint8_t wireWrap::requestTransaction(uint8_t prio, transactCallback cb) {
	if (prio > 3) { 
		return(_WW_API_ERROR);
	}
	//set per channel channel mutex
	//
	if (transactQueues[_channel][prio] == NULL) {
		transactQueues[_channel][prio] = (struct transact*)malloc((sizeof(struct transact)));
		transactQueues[_channel][prio]->prev = NULL;
		transactQueues[_channel][prio]->chObj = this;
		transactQueues[_channel][prio]->fp = cb;
		transactQueues[_channel][prio]->next = NULL;
	}
	else {
		transact* p = transactQueues[_channel][prio];
		int i = 0;
		while (p->next != NULL) {
			p = p->next;
			if (++i > _maxPrioQueueDepth) {
				//release mutex
				//
				return(_WW_OUT_OF_RESOURCES);
				break;
			}
		}
		p->next = (struct transact*)malloc((sizeof(struct transact)));
		p->next->prev = p;
		p->next->chObj = this;
		p->next->fp = cb;
		p->next->next = NULL;
	}
	//release mutex
	//
	return(_WW_OK);
}

/* ********************************************************************************************* */
/* wireWrap abort transaction function                                                           */
/* Type: User                                                                                    */
/* Description: TODO                                                                             */
uint8_t wireWrap::abortTransaction() {
}

/* ********************************************************************************************* */
/* wireWrap write function                                                                       */
/* Type: User                                                                                    */
/* Description: TODO                                                                             */
uint8_t wireWrap::write(uint8_t baseAddr, uint8_t* data, uint8_t size) {
//Check that the class object has been granted a transaction slot
}

/* ********************************************************************************************* */
/* wireWrap read function                                                                        */
/* Type: User                                                                                    */
/* Description: TODO                                                                             */
uint8_t wireWrap::read(uint8_t baseAddr, uint8_t* data, uint8_t size) {
//Check that the class object has been granted a transaction slot
}

/* ********************************************************************************************* */
/*                         ***END***Class user functions***END**                                 */
/* ********************************************************************************************* */
/* ********************************************************************************************* */



/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*                      ***BEGIN***Class server functions***BEGIN***                             */
/* ********************************************************************************************* */

/* ********************************************************************************************* */
/* wireWrap scheduleTransaction function                                                         */
/* Type: RTOS server                                                                             */
/* Description: TODO                                                                             */
uint8_t wireWrap::scheduleTransaction(uint8_ channel) {
	uint8_t i = 0;
	while (true) {
		while (transactQueues[channel][_WW_TRANSACT_QOS_PRIO] != NULL) {
			transaction(transactQueues[channel][_WW_TRANSACT_QOS_PRIO]);
		}
		if (i << 5 != 7 && transactQueues[channel][_WW_TRANSACT_QOS_WFQ_1 != NULL]) {
			scheduleTransaction(transactQueues[channel][_WW_TRANSACT_QOS_WFQ_1]);
		}
		else if (_WW_TRANSACT_QOS_WFQ_1 != NULL) {
			scheduleTransaction(transactQueues[channel][_WW_TRANSACT_QOS_WFQ_8]);
		}
		i++;
	}
}

/* ********************************************************************************************* */
/* wireWrap ~scheduleTransaction function                                                       */
/* Type: RTOS server                                                                                    */
/* Description: TODO                                                                             */
uint8_t wireWrap::~scheduleTransaction(uint8_ channel) {
}

/* ********************************************************************************************* */
/* wireWrap transactTimeOut function                                                       */
/* Type: RTOS server                                                                                    */
/* Description: TODO                                                                             */
uint8_t wireWrap::transactTimeOut(uint8_ channel) {
//Tricky
//Assert the big lock (not further analyzed)
//Mark the running object transaction (and possibly the whole channel) as failed. 
//Kill the forked callback process, the termination of the process needs to always be caught by the schedule process (to kill the watchdog, etc)
//Kill the parent object process - which needs to be monitored by it's parent process
//Thoroughly Destroy the global- and this objects involved.
// log the event as Critical.
}
/* ********************************************************************************************* */
/*                        ***END***Class server functions***END**                                */
/* ********************************************************************************************* */
/* ********************************************************************************************* */




/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*                      ***BEGIN***Class helper functions***BEGIN***                             */
/* ********************************************************************************************* */

uint8_t createTransactObj(*void root, *void object, uint8_t method) {
// TODO, method = {_WW_TRANSACT_OBJECT_FIRST, _WW_TRANSACT_OBJECT_LAST}
}

uint8_t destroyTransactObj(*void root, uint8_t method, objId = NULL) {
// TODO, method = {_WW_TRANSACT_OBJECT_FIRST, _WW_TRANSACT_OBJECT_LAST, _WW_TRANSACT_OBJECT_OBJID, _WW_TRANSACT_OBJECT_ALL}
}

uint8_t createChannelObj(*void root, *void object, uint8_t method) {
	// TODO, method = {_WW_TRANSACT_OBJECT_FIRST, _WW_TRANSACT_OBJECT_LAST}
}

uint8_t destroyChannelObj(*void root uint8_t method, objId = NULL) {
// TODO, method = {_WW_CHANNEL_OBJECT_FIRST, _WW_CHANNEL_OBJECT_LAST, _WW_CHANNEL_OBJECT_OBJID, _WW_CHANNEL_OBJECT_ALL
}
uint8_t wireWrap::transaction(uint8_t channel, transact* transactQueue) {
	//set per channel mutex
	//
	transactQueue.next->chObj->_transactStatus = _WW_TRANSACT_ACTIVE //todo in sched add _WW_TRANSACT_QUEUED
	// add a global per channel transaction status
	transactCallback p = (*(transactQueue.next->transactCallback))(1000); //1000 uS watchdog
	destroyTransactObj(transactQueue, _WW_TRANSACT_OBJECT_FIRST); //TODO Move to after the the return for fault managementpurposes ..destroyTransactObj(_WW_TRANSACT_OBJECT_ID, ObjectID) ); 
		// start the per channel transaction watch dog
		// release the mutex
		if (p != NULL) {
			p; //Should be forked to a process
		}
	// stop the watch dog
		return(_WW_OK);
}

uint8_t wireWrap::addChannelUserObj(ch channel){
  chObjStat* p = (chObjStat*)malloc(sizeof(struct chObjStat));
  chObjStat* pp;
  
  if (p == NULL) {
    return(_WW_OUT_OF_RESOURCES);
  }
  else{
    p->chObj = this;
    if (channel.next == NULL) {
      channel.next = p;
      return(_WW_OK);
    }
    else {
      pp = channel.next;
      while (pp->next != NULL) {
        pp=pp->next; 
      }
      pp->next = p;
      p->prev = pp;
      return(_WW_OK); 
    }
  }
}

uint8_t wireWrap::createChannel(int8_t sda,  uint8_t scl){
  int i;
  if(sda == scl) {
    return(_WW_I2CONFLICT);
  }
  for (i=0; 3; i++) {
    if (channels[i].chState ==_INIT && channels[i].sda == sda && channels[i].scl == scl) {
      //Create an object
      _channel = i;
      addChannelUserObj(channels[i]);
      return(_WW_OK);
      break;
    } 
    else {
      if (channels[i].sda == sda || channels[i].sda == scl || channels[i].scl == sda || channels[i].scl == scl) {
        return(_WW_I2CONFLICT);
        break;
      }
    }
  }
  for (i=0; 3; i++) {
    if (channels[i].chState ==_UNINIT) {
      _channel = i;
      channels[i].chState =_INIT;
      channels[i].sda = sda;
      channels[i].scl = scl;
      //Create an object
      return(_WW_OK);
      break;
    }
    return(_WW_OUT_OF_RESOURCES);
  }
}
/* ********************************************************************************************* */
/*                        ***END***Class helper functions***END**                                */
/* ********************************************************************************************* */
/* ********************************************************************************************* */




/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*                        ***BEGIN***freeRTOS run time***BEGIN***                                */
/* ********************************************************************************************* */

/* ********************************************************************************************* */
/*                           ***END***freeRTOS run time***END**                                  */
/* ********************************************************************************************* */
/* ********************************************************************************************* */




/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*                           ***BEGIN***Applications***BEGIN***                                  */
/* ********************************************************************************************* */

/* ********************************************************************************************* */
/*                             ***END***Applications***END**                                     */
/* ********************************************************************************************* */
/* ********************************************************************************************* */




/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*                         ***BEGIN***ARDUINO run time***BEGIN***                                */
/* ********************************************************************************************* */
void setup() {
  // put your setup code here, to run once:
}

void loop() {
  // put your main code here, to run repeatedly:
}
/* ********************************************************************************************* */
/*                           ***END***Arduino run time***END**                                   */
/* ********************************************************************************************* */
/* ********************************************************************************************* */