
/* Includes ------------------------------------------------------------------*/
//#include "cp_user.h"
#include "cp_user.h"
#include "cp_intern.h"
#include "can.h"

/******************************************************************************/
/** Receive Functions */
/******************************************************************************/

/*******************************************************************************
 *
 * 		Intitialisierung der Empfangs-Routine für einen bestimmten Identifier
 *
 *		Interne Aufgaben:
 *		- Prüfung ob ID bereits angelegt wurde oder lege diese an.
 *
 *		- Kurzzeitiges Stoppen der Interrupt-Steuerung für den Empfang von
 *			CAN-Botschaften um einen Interrupt während der Initialisierung
 *			zu verhindern.
 *
 *   	- Initialisierung der ControllFields, die Informationen zum aktuellen
 *   		Stand der Empfangs-Routine einer bestimmten ID speichern.
 *
 *		- Erneutes Starten der Empgangs-Interrupt-Routine
 *			(Jedoch noch nicht für diese ID, dafür ist die Funktion
 *			CP_StartRx() zuständig.)
 *
 * @param data Location of Data Object
 * @param Size of Data Object
 * @param ID for Receiving the Data Object
 * @return Status ob Empfangs Initialisierung für übergebene Parameter erfolgreich war.
 */
CP_StatusTypeDef CP_InitRx(uint8_t *ptrDataObj, uint32_t size, uint16_t ID)
{
// TODO - Stop IR-Routine

//		Bei Erstinitialisierung: (ID nicht in Array)
//        -> ID anlegen, Legth setzen, Offset = 0, PtrObj setzen, StatusRx = Init
//    WENN ID existiert
//        -> Werte für ID zurücksetzen

// TODO - Start IR-Routine

	/** Suche, ob ID bereits angelegt ist.*/
  int16_t arrIndex;
  arrIndex = CP_searchIDRx(ID);

  /** Wenn ID noch nicht vorhanden, suche nach einem freien Feld zum Anlegen (0xFFFF)*/
  if (arrIndex == -1)
  {
    arrIndex = CP_searchIDRx(0xFFFF);
  }
  if (arrIndex != -1)
  {
  	if (ID <= 0x07FF)
  	{
  		CPcontrolFieldsRX.arrID[arrIndex] = ID;
  	}
  	else
  	{
  		CP_SaveErrorCode(10);
  		return CP_ERROR;
  	}

    CPcontrolFieldsRX.arrLength[arrIndex]   = size;
    CPcontrolFieldsRX.arrOffset[arrIndex]   = 0x00;
    CPcontrolFieldsRX.arrPtrObj[arrIndex]   = ptrDataObj;
    CPcontrolFieldsRX.arrStatus[arrIndex]   = CP_OK;
    return CP_OK;
  }
  else
  {
  	/** Error, sofern ID noch nicht angelegt ist und sich kein freier Platz in dem Array befindet */
  	CP_SaveErrorCode(4);
  	return CP_ERROR;
  }
// TODO - Start IR-Routine

}


/******************************************************************************/
/** Einmaliges Starten der Empfangs-Routine von einem Datenobjekt
 *
 *
 *
 *
 *
 */
CP_StatusTypeDef CP_StartRx(uint16_t ID)
{
  int16_t arrIndex;
  arrIndex = CP_searchIDRx(ID);

  if (arrIndex != -1)
  {
  	if (CPcontrolFieldsRX.arrStatus[arrIndex] == CP_OK)
  	{
				CPcontrolFieldsRX.arrStatus[arrIndex] = CP_BUSY;
				return CP_OK;
  	}
    else
    {
    	CP_SaveErrorCode(5);
    	return CP_ERROR;
    }

  }
  else
  {
  	CP_SaveErrorCode(3);
    return CP_ERROR;
  }
}

/*******************************************************************************
 * Abfrage des aktuellen Empfangs-Status von einem Datenobjekt
 *
 *
 *
 */
CP_StatusTypeDef CP_StatusRx(uint16_t ID)
{
	int16_t arrIndex = CP_searchIDRx(ID);
  if (arrIndex != -1)
  {
    return CPcontrolFieldsRX.arrStatus[arrIndex] ;
  }
  else
  {
    return CP_ERROR;
  }
}

/*******************************************************************************
 *
 *
 * Zurücksetzen der ID und Löschen der Elemente im Steuer-Array
 *
 ************** AUCH IR SPERREN - ABER DANN GEHT Nachricht verloren??
 */
CP_StatusTypeDef CP_DeleteRx(uint16_t ID)
{
	int16_t arrIndex;
	arrIndex = CP_searchIDRx(ID);

  if (arrIndex != -1)
  {
    CPcontrolFieldsRX.arrID[arrIndex]       = 0xFFFF;
    CPcontrolFieldsRX.arrLength[arrIndex]   = 0x00;
    CPcontrolFieldsRX.arrOffset[arrIndex]   = 0x00;
    CPcontrolFieldsRX.arrPtrObj[arrIndex]   = 0x00;
    CPcontrolFieldsRX.arrStatus[arrIndex]   = CP_OK;
    return CP_OK;
  }
  else
  {
  	CP_SaveErrorCode(3);
    return CP_ERROR;
  }
}

/******************************************************************************/
/** Transmit Functions */
/******************************************************************************/


/*******************************************************************************
 *
 * 		Intitialisierung der Tranmit-Routine für einen bestimmten Identifier
 *
 *		Interne Aufgaben:
 *		- Prüfung ob ID bereits angelegt wurde oder lege diese an.
 *
 *		- Kurzzeitiges Stoppen der Interrupt-Steuerung für das Senden von
 *			CAN-Botschaften um einen Interrupt während der Initialisierung
 *			zu verhindern.
 *
 *   	- Initialisierung der ControllFields, die Informationen zum aktuellen
 *   		Stand der Sende-Routine einer bestimmten ID speichern.
 *
 *		- Erneutes Starten der Sende-Interrupt-Routine
 *			(Jedoch noch nicht für diese ID, dafür ist die Funktion
 *			CP_StartTx() zuständig.)
 *
 * @param data Location of Data Object
 * @param Size of Data Object
 * @param ID for Sending the Data Object
 * @return Status ob Empfangs Initialisierung für übergebene Parameter erfolgreich war.
 */
CP_StatusTypeDef CP_InitTx(uint8_t *ptrDataObj, uint32_t size, uint16_t ID)
{
	/** Sperren der Bearbeitung von Sende-Interrupts während der Initialisierung */
//	CP_LockCANtx = 1;

	if (ID > 0x07FF)
	{
		CP_SaveErrorCode(10);
		CPcontrolFieldsTX.arrStatus   = CP_ERROR;
		return CP_ERROR;
	}

	CPcontrolFieldsTX.arrID 			= ID;
	CPcontrolFieldsTX.arrLength   = size;
	CPcontrolFieldsTX.arrOffset		= 0x00;
	CPcontrolFieldsTX.arrPtrObj   = ptrDataObj;
	CPcontrolFieldsTX.arrStatus   = CP_OK;

//	/** Freigabe CP_Lock für nächsten SendeProzess */
//	CP_LockCANtx = 0;

  return CP_OK;
}


/*******************************************************************************
 *	Starte Sendevorgang der initialisierten ID
 *
 *	Möglich sofern gerade keine Daten versendet werden und kein Fehler vorliegt
 *
 *	@return Status ob der Sende-Prozess gestartet werden konnte
 */
CP_StatusTypeDef CP_StartTx()
{
	if (CPcontrolFieldsTX.arrStatus == CP_OK)
	{
		if (CPcontrolFieldsTX.arrLength == 0 | CPcontrolFieldsTX.arrPtrObj == 0)
		{
			CP_SaveErrorCode(21);
			CPcontrolFieldsTX.arrStatus   = CP_ERROR;
			return CP_ERROR;
		}

		CPcontrolFieldsTX.arrOffset = 0;
	  CPcontrolFieldsTX.arrStatus = CP_BUSY;

	  /** Interrupt gesteruerten Sende-Prozess starten */
	  HAL_CAN_TxCpltCallback(&hcan);
	  return CP_OK;
	}
	else
	{
		CP_SaveErrorCode(5);
		return CP_ERROR;
	}
}


/*******************************************************************************
 *
 */
CP_StatusTypeDef CP_StatusTx()
{
	return CPcontrolFieldsTX.arrStatus;
}
