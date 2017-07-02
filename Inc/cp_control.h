/**
 * CP_Steuerungsschicht
 * @file        		cp_control.h
 * @ingroup     	CP_Control_Layer
 * @author      	Christoph Scharf
*/


#ifndef __cp_control_H
#define __cp_control_H
#ifdef __cplusplus
 extern "C" {
#endif

/****************************************************************************/
/* Includes */
/****************************************************************************/

#include "stm32f3xx_hal.h"
#include "cp_user.h"


/****************************************************************************/
/* DEFINES */
/****************************************************************************/

//#define TIMEOUTCANTX	1500


/****************************************************************************/
/* VARIABLES AND CONSTANTS */
/****************************************************************************/

/**
 * @defgroup Steuerungsfelder Steuerungsfelder
 * @ingroup CP_Control_Layer
 * @{
 *
 * CP Protokoll Steuerungsfelder / ControlFields.
 * 
 * Der aktuelle Sende- oder Empfangszustand an einer Stelle gespeichert 
 * und abrufbar sein, um an der zuletzt bearbeiteten Position der Kommunikation 
 * fortzufuehren. 
 * Die „Steuerungsfelder“ sind die zentralen Zustandsvariablen, in denen 
 * alle Informationen ueber den aktuellen Sende- und Empfangsprozess 
 * gespeichert werden. Ein Steuerungsfeld fuer das Senden, eines fuer den 
 * Empfang. Hier wird gespeichert, welche IDs verwendet werden, dessen 
 * Zuordnung zu dem @ref Datenobjekt, und der aktuelle Status des 
 * Versendens / des Empfangs. 
 * 
 */
 
 
/****************************************************************************/
/**
 * CP Sende Steuerungsfeld / CP Controlfield Tx.
 * 
 * Bei den Steuerelementen des Sendevorgangs handelt es sich um Elemente 
 * in einem Strukturelement mit dem Variablennamen: „CPcontrolFieldsTX“, 
 * wie in nachfolgender Abbildung dargestellt.
 * Die „ID“ und dessen Zuordnung zum @ref Datenobjekt, d.h. die Adresse: 
 * „Pointer“, sowie die Groesse „Length“ des Datenobjekts, wird in der der 
 * Init-Funktion CP_InitTx(...) in das Strukturelement gespeichert. 
 * Im Element „Offset“ wird waehrend eines Sendeprozesses das zuletzt 
 * versendete Byte gespeichert, um bei segmentierten Datenobjekten bei 
 * dem letzten Zustand das Senden fortzusetzen. Im Feld „Status“ wird 
 * der aktuelle Sendestatus gespeichert. Dieser ist in der Initialisierungsphase, 
 * sowie nach einem Erfolgreichen Versenden des Datenobjekts „CP_OK“. 
 * Waehrend des Sendevorgangs ist der Status „CP_BUSY“ und bei einem 
 * aufgetretenen Fehler „CP_ERROR“. Sofern ein Fehler aufgetreten ist, 
 * kann dieser in der Fehlerliste ausgelesen werden.
 * 
 * Da der Sendeprozess gezielt angesteuert werden kann, reicht es hier aus, 
 * dass nur jeweils eine ID und damit ein Datenobjekt initialisiert und 
 * gesendet wird. Nach dem Senden dieses Datenobjekts kann das 
 * naechste zu versendende Datenobjekt initialisiert und versendet werden.
 * 
 */
struct CPcontrolFieldsTx{
	uint16_t  arrID;			/**< Initialisierte ID */
	uint32_t  arrLength;	/**< Groesse des Datenobjekts in Byte */
	uint32_t  arrOffset;	/**< Anzahl bereits versendete Datenbytes  */
	uint8_t   *arrPtrObj;	/**< Zeiger auf Datenobjekt */
	uint8_t 	arrStatus;	/**< Kommunikationszustand des Sendeobjekts #CP_StatusTypeDef*/
}CPcontrolFieldsTX;

/****************************************************************************/
/**
 *
 * CP Empfangs Steuerungsfeld / CP Controlfield Rx.
 * 
 * Bei dem Empfang von einem @ref Datenobjekt ist nicht bekannt, zu welcher Zeit 
 * welche Datenobjekte von den anderen CAN-Knoten versendet werden. 
 * Daher wurde hier vorgesehen, dass mehrere IDs und damit mehrere 
 * Datenobjekte gleichzeitig fuer den Empfang initialisiert werden koennen.
 * 
 * Dementsprechend wird jedes Steuerungsfeld des Empfangs als Array
 * ausgelegt. Die maximale Anzahl von gleichzeitig zu empfangenden IDs
 * wird durch den Anwender in der Datei **cp_user.h** in der Definition 
 * „NUMBDATAOBJMAX“ festgelegt. 
 * 
 */
struct CPcontrolFieldsRx{
  uint16_t  arrID[NUMBDATAOBJMAX];			/**< Initialisierte ID */
  uint32_t  arrLength[NUMBDATAOBJMAX];	/**< Groesse des Datenobjekts in Byte */
  uint32_t  arrOffset[NUMBDATAOBJMAX];		/**< Anzahl bereits empfangenen Datenbytes  */
  uint8_t   *arrPtrObj[NUMBDATAOBJMAX];	/**< Zeiger auf Datenobjekt */
  uint8_t 	arrStatus[NUMBDATAOBJMAX];		/**< Kommunikationszustand des Empfangsobjekts #CP_StatusTypeDef*/
}CPcontrolFieldsRX;
/** @} */

/** Variable zum Sperren der Parallelen Bearbeitung des CallBack-SendeProzess*/
extern uint8_t CP_LockCANtx;
extern uint16_t missed_ir;

/** 2D Array: Fehler Beschreibung des CAN-PROTOKOLLS */
extern const char *CP_ERROR_DESCRIPTION[];
/** Fehler-Historie der aufgetretenen Fehler. Neuester Fehler auf Position 0 */
extern 	uint8_t CP_LastErrorCodes[3];



/****************************************************************************/
/* FUNCTION DECLARATIONS */
/****************************************************************************/

/** Interrupt Callback Functions */
extern void HAL_CAN_TxCpltCallback(CAN_HandleTypeDef *CanHandle);

/** Other Functions */
extern CP_StatusTypeDef 	CP_CAN_Init();
extern void 				CP_InitControlFieldsTx();
extern void 				CP_InitControlFieldsRx();
extern int16_t 				CP_searchIDRx(uint16_t ID);
extern void 				CP_SaveErrorCode(uint32_t ErrorCode);

 








 
#ifdef __cplusplus
}
#endif
#endif /*__ cp_control_H */
 
 
