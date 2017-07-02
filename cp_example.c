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
 * dienen. Dabei schreibt und sendet Mikrocontroller 1 das 
 * „Datenobjekt_1“ #DataObj_MC1 und Mikrocontroller 2 schreibt 
 * und sendet das „Datenobjekt_2“ #DataObj_MC2. 
 * Die Auswahl, #MC_StatusTypeDef,
 * welcher Mikrocontroller als 1 oder 2 (nachfolgende MC1 / MC2 genannt) 
 * arbeitet, wird entschieden, indem zu Beginn des Programm-Ablaufs der 
 * digitale Eingang geprueft wird. Ist der digitale Eingang bei Start des 
 * Mikrocontrollers auf „High“ gesetzt, fungiert dieser Mikrocontroller als MC1. 
 * Ist der Eingang auf „Low“ gesetzt, wird der Mikrocontroller automatisch 
 * zu MC2.
 * Das Ziel ist, dass sich die Mikrocontroller einmal in der Sekunde 
 * gegenseitig ueber den CAN-Bus ein Steuer-Signal fuer den  digitalen 
 * Ausgang schicken. Je nach Nachrichteninhalt setzt der Mikrocontroller 
 * nach dem Empfang der Botschaft seinen digitalen Ausgang auf High 
 * oder Low. Welches Steuersignal verschickt wird, haengt davon ab, ob 
 * der digitale Eingang durch den Anwender auf High oder Low gesetzt wird.
 * Das heisst, der digitale Eingang von MC1 schaltet den digitalen Ausgang 
 * auf MC2 (und somit die LED auf MC2). Der digitale Eingang von MC2 
 * setzt die LED auf MC1. 
 *
 */

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* INCLUDES */
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

#include "cp_user.h"
#include "cp_control.h"




/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* DEFINES */
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

typedef enum
{
	MC1      	= 0,			 /**< 0, Auswahl Mikrocontroller 1*/
	MC2			= 1,			 /**< 1, Auswahl Mikrocontroller 2*/
}MC_StatusTypeDef;


/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* DEFINITION OF VARIABLES */
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/** Variable fuer Auswahl Mikrocontroller 1 oder 2 */
MC_StatusTypeDef MC = MC1;	


/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
/* FUNCTIONS */
/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

/*__________________________________________________________________________*/
/**
 * Beispielprogamm zur Nutzung des CP-Protokolls
 *
 *
 *
 */
void CP_example()
{
	MC = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4);

	if (MC == MC1)
	{
		/* Inititalisiere Sendeprozess */
		CP_InitTx(&DataObj_MC1_1,sizeof(DataObj_MC1_1),0x001);
	}



	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);



	/* Receive Functions */
	CP_InitRx(&DataObj_MC1_1, sizeof(DataObj_MC1_1), 0x001);
	CP_StartRx(0x001);
	CP_StatusRx(0x001);
	CP_DeleteRx(0x001);

	/* Transmit Functions */
	CP_InitTx(&DataObj_MC1_1, sizeof(DataObj_MC1_1), 0x001);
	CP_StartTx();
	CP_StatusTx();


}
/** @} */


/**
 * @defgroup Weitere Weitere
 * @{
 *  Weitere Module
 *
 *  Weitere Module ist nur eine Gruppe für die Dokumentatation in Doxygen 
 *  für definierte Module
 *
 */
 
 /** @} */