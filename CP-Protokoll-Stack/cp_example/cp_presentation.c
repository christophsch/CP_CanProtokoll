
/* Includes ------------------------------------------------------------------*/
#include "presentation.h"
#include "stm32f3xx_hal.h"

#include "adc.h"
#include "can.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

#include "cp_user.h"
#include "cp_intern.h"

#include "string.h"



/******************************************************************************/
/** DECLARATION CONSTANTS AND VARIABLES */
/******************************************************************************/

 /** main --------------------------------------------------------------------*/
 HAL_StatusTypeDef hal_status;
 CP_StatusTypeDef	cp_status;

 char*		MC = "MC1";

 uint32_t tickStartTx = 0;
 uint32_t tickStartRx = 0;
 uint32_t tickStartADC = 0;
 uint32_t tickStartPWM = 0;
 uint32_t tickStartButtonIR = 0;


 // Count Variable fuer Messwert-Array
 uint8_t numbSensorValue = 0;
#define MAXSENSORVALUES 10


const uint8_t TCYCLE_TX =  30;

const uint8_t TCYCLE_RX =  10;
const uint8_t TCYCLE_MC1 = 10;
const uint8_t TCYCLE_MC2 = 10;
const uint8_t TCYCLE_MC3 = 10;

const uint16_t TCYCLE_BUTTONIR = 200;



 /** CAN --------------------------------------------------------------------*/
 uint32_t IDSelect1 = 0x111;
 uint32_t IDSelect2 = 0x222;
 uint32_t IDSelect3 = 0x333;
 uint32_t IDSelect4 = 0xFFFF;

 uint32_t IDtmp;
 uint32_t IDzykl;


 uint32_t* DataObjSelectPtr = 0;
 uint32_t	 DataObjSelectSize = 0;

 uint8_t  cpState = 0;

// // Hilsvars fuer Anzeige empfangene CAN-Message fuer IDNummer 1-5
// uint8_t dataReceived[5] = {0,0,0,0,0};
// uint8_t dataReceivedLast[5] = {0,0,0,0,0};


 /** ADC - DAC ---------------------------------------------------------------*/
 uint32_t adcval;
 uint32_t *adcmemory;

#define MAXADCVAL 4094		// 12 bit


 /** BUTTONS -----------------------------------------------------------------*/
 uint8_t button0 = 0;
 uint8_t button1 = 0;
 uint8_t buttonIR = 0;

 uint8_t button0Last = 0;
 uint8_t button1Last = 0;


 uint8_t LockButtons = 0;

 /** UART --------------------------------------------------------------------*/
 char uartTxBuffer[160];
 char uartRxBuffer[1];

#define UARTTIMEOUT 3000
 // TODO: TEST
#define UARTMAXBUFF 250
 // TODO: TEST
 char uartRxBufferAll[UARTMAXBUFF] = "";
 //TODO: test
 uint32_t UARTnumbMsg = 0;

 char uartRxMenu = 0;								// Auswahl der Menu-Nummer
 uint8_t LockUARTrx = 0;						// Sperre Interrupt Behandlung von Uart-Rx

 uint8_t selectTxZykl	= 0;
 uint8_t selectRxZykl	= 0;


 /** Text fuer Haupt-Menu */
 const char *uartTxMenu = \
"\r\r\r\r\r\r\r\r\r\r\r\r\r***************************************************\r\
Haupt-Auswahl-Menu:\r\r\
\
[1] CP_InitTx()\r\
[2] CP_StartTx()\r\
[3] CP_StatusTx()\r\r\
\
[4] CP_InitRx()\r\
[5] CP_StartRx()\r\
[6] CP_StatusRx()\r\
[7] CP_DeleteRx()\r\r\
\
[8] Start / Stop zyklischer Versand \r\
[9] Starte / Stop zyklischer Empfang \r\r\
\
[0] Remote Steuerung von Mikrocontroller 3 \r\r\
\
[-] Datenobjekte anzeigen\r\
[+] Datenobjekte aendern\r\r\
[F] Fehlerliste\r\
[A] Mikrocontrollerauswahl 1/2/3 \r\
*************************************************\r\r";

 const char *uartTxMenuID = \
"***************************************************\r\
ID-Auswahl-Menu:\r\r\
[1] ID = 0x111\r\
[2] ID = 0x222\r\
[3] ID = 0x333\r\
[4] ID = 0xFFFF\r\
*************************************************\r\r";

 const char *uartTxMenuDataObj = \
"***************************************************\r\
Datenobjekt-Auswahl-Menu:\r\r\
[1] Datenobjektnummer 1.1 -> Remote-Control\r\
[2] Datenobjektnummer 1.2 -> Presaentations Daten\r\
[3] Datenobjektnummer 1.3 -> Text, Messwert 16 bit Manuell und ADC\r\
*************************************************\r\r";

 const char *uartTxMenuDataObj1_3 = \
"***************************************************\r\
Datenobjekt-Auswahl-Menu:\r\r\
[1] Text eingeben\r\
[2] Messwert Manuell\r\
*************************************************\r\r";

 const char *uartTxMenuRemote = \
"***************************************************\r\
Remote-Auswahl-Menu:\r\r\
[1] Initialisiere IDs (0x222,0x333)\r\
[2] Starte Empfang von IDs (0x222,0x333)\r\
[3] Starte Empfang zyklisch von IDs (0x222,0x333)\r\
*************************************************\r\r";
 /** PWM --------------------------------------------------------------------*/
uint32_t pwm = 0;

/******************************************************************************/
/** FUNCTIONS */
/******************************************************************************/


 /******************************************************************************
  *
  */
	void presi_Init()
	{
	 hal_status = HAL_OK;
	 cp_status	 = CP_OK;

	 //// TODO: TestVars ADC
		adcval = 0;
		adcmemory = 0;


		cptest_setDataObjects();
		buttons_init();

		MC = "MC3";

		HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_1);	// PB_4 / D12 - Links unten
		HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_2);	// PB_5 / D11	- Links unten (2)
		HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_3);	// PA_2 / A7	- Rechts oben


	}

/******************************************************************************
 *
 */
	void cptest_setDataObjects()
	{
		DataObj_MC1_1.D1 = '$';


		DataObj_MC1_2.Value[0] = 0;

		DataObj_MC1_2.Value[1] = 0;
		DataObj_MC1_2.Value[2] = 0;
		DataObj_MC1_2.Value[3] = 0;

		for(int i = 0; i<MAXLENGTHTEXT; i++)
		{
			DataObj_MC1_3.c[i] = 0;
		}
		sprintf(DataObj_MC1_3.c,"TEXT BUFFER TO SEND ON CP_CAN\r");

		DataObj_MC1_3.manual = 0;
		DataObj_MC1_3.button_state = 0;

		DataObj_MC2_1.adcval = 0;
	}



/******************************************************************************/
/** UART Functions UART UART UART UART UART UART UART UART UART UART UART UART  */
/******************************************************************************/



/******************************************************************************
 *
 */
	HAL_StatusTypeDef	UART_Wait_Ready(timeout)
	#ifdef UARTDEBUG
	{
		uint32_t duration_uart = 0;

		while ( ( huart2.gState != HAL_UART_STATE_READY) & ( duration_uart <= timeout ) )
		{
			HAL_Delay(40);
			duration_uart += 40;
		}
		if (duration_uart <= timeout)
		{
			return HAL_OK;
		}
		else
		{
			return HAL_BUSY;
		}

	#endif
	}

/******************************************************************************
 *
 */
	HAL_StatusTypeDef	HAL_UART_Transmit_Presi(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
	#ifdef UARTDEBUG
	{
		uint32_t duration_uart = 0;
		while ( ( (hal_status = HAL_UART_Transmit_IT(huart,pData,Size)) != HAL_OK) & ( duration_uart <= UARTTIMEOUT ) )
		{
			HAL_Delay(40);
			duration_uart += 40;
		}
		return hal_status;

	#endif
	}

/******************************************************************************
 *
 */
	uint32_t scanID()
	{
		HAL_UART_Transmit_Presi(&huart2,uartTxMenuID,strlen(uartTxMenuID));
		uartRxMenu = 0;

		while	( (	(uartRxMenu != '1') & (uartRxMenu != '2') & (uartRxMenu != '3') &\
							(uartRxMenu != '4') ) )
		{
				LockUARTrx = 0;
				HAL_Delay(100);
		}
		LockUARTrx = 1;

	  switch(uartRxMenu)
	  {
	  	case '1': {return IDSelect1; }break;
	  	case '2': {return IDSelect2; }break;
	  	case '3': {return IDSelect3; }break;
	  	case '4': {return IDSelect4; }break;
	  }


	}

	/******************************************************************************
	 *
	 */
		uint8_t scanDataObj()
		{
			HAL_UART_Transmit_Presi(&huart2,uartTxMenuDataObj,strlen(uartTxMenuDataObj));
			uartRxMenu = 0;
			LockUARTrx = 0;

			while	( (	(uartRxMenu != '1') 		& (uartRxMenu != '2') 			& (uartRxMenu != '3') 			)	)
			{
					LockUARTrx = 0;
					HAL_Delay(100);
			}
			LockUARTrx = 1;

		  switch(uartRxMenu)
		  {
		    case '1':
					{
						DataObjSelectPtr = &DataObj_MC1_1;
						DataObjSelectSize = sizeof(DataObj_MC1_1);
						return 1;
					}break;
		    case '2':
					{
						DataObjSelectPtr = &DataObj_MC1_2;
						DataObjSelectSize = sizeof(DataObj_MC1_2);
						return 2;
					}break;
		    case '3':
					{
						DataObjSelectPtr = &DataObj_MC1_3;
						DataObjSelectSize = sizeof(DataObj_MC1_3);
						return 3;
					}break;
		  }
		}

/******************************************************************************
 *
 */
	void scanMC()
	{
		HAL_UART_Transmit_Presi(&huart2,"Nummer eingeben: ",strlen("Nummer eingeben: "));
		uartRxMenu = 0;
		LockUARTrx = 0;

		while	( (	(uartRxMenu != '1') 		& (uartRxMenu != '2') 			& (uartRxMenu != '3') 			)	 )
		{
				LockUARTrx = 0;
				HAL_Delay(100);
		}
		LockUARTrx = 1;

		switch(uartRxMenu)
		{
			case '1': {MC = "MC1";} break;
			case '2': {MC = "MC2";} break;
			case '3': {MC = "MC3";} break;
		}
		uartRxMenu = 0;
	}

	/******************************************************************************
	 *
	 */
		char scanRemote()
		{
			HAL_UART_Transmit_Presi(&huart2,uartTxMenuRemote,strlen(uartTxMenuRemote));
			uartRxMenu = 0;

			while	( (uartRxMenu != '1') & (uartRxMenu != '2') & (uartRxMenu != '3') )
			{
					LockUARTrx = 0;
					HAL_Delay(100);
			}
			LockUARTrx = 1;

		  switch(uartRxMenu)
		  {
		  	case '1': {return '4'; }break;
		  	case '2': {return '5'; }break;
		  	case '3': {return '9'; }break;
		  }


		}

	/******************************************************************************
	 *
	 */
	 void UART_PrintMainMenu()
	 {
			/** HAUPTMENUE */
			while (HAL_UART_Transmit_Presi(&huart2,uartTxMenu,strlen(uartTxMenu)) != HAL_OK)
			{ HAL_Delay(100);}
	 }


	/******************************************************************************
	 *
	 */
	 void UART_printDataObj()
	 {
		 uint8_t auswahltmp = scanDataObj();
//		 UART_PrintMainMenu();
		 switch(auswahltmp)
		 {
			 case 1:
				 {
					 	UART_Wait_Ready(UARTTIMEOUT);
						sprintf(uartTxBuffer,"Datenobjekt Nummer 1: %c\r",DataObj_MC1_1.D1);
						HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));
				 }break;
			 case 2:
				 {
					 	UART_Wait_Ready(UARTTIMEOUT);
						sprintf(uartTxBuffer,"Datenobjekt Nummer 2: \r");
						HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));

						uint32_t *ptrtmp = &DataObj_MC1_2;
						for (int i = 1; i <= sizeof(DataObj_MC1_2)/4; i++)
						{
							UART_Wait_Ready(UARTTIMEOUT);
							sprintf(uartTxBuffer,"4-Byte Datenblock Nummer %d: 0x%x = %d\r",i,*ptrtmp,*ptrtmp);
							HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));
							ptrtmp++;
						}
				 }break;
			 case 3:
				 {
						UART_Wait_Ready(UARTTIMEOUT);
						sprintf(uartTxBuffer,"\rDatenobjekt 1_3: Messwert Manuell = %d\r",DataObj_MC1_3.manual);
						HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));

						UART_Wait_Ready(UARTTIMEOUT);
						sprintf(uartTxBuffer,"Datenobjekt 1_3: Messwert ADC = %d\r",DataObj_MC1_3.adcval);
						HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));

					 	UART_Wait_Ready(UARTTIMEOUT);
						sprintf(uartTxBuffer,"Datenobjekt 1_3: Text: \r");
						HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));

						HAL_UART_Transmit_Presi(&huart2,DataObj_MC1_3.c,strlen(DataObj_MC1_3.c));
				 }break;
			 default:
				 break;

		 }
	 }

	 void UART_changeDataObj()
	 {
		 uint8_t auswahltmp = scanDataObj();
//		 UART_PrintMainMenu();

		 switch(auswahltmp)
		 {
			 case 1:
				 {
					 	UART_Wait_Ready(UARTTIMEOUT);
					 	sprintf(uartTxBuffer,"Datenobjekt 1 eingeben. (1 Zeichen): ");
						HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));
						UART_scanChars(1);
						sscanf(uartRxBufferAll,"%c",&DataObj_MC1_1.D1);

						HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartRxBufferAll, strlen(uartRxBufferAll));
				 }break;
			 case 2:
				 {
					  UART_Wait_Ready(UARTTIMEOUT);
						sprintf(uartTxBuffer,"Datenobjekt 2. (Je 4-Byte-Wert 8 Hex-Zahlen)\r");
						HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));

						uint32_t *ptrtmp = &DataObj_MC1_2;
						for (int i = 1; i <= sizeof(DataObj_MC1_2)/4; i++)
						{
							UART_Wait_Ready(UARTTIMEOUT);
							sprintf(uartTxBuffer,"\rEingabe Datenbyte Nummer %d: ",i);
							HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));

							UART_scanChars(8);
							sscanf(uartRxBufferAll,"%x",ptrtmp);
							HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartRxBufferAll, strlen(uartRxBufferAll));

							ptrtmp++;
						}
				 }break;
			 case 3:
				 {
					 /* MESSWERT EINGABE **/
					  UART_Wait_Ready(UARTTIMEOUT);
					  sprintf(uartTxBuffer,"Eingabe Datenobjekt 4: Messwert (12 Bit; Maximalwert= 4094): ");
						HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));

						UART_scanChars(4);
						sscanf(uartRxBufferAll,"%d",&DataObj_MC1_3.manual);
						if (DataObj_MC1_3.manual > 4094)
						{
							DataObj_MC1_3.manual = 4094;
						}
						HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartRxBufferAll, strlen(uartRxBufferAll));

						/* TEXTEINGABE **/
					 	for (int i = 0; i< MAXLENGTHTEXT ; i++)
					 	{
					 		DataObj_MC1_3.c[i] = 0;
					 	}
					 	UART_Wait_Ready(UARTTIMEOUT);
						sprintf(uartTxBuffer,"\rEingabe Datenobjekt 3: Text. (Max Laenge Zeichen = %d)\r",MAXLENGTHTEXT);
						HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));
						UART_scanChars(MAXLENGTHTEXT);
						strcpy(DataObj_MC1_3.c,uartRxBufferAll);

						HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartRxBufferAll, strlen(uartRxBufferAll));

				 }break;
		 }
		 HAL_UART_Transmit_Presi(&huart2,"\rErfolgreiche Eingabe.\r", strlen("Erfolgreiche Eingabe.\r"));
	 }

 /******************************************************************************
	*
	*/
	void UART_scanChars(uint16_t numbchars)
	{
		UARTnumbMsg = 0;

	 while (UARTnumbMsg < numbchars)
	 {
		 HAL_Delay(100);
		 if (uartRxBufferAll[UARTnumbMsg-1] == '\r')
		 {
			 uartRxBufferAll[UARTnumbMsg-1] = '\0';
			 break;
		 }
	 }

	 uartRxBufferAll[numbchars]= '\0';


//	 for (int i = 0; i < numbchars; i++)
//	 {
//		 uartRxBufferAll[i]
//	 }
	}

/******************************************************************************
 *
 */
 void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
 {

 }



 /******************************************************************************
  *
  */
 void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
 {

	 uartRxBufferAll[UARTnumbMsg] = *uartRxBuffer;

	 if (LockUARTrx == 0)
	 {
		 LockUARTrx = 1;
		 uartRxMenu = *uartRxBuffer;
	 }

	 if (UARTnumbMsg <= (UARTMAXBUFF-3))
	 {
		 UARTnumbMsg++;
	 }
	 else
	 {
		 UARTnumbMsg = 0;
	 }

	 if (HAL_UART_Receive_IT(&huart2,uartRxBuffer,(uint16_t)1) != HAL_OK)
	 {
		 HAL_UART_Transmit_Presi(&huart2,"error HAL_UART_Receive_IT",strlen("error HAL_UART_Receive_IT"));
	 }
 }

 /******************************************************************************
  *
  */
 void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
 {
 }


 /******************************************************************************
  *
  */
 void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
 {

 }

 void presi_Error_Handler()
 {

   while(1)
   {
 		switch(hal_status)
 		{
 				case HAL_ERROR: {
 						UART_Wait_Ready(UARTTIMEOUT);
 						sprintf(uartTxBuffer,"HAL_StatusTypeDef == HAL_ERROR\r"); break;
 				}
 				case HAL_BUSY: {
 						UART_Wait_Ready(UARTTIMEOUT);
 						sprintf(uartTxBuffer,"HAL_StatusTypeDef == HAL_BUSY\r"); break;
 				}
 				case HAL_TIMEOUT: {
 					UART_Wait_Ready(UARTTIMEOUT);
 					sprintf(uartTxBuffer,"HAL_StatusTypeDef == HAL_TIMEOUT\r"); break;
 				}
 		}
 		HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));


   	HAL_Delay(3000);
   	UART_Wait_Ready(UARTTIMEOUT);
   	sprintf(uartTxBuffer,"Neustart der Applikation \r");
 		HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));

   	main();
   }
 }

 void cp_errorhandler()
 {
	  UART_Wait_Ready(UARTTIMEOUT);
	  sprintf(uartTxBuffer,"Ein Fehler ist aufgetreten. Fehlercode: %d \r",CP_LastErrorCodes[0]);
		HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));

		UART_Wait_Ready(UARTTIMEOUT);
		sprintf(uartTxBuffer,"Fehlerbeschreibung: ");
		HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));

		UART_Wait_Ready(UARTTIMEOUT);
		sprintf(uartTxBuffer,CP_ERROR_DESCRIPTION[CP_LastErrorCodes[0]]);
		HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));
 }

 void cp_printAllErrors()
 {
	  UART_Wait_Ready(UARTTIMEOUT);
	  sprintf(uartTxBuffer,"Fehlerauflistung, beginnend mit dem Neuesten: \r\r");
		HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));

	 for (int i = 0; i<3; i++)
	 {
		  UART_Wait_Ready(UARTTIMEOUT);
		  sprintf(uartTxBuffer,"%d. Fehlercode: %d => ",i+1,CP_LastErrorCodes[i]);
			HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));

			UART_Wait_Ready(UARTTIMEOUT);
			sprintf(uartTxBuffer,CP_ERROR_DESCRIPTION[CP_LastErrorCodes[i]]);
			HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));
	 }
 }

/******************************************************************************/
/** BUTTON FUNCTIONS BUTTON BUTTON BUTTON BUTTON BUTTON BUTTON BUTTON BUTTON  */
/******************************************************************************/
   void buttons_init()
   {
   	button0 = 0;
   	button1 = 0;
   	buttonIR = 0;
   }

   void buttons_read()
   {
  	 /** IF Positive Edge BUTTON 0 */
  	 button0 = HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0);

  	 if ( (button0 == 0) & (button0Last == 1) )
  	 {
  		 button0Last = button0;
  		 button0 = 1;
  	 }
  	 else
  	 {
  		 button0Last = button0;
  		 button0 = 0;
  	 }

  	 /** IF Positive Edge BUTTON 1 */
  	 button1 = HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_1);

  	 if ( (button1 == 0) & (button1Last == 1) )
  	 {
  		 button1Last = button1;
  		 button1 = 1;
  	 }
  	 else
  	 {
  		 button1Last = button1;
  		 button1 = 0;
  	 }
   }

   void button_printstates()
   {
  	 UART_Wait_Ready(UARTTIMEOUT);
  	 sprintf(uartTxBuffer,"Button0: %d, Button1: %d \r",button0,button1);
  	 HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));
   }


   void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
   {

  	if ((HAL_GetTick() - tickStartButtonIR) > TCYCLE_BUTTONIR)
  	{
  		tickStartButtonIR = HAL_GetTick();
     	buttonIR = 1;
  	}
   }





void presi_main()
{
//	presi_Init();

  while ((button0== 0) & (button1 == 0))
  {
  	buttons_read();
  }

  while (1)
  {
 		buttons_read();

		if ((button0== 1) & (button1 == 0) )
  	if ((button0== 0) & (button1 == 1) )
		if ((button0== 1) & (button1 == 1) )
		if (buttonIR == 1)
		{
			UART_Wait_Ready(UARTTIMEOUT);
			sprintf(uartTxBuffer,"ButtonIR: %d \r",buttonIR);
			HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));
			buttonIR = 0;
			uartRxMenu = 0;
		}


    for (int i = 0; i< NUMBDATAOBJMAX; i++ )
    {
  		if (CPcontrolFieldsRX.arrStatus[i] == CP_ERROR)
  		{
  			HAL_UART_Transmit_Presi(&huart2,(uint8_t *) CP_ERROR_DESCRIPTION[CP_LastErrorCodes[0]], strlen(CP_ERROR_DESCRIPTION[CP_LastErrorCodes[0]]));
  			break;
  		}
    }

		if (CPcontrolFieldsTX.arrStatus == CP_ERROR)
		{
			HAL_UART_Transmit_Presi(&huart2,(uint8_t *) CP_ERROR_DESCRIPTION[CP_LastErrorCodes[0]], strlen(CP_ERROR_DESCRIPTION[CP_LastErrorCodes[0]]));
		}



		UART_Wait_Ready(UARTTIMEOUT);
		sprintf(uartTxBuffer,"--------------------------------\r");
		HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));
		HAL_Delay(500);
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
  }
}

void presi_main_uart()
{
	presi_Init();
	HAL_UART_Receive_IT(&huart2,uartRxBuffer,(uint16_t)1);
	uartRxMenu = 'M';

	tickStartADC = HAL_GetTick();
	tickStartPWM = HAL_GetTick();
	tickStartRx = HAL_GetTick();
	while(1)
	{
		buttons_read();

		/** BUTTON STEUERUNG */

		if (button0 == 1)
		{
	 	  if (HAL_UART_Init(&huart2) != HAL_OK)
	 	  {
	 	    Error_Handler();
	 	  }

	 	  HAL_UART_Transmit_Presi(&huart2,uartTxMenu,strlen(uartTxMenu));
			LockUARTrx = 0;
			HAL_UART_RxCpltCallback(&huart2);

	 	  if (HAL_CAN_Init(&hcan) != HAL_OK)
	 	  {
	 	    Error_Handler();
	 	  }
	 	  HAL_Delay(1000);
	 	  HAL_CAN_Receive_IT(&hcan, CAN_FIFO0);
	 	  HAL_Delay(500);
	 	  HAL_CAN_Transmit_IT(&hcan);


		}

		if (button1 == 1)
		{
		}

		if (buttonIR == 1)
		{
				if (DataObj_MC1_3.button_state == 0)
				{
					DataObj_MC1_3.button_state = 1;
				}
				else
				{
					DataObj_MC1_3.button_state = 0;
				}

				buttonIR = 0;

		}


		/** UART STEUERUNG */

		switch(uartRxMenu)
	  {
	  	case 'm':
	    case 'M':
				{
					UART_PrintMainMenu();
					LockUARTrx = 0;
					uartRxMenu = '$';
				}break;

	    case '-':
				{
					UART_printDataObj();
					LockUARTrx = 0;
					uartRxMenu = '$';
				}break;

	    case '+':
				{
					UART_changeDataObj();
					LockUARTrx = 0;
					uartRxMenu = '$';
				}break;

			/* CAN TRANSMIT PROCESS **********************************************/
	    case '1':
				{
					/** CP_InitTx() */
					IDtmp = scanID();		// UART Scan ID-Selection
					scanDataObj();			// UART Scan Dataobj-Selection

//					UART_PrintMainMenu();

					if (CP_InitTx(DataObjSelectPtr,DataObjSelectSize,IDtmp) != CP_OK)
					{
						cp_errorhandler();
					}
					else
					{
						UART_Wait_Ready(UARTTIMEOUT);
						sprintf(uartTxBuffer,"Sende-ID %x initialisiert.\r",IDtmp);
						HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));
					}
					uartRxMenu = '$';
				}break;
	    case '2':
				{
					/** CP_StartTx() */
					if ( CP_StartTx() != CP_OK)
					{
						cp_errorhandler();
					}
					else
					{
						UART_Wait_Ready(UARTTIMEOUT);
						sprintf(uartTxBuffer,"Senden von Daten fuer ID %x gestartet.\r",IDtmp);
						HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));
					}
					uartRxMenu = '$';
				}break;
	    case '3':
				{
					/** CP_StatusTx() */
					if ((cp_status = CP_StatusTx()) == CP_OK)
					{
						UART_Wait_Ready(UARTTIMEOUT);
						sprintf(uartTxBuffer,"Status: Datenobjekt kann versendet werden.\r");
						HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));
					}
					else if (cp_status == CP_BUSY)
					{
						UART_Wait_Ready(UARTTIMEOUT);
						sprintf(uartTxBuffer,"Status: Datenobjekt wird im Moment versendet.\r");
						HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));
					}
					else if (cp_status == CP_ERROR)
					{
						UART_Wait_Ready(UARTTIMEOUT);
						sprintf(uartTxBuffer,"Status: Fehler bei Sendevorgang.\r");
						HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));
					}
					uartRxMenu = '$';
				}break;




			/* CAN RECEIVE PROCESS **********************************************/
	    case '4':
				{
					/** CP_InitRx() */
					IDtmp = scanID();		// UART Scan ID-Selection
					scanDataObj();			// UART Scan Dataobj-Selection

//					UART_PrintMainMenu();

					if ( CP_InitRx(DataObjSelectPtr,DataObjSelectSize,IDtmp) != CP_OK)
					{
						cp_errorhandler();
					}
					else
					{
						UART_Wait_Ready(UARTTIMEOUT);
						sprintf(uartTxBuffer,"Empfangs-Id %x initialisiert.\r",IDtmp);
						HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));
					}
					uartRxMenu = '$';
				}break;
	    case '5':
				{
					/** CP_StartRx() */
					IDtmp = scanID();		// UART Scan ID-Selection

//					UART_PrintMainMenu();
					if (CP_StartRx(IDtmp) != CP_OK)
					{
						cp_errorhandler();
					}
					else
					{
						UART_Wait_Ready(UARTTIMEOUT);
						sprintf(uartTxBuffer,"Empfang fuer Id %x gestartet.\r",IDtmp);
						HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));
					}
					uartRxMenu = '$';
				}break;
	    case '6':
				{
					/** CP_StatusRx() */
					IDtmp = scanID();		// UART Scan ID-Selection

//					UART_PrintMainMenu();
					if ((cp_status = CP_StatusRx(IDtmp)) == CP_OK)
					{
						UART_Wait_Ready(UARTTIMEOUT);
						sprintf(uartTxBuffer,"Status: Empfang fuer ID %x ist initialisiert.\r",IDtmp);
						HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));
					}
					else if (cp_status == CP_BUSY)
					{
						UART_Wait_Ready(UARTTIMEOUT);
						sprintf(uartTxBuffer,"Status: Empfangsmodus fuer ID %x ist gestartet.\r",IDtmp);
						HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));
					}
					else if (cp_status == CP_ERROR)
					{
						UART_Wait_Ready(UARTTIMEOUT);
						sprintf(uartTxBuffer,"Status: Fehler bei Empfang fuer ID %x.\r",IDtmp);
						HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));
					}
					uartRxMenu = '$';
				}break;
	    case '7':
				{
					/** CP_DeleteRx() */
					IDtmp = scanID();		// UART Scan ID-Selection

//					UART_PrintMainMenu();
					if (CP_DeleteRx(IDtmp) != CP_OK)
					{
						cp_errorhandler();
					}
					else
					{
//						UART_PrintMainMenu();
						UART_Wait_Ready(UARTTIMEOUT);
						sprintf(uartTxBuffer,"Empfangs ID %x entfernt.\r",IDtmp);
						HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));
					}
					uartRxMenu = '$';
				}break;

				/* Zyklisch Senden **************************************************************/
	    case '8':
				{
					/* Starte / Stoppe zyklischen Versand von Datenobjekt **/
					if (selectTxZykl == 0)
					{
						selectTxZykl = 1;
						uartRxMenu = '1';
						tickStartTx = HAL_GetTick();
					}
					else
					{
						selectTxZykl = 0;
						uartRxMenu = 'm';
						UART_Wait_Ready(UARTTIMEOUT);
						sprintf(uartTxBuffer,"Zyklisches Senden beendet.\r");
						HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));
					}

				}break;

				/* Zyklisch Empfangen **************************************************************/
	    case '9':
				{
					/* Starte / Stoppe zyklischen Empfang von Datenobjekt **/
					if (selectRxZykl == 0)
					{
						selectRxZykl = 1;
						tickStartRx = HAL_GetTick();
						/** CP_InitRx() */
						IDzykl = scanID();		// UART Scan ID-Selection
						scanDataObj();			// UART Scan Dataobj-Selection

						if ( CP_InitRx(DataObjSelectPtr,DataObjSelectSize,IDzykl) != CP_OK)
						{
							cp_errorhandler();
						}
						else
						{
//							UART_PrintMainMenu();
							UART_Wait_Ready(UARTTIMEOUT);
							sprintf(uartTxBuffer,"Zykl. Empfangs-ID %x initialisiert.\r",IDtmp);
							HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));
						}
					}
					else
					{
						selectRxZykl = 0;
						/** CP_DeleteRx() */
						if (CP_DeleteRx(IDzykl) != CP_OK)
						{
							cp_errorhandler();
						}
						else
						{
							UART_Wait_Ready(UARTTIMEOUT);
							sprintf(uartTxBuffer,"Zykl. Empfangs-ID %x entfernt.\r",IDzykl);
							HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));
						}
					}
					uartRxMenu = '$';
				}break;

				/* MC 3 /INIT / START / STOP ********************************************************/
	    case '0':
				{
					uint32_t IDtmpOld = IDtmp;
					uint32_t* DataObjSelectPtrOld = DataObjSelectPtr;
					uint32_t	DataObjSelectSizeOld = DataObjSelectSize;

					DataObj_MC1_1.D1 = scanRemote();

					IDtmp = 0x111;
					DataObjSelectPtr = &DataObj_MC1_1;
					DataObjSelectSize = sizeof(DataObj_MC1_1);

					CP_InitTx(DataObjSelectPtr,DataObjSelectSize,IDtmp);
					CP_StartTx();

					IDtmp = IDtmpOld;
					DataObjSelectPtr = DataObjSelectPtrOld ;
					DataObjSelectSize = DataObjSelectSizeOld ;
					CP_InitTx(DataObjSelectPtr,DataObjSelectSize,IDtmp);

					UART_Wait_Ready(UARTTIMEOUT);
					sprintf(uartTxBuffer,"Aktion wurde durchgefuehrt.\r",IDtmp);
					HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));


					uartRxMenu = '$';
				}break;

				/* AUSWAHL MC Mikrocontroller ********************************************************/
	    case 'a':
	    case 'A':
				{
					scanMC();

//					UART_PrintMainMenu();
					UART_Wait_Ready(UARTTIMEOUT);
					sprintf(uartTxBuffer,"Mikrocontrollerauswahl: = %s \r",MC);
					HAL_UART_Transmit_Presi(&huart2,(uint8_t *) uartTxBuffer, strlen(uartTxBuffer));

				}break;

				/* SHOW ERRORS *********************************************************************/
		    case 'f':
		    case 'F':
					{
						/** Show Errors */
	//					UART_PrintMainMenu();
						cp_printAllErrors();
						uartRxMenu = '$';
					}break;

	    default:
				{
					LockUARTrx = 0;
					uartRxMenu = '$';
				}

	  }
		LockUARTrx = 0;

		/* Zyklischer Versand von CAN-Datenobjekt ******************************************/
		if ( (selectTxZykl == 1) & ((HAL_GetTick() - tickStartTx) > TCYCLE_TX) )
		{
			/* Sende Nachricht, sofern letzte Nachricht fertig versendet wurde **/
			if (CP_StatusTx() == CP_OK)
			{
				/** CP_StartTx() */
				if ( CP_StartTx() != CP_OK)
				{
					cp_errorhandler();
				}
				else
				{
					tickStartTx = HAL_GetTick();
				}
			}
		}

		/* Zyklischer Empfang von CAN-Datenobjekt *****************************************/
		if ( (selectRxZykl == 1) & ((HAL_GetTick() - tickStartRx) > TCYCLE_RX) )
		{
			/* Empfange Nachricht, sofern letzte Nachricht fertig empfangen wurde **/
			if (CP_StatusRx(IDzykl) == CP_OK)
			{
				/** CP_StartTx() */
				if ( CP_StartRx(IDzykl) != CP_OK)
				{
					cp_errorhandler();
				}
				else
				{
					tickStartRx = HAL_GetTick();
				}
			}
		}

	  /** MAIN TASK MC 1 ************************************************************/

		if (MC == "MC1")
		{
			/* Get New ADCVal *******************************************************/
			if ( (HAL_GetTick() - tickStartADC) > TCYCLE_MC1)
			{

				HAL_ADC_Start(&hadc1);
				DataObj_MC1_3.adcval = HAL_ADC_GetValue(&hadc1);

				tickStartADC = HAL_GetTick();

				DataObj_MC1_3.sin_val+=20;
				if (DataObj_MC1_3.sin_val >= MAXADCVAL)
				{
					DataObj_MC1_3.sin_val = 0;
				}

			}

		}



	  /** MAIN TASKS MC 2 ************************************************************/

		if (MC == "MC2")
		{
		}

	  /** MAIN TASKS MC 3 ************************************************************/


			// PA_6 / A5	- Rechts mitte (LED Blink) - CP_STATUS
			if ( (MC == "MC3") & ( (HAL_GetTick() - tickStartPWM) > TCYCLE_MC3))
			{
				if (  (CP_StatusRx(0x333) == CP_OK) )
				{
//					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
					pwm = htim2.Init.Period;
					__HAL_TIM_SetCompare(&htim2,TIM_CHANNEL_3,pwm);		// PA_2 / A7	- Rechts oben
					cpState = 0;
				}
				if ( (CP_StatusRx(0x333) == CP_BUSY) )
				{
//					HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
					pwm = htim2.Init.Period/2;
					__HAL_TIM_SetCompare(&htim2,TIM_CHANNEL_3,pwm);		// PA_2 / A7	- Rechts oben
					cpState = 1;
				}
				if ( (CP_StatusRx(0x222) == CP_ERROR) | (CP_StatusRx(0x333) == CP_ERROR) )
				{
//					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
					pwm = 2;
					__HAL_TIM_SetCompare(&htim2,TIM_CHANNEL_3,pwm);		// PA_2 / A7	- Rechts oben
					cpState = 2;
				}


				/* INIT ****************************************************************************/
				IDzykl = 0x111;
				if (CP_StatusRx(IDzykl) == CP_ERROR)
				{
					/** CP_StartTx() */
					DataObjSelectPtr = &DataObj_MC1_1;
					DataObjSelectSize = sizeof(DataObj_MC1_1);
					if ( CP_InitRx(DataObjSelectPtr,DataObjSelectSize,IDzykl) != CP_OK)
					{
						cp_errorhandler();
					}
				}
				if (DataObj_MC1_1.D1 == '4')
				{
					DataObj_MC1_1.D1 = '$';

					IDzykl = 0x222;
					DataObjSelectPtr = &DataObj_MC1_2;
					DataObjSelectSize = sizeof(DataObj_MC1_2);
					if ( CP_InitRx(DataObjSelectPtr,DataObjSelectSize,IDzykl) != CP_OK)
					{
						cp_errorhandler();
					}

					IDzykl = 0x333;
					DataObjSelectPtr = &DataObj_MC1_3;
					DataObjSelectSize = sizeof(DataObj_MC1_3);
					if ( CP_InitRx(DataObjSelectPtr,DataObjSelectSize,IDzykl) != CP_OK)
					{
						cp_errorhandler();
					}

				}

				/* Zyklischer Empfang von CAN-Datenobjekt *****************************************/
				IDzykl = 0x111;
				if (CP_StatusRx(IDzykl) == CP_OK)
				{
					/** CP_StartTx() */
					if ( CP_StartRx(IDzykl) != CP_OK)
					{
						cp_errorhandler();
					}
				}

				if ( (DataObj_MC1_1.D1 == '9') | (DataObj_MC1_1.D1 == '5') )
				{
					if (DataObj_MC1_1.D1 == '5')
					{
						DataObj_MC1_1.D1 = '$';
					}

					IDzykl = 0x222;
					if (CP_StatusRx(IDzykl) == CP_OK)
					{
							/** CP_StartTx() */
							if ( CP_StartRx(IDzykl) != CP_OK)
							{
								cp_errorhandler();
							}
						}
						IDzykl = 0x333;
						if (CP_StatusRx(IDzykl) == CP_OK)
						{
							/** CP_StartTx() */
							if ( CP_StartRx(IDzykl) != CP_OK)
							{
								cp_errorhandler();
							}
						}
				}

				/* Set New PWMVal *******************************************************/

				if(DataObj_MC1_3.button_state == 0)
				{
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
					pwm = 200 * DataObj_MC1_3.manual / MAXADCVAL;
					__HAL_TIM_SetCompare(&htim3,TIM_CHANNEL_2,pwm);		// PB_5 / D11	- Links unten (2)
				}
				else
				{
					HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
					pwm = 200 * DataObj_MC1_3.sin_val / MAXADCVAL;
					__HAL_TIM_SetCompare(&htim3,TIM_CHANNEL_2,pwm);		// PB_5 / D11	- Links unten (2)
				}


				pwm = 200 * DataObj_MC1_3.adcval / MAXADCVAL;
				__HAL_TIM_SetCompare(&htim3,TIM_CHANNEL_1,pwm);		// PB_4 / D12 - Links unten

				tickStartRx = HAL_GetTick();
			}
		}

	}
