/**
 * CP Example.
 * @file        		cp_example.c
 * @ingroup     	CP_Example
 * @author      	Christoph Scharf
*/

/**
 * @defgroup CP_Example Beispiel Programm
 * @ingroup Weitere
 * @{
 *
 * Beispielprogramm zur Anwendung des CP-Protokolls.
 *
 * Es werden 2 Datenobjekte angelegt, die zum Austausch der Daten 
 * dienen. Der Ablauf ist folgender:
 *
 * Mikrocontroller 1 schreibt und sendet Datenobjekt 1.
 * Microcontroller 2 schreibt und sendet Datenobjekt 2.
 *
 * Mikrocontroller 1 empfaengt und liest Datenobjekt 2.
 * Mikrocontroller 2 empfaengt und liest Datenobjekt 1.
 *
 *
 * Die Auswahl, #MC_StatusTypeDef,
 * welcher Mikrocontroller als 1 oder 2 (nachfolgende MC1 / MC2 genannt) 
 * arbeitet, wird entschieden, indem zu Beginn des Programm-Ablaufs der 
 * digitale Eingang geprueft wird.
 *
 * Ist bei Start des Mikrocontrollers und dabei
 * der digitale Eingang auf High gesetzt, fungiert dieser Mikrocontroller als MC1.
 * der digitale Eingang auf Low  gesetzt, fungiert dieser Mikrocontroller als MC2.
 *
 *
 * Die Mikrocontroller haben einen identischen Programmablauf, mit dem Unterschied
 * der Indizes. D.h. MC1 senden mit Identifier 1, MC2 sendet mit Identifier 2, ...
 * Damit nicht alle Funktionsabläufe doppelt programmiert werden müssen,
 * wird eine variable Zuordnung über Hilfsvariablen mit den Indizes a und b
 * getätigt.
 * Z.B. für Mikrocontroller 1: ID_tx = 1, ID_rx = 2
 * 		für Mikrocontroller 2: ID_tx = 2, ID_rx = 1
 *
 * Das Ziel ist, dass sich die Mikrocontroller einmal in der Sekunde 
 * gegenseitig ueber den CAN-Bus ein Steuer-Signal fuer den  digitalen 
 * Ausgang des anderen schicken.
 * Je nach Nachrichteninhalt setzt der Mikrocontroller
 * nach dem Empfang der Botschaft seinen digitalen Ausgang auf High 
 * oder Low. Welches Steuersignal verschickt wird, haengt davon ab, ob 
 * der digitale Eingang durch den Anwender auf High oder Low gesetzt wird.
 *
 * Das heisst,
 * der digitale Eingang von MC1 schaltet den digitalen Ausgang auf MC2 (LED auf MC2)
 * der digitale Eingang von MC2 schaltet den digitalen Ausgang auf MC1 (LED auf MC1)
 *
 */

/****************************************************************************/
/* INCLUDES */
/****************************************************************************/

#include "cp_user.h"


/****************************************************************************/
/* DEFINES */
/****************************************************************************/

typedef enum
{
	MC1      	= 0,			 /**< 0, Auswahl Mikrocontroller 1*/
	MC2			= 1,			 /**< 1, Auswahl Mikrocontroller 2*/
}MC_StatusTypeDef;


/****************************************************************************/
/* DEFINITION OF VARIABLES */
/****************************************************************************/

/* Variable for selecting microcontroller 1 or 2 */
MC_StatusTypeDef MC = MC1;	

/* Definition of Identifiers */
ID1 = 0x001;
ID2 = 0x002;

/* Define cycle to Send CAN message in milliseconds*/
const uint32_t TCYCLE_CanTx = 1000;
/* Timer variable  */
uint32_t tickStartCanTx = 0;

/* variable to save status of functions*/
CP_StatusTypeDef CP_Status = CP_OK;

/* textBuffer for saving error description */
char saveErrorDescription[100] = "";
/* Variable for saving error code */
uint8_t saveErrorCode = 0;


/* Mapping variables - Pointer to Dataobject*/
uint8_t* ptr_DataObj_tx = &DataObj_MC1;
uint8_t* ptr_DataObj_rx = &DataObj_MC2;
/* Mapping variables - Size of Dataobject*/
uint32_t size_DataObj_tx = 0;
uint32_t size_DataObj_rx = 0;
/* Mapping variables - Identifier*/
uint16_t ID_tx = 0;
uint16_t ID_rx = 0;
/****************************************************************************/
/* FUNCTIONS */
/****************************************************************************/

/****************************************************************************/
/**
 * Beispielprogamm zur Nutzung des CP-Protokolls
 *
 *
 *
 */
void CP_example()
{

	/* Digital Input is 'high' -> Selection = MC1 */
	if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4) == 1)
	{
		MC = MC1;
	}
	else
	{
		MC = MC2;
	}

	/* Mapping of variables, depending on selected Microcontroller 1/2 */
	if (MC == MC1)
	{
		ptr_DataObj_tx = &DataObj_MC1;
		ptr_DataObj_rx = &DataObj_MC2;

		size_DataObj_tx = sizeof(DataObj_MC1);
		size_DataObj_rx = sizeof(DataObj_MC2);

		ID_tx = ID1;
		ID_rx = ID2;
	}
	else if (MC == MC2)
	{
		ptr_DataObj_tx = &DataObj_MC2;
		ptr_DataObj_rx = &DataObj_MC1;

		size_DataObj_tx = sizeof(DataObj_MC2);
		size_DataObj_rx = sizeof(DataObj_MC1);

		ID_tx = ID2;
		ID_rx = ID1;
	}


	/* Init Transmit Dataobject 1 with ID 1 */
	CP_Status = CP_InitTx(ptr_DataObj_tx, size_DataObj_tx, ID_tx);
	/* Init Receive  Dataobject 2 with ID 2*/
	CP_Status = CP_InitRx(ptr_DataObj_rx, size_DataObj_rx, ID_rx);


	/* If one CP-Function returned an Error, use for example the function
	 * Error-Handler. It's defined below. */
	if (CP_Status == CP_ERROR)
	{
		Error_Handler();
	}


	/* Get recent tick for counting cycle time */
	tickStartCanTx = HAL_GetTick();


	while(1)
	{
		/*Enter loop every defined cycle time: TCYCLE_CanTx (default=1000ms) */
		if ((HAL_GetTick() - tickStartCanTx) > TCYCLE_CanTx)
		{
			/* Get recent tick for counting next cycle time */
			tickStartCanTx = HAL_GetTick();

			if (MC == MC1)
			{
				/*Read out digital input pin and store result in DataObj (for sending)*/
				DataObj_MC1.Value = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4);
			}
			else if (MC == MC2)
			{
				/*Read out digital input pin and store result in DataObj (for sending)*/
				DataObj_MC2.Value = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4);
			}


			/***********************S-E-N-D-I-N-G*********************************/

			/*Check status of CP sending process */
			if (CP_StatusTx() == CP_OK)
			{
				/*if ready, start sending*/
				CP_StartTx();
			}
			else if (CP_StatusTx() == CP_ERROR)
			{
				/* Function for the user, it is defined below */
				ErrorHandler();

				/* Reinitialize Transmit Dataobject tx with ID tx */
				CP_Status = CP_InitTx(ptr_DataObj_tx, size_DataObj_tx, ID_tx);
			}


			/**********************R-E-C-E-I-V-E**********************************/

			/* Before you read out the received Dataobject, check if the
			 * receiving process has already finished and there was no error.
			 * (Not important in this case with only one variable)*/
			if (CP_StatusRx(ID_rx) == CP_OK)
			{
				if (MC == MC1)
				{
					/*Write digital output pin 1 as MC1*/
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, DataObj_MC2.Value);
				}
				else if (MC == MC2)
				{
					/*Write digital output pin 2 as MC2*/
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, DataObj_MC1.Value);
				}

				/* Start receiving new Message*/
				CP_StartRx(ID_rx);


			}
			else if (CP_StatusRx(ID_rx) == CP_ERROR)
			{
				/* Function for the user, it is defined below */
				ErrorHandler();

				/* Init Transmit Dataobject 1 with ID 1 */
				CP_Status = CP_InitRx(ptr_DataObj_rx, size_DataObj_rx, ID_rx);
			}

		}
	}

}


void ErrorHandler()
{
	/* Do something.. for example reading out Error */
	/* Reading out all Error Code in error list*/
	saveErrorCode = CP_LastErrorCodes[0];
	saveErrorCode = CP_LastErrorCodes[1];
	saveErrorCode = CP_LastErrorCodes[2];

	/* Reading out last Error description and save to buffer */
	sprintf(saveErrorDescription,CP_ERROR_DESCRIPTION[CP_LastErrorCodes[0]]);

	/* Waiting 3 seconds */
	HAL_Delay(3000);
}












/** @} */



