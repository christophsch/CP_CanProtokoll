
/* Includes ------------------------------------------------------------------*/
//#include "cp_user.h"
//#include "cp_intern.h"
#include "cp_intern.h"
#include "can.h"

/******************************************************************************/
/** DEFINITION OF VARIABLES */
/******************************************************************************/

	CAN_FilterConfTypeDef  		  	sFilterConfig;
	static CanTxMsgTypeDef        TxMessage;
	static CanRxMsgTypeDef        RxMessage;

	/** Variable zum Sperren der Parallelen Bearbeitung des CallBack-SendeProzess*/
	uint8_t CP_LockCANtx = 0;
	uint16_t missed_ir = 0;


	/** Fehler-Historie der aufgetretenen Fehler. Neuester Fehler auf Position 0 */
	uint8_t CP_LastErrorCodes[3] = {0,0,0};


	/** 2D Array: Fehler Beschreibung des CAN-PROTOKOLLS */
	const char *CP_ERROR_DESCRIPTION[] = \
			{	\
			/** errCode: 00 */ \
			"CP_ERROR: Kein Fehler.\r\r",\
			/** errCode: 01 */ \
			"CP_ERROR: Fehler beim Senden von CAN-Nachrichten in HAL_CAN_Transmit_IT()\r\r",\
			/** errCode: 02 */ \
			"CP_ERROR: Fehler beim Empfang von CAN-Nachrichten in HAL_CAN_Receive_IT()\r\r",\
			/** errCode: 03 */ \
			"CP_ERROR: Empfangs-Identifier ist nicht initialisiert. \r\r",\
			/** errCode: 04 */ \
			"CP_ERROR: Maximale Anzahl von IDs fuer Empfang verwendet. \r\r",\
			/** errCode: 05 */ \
			"CP_ERROR: Erwarteter Zustand ist UNGLEICH dem Init-Zustand CP_OK. \r\r",\
			/** errCode: 06 */ \
			"CP_ERROR: Empfangene Daten ueberschreiteten Gesamtlaenge des Datenojekts.\r\r",\
			/** errCode: 07 */ \
			"CP_ERROR: Empfangsfehler: Unerwartete Nachrichten-Nummer eines zu empfangenden Datenobjekts erhalten.\r\r",\
			/** errCode: 08 */ \
			"CP_ERROR: Fehler bei Initialisierung von CAN\r\r",\
			/** errCode: 09 */ \
			"CP_ERROR: Fehler bei CAN-Filter-Konfiguration.\r\r",\
			/** errCode: 10 */ \
			"CP_ERROR: Zu initialisierende ID liegt nicht im Bereich 0x000-0x7FF\r\r",\
			/** errCode: 11 */ \
			"CP_ERROR: CAN-BUS-Fehler: 'EWG error' \r\r",\
			/** errCode: 12 */ \
			"CP_ERROR: CAN-BUS-Fehler: 'EPV error' \r\r",\
			/** errCode: 13 */ \
			"CP_ERROR: CAN-BUS-Fehler: 'BOF error' \r\r",\
			/** errCode: 14 */ \
			"CP_ERROR: CAN-BUS-Fehler: 'Stuff error' \r\r",\
			/** errCode: 15 */ \
			"CP_ERROR: CAN-BUS-Fehler: 'Form error' \r\r",\
			/** errCode: 16 */ \
			"CP_ERROR: CAN-BUS-Fehler: 'Acknowledgment error' \r\r",\
			/** errCode: 17 */ \
			"CP_ERROR: CAN-BUS-Fehler: 'Bit recessive' \r\r",\
			/** errCode: 18 */ \
			"CP_ERROR: CAN-BUS-Fehler: 'LEC dominant' \r\r",\
			/** errCode: 19 */ \
			"CP_ERROR: CAN-BUS-Fehler: 'LEC transfer error' \r\r",\
			/** errCode: 20 */ \
			"CP_ERROR: CAN-BUS-Fehler: 'Unbekannte Ursache' \r\r",\
			/** errCode: 21 */ \
			"CP_ERROR: Ungueltiges Datenobjekt\r\r",\
			/** errCode: 22 */ \
			"CP_ERROR: Reserve\r\r"};


/******************************************************************************/
/** FUNCTION DEFINITIONS */
/******************************************************************************/


/******************************************************************************
 *  Interrupt Callback Transmit
 *
 *	Beschreibung und Inhalte der Funktion:
 *
 *	Während der Funktion weiteren Sendeprozess blockieren, durch
 *	setzen von CP_LockCANtx.
 *
 *	Dies ist nötig, sofern während der Abarbeitung dieser Senderoutine,
 *	diese Funktion erneut aufgerufen wird. Dies würde zu
 *	unerwarteten Ergebnissen oder Fehlern führen.
 *
 *	Eintreten kann dieser Fall, wenn diese Funktion durch CP_StartTx()
 *	aufgerufen wird und während dem Ablauf ein Sende-Interrupt
 *	den Ablauf unterbricht. Der Interrupt wird ebenfalls diese Funktion
 *	aufrufen, wodurch Daten überschrieben werden.
 *
 *	Sofern eine ID festgelegt wurde UND der Sendeprozess gestartet wurde
 *	(=CP_BUSY), wird SendeProzess fortgesetzt, anderenfalls abgebrochen.
 *
 *	Berechnung der aktuellen fortlaufenden Nummer der CAN-Botschaft
 *	Diese FrameNummer wird in die ersten zwei Datenbytes der
 *	CAN-Botschaft geschrieben zur Informationsverarbeitung für den
 *	Empfänger bei aufgesplitteten Datenobjekten.
 *
 *	Berechnung der zu sendenden Datenlänge der CAN-Nachricht.
 *	Diese kann maximal 8 Byte lang sein.
 *
 *	Festlegung eines Pointers "pNow". Dieser Zeiger zeigt an das aktuell
 *	zu verarbeitende Byte im zu sendenden Datenobjekt.
 *
 *	Sofern der gespeicherte "Offset" größer ist, als das zu
 *	sendende Datenobjekt ist ein Fehler entstanden. Z.B. wenn der
 *	Offset-Wert von außerhalb verändert wurde.
 *
 *	Im nächsten Schritt erfolgt das Speichern der Bytes des Datenobjekts
 *	in den Sende-Daten-Puffer der CAN-Nachricht. Dies geschieht solange
 *	bis die Größe des DLC erreicht ist.
 *
 *	Nach der Konfiguration der Sende-Parameter für den CAN-Transmit
 *	wird die interrupt-getriebene Funktion zum Senden der
 *	CAN-Nachricht aufgerufen um die Daten zu versenden.
 *
 *	Wenn alle Daten des Datenobjekts verschickt wurden, wird der Status
 *	CP_OK gesetzt und ein erneuter expliziter Aufruf der
 *	Funktion CP_StartTx muss zum erneuten Senden erfolgen.
 *
 *	Sofern noch nicht alle Daten des Datenobjekts verschickt wurden,
 *	bleibt der Status auf "CP_BUSY" und der Interrupt-getriebene
 *	Sende-Prozess wird bei dem erneuten Aufruf dieser Funktion fortgesetzt.
 *
 *	Am Ende Freigabe für den nächsten Sendeprozess.
 *
 *@param CAN_Handle Object
 *
 **/
void HAL_CAN_TxCpltCallback(CAN_HandleTypeDef *CanHandle)
{
	if (CP_LockCANtx == 1)
	{
		//test
		int16_t arrIndex;
		arrIndex = CP_searchIDRx(0xFFFF);
		/******************************************DEBUG**********************************************/

		// SPEICHERE ZUSTAND VERLORENEN IR
		missed_ir ++;
		return;

	}
	/** Blockieren von weiterem SendeProzess */
	CP_LockCANtx = 1;

	if (CPcontrolFieldsTX.arrID > 0x07FF | \
			CPcontrolFieldsTX.arrStatus != CP_BUSY )
	{
		/** Freigabe CP_Lock für nächsten SendeProzess */
		CP_LockCANtx = 0;
		return;
	}

	/** Aktuell zu versendenden FrameNummer */
	uint16_t FrameNumberMsg = CPcontrolFieldsTX.arrOffset /6 + 1;

	/** Aufteilen der  FrameNumber auf 2 separate Bytes und */
	/** Speichern der Bytes in Sende-Daten-Puffer */
	CanHandle->pTxMsg->Data[0] = (uint8_t)(FrameNumberMsg >> 8);
	CanHandle->pTxMsg->Data[1] = (uint8_t)(FrameNumberMsg);

	/** DLC - Länge des aktuellen Frames */
	uint8_t DLCNow = CanHandle->pTxMsg->DLC = 2 + CPcontrolFieldsTX.arrLength - CPcontrolFieldsTX.arrOffset;
	if (DLCNow > 8 )
	{
		DLCNow = 8;
	}
	CanHandle->pTxMsg->DLC = DLCNow;

	/* Erstelle Pointer auf das aktuelle Byte des Datenobjekts */
	uint8_t *pNow = CPcontrolFieldsTX.arrPtrObj + CPcontrolFieldsTX.arrOffset;

	for (int byteInFrame = 2; byteInFrame < DLCNow; byteInFrame++)
	{
		if (CPcontrolFieldsTX.arrOffset > CPcontrolFieldsTX.arrLength)
		{
			/** Offset ueberschreitete Gesamt Größe des Datenojekts*/
			CPcontrolFieldsTX.arrStatus = CP_ERROR;
			/** Freigabe CP_Lock für nächsten SendeProzess */
			CP_LockCANtx = 0;
			CP_SaveErrorCode(6);
	  	return;
		}

		/** Kopieren der Daten aus Datenobjekt in CAN-Sende-Puffer*/
		CanHandle->pTxMsg->Data[byteInFrame] = *pNow;

		pNow += 0x01;
		CPcontrolFieldsTX.arrOffset++;
	}

	/** Ueberprüfung ob Datenobjekt komplett versendet wurde */
	/** Offset = Größe (Nach Sendevorg.): z.B. 1 Byte senden -> offset = 1*/
	if (CPcontrolFieldsTX.arrOffset == CPcontrolFieldsTX.arrLength)
	{
		/** Zurücksetzen des Offset für neues Senden */
		CPcontrolFieldsTX.arrOffset = 0;
		CPcontrolFieldsTX.arrStatus = CP_OK;
	}
	else
	{
		CPcontrolFieldsTX.arrStatus = CP_BUSY;
	}

	/** Konfiguration der CAN-Sende-Parameter */
	hcan.pTxMsg->StdId = CPcontrolFieldsTX.arrID;
	hcan.pTxMsg->ExtId = 0x01; // todo: relevant?
	hcan.pTxMsg->RTR = CAN_RTR_DATA;
	hcan.pTxMsg->IDE = CAN_ID_STD;
	if ((HAL_CAN_Transmit_IT(CanHandle)) != HAL_OK)
	{
		uint32_t tickStartCANTx = 0;
		uint8_t	 CANTxErr = 1;

		tickStartCANTx = HAL_GetTick();
		while (( (HAL_GetTick() - tickStartCANTx) < TIMEOUTCANTX) & (CANTxErr == 1))
		{
			if ((HAL_CAN_Transmit_IT(CanHandle)) == HAL_OK)
			{
				CANTxErr = 0;
			}
		}
		if (CANTxErr)
		{
			CPcontrolFieldsTX.arrStatus = CP_ERROR;
			/** Freigabe CP_Lock für nächsten SendeProzess */
			CP_LockCANtx = 0;
			CP_SaveErrorCode(1);

		}
	}

	uint8_t test = CPcontrolFieldsTX.arrStatus;

	/** Freigabe CP_Lock für nächsten SendeProzess */
	CP_LockCANtx = 0;
	if (missed_ir > 0)
	{
		missed_ir --;
		HAL_CAN_TxCpltCallback(CanHandle);
	}
	return;
}


/******************************************************************************
 *
 *	INTERRUPT ROUTINE FÜR CAN-EMPFANGS-PROZESS
 *
 *	Beschreibung und Inhalte der Funktion:
 *
 *	Suche der ID in ID-Array ob ID angelegt wurde.
 *	Wenn nicht, ist Empfang nicht erwünscht: Verwerfen der CAN-Nachricht
 *
 *	Prüfen ob der Empfang für angelegte ID bereits freigeschaltet ist.
 *	Die Freischaltung erfolgt in der Funktion CP_StartRx() durch das
 *	Setzen des StatusRx auf "CP_BUSY"
 *
 *	Sollte ein Datenobjekt auf mehrere CAN-Nachrichten aufgeteilt werden,
 *	wird vorrausgesetzt, dass die Reihenfolge der aufgeteilten Nachrichten
 *	eingehalten wird.
 *	Somit wird geprüft, ob die erwaretete FrameNummer (=offset/6+1)
 *	der in der CAN-Botschaft enthaltener FrameNummer entspricht.
 *	Dafür wird die FrameNummer aus den ersten zwei Daten-Bytes
 *	des CAN-Empfangs-Puffer gelesen.
 *
 *	Sofern diese empfange FrameNummer nicht die erwaretete ist, kann nicht
 *	sichergestellt werden, dass alle Nachrichten für dieses Datenobjekt
 *	empfangen werden. Daher muss der Empfang für dieses Datenobjekt abgebrochen
 *	werden und der Status für dieses ID wird auf "CP_ERROR" gesetzt.
 *	Sollte es sich jedoch um die erste FrameNummer handeln (=1),
 *	wird nur der Offset zurück gesetzt und somit der Empfang des Datenobjekts
 *	erneut begonnen, ohne dass der Empfang für diese ID erneut aktiviert werden
 *	muss.
 *
 *	Festlegung eines Pointers "pNow". Dieser Zeiger zeigt an das aktuell
 *	zu verarbeitende Daten-Byte im Datenobjekt.
 *
 *	Im nächsten Schritt erfolgt das Speichern des aktuellen Bytes aus
 *	der empfangenen CAN-Nachricht in das aktuelle Byte des Datenobjekts.
 *	Dies geschieht solange bis die Länge der Nachricht (DLC) erreicht ist.
 *	Da die ersten zwei Bytes die FrameNummer enthalten also 1 bis maximal
 *	6 Durchläufe.
 *
 *	Sofern der gespeicherte "Offset" größer oder gleich ist, als die
 *	Größe des Datenobjekts ist ein Fehler entstanden. Z.B. weil der
 *	gespeicherte Offset-Wert von außerhalb verändert wurde.
 *
 *	Wenn alle Daten des Datenobjekts empfangen wurden, wird der Status
 *	auf "CP_OK" gesetzt und ein erneuter expliziter Aufruf der
 *	Funktion CP_StartTx muss zum erneuten Empfang für diese ID erfolgen.
 *
 *	Sofern noch nicht alle Daten des Datenobjekts empfangen wurden,
 *	bleibt der Status auf "CP_BUSY" und der Empfangs-Prozess wird bei dem
 *	erneuten Aufruf dieser Funktion fortgesetzt.
 *
 *	Am Ende muss durch den Aufruf der Interrupt-Empfangs-Funktion:
 *	HAL_CAN_Receive_IT() der interrupt-getriebene Empfang wieder aktiviert
 *	werden. Die Interrupts wurden vor dem Aufruf dieser Funktion
 *	ausgesetzt, sodass keine noch nicht verarbeiteten Daten überschrieben
 *	werden können.
 *
 *@param CAN_Handle Object
 *
 **/
void HAL_CAN_RxCpltCallback(CAN_HandleTypeDef *CanHandle)
{

	// Nach Aufruf der Funktion ist Receive Interrupt deaktiviert?

	uint32_t ID = CanHandle->pRxMsg->StdId;

	/** Suche, ob ID bereits angelegt ist.*/
  int16_t arrIndex;
  arrIndex = CP_searchIDRx(ID);


  if (arrIndex == -1)
  {
  	/** Botschaft verwerfen */
  	/** Starte erneut unterbrochenen Interrupt-getriebenen Empfang */
  	if (HAL_CAN_Receive_IT(&hcan, CAN_FIFO0) != HAL_OK)
  	{
  		/* Reception Error */
  		CP_SaveErrorCode(2);
  		CPcontrolFieldsRX.arrStatus[arrIndex] = CP_ERROR;
  	}
     return;
  }

  if (CPcontrolFieldsRX.arrStatus[arrIndex] != CP_BUSY)
  {
  	/** Botschaft verwerfen */
  	/** Starte erneut unterbrochenen Interrupt-getriebenen Empfang */
  	if (HAL_CAN_Receive_IT(&hcan, CAN_FIFO0) != HAL_OK)
  	{
  		/* Reception Error */
  		CP_SaveErrorCode(2);
  		CPcontrolFieldsRX.arrStatus[arrIndex] = CP_ERROR;
  	}
     return;
  }

  /* FrameNumber aus den ersten zwei Bytes der CAN-Botschaft lesen **/
  uint16_t FrameNumberMsg = ((CanHandle->pRxMsg->Data[0] << 8) | CanHandle->pRxMsg->Data[1]);
  uint16_t FrameNumberExp = CPcontrolFieldsRX.arrOffset[arrIndex] /6 + 1;

  /* WENN Frame-Nummer in CAN-Botschaft nicht identisch mit erwarteter Frame-Nummer*/
  if (FrameNumberMsg != FrameNumberExp)
  {
      if (FrameNumberMsg == 1)
      {
        /* Frame-Nummer in CAN-Botschaft == 1, Daher starte Empfang von Beginn */
        CPcontrolFieldsRX.arrOffset[arrIndex] = 0;
        FrameNumberExp = FrameNumberMsg;
      }
      else
      {
      	/** Botschaft verwerfen */

        /* Die Reihenfolge der CAN-Botschaften ist nicht richtig, Zurücksetzen des aktuellen Empfangs*/
        CPcontrolFieldsRX.arrStatus[arrIndex] = CP_ERROR;
        CPcontrolFieldsRX.arrOffset[arrIndex] = 0;

    		/* Reception Error */
        CP_SaveErrorCode(7);

      	/** Starte erneut unterbrochenen Interrupt-getriebenen Empfang */
      	if (HAL_CAN_Receive_IT(&hcan, CAN_FIFO0) != HAL_OK)
      	{
      		/* Reception Error */
      		CP_SaveErrorCode(2);
					CPcontrolFieldsRX.arrStatus[arrIndex] = CP_ERROR;
      	}
        return;
      }
  }

  /* Erstelle Pointer auf das aktuelle Byte des Datenobjekts */
  uint8_t *pNow = CPcontrolFieldsRX.arrPtrObj[arrIndex] + CPcontrolFieldsRX.arrOffset[arrIndex];

  for (int byteInFrame = 2; byteInFrame < CanHandle->pRxMsg->DLC; byteInFrame++)
  {
		if  (CPcontrolFieldsRX.arrOffset[arrIndex] < CPcontrolFieldsRX.arrLength[arrIndex])
		{
			/* Speichern der Daten in DatenObjekt */
			*pNow = CanHandle->pRxMsg->Data[byteInFrame];

			pNow+=0x01;
			CPcontrolFieldsRX.arrOffset[arrIndex]++;
		}
		else
		{
			/** Offset überschreitet Größe des Datenobjekts */

      CPcontrolFieldsRX.arrStatus[arrIndex] = CP_ERROR;
      CPcontrolFieldsRX.arrOffset[arrIndex] = 0;

  		/* Reception Error */
      CP_SaveErrorCode(6);


    	/** Starte erneut unterbrochenen Interrupt-getriebenen Empfang */
    	if (HAL_CAN_Receive_IT(&hcan, CAN_FIFO0) != HAL_OK)
    	{
    		/* Reception Error */
    		CP_SaveErrorCode(2);
				CPcontrolFieldsRX.arrStatus[arrIndex] = CP_ERROR;
    	}
			return;
		}
  }
  /** Überprüfe ob Datenobjekt nun vollständig versand wurde */
  if (CPcontrolFieldsRX.arrOffset[arrIndex] == CPcontrolFieldsRX.arrLength[arrIndex])
  {
    CPcontrolFieldsRX.arrStatus[arrIndex] = CP_OK;
    CPcontrolFieldsRX.arrOffset[arrIndex] = 0;
  }
  else
  {
    CPcontrolFieldsRX.arrStatus[arrIndex] = CP_BUSY;
  }

	/** Starte erneut unterbrochenen Interrupt-getriebenen Empfang */
	if (HAL_CAN_Receive_IT(&hcan, CAN_FIFO0) != HAL_OK)
	{
		/* Reception Error */
		CP_SaveErrorCode(2);
		CPcontrolFieldsRX.arrStatus[arrIndex] = CP_ERROR;
	}

	return;
}


/** Other Functions */

void 	CP_CAN_Init()
{
	CP_InitControlFieldsTx();
	CP_InitControlFieldsRx();

	/** Freigabe CP_Lock für nächsten SendeProzess */
	CP_LockCANtx = 0;



	// Nochmalig CAN initialisieren. Damit im Fehlerfall durch nochmaligen Aufruf der Funktion
	// CAN zurückgesetzt werden kann.
  if (HAL_CAN_Init(&hcan) != HAL_OK)
  {
  	CP_SaveErrorCode(8);
  }


	/*##-1- Configure the CAN peripheral #######################################*/
	hcan.pTxMsg = &TxMessage;
	hcan.pRxMsg = &RxMessage;

	/*##-2- Configure the CAN Filter ###########################################*/
	sFilterConfig.FilterNumber = 0;
	sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
	sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
	sFilterConfig.FilterIdHigh = 0x0000;
	sFilterConfig.FilterIdLow = 0x0000;
	sFilterConfig.FilterMaskIdHigh = 0x0000;
	sFilterConfig.FilterMaskIdLow = 0xFFFF;
	sFilterConfig.FilterFIFOAssignment = 0;
	sFilterConfig.FilterActivation = ENABLE;
	sFilterConfig.BankNumber = 14;

	if (HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) != HAL_OK)
	{
		/* Filter configuration Error */
		CP_SaveErrorCode(9);
	}


	/** Start the Reception process */
	if (HAL_CAN_Receive_IT(&hcan, CAN_FIFO0) != HAL_OK)
	{
		/* Reception Error */
		CP_SaveErrorCode(2);
		return;
	}
}


/******************************************************************************
 * Initialisierung von Variablen
 *
 * */
void CP_InitControlFieldsRx()
{
    for (int i = 0; i< NUMBDATAOBJMAX; i++ )
  {
    CPcontrolFieldsRX.arrID[i]       = 0xFFFF;
    CPcontrolFieldsRX.arrLength[i]   = 0x00;
    CPcontrolFieldsRX.arrOffset[i]   = 0x00;
    CPcontrolFieldsRX.arrPtrObj[i]   = 0x00;
    CPcontrolFieldsRX.arrStatus[i]   = CP_OK;
  }
}

/******************************************************************************
 * Initialisierung von Variablen
 *
 * */
void CP_InitControlFieldsTx()
{
  CPcontrolFieldsTX.arrID       = 0xFFFF;
  CPcontrolFieldsTX.arrLength   = 0x00;
  CPcontrolFieldsTX.arrOffset   = 0x00;
  CPcontrolFieldsTX.arrPtrObj   = 0x00;
  CPcontrolFieldsTX.arrStatus   = CP_OK;
}



/******************************************************************************
 *
 */
int16_t CP_searchIDRx(uint16_t ID)
{
	int16_t arrIndex = -1;
  for (int i = 0; (i < NUMBDATAOBJMAX)&&(arrIndex == -1); i++)
  {
    if (CPcontrolFieldsRX.arrID[i] == ID)
    {
      arrIndex = i;
    }
  }
  return arrIndex;
}

/******************************************************************************
 *
 */
void CP_SaveErrorCode(uint32_t ErrorCode)
{
	CP_LastErrorCodes[2] = CP_LastErrorCodes[1];
	CP_LastErrorCodes[1] = CP_LastErrorCodes[0];
	CP_LastErrorCodes[0] = ErrorCode;
}


/******************************************************************************
 *
 */
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
	switch(hcan->ErrorCode) {

		case HAL_CAN_ERROR_NONE: {
			CP_SaveErrorCode(0); break;
		}
		case HAL_CAN_ERROR_EWG: {
			CP_SaveErrorCode(11); break;
		}
		case HAL_CAN_ERROR_EPV: {
			CP_SaveErrorCode(12); break;
		}
		case HAL_CAN_ERROR_BOF: {
			CP_SaveErrorCode(13); break;
		}
		case HAL_CAN_ERROR_STF: {
			CP_SaveErrorCode(14); break;
		}
		case HAL_CAN_ERROR_FOR: {
			CP_SaveErrorCode(15); break;
		}
		case HAL_CAN_ERROR_ACK: {
			CP_SaveErrorCode(16); break;
		}
		case HAL_CAN_ERROR_BR: {
			CP_SaveErrorCode(17); break;
		}
		case HAL_CAN_ERROR_BD: {
			CP_SaveErrorCode(18); break;
		}
		case HAL_CAN_ERROR_CRC: {
			CP_SaveErrorCode(19); break;
		}
		default: {
			CP_SaveErrorCode(20); break;
		}
	}
	return;
}

