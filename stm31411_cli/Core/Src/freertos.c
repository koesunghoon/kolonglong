/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 2048 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for myTaskLed */
osThreadId_t myTaskLedHandle;
const osThreadAttr_t myTaskLed_attributes = {
  .name = "myTaskLed",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for myTaskTemp */
osThreadId_t myTaskTempHandle;
const osThreadAttr_t myTaskTemp_attributes = {
  .name = "myTaskTemp",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for myTaskMonitor */
osThreadId_t myTaskMonitorHandle;
const osThreadAttr_t myTaskMonitor_attributes = {
  .name = "myTaskMonitor",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for myTaskArm */
osThreadId_t myTaskArmHandle;
const osThreadAttr_t myTaskArm_attributes = {
  .name = "myTaskArm",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void ledSystemTask(void *argument);
void tempSystemTask(void *argument);
void monitorSystemTask(void *argument);
void armSystemTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of myTaskLed */
  myTaskLedHandle = osThreadNew(ledSystemTask, NULL, &myTaskLed_attributes);

  /* creation of myTaskTemp */
  myTaskTempHandle = osThreadNew(tempSystemTask, NULL, &myTaskTemp_attributes);

  /* creation of myTaskMonitor */
  myTaskMonitorHandle = osThreadNew(monitorSystemTask, NULL, &myTaskMonitor_attributes);

  /* creation of myTaskArm */
  myTaskArmHandle = osThreadNew(armSystemTask, NULL, &myTaskArm_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
__weak void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */

  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_ledSystemTask */
/**
* @brief Function implementing the myTask02 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_ledSystemTask */
__weak void ledSystemTask(void *argument)
{
  /* USER CODE BEGIN ledSystemTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END ledSystemTask */
}

/* USER CODE BEGIN Header_tempSystemTask */
/**
* @brief Function implementing the myTask03 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_tempSystemTask */
__weak void tempSystemTask(void *argument)
{
  /* USER CODE BEGIN tempSystemTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END tempSystemTask */
}

/* USER CODE BEGIN Header_monitorSystemTask */
/**
* @brief Function implementing the myTaskMonitor thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_monitorSystemTask */
__weak void monitorSystemTask(void *argument)
{
  /* USER CODE BEGIN monitorSystemTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END monitorSystemTask */
}

/* USER CODE BEGIN Header_armSystemTask */
/**
* @brief Function implementing the myTaskArm thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_armSystemTask */
__weak void armSystemTask(void *argument)
{
  /* USER CODE BEGIN armSystemTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END armSystemTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

