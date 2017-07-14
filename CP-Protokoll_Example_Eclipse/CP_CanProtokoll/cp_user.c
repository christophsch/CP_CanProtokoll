/**
 * CP Anwender-Interface.
 * @file        		cp_user.c
 * @ingroup     	CP_User_Interface
 * @author      	Christoph Scharf
*/

/**
 * @defgroup CP_User_Interface CP Anwender Interface
 * @{
 *
 * User-Interface / Anwender-Interface.
 * 
 * Zur Nutzung der entwickelten Protokollschichten werden dem Anwender 
 * Funktionen zur Verfuegung gestellt, mit denen das CAN-Uebertragungsprotokoll 
 * verwendet werden kann.
 * Fuer das Senden / Empfangen eines Datenobjekts muss dieses als erstes mit 
 * der Funktion CP_InitXx(…) initialisiert werden. Anschliessend kann das 
 * Datenobjekt durch Aufruf der Funktion CP_StartXx(...) versendet werden, 
 * bzw. der Empfang  aktiviert werden. Der aktuelle Kommunikations-Zustand 
 * (Status) eines initialisierten Datenobjekts kann mit der Funktion 
 * CP_StatusXx(...) abgerufen werden.
 * 
 * ###Zustandswerte:
 *
 *   Werte      | Steuerungsfeld Status / Zustand         			|  Rueckgabewerte Anwenderfunktionen
 *   -----------|---------------------------------------------------|-------------------------------------------
 *   CP_OK      | Initialisiert / Erfolgreich versendet / empfangen | Erfolgreich
 *   CP_ERROR   |  Fehler                                           | Fehler
 *   CP_BUSY    | Im Prozess Senden / Empfangen                     | Beschaeftigt
 *   CP_TIMEOUT | Zeitueberschreitung                               | Nicht verwendet
 *
 * Jede Anwenderfunktion erhaelt den Rueckgabetyp #CP_StatusTypeDef. 
 * Dieser beschreibt, ob die Funktion erfolgreich ausgefuehrt wurde. 
 * Die gleichen Werte werden, auch fuer den Kommunikations-Zustand (Status) 
 * der Datenobjekte verwendet, wie gerade in der Tabelle beschrieben.
 *
 */
 

 
/****************************************************************************/
 /* Includes */
/****************************************************************************/

 //#include "cp_user.h"
#include "cp_user.h"
#include "cp_control.h"
#include "can.h"



/****************************************************************************/
/* Transmit Functions */
/****************************************************************************/

/****************************************************************************/
/**
 * Intitialisierung der Transmit-Routine fuer einen bestimmten Identifier.
 * 
 * Um Daten und damit ein Datenobjekt zu versenden sind folgende Angaben
 * notwendig: Adresse des Datenobjekts im Speicher (Pointer), Speichergroesse 
 * des Datenobjekts und der Identifier als Zuordnung der CAN-Nachricht zum 
 * Datenobjekt. Diese Parameter muessen daher vor dem Senden eines 
 * Datenobjekts vom Anwender bestimmt werden. Dies geschieht mit der 
 * Initialisierungs-Funktion:
 *
 * Interne Aufgaben:
 *
 *  - Kurzzeitiges Stoppen der Interrupt-Steuerung fuer das Senden von
 *  CAN-Botschaften um einen Interrupt waehrend der Initialisierung
 *  zu verhindern.
 *  - Erst-Initialisierung / Zueruecksetzen der ControllFields *	
 *  - Zuordnung der ID zu Datenobjekt
 *  - Offset zurueckzusetzten (Zustand von bereits gesendeten Bytes) 
 *  - Setzte Status CP_OK (=Zustand fuer "Initialisiert" und "erfolgreich gesendet"
 *
 *  - Freigabe der Sende-Interrupt-Routine
 *  (Die Daten werden jedoch noch nicht versendet, erst nach Startfunktion)
 *
 * @param *ptrDataObj Location of Data Object
 * @param size of Data Object
 * @param ID for Sending the Data Object
 * @return Status ob Empfangs Initialisierung fuer uebergebene Parameter erfolgreich war.
 */
CP_StatusTypeDef CP_InitTx(uint8_t *ptrDataObj, uint32_t size, uint16_t ID)
{
	/* Sperren der Bearbeitung von Sende-Interrupts waehrend der Initialisierung */
	CP_LockCANtx = 1;

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

	/* Freigabe CP_Lock fuer naechsten SendeProzess */
	CP_LockCANtx = 0;

  return CP_OK;
}


/****************************************************************************/
/**
 *	Starte Sendevorgang der initialisierten ID.
 *
 *	Moeglich sofern gerade keine Daten versendet werden und kein Fehler vorliegt
 *
 *	@return Status ob der Sende-Prozess gestartet werden konnte
 */
CP_StatusTypeDef CP_StartTx()
{
	if (CPcontrolFieldsTX.arrStatus == CP_OK)
	{
		/* Ueberpruefung ob Datenobjekt richtig initialisiert ist*/
		if ((CPcontrolFieldsTX.arrLength == 0) | (CPcontrolFieldsTX.arrPtrObj == 0))
		{
			CP_SaveErrorCode(21);
			CPcontrolFieldsTX.arrStatus   = CP_ERROR;
			return CP_ERROR;
		}

	  /* Anzahl bereits versendeter Daten zuruecksetzen */
	  CPcontrolFieldsTX.arrOffset = 0;
	  /* Sendefreigabe erteilen */
	  CPcontrolFieldsTX.arrStatus = CP_BUSY;

	  /* Interrupt gesteruerten Sende-Prozess starten */
	  HAL_CAN_TxCpltCallback(&hcan);
	  return CP_OK;
	}
	else
	{
		CP_SaveErrorCode(5);
		return CP_ERROR;
	}
}


/****************************************************************************/
/**
 * Abfrage des aktuellen Sende-Status von einem Datenobjekt.
 * @return Status / Zustand des Sendevorgangs
 */
CP_StatusTypeDef CP_StatusTx()
{
	return CPcontrolFieldsTX.arrStatus;
}



/****************************************************************************/
/* Receive Functions */
/****************************************************************************/
 
/****************************************************************************/
/**
 * Intitialisierung der Empfangs-Routine fuer einen bestimmten Identifier.
 *
 * Interne Aufgaben:
 *  - Pruefung ob ID bereits angelegt wurde oder lege diese an.
 *  - Initialisierung der ControllFields.
 *  - Bei Erstinitialisierung: (ID nicht in Array)
 *      - ID anlegen, Legth setzen, Offset = 0, PtrObj setzen, StatusRx = Init
 *  - Falls ID bereits existiert:
 *      - Werte fuer ID zuruecksetzen
 *
 * @param `*ptrDataObj Location of Data Object
 * @param size of Data Object
 * @param ID for Receiving Data Object
 * @return Status ob Empfangs-Initialisierung fuer uebergebene Parameter erfolgreich war.
 */
CP_StatusTypeDef CP_InitRx(uint8_t *ptrDataObj, uint32_t size, uint16_t ID)
{
	/* Suche, ob ID bereits angelegt ist.*/
  int16_t arrIndex;
  arrIndex = CP_searchIDRx(ID);

  /* Wenn ID noch nicht vorhanden, suche nach einem freien Feld zum Anlegen (0xFFFF)*/
  if (arrIndex == -1)
  {
    arrIndex = CP_searchIDRx(0xFFFF);
  }
  if (arrIndex != -1)
  {
  /* Pruefe ob uebergebene ID innerhalb Grenze des 11 bit Identifiers*/
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
  	/* Error, sofern ID noch nicht angelegt ist und sich kein freier Platz in dem Array befindet */
  	CP_SaveErrorCode(4);
  	return CP_ERROR;
  }

}


/****************************************************************************/
/**
 *  Einmaliges Starten der Empfangs-Routine von einem
 *   initialisierten Datenobjekt.
 * 
 *  Der Empfang einer initialisierten ID kann gestartet werden, sofern
 *  diese ID im Status "CP_OK" ist (initialisiert/erfolgreich empfangen).
 *
 * @param ID ID, zu dessen Datenobjekt empfangen werden soll
 * @return Rueckmeldung, ob Starten des Empfangs fuer ID erfolgreich
 */
CP_StatusTypeDef CP_StartRx(uint16_t ID)
{
  int16_t arrIndex;
  arrIndex = CP_searchIDRx(ID);

  /* Ist ID angelegt? */
  if (arrIndex != -1)
  {
  	if (CPcontrolFieldsRX.arrStatus[arrIndex] == CP_OK)
  	{
				/* Freigabe  Empfangsprozess zu der ID */
				CPcontrolFieldsRX.arrStatus[arrIndex] = CP_BUSY;
				return CP_OK;
  	}
    else
    {
		/* Status ungleich Initialisierungszustand */
    	CP_SaveErrorCode(5);
    	return CP_ERROR;
    }

  }
  else
  {
	/* ID ist nicht angelegt */
  	CP_SaveErrorCode(3);
    return CP_ERROR;
  }
}

/****************************************************************************/
/**
 * Abfrage des aktuellen Empfangs-Status von einem Datenobjekt.
 *
 * @param ID ID, dessen Status abgefragt wird
 * @return Status / Zustand des Empfangsvorgangs zu uebergebener ID
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

/****************************************************************************/
/**
 * Zuruecksetzen der ID und Loeschen der Elemente im Steuerungselement.
 *
 * @param ID ID, die aus dem Steuerungselement entfernt werden soll
 * @return Status of Loeschung erfolgreich
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
/** @} */
