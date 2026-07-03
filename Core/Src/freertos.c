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
#include "app_main.h"

#include "usart.h"
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
/* Definitions for Main_tasks */
osThreadId_t Main_tasksHandle;
const osThreadAttr_t Main_tasks_attributes = {
  .name = "Main_tasks",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for Motor_angle_get */
osThreadId_t Motor_angle_getHandle;
const osThreadAttr_t Motor_angle_get_attributes = {
  .name = "Motor_angle_get",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for chassis_control */
osThreadId_t chassis_controlHandle;
const osThreadAttr_t chassis_control_attributes = {
  .name = "chassis_control",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for queue */
osMessageQueueId_t queueHandle;
const osMessageQueueAttr_t queue_attributes = {
  .name = "queue"
};
/* Definitions for motor */
osMutexId_t motorHandle;
const osMutexAttr_t motor_attributes = {
  .name = "motor"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void Main_task(void *argument);
void Motor_angle_get_task(void *argument);
void chassis_control_task(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of motor */
  motorHandle = osMutexNew(&motor_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of queue */
  queueHandle = osMessageQueueNew (16, sizeof(int64_t), &queue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of Main_tasks */
  Main_tasksHandle = osThreadNew(Main_task, NULL, &Main_tasks_attributes);

  /* creation of Motor_angle_get */
  Motor_angle_getHandle = osThreadNew(Motor_angle_get_task, NULL, &Motor_angle_get_attributes);

  /* creation of chassis_control */
  chassis_controlHandle = osThreadNew(chassis_control_task, NULL, &chassis_control_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_Main_task */
/**
  * @brief  Function implementing the Main_tasks thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_Main_task */
void Main_task(void *argument)
{
  /* USER CODE BEGIN Main_task */
  /* 调度器已运行，组装并启动各子系统（电机 + 陀螺仪），仅一次 */
  uint8_t cmd[5] = {0xFF, 0xAA, 0x69, 0x88, 0xB5};
	HAL_UART_Transmit(&huart2, cmd, 5,100);
	osDelay(210);
	cmd[2] = 0x76;
	cmd[3] = 0x00;
	cmd[4] = 0x00;
	HAL_UART_Transmit(&huart2, cmd, 5,100);
	cmd[2] = 0x00;
	osDelay(510);
	HAL_UART_Transmit(&huart2, cmd, 5,100);
  (void)app_init();
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Main_task */
}

/* USER CODE BEGIN Header_Motor_angle_get_task */
/**
* @brief Function implementing the Motor_angle_get thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Motor_angle_get_task */
void Motor_angle_get_task(void *argument)
{
  /* USER CODE BEGIN Motor_angle_get_task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Motor_angle_get_task */
}

/* USER CODE BEGIN Header_chassis_control_task */
/**
* @brief Function implementing the chassis_control thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_chassis_control_task */
void chassis_control_task(void *argument)
{
  /* USER CODE BEGIN chassis_control_task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END chassis_control_task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

