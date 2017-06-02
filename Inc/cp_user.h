
#ifndef __cp_user_H
#define __cp_user_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"

/******************************************************************************/
/** DEFINES */
/******************************************************************************/
/** Maximale Anzahl von unterschiedlichen Empfangs-IDs */
#define NUMBDATAOBJMAX  10

typedef enum
{
	CP_OK      	= 0x00,
	CP_ERROR    = 0x01,
	CP_BUSY    	= 0x02,
	CP_TIMEOUT  = 0x03
}CP_StatusTypeDef;


/******************************************************************************/
/** VARIABLES - DECLARATION OF DATAOBJECTS */
/******************************************************************************/

/**
* Data Object Senden Board1
*/
struct DataObj1
	{
			char		  D1;
	}DataObj_MC1_1;

/**
 * Data Object Senden Board1
 */
struct DataObj2
	{
		uint32_t		Value[4];      /*> 4*4 Byte  */
	}DataObj_MC1_2;

/**
 * Data Object Senden Board1
 */
#define MAXLENGTHTEXT 50
struct DataObj3
	{
		uint8_t		  c[MAXLENGTHTEXT];      /*> char array  */
		uint8_t			button_state;
		uint16_t		manual;      /*> Messwert Manuell */
		uint16_t		adcval;      /*> Messwert ADC */
		uint16_t		sin_val;		 /*> Sinus Value float=4 Byte */
	}DataObj_MC1_3;


/**
 * Data Object Senden Board2
 */
struct DataObj4
	{
		uint16_t		  adcval;      /*> Messwert ADC */
	}DataObj_MC2_1;



/******************************************************************************/
/** FUNCTION DECLARATIONS */
/******************************************************************************/

//int16_t CP_searchIDRx(uint16_t ID);

/** Receive Functions */
CP_StatusTypeDef CP_InitRx(uint8_t *ptrDataObj, uint32_t size, uint16_t ID);
extern CP_StatusTypeDef CP_StartRx(uint16_t ID);
extern CP_StatusTypeDef CP_StatusRx(uint16_t ID);
extern CP_StatusTypeDef CP_DeleteRx(uint16_t ID);

/** Transmit Functions */
extern CP_StatusTypeDef CP_InitTx(uint8_t *ptrDataObj, uint32_t size, uint16_t ID);
extern CP_StatusTypeDef CP_StartTx();
extern CP_StatusTypeDef CP_StatusTx();



/******************************************************************************/
/** EXTERN REFERENCES (ONLY FOR USER INFORMATION)*/
/******************************************************************************/

/** 2D Array: Fehler Beschreibung des CAN-PROTOKOLLS */
extern const char *CP_ERROR_DESCRIPTION[];
/** Fehler-Historie der aufgetretenen Fehler. Neuester Fehler auf Position 0 */
extern 	uint8_t CP_LastErrorCodes[3];

 
 
 
 
 
 
 
 
 
 
 
 
 
 
#ifdef __cplusplus
}
#endif
#endif /*__ cp_user_H */
 
 
