#ifndef _CAN_DEV_ERRORCODE_H
#define _CAN_DEV_ERRORCODE_H


// error code define: errno-base.h  +  errno.h

#define ECAN_NoError            0
#define ECAN_RequestAborted     501 // or timeout
#define ECAN_ArbitrationLost    502
#define ECAN_BusError           503
#define ECAN_BusOff             504
#define ECAN_Stopped            505 // hardware stop
#define ECAN_AbortFailure       506 // restart can


#endif
