/* ********************************************************************************************* */
/* ********************************************************************************************* */
/*                   ***BEGIN***wireWrapper heading file***BEGIN***                              */
/* ********************************************************************************************* */

#ifdef ................

#include <Wire.h>
//and many more

/* ********************************************************************************************* */
/* wireWrap data type definitions                                                                */
/* Type: User                                                                                    */
/* Description: TODO                                                                             */

 //parameter provides uS allowed to spend in the callback routine
typedef void(transactCallback*)(void* transactHandle_p, void* transactCallbackParam_p);
typedef void(transactResCallback*)(void* transactHandle_p, void* transactResCallbackParam_p);

typedef struct chObjStat chObjStat;
//Common data structures 

struct wireStateStruct {
	uint8_t WireState;
	uint8_t sda;
	uint8_t scl;
	uint32_t speed;
};

struct transact {
	transact* prev_p;
	transactCallback* transactCallback_p;
	void* transactCallbackParam_p;
	transactResCallback* transactResCallback_p;
	void* transactResCallbackParam_p;
	transact* next;
};

//I2C Wirwe State should those be aligned with the API return codes?
const uint8_t _WW_WIRE_OK_ = 0;
const uint8_t _WW_WIRE_UNINIT_ = 1;
const uint8_t _WW_WIRE_GENERAL_FAIL_ = 2;
const uint8_t _WW_WIRE_IN_USE_ = 3;
const uint8_t _WW_WIRE_RECOVERY_ = 4;

//API return codes
//Needs alignment with the CoreOS return codes
const uint8_t _WW_OK = 0;
const uint8_t _WW_I2CONFLICT = 1;
const uint8_t _WW_RESOURCE_CONFLICT = 2;
const uint8_t _WW_INIT_FAILURE = 3;
const uint8_t _WW_GENERAL_FAILURE = 4;
const uint8_t _WW_TRANSACTION_NOT_ACTIVE = 5;
const uint8_t _WW_OUT_OF_RESOURCES = 6;
const uint8_t _WW_API_ERROR = 7;

//Below needs stricter definition and purpose
enum wwWireWrapTransactQoS { _WW_TRANSACT_QOS_BEF = 0, _WW_TRANSACT_QOS_WFQ_1 = 8, _WW_TRANSACT_QOS_WFQ_1 = 2, _WW_TRANSACT_QOS_STRICT_PRIO = 3 };

/* ********************************************************************************************* */
/* wireWrap class definitions                                                                    */
/* Type: User, Helpers & RTOS                                                                    */
/* Description: TODO                                                                             */
class wireWrap {
public:
	wireWrap(); //Initiate a new I2C channel object (but not the I2C channel it self)
	~wireWrap(); //Destruction of channel object not supported, will cause panic
	uint8_t wireChInit(uint8_t sda, uint8_t scl, ??? speed); //Initiate an I2C channel
	void* requestWireTransact(uint8_t prio, TickType_t timeout, transactCallback* tcb, void* tcbParam, resultCallback* trcb, void* trcbParam); //Requests an I2C transaction slot of length of 
											 // max timeOut ticks. The callback runs to completion
											 // and may therefore never block for any significant time
											 // if time-out occurs, the wireWrap worker process running the
											 // calback is restarted causing the callback to instantly be terminated.
											 // When the call back returns or if time-out and the resultcallback is called
	void* ISRrequestTransaction(uint8_t prio, TickType_t timeout, transactCallback cb resultCallback rcb, void* cbParam); //much the same as above, but from and ISR
	uint8_t wireStatus();	//Requests the i2c channel status
	uint8_t wireTransactStatus(void* transactHandle); //Requests the status on a transaction 
	uint8_t wireWrite(uint8_t baseAddr, uint8_t* data, uint8_t size = 1); //Write to an i2c address
	uint8_t wireRead(uint8_t baseAddr, uint8_t* data, uint8_t size = 1); //Read from an i2c address
	uint8_t wireAbortTransact(void* transactHandle);

private:
	// common private class data structures
	static uint8_t createdWires;
	// private class data structures
	wireStateStruct wireProperties;
	transact* wireTransactQueue[_MAX_I2C_PRIOS_];
	uint8_t wireTransactPrioQueueDepth[_MAX_I2C_PRIOS_]; //define constant _MAX_PRIO_QUEUE_DEPTH[_MAX_I2C_PRIOS_]
	uint8_t wireTransactTotQueueDepth; //define constant _MAX_TOT_QUEUE_DEPTH
	uint32_t wireTransactPrioDrops[_MAX_I2C_PRIOS_]; 
	//Define statistics and watermarks and escalations more carefully!!
	uint8_t wireStatus;
};
/* ********************************************************************************************* */
/*                     ***END***wireWrapper heading file***END***                                */
/* ********************************************************************************************* */
/* ********************************************************************************************* */
