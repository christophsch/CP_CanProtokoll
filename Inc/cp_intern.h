
#ifndef __cp_intern_H
#define __cp_intern_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"
#include "cp_user.h"




/******************************************************************************/
/** DEFINES */
/******************************************************************************/

#define TIMEOUTCANTX	1500


/******************************************************************************/
/** VARIABLES AND CONSTANTS */
/******************************************************************************/

/**
* CAN RROTOCOL CONTROL FIEDLDS
*/
struct CPcontrolFieldsTx{
	uint16_t  arrID;
	uint32_t  arrLength;
	uint32_t  arrOffset;
	uint8_t   *arrPtrObj;
	uint8_t 	arrStatus;
}CPcontrolFieldsTX;

struct CPcontrolFieldsRx{
  uint16_t  arrID[NUMBDATAOBJMAX];
  uint32_t  arrLength[NUMBDATAOBJMAX];
  uint32_t  arrOffset[NUMBDATAOBJMAX];
  uint8_t   *arrPtrObj[NUMBDATAOBJMAX];
  uint8_t 	arrStatus[NUMBDATAOBJMAX];
}CPcontrolFieldsRX;

/** Variable zum Sperren der Parallelen Bearbeitung des CallBack-SendeProzess*/
extern uint8_t CP_LockCANtx;
extern uint16_t missed_ir;

/** 2D Array: Fehler Beschreibung des CAN-PROTOKOLLS */
extern const char *CP_ERROR_DESCRIPTION[];
/** Fehler-Historie der aufgetretenen Fehler. Neuester Fehler auf Position 0 */
extern 	uint8_t CP_LastErrorCodes[3];



/******************************************************************************/
/** FUNCTION DECLARATIONS */
/******************************************************************************/

/** Interrupt Callback Functions */
extern void HAL_CAN_TxCpltCallback(CAN_HandleTypeDef *CanHandle);

/** Other Functions */
extern void 		CP_CAN_Init();
extern void 		CP_InitControlFieldsTx();
extern void 		CP_InitControlFieldsRx();
extern int16_t 	CP_searchIDRx(uint16_t ID);
extern void 		CP_SaveErrorCode(uint32_t ErrorCode);

 








 
#ifdef __cplusplus
}
#endif
#endif /*__ cp_intern_H */
 
 
