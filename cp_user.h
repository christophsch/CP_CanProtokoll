/**
 * CP Anwender-Interface.
 * @file        		cp_user.h
 * @ingroup     	CP_User_Interface
 * @author      	Christoph Scharf
*/

#ifndef __cp_user_H
#define __cp_user_H
#ifdef __cplusplus
 extern "C" {
#endif

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* Includes */
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

#include "stm32f3xx_hal.h"

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* DEFINES */
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/** Maximale Anzahl von unterschiedlichen Empfangs-IDs */
#define NUMBDATAOBJMAX  10
/** Erneuter Aufruf der Interrupt-Transmitfunktion solange bis Timeout erreicht */ 
#define TIMEOUTCANTX	1500

/**
* Statuswerte.
*
* Wird zweierlei verwendet. Zum einen als Rueckgabe-Wert der
* Anwenderfunktionen, ob diese erfolgreich ausgefuehrt wurden.
* Zum Zweiten werden die Werte fuer die @ref Steuerungsfelder Status
* verwendet.
*/
typedef enum
{
	CP_OK      	= 0x00,		/**<  Rueckgabewert: Erfolgreich        / Zustand: Initialisiert*/
	CP_ERROR    = 0x01,		/**<  Rueckgabewert: Fehler                / Zustand: Fehler*/
	CP_BUSY    	= 0x02,		/**<  Rueckgabewert: Beschaeftigt        / Zustand: Kommunikationsprozess aktiv*/
	CP_TIMEOUT  = 0x03		/**<  Rueckgabewert: Nicht verwendet / Zustand: Zeitueberschreitung*/
}CP_StatusTypeDef;


/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* VARIABLES - DECLARATION OF DATAOBJECTS */
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/**
 * @defgroup Datenobjekt Datenobjekte
 * @ingroup CP_User_Interface
 * @{
 * Hintergrund und Aufbau zu Datenobjekten
 * Jeder Mikrocontroller besitzt seine eigenen Datenobjekte, sowie 
 * Datenobjekte anderer Teilnehmer im CAN-Netzwerk. Die eigenen 
 * Datenobjekte werden zur Laufzeit veraendert und koennen auf den 
 * CAN-Bus gesendet werden, sodass diese fuer andere Knoten im 
 * Netzwerk zur Verfuegung stehen. Die Empfaenger eines Datenobjekts 
 * muessen eine identische Datenstruktur im Speicher deklariert haben 
 * um dieses Datenobjekt empfangen, beziehungsweise speichern zu 
 * koennen. Bei dem Empfang werden die Daten dann automatisch in 
 * das deklarierte Datenobjekt geschrieben. Von hier aus koennen die 
 * Daten, nach erfolgreichem Empfang, durch den Anwender 
 * ausgelesen und verwendet werden.
 * 
 * Deklariert werden diese in der Datei **cp_user.h**. Es ist praktikabel, 
 * ein Strukturelement als Datenobjekt zu verwenden, da so mehrere 
 * Variablen in nur einem Datenobjekt versendet werden koennen.
 * Ein Datenobjekt kann jedoch jeder beliebige Datentyp sein, da ein 
 * Datenobjekt einzig und allein durch dessen Adresse im Speicher 
 * und die Groesse in Bytes definiert ist. 
 */

 
/**
* Data Object Senden Board1.
*/
struct DataObj1
	{
		uint8_t	Value;
	}DataObj_MC1;

/**
* Data Object Senden Board2
*/
struct DataObj2
	{
		uint8_t	Value;
	}DataObj_MC2;

/** @} */

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* FUNCTION DECLARATIONS */
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

//int16_t CP_searchIDRx(uint16_t ID);

/* * Receive Functions */
extern CP_StatusTypeDef CP_InitRx(uint8_t *ptrDataObj, uint32_t size, uint16_t ID);
extern CP_StatusTypeDef CP_StartRx(uint16_t ID);
extern CP_StatusTypeDef CP_StatusRx(uint16_t ID);
extern CP_StatusTypeDef CP_DeleteRx(uint16_t ID);

/* * Transmit Functions */
extern CP_StatusTypeDef CP_InitTx(uint8_t *ptrDataObj, uint32_t size, uint16_t ID);
extern CP_StatusTypeDef CP_StartTx();
extern CP_StatusTypeDef CP_StatusTx();

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* EXTERN REFERENCES (FOR USER USAGE)*/
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/** Initialisierung / Zuruecksetzen der CP-Funktionen */
extern void 		CP_CAN_Init();

/** 2D Array: Fehler Beschreibung des CAN-PROTOKOLLS */
extern const char *CP_ERROR_DESCRIPTION[];
/** Fehler-Historie der aufgetretenen Fehler. Neuester Fehler auf Position 0 */
extern 	uint8_t CP_LastErrorCodes[3];

 
 
 
 
 
 
 
 
 
 
 
 
 
 
#ifdef __cplusplus
}
#endif
#endif /*__ cp_user_H */
 
 
