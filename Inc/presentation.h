
#ifndef __presentation_H
#define __presentation_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "cp_user.h"



/******************************************************************************/
/** DEFINES */
/******************************************************************************/

#define UARTDEBUG

#define M_PI 3.14159265358979323846

/******************************************************************************/
/** VARIABLES */
/******************************************************************************/

 /* main main main main main main main main main main main -------------------*/
 extern HAL_StatusTypeDef hal_status;
 extern CP_StatusTypeDef	cp_status;


 /* ADC - DAC - ADC - DAC - ADC - DAC - ADC - DAC - ADC - DAC ----------------*/
 extern uint32_t adcval;
 extern uint32_t *adcmemory;


 /* UART UART UART UART UART UART UART UART UART -----------------------------*/
 	extern char TxBufferUART[240];
  extern const char *uartTxMenu;

 /* BUTTONS BUTTONS BUTTONS BUTTONS BUTTONS BUTTONS --------------------------*/

 	extern uint8_t button0;
 	extern uint8_t button1;
 	extern uint8_t buttonIR;

/******************************************************************************/
/** FUNCTION REFERENCE */
/******************************************************************************/

 /* MAIN - MAIN - MAIN - MAIN - MAIN - MAIN - MAIN  --------------------------*/
 	extern void presi_main();
 	extern void presi_main_uart();
  void presi_Error_Handler();
  void cp_errorhandler();

 /* CP  -  CP  -  CP  -  CP  -  CP  -  CP  -  CP  -  CP  ---------------------*/
 	extern void cptest_setDataObjects();






 /* UART UART UART UART UART UART UART UART UART -----------------------------*/
 void uart_init_mainmenu();

 extern void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
 extern void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
 extern void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);
 extern void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);
 
 
 /* BUTTONS BUTTONS BUTTONS BUTTONS BUTTONS BUTTONS --------------------------*/
 extern void buttons_init();
 extern void buttons_read();
 extern void button_printstates();
 extern void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

 
 
 
 
 
#ifdef __cplusplus
}
#endif
#endif /*__ presentation_H */
 
 
