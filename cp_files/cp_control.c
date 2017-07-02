/**
 * CP_Steuerungsschicht.
 * @file        		cp_control.c
 * @ingroup     	CP_Control_Layer
 * @author      	Christoph Scharf
*/

 /**
 * @defgroup CP_Control_Layer CP Interne Steuerung
 * @{
 *  
 *  Interne Steuerung des CP-Protokolls.
 *
 * Der Kern der Steuerungsschicht ist der automatisierte Sende- und 
 * Empfangsprozess. Damit keine Wartezeiten auftreten, bis CAN-Nachrichten 
 * empfangen oder versendet sind, werden beide Funktionen durch Interrupts 
 * gesteuert. Hierfuer muss der Zustand des aktuellen Sende- oder 
 * Empfangsvorgang in @ref Steuerungsfelder gespeichert werden. 
 * Zusaetzlich finden in der Steuerungs-Schicht mehrere Hilfsfunktionen Platz, 
 * um z.B. um in einer Liste nach bereits initialisierten Identifier zu suchen:
 * CP_searchIDRx(). Oder eine Funktion um aufgetretene Fehler zu sortieren 
 * und in einer Liste zu speichern: CP_SaveErrorCode(). 
 *
  */
  
/****************************************************************************/
/* Includes */
/****************************************************************************/

//#include "cp_user.h"
//#include "cp_control.h"
#include "cp_control.h"
#include "can.h"

/****************************************************************************/
/* DEFINITION OF VARIABLES */
/****************************************************************************/

	/** Variable fuer die CAN-Filterkonfiguration */
	CAN_FilterConfTypeDef  		  	sFilterConfig;
	/** Variable fuer CAN-Sende-Buffer */
	static CanTxMsgTypeDef        TxMessage;
	/** Variable fuer CAN-Empfangs-Buffer */
	static CanRxMsgTypeDef        RxMessage;

	/** Variable zum Sperren der Parallelen Bearbeitung des CallBack-SendeProzess*/
	uint8_t CP_LockCANtx = 0;
	/** Variable zum Zaehlen der blockierten Interrupts fuer eine spaetere wiederholte Ausfuehrung.*/
	uint16_t missed_ir = 0;


 /**
 * @defgroup Fehlermanagement Fehlermanagement
 * @ingroup CP_User_Interface
 * @ingroup CP_Control_Layer
 * @{
 * 
 * Fehlermanagement und Auslesen der Fehlerliste.
 *
 * Jeder erkannte Fehler wird mit einem Fehlercode versehen und dieser 
 * in einer Fehlerliste gespeichert. Derzeit nimmt die Fehlerliste insgesamt 
 * 3 aufgetretene Fehler nach dem FIFO-Prinzip auf. D.h. immer die 3 zuletzt 
 * aufgetretenen Fehler sind in der Liste abrufbar. 
 * 
 * Diese Fehlerliste <b>CP_LastErrorCodes[3]</b> ist ein Array mit 3 Feldern, 
 * wobei sich an erster Stelle immer der Fehlercode des zuletzt 
 * aufgetretenen Fehlers befindet.
 * Dazu existiert ein Fehlerbeschreibungs-Array <b>CP_ERROR_DESCRIPTION[]</b>, 
 * indem ueber den Array-Index (welcher dem Fehlercode entspricht) die jeweilige 
 * Fehlerbeschreibung als ASCI-Text ausgelesen werden kann. 
 * 
 * Will man nun den Fehlertext des zuletzt aufgetretenen Fehlers, kann 
 * folgendermassen auf diesen zugegriffen werden: 
 * <b>CP_ERROR_DESCRIPTION[CP_LastErrorCodes[0]]</b>
  */
 
	/** Fehler-Historie der aufgetretenen Fehler. Neuester Fehler auf Position 0 */
	uint8_t CP_LastErrorCodes[3] = {0,0,0};

	
	/****************************************************************************/
	/**
	*  Fehlerbeschreibungsliste als 2 dimensionales char Array.
	*  Gleichzusetzen mit String-Array
	*  Fehlercodes von 0 bis derzeit 22.
    *  Anwahlbar ueber Fehlercodenummer als Array-Index
	*/
	const char *CP_ERROR_DESCRIPTION[] = \
			{	\
			/** errCode: 00 */ \
			"CP_ERROR: Kein Fehler.\r\r"/**< errCode: 00 */,\
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
/** @} */



/****************************************************************************/
/* FUNCTION DEFINITIONS */
/****************************************************************************/

/****************************************************************************/
/**
 * CAN-SENDE-PROZESS 
 * Interrupt Callback Transmit
 *
 * Der CAN-Sendeprozess eines Datenobjekts wird durch den Anwender ueber die
 * Anwenderfunktionen CP_StartTx()in Gang gesetzt. Daraufhin erledigt diese
 * Callback-Funktion den kompletten Sendeprozess indem diese mehrmals abgearbeitet
 * wird. Zu Ihren Aufgaben gehoert das Datenmanagement der Segmentierung und das
 * Versenden der einzelnen CAN-Nachrichten bis die Uebertragung des Datenobjekts
 * abgeschlossen ist.
 *
 * Um das Senden der einzelnen CAN-Nachrichten schnellst moeglich voranzutreiben,
 * wird ein Interrupt des CAN-Controllers verwendet. Bei einem Sendevorgang wird
 * die CAN-Nachricht zum Senden in eine Transmit-Mailbox abgelegt. Sobald die
 * Nachricht versendet wurde, wird der Inhalt der Mailbox geloescht und Sie ist
 * wieder leer. Dieses Ereignis Transmit Mailbox becomes empty  loest einen
 * Interrupt aus, der die Callback-Funktion Transmission Complete
 * HAL_CAN_TxCpltCallback(..) aufruft. Bei jedem Aufruf dieser Funktion werden
 * 6 Bytes des Datenobjekts ausgelesen, und in einer CAN-Nachricht versendet,
 * worauf wieder ein Interrupt ausloest, der zum Senden des naechsten Datenpakets
 * fuehrt. Dies wiederholt sich solange, bis alle Daten des Datenpakets gesendet
 * wurden. Die Callback-Funktion ist in der HAL-Bibliothek in stm32f3xx_hal_can.c
 * deklariert und zur eigenen Implementierung vorgesehen.
 *
 * Waehrend der Abarbeitung der Callback-Funktion kann es durch einen erneut
 * auftretenden Mailbox-Empty-Interrupt  dazu kommen, dass die gleiche
 * Funktion wieder aufgerufen wird. Das fuehrt dazu, dass die aktuelle Bearbeitung
 * an unbestimmter Stelle innerhalb der Funktion durch den Interrupt
 * unterbrochen / pausiert wird und diese Callback-Funktion, durch den zuletzt
 * entstandenen Interrupt, erneut abgearbeitet wird. Dies wuerde zu undefinierbaren
 * Fehlern fuehren. So koennte nach der Aenderung des Steuerungsfeldes Offset eine
 * Unterbrechung stattfinden, ohne dass das Datenpaket ueberhaupt versendet wurde,
 * nur um hier ein Beispiel zu nennen.
 *
 * Aus genannten Gruenden wird der Sendeprozess zu Beginn der Funktion fuer eine
 * parallele Nutzung blockiert durch das Setzen einer Variable CP_LockCANtx.
 * Diese Lock-Variable wird nach der Abarbeitung der Funktion am Ende wieder
 * freigegeben.
 * Das Blockieren des Sendeprozess fuehrte jedoch zu folgendem Problem:
 * Da zu dem Zeitpunkt des Interrupts die Callback-Funktion noch nicht fertig
 * abgearbeitet war, ging somit der erneute Aufruf der Funktion durch die
 * Blockierung verloren. Es gab nun keinen Prozess mehr, der die Callback-Funktion
 * erneut aufruft. Somit war der CP-Sende-Vorgang unvollstaendig abgebrochen.
 * Zur Loesung des Problems werden blockierte Aufrufe der Callback-Funktionen in
 * einem Zaehler missed_IRs aufaddiert und nach der Abarbeitung, erneut aufgerufen.
 * Somit laeuft kein Interrupt ins Leere. Jeder Interrupt wird benoetigt, da der
 * Sendeprozess so oft aufgerufen werden muss, bis in diesem das Ende des
 * Datenobjekts erreicht wird.
 *
 *
 * Zu Beginn wird nochmals geprueft, ob die ID im zugelassenen Wertebereich liegt
 * und ob der das Steuerungsfeld Status auf CP_BUSY (Sendefreigabe) gesetzt ist.
 *
 * Ist dies der Fall, wird die Nummerierung des zu versendenden Datenpaketes berechnet.
 * Dieser Wert wird dann in die ersten 2 Datenbytes des CAN-Sendepuffers geschrieben.
 * Als naechstes wird die benoetigte Laenge (DLC) der zu sendenden CAN-Botschaft berechnet.
 * Dazu wird einfach die Laenge der noch insgesamt zu versendenden Bytes berechnet.
 * Sollte die Laenge groesser oder gleich 8 Bytes sein, wird die Laenge auf die
 * Maximal-Laenge eines normalen CAN-Frames gesetzt.
 * Fuer das Kopieren der Daten aus dem Datenobjekt in den CAN-Sende-Puffer wird ein
 * Adress-Zeiger auf das erste zu versendende Byte des Datenpakets gebildet.
 * Dieser wird fuer das Kopieren des aktuellen Bytes in einer Schleife benoetigt.
 * Mit jedem Schleifendurchlauf wird die Adresse des Zeigers um ein Byte erhoeht
 * und das Datenbyte in den Sendepuffer kopiert. Dies geschieht solange, bis die
 * berechnete Laenge des CAN-Frames (DLC) erreicht ist.
 *
 * Zusaetzlich wird geprueft, ob der Offset, d.h. die bereits abgearbeiteten Bytes
 * des Datenobjekts die Gesamtgroesse des Datenobjekts uebersteigt. Ist dies der Fall
 * liegt ein Fehler vor und der Sendevorgang wird abgebrochen. Nach dem die Daten
 * der Sendepuffer mit den Daten gefuellt ist, werden die Konfigurations-Parameter
 * der CAN-Nachricht im CAN-Handle-Objekt gesetzt und das Senden des Datenpakets
 * kann mit der CAN-Sende-Funktion gestartet werden.
 *
 * Die Interrupt-getriebene CAN-Sende-Funktion bricht nach dem ersten fehlgeschlagenen
 * Sendeversuch mit einem Fehler ab. Dies ist auch so gewollt. Jedoch ¬hat sich
 * herausgestellt, dass der erste Sendeversuch ueber die Interrupt-getriebene
 * Sendefunktion in selten Faellen nicht erfolgreich war. Die Gruende sind nicht
 * bekannt. Es koennte zum Beispiel der Fall eingetreten sein, dass alle
 * Transmit-Mailboxen belegt waren. Um den CP-Sendeprozess robuster gegen Ausfaelle
 * zu machen, wurde daher eine Wiederholung von fehlgeschlagenen Sendeversuchen
 * eingebaut. Abgebrochen werden die Sendeversuche dann erst wenn eine einstellbare
 * Zeit ueberschritten wurde, die ueber den Parameter TIMEOUTCANTX, in der Datei
 * cp_user.h in eingestellt werden kann.
 * Zum Ende des Sendeprozesses wird geprueft, ob mit der zuletzt versendeten
 * CAN-Nachricht alle Datenpakete des Datenobjekts versendet wurden. Ist dies der
 * Fall wird der Offset zurueckgesetzt und das Steuerungsfeld Status auf den Zustand
 * initialisiert gesetzt. Mit diesem Status wird die Callback-Funktion dann durch
 * obige Pruefung trotz verbleibenden Interrupts oder Aufrufen durch verlorene
 * Interrupts auch nicht mehr bearbeitet. Ein erneuter expliziter Aufruf durch die
 * Funktion CP_StartTx muss zum erneuten Senden des Datenobjekts erfolgen. Sofern
 * noch nicht alle Daten des Datenobjekts verschickt wurden,  bleibt der Status auf
 * CP_BUSY und der Interrupt-getriebene Sende-Prozess wird bei dem erneuten
 * Aufruf dieser Funktion fortgesetzt.
 *
 *
 *
 *@param *CanHandle CAN_Handle Object
 *
 **/
void HAL_CAN_TxCpltCallback(CAN_HandleTypeDef *CanHandle)
{
	/* Zugriff auf Sendeprozess nur, sofern aktuell
	 * nicht gesendet wird */
	if (CP_LockCANtx == 1)
	{

		/* SPEICHERE ZUSTAND VERLORENEN IR */
		missed_ir ++;
		return;

	}
	/* Blockieren von weiterem SendeProzess */
	CP_LockCANtx = 1;

	/* Pruefe ob ID im Wertebereich und Status == Senden?*/
	if ((CPcontrolFieldsTX.arrID > 0x07FF) | \
			(CPcontrolFieldsTX.arrStatus != CP_BUSY ))
	{
		/* Freigabe CP_Lock fuer naechsten SendeProzess */
		CP_LockCANtx = 0;
		return;
	}

	/* Berechne aktuell zu versendenden FrameNummer */
	uint16_t FrameNumberMsg = CPcontrolFieldsTX.arrOffset /6 + 1;

	/* Aufteilen der  FrameNumber auf 2 separate Bytes und */
	/* Speichern der Bytes in Sende-Daten-Puffer */
	CanHandle->pTxMsg->Data[0] = (uint8_t)(FrameNumberMsg >> 8);
	CanHandle->pTxMsg->Data[1] = (uint8_t)(FrameNumberMsg);

	/* DLC - Berechnung der Restgroesse von zu versendenden Datenbytes
	 * fuer die Bestimmung der CAN-Nachrichten-Laenge (DLC)*/
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
			/* Offset ueberschreitete Gesamt Groesse des Datenojekts*/
			CPcontrolFieldsTX.arrStatus = CP_ERROR;
			/* Freigabe CP_Lock fuer naechsten SendeProzess */
			CP_LockCANtx = 0;
			CP_SaveErrorCode(6);
	  	return;
		}

		/* Kopieren der Daten aus Datenobjekt in CAN-Sende-Puffer*/
		CanHandle->pTxMsg->Data[byteInFrame] = *pNow;

		pNow += 0x01;
		CPcontrolFieldsTX.arrOffset++;
	}

	/* Ueberpruefung ob Datenobjekt komplett versendet wurde */
	/* Offset = Groesse (Nach Sendevorg.): z.B. 1 Byte senden -> offset = 1*/
	if (CPcontrolFieldsTX.arrOffset == CPcontrolFieldsTX.arrLength)
	{
		/* Zuruecksetzen des Offset fuer neues Senden */
		CPcontrolFieldsTX.arrOffset = 0;
		CPcontrolFieldsTX.arrStatus = CP_OK;
	}
	else
	{
		CPcontrolFieldsTX.arrStatus = CP_BUSY;
	}

	/* Konfiguration der CAN-Sende-Parameter */
	hcan.pTxMsg->StdId = CPcontrolFieldsTX.arrID;
	hcan.pTxMsg->ExtId = 0x01;
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
			/* Freigabe CP_Lock fuer naechsten SendeProzess */
			CP_LockCANtx = 0;
			CP_SaveErrorCode(1);

		}
	}

	/* Freigabe CP_Lock fuer naechsten SendeProzess */
	CP_LockCANtx = 0;
	if (missed_ir > 0)
	{
		missed_ir --;
		HAL_CAN_TxCpltCallback(CanHandle);
	}
	return;
}


/****************************************************************************/
/**
 *
 * CAN-EMPFANGS-PROZESS
 * 
 +  Beschreibung und Inhalte der Funktion:
 *
 *  Pruefen ob der Empfang fuer angelegte ID bereits freigeschaltet ist.
 *  Die Freischaltung erfolgt in der Funktion CP_StartRx() durch das
 *  Setzen des StatusRx auf CP_BUSY
 *
 * Sollte ein Datenobjekt auf mehrere CAN-Nachrichten aufgeteilt werden,
 * wird vorausgesetzt, dass die Reihenfolge der aufgeteilten Nachrichten
 * eingehalten wird. Ohne diese Bedingung haette jede empfangene Nachrichten-Nummer
 * gespeichert werden muessen um bei jedem Empfang eines Datenpakets zu pruefen,
 * ob dieses Paket bereits empfangen wurden und welche noch ausstehen.
 *
 * Fuer die Ueberpruefung ob die erwartete Framenummer erhalten wurde, wird diese
 * Anzahl aus dem Offset (d.h. der bereits Empfangenen Datenbytes) berechnet und
 * anschliessend mit der Nachrichten Nummer aus der empfangenen
 * CAN-Nachricht verglichen.
 *
 *
 * Sofern dies empfange Paketnummer nicht die Erwaretete ist, kann nicht
 * sichergestellt werden, dass alle Nachrichten fuer dieses Datenobjekt
 * empfangen werden. Daher muss der Empfang fuer dieses Datenobjekt abgebrochen
 * werden. Das Steuerungsfeld Status fuer diese ID wird dann auf den Fehlerstatus
 * CP_ERROR gesetzt.
 * Sollte es sich jedoch um die erste FrameNummer (=1) handeln, d.h. das erste
 * Datenpaket eines Datenobjekts, wird der Offset zurueck gesetzt und somit der
 * Empfang des Datenobjekts
 * erneut begonnen, ohne dass der Empfang fuer diese ID erneut aktiviert werden
 * muss. Somit kann ein Knoten, der das Senden eines Datenobjekts aufgrund eines
 * Fehlers abbricht, den Sendevorgang neu starten ohne dass der Empfaenger in den
 * Fehlerzustand kommt. Allerdings ist das Datenobjekt in dieser Zwischenzeit
 * immer im Status des Empfangsmodus und sollte nicht ausgelesen werden, da die
 * Daten nicht konsistent sind.
 *
 * Im naechsten Schritt wird ein Zeiger erstellt, der fuer auf die Adresse des
 * naechsten zu bearbeitenden Bytes im Datenobjekt zeigt.
 *
 * Aehnlich wie beim Sendeprozess, werden nun in einer Schleife die Dateninhalte
 * zwischen der CAN-Nachricht und dem Datenobjekt kopiert. Mit jedem
 * Schleifendurchlauf wird ein Byte aus der empfangenen CAN-Nachricht in das aktuelle
 * Byte des Datenobjekts gespeichert. Anschliessend wird vor dem naechsten Durchlauf
 * die Adresse des Zeigers und der Offst um eine Stelle erhoeht. Dies geschieht
 * solange bis die Laenge der Nachricht (DLC) erreicht ist. Da die ersten zwei Bytes
 * die Paketnummerierung enthalten also bis maximal 6 Durchlaeufe.
 *
 * Sofern der gespeicherte Offset groesser ist, als die
 * Groesse des Datenobjekts ist ein Fehler entstanden. Dieser Fall kann zum
 * Beispiel auftreten, wenn der Empfaengerknoten unter diesem Identifier ein
 * anderes (kleineres) Datenobjekt erwartet, als vom Sende-Knoten initialisiert
 * und gesendet wird.
 *
 * Wenn alle Daten des Datenobjekts empfangen wurden, wird der Status
 * auf CP_OK gesetzt und ein erneuter expliziter Aufruf der
 * Funktion CP_StartTx() muss zum erneuten Empfang fuer diese ID erfolgen.
 *
 * Sofern noch nicht alle Daten des Datenobjekts empfangen wurden,
 * bleibt der Status auf CP_BUSY und der Empfangs-Prozess wird bei dem
 * erneuten Aufruf dieser Funktion fortgesetzt.
 *
 * Am Ende muss durch den Aufruf der Interrupt-Empfangs-Funktion:
 * HAL_CAN_Receive_IT() der Interrupt-getriebene Empfang wieder aktiviert
 * werden. Die Interrupts wurden von der HAL-Ebene vor dem Aufruf dieser
 * Funktion ausgesetzt, sodass keine noch nicht verarbeiteten Daten
 * ueberschrieben werden koennen.
 *
 *@param *CanHandle CAN_Handle Object
 *
 **/
void HAL_CAN_RxCpltCallback(CAN_HandleTypeDef *CanHandle)
{

	// Nach Aufruf der Funktion ist Receive Interrupt deaktiviert?

	uint32_t ID = CanHandle->pRxMsg->StdId;

	/* Suche, ob ID bereits angelegt ist.*/
  int16_t arrIndex;
  arrIndex = CP_searchIDRx(ID);


  if (arrIndex == -1)
  {
  	/* Botschaft verwerfen */
  	/* Starte erneut unterbrochenen Interrupt-getriebenen Empfang */
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
  	/* Botschaft verwerfen */
  	/* Starte erneut unterbrochenen Interrupt-getriebenen Empfang */
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
      	/* Botschaft verwerfen */

        /* Die Reihenfolge der CAN-Botschaften ist nicht richtig, Zuruecksetzen des aktuellen Empfangs*/
        CPcontrolFieldsRX.arrStatus[arrIndex] = CP_ERROR;
        CPcontrolFieldsRX.arrOffset[arrIndex] = 0;

    		/* Reception Error */
        CP_SaveErrorCode(7);

      	/* Starte erneut unterbrochenen Interrupt-getriebenen Empfang */
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
			/* Offset ueberschreitet Groesse des Datenobjekts */

      CPcontrolFieldsRX.arrStatus[arrIndex] = CP_ERROR;
      CPcontrolFieldsRX.arrOffset[arrIndex] = 0;

  		/* Reception Error */
      CP_SaveErrorCode(6);


    	/* Starte erneut unterbrochenen Interrupt-getriebenen Empfang */
    	if (HAL_CAN_Receive_IT(&hcan, CAN_FIFO0) != HAL_OK)
    	{
    		/* Reception Error */
    		CP_SaveErrorCode(2);
				CPcontrolFieldsRX.arrStatus[arrIndex] = CP_ERROR;
    	}
			return;
		}
  }
  /* Ueberpruefe ob Datenobjekt nun vollstaendig versand wurde */
  if (CPcontrolFieldsRX.arrOffset[arrIndex] == CPcontrolFieldsRX.arrLength[arrIndex])
  {
    CPcontrolFieldsRX.arrStatus[arrIndex] = CP_OK;
    CPcontrolFieldsRX.arrOffset[arrIndex] = 0;
  }
  else
  {
    CPcontrolFieldsRX.arrStatus[arrIndex] = CP_BUSY;
  }

	/* Starte erneut unterbrochenen Interrupt-getriebenen Empfang */
	if (HAL_CAN_Receive_IT(&hcan, CAN_FIFO0) != HAL_OK)
	{
		/* Reception Error */
		CP_SaveErrorCode(2);
		CPcontrolFieldsRX.arrStatus[arrIndex] = CP_ERROR;
	}

	return;
}


/****************************************************************************/
/* Weitere Funktionen */
/****************************************************************************/

/****************************************************************************/
/**
 *
 *  - Intitialisierung des CP-Protokolls.
 *
 *  - Erst-Initialisierung der ControllFields.
 *
 *  - CAN initialisieren
 *
 *  - CAN Peripherie initialisieren
 *
 *  - CAN Empfangsfilter erstellen
 *
 *  - CAN Start Interrupts fuer Empfang
 *
 * @return Status ob Funktion ohne Fehler ausgefuehrt wurde
 */
CP_StatusTypeDef CP_CAN_Init()
{
	CP_InitControlFieldsTx();
	CP_InitControlFieldsRx();

	/* Freigabe CP_Lock fuer naechsten SendeProzess */
	CP_LockCANtx = 0;



	/* Nochmalig CAN initialisieren. Damit im Fehlerfall durch nochmaligen Aufruf der Funktion */
	/* CAN zurueckgesetzt werden kann. */
  if (HAL_CAN_Init(&hcan) != HAL_OK)
  {
  	CP_SaveErrorCode(8);
	return CP_ERROR;
  }


	/* Configure the CAN peripheral */
  	/* Zuordnung der Sende- und Empfangsstruktur zu CanHandleTypeDef-Objekt **/
	hcan.pTxMsg = &TxMessage;
	hcan.pRxMsg = &RxMessage;

	/* Konfiguration eines CAN Filters der im Mode: Mask so konfiguriert ist,*/
	/* dass jede CAN-Nachricht empfangen wird. */

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

	/* Ueberpruefe und Aktiviere Filter */
	if (HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) != HAL_OK)
	{
		/* Filter configuration Error */
		CP_SaveErrorCode(9);
		return CP_ERROR;
	}


	/* Start des Interrupt-getriebenen Empfang von CAN-Nachrichten */
	if (HAL_CAN_Receive_IT(&hcan, CAN_FIFO0) != HAL_OK)
	{
		/* Reception Error */
		CP_SaveErrorCode(2);
		return CP_ERROR;
	}
	
	return CP_OK;
}


/****************************************************************************/
/**
 * Initialisierung / Zuruecksetzen aller Steuerungsfelder
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


/****************************************************************************/
/**
 *  Initialisierung / Zuruecksetzen aller Steuerungsfelder
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


/****************************************************************************/
/**
 *		Suche einen Identifier in dem ID-Steuerungsfeld-Array.
 *
 *		@return Array-Index des gefundenen Identifiers, Ansonsten -1
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


/****************************************************************************/
/**
 *   Speichern eines Fehlercodes in die Fehlerliste.
 *
 *  Diese Funktion speichert den uebergebenen Fehlercode in ein
 *  Array mit 3 Feldern. Die sogenannte Fehlerliste #CP_LastErrorCodes[3]
 *  Der zuletzt entstandene Fehler ist immer auf
 *  dem Arrayplatz 0. Bei einem neuen Fehler, ruecken alle Fehler um
 *  einen Platz nach unten in der Liste. Der letzte fliegt raus.
 *
 */
void CP_SaveErrorCode(uint32_t ErrorCode)
{
	CP_LastErrorCodes[2] = CP_LastErrorCodes[1];
	CP_LastErrorCodes[1] = CP_LastErrorCodes[0];
	CP_LastErrorCodes[0] = ErrorCode;
}


/****************************************************************************/
/**
 *  Fangen der Canfehler aus der HAL-Schicht.
 *
 *  Bei einem entstehenden CAN-Fehler auf Controller-Ebene
 *  wird ein Interrupt ausgeloesst.
 *  Diese Callback-Funktion wird daraufhin aufgerufen.
 *  Dessen Aufgabe ist es aus dem CAN-Handle-Objekt
 *  den Fehlercode auszulesen und diesen mit der Funktion
 *  CP_SaveErrorCode() in die Fehlerliste zu speichern. Siehe auch@ref Fehlermanagement.
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

/** @} */