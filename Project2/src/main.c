/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2014 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "diag/trace.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"

#define CCM_RAM __attribute__((section(".ccmram")))

// ----------------------------------Definitions--------------------------------------
#define Queue_Size          3

#define Max_Messages        1000

#define Num_Senders         3

#define T_Receiver_ms       100


#define Task_1_Id			1
#define Task_2_Id			2
#define Task_3_Id			3

#define LONG_TIME           0xffff

/* task priorities */
#define SENDER_1_PRIORITY   1
#define SENDER_2_PRIORITY   1
#define SENDER_3_PRIORITY   2
#define RECEIVER_PRIORITY   3

// ----------------------------------Tasks & Queues Handles--------------------------------------
TaskHandle_t Sender1;
TaskHandle_t Sender2;
TaskHandle_t Sender3;
TaskHandle_t Receiver;


QueueHandle_t messageQueue;

// ----------------------------------Counters------------------------------------------


uint32_t Total_Transmitted_Messages = 0;
uint32_t Total_Blocked_Messages = 0;
uint32_t Total_Received_Messages = 0;

// ----------------------------------Counters Per Sender------------------------------------------

uint32_t Transmitted_Messages_Per_Sender[Num_Senders] = {0};
uint32_t Blocked_Messages_Per_Sender[Num_Senders] = {0};

/*-------------------------------Timers------------------------------------------------------*/
TimerHandle_t xTimer1;
TimerHandle_t xTimer2;
TimerHandle_t xTimer3;
TimerHandle_t xTimer4; /* timer 4 is for the receiver task */


/*----------------------------------Periods for Senders------------------------------------=*/
uint32_t 		Sender_1_Period=0;
uint32_t		Sender_2_Period=0;
uint32_t 		Sender_3_Period=0;


TickType_t	T1avg=0;
TickType_t	T2avg=0;
TickType_t	T3avg=0;
/*---------------------------------Semaphore handles------------------------------------------------------*/

SemaphoreHandle_t Sem_sender_Task_1;
SemaphoreHandle_t Sem_sender_Task_2;
SemaphoreHandle_t Sem_sender_Task_3;
SemaphoreHandle_t Sem_receiver_Task;



/*---------------------------------Upper&Lower Bounds------------------------------------------------------*/

uint32_t  Lower_Bound []= {50,80,110,140,170,200};
uint32_t  Upper_Bound []= {150,200,250,300,350,400};



uint32_t iteration =0;


uint32_t Random_Time(int TaskId)
{

	srand(time(NULL)*(TaskId<<rand()%10)); /*seed different value for each task
											to create different random Numbers at the same time*/

	return (rand()%(Upper_Bound[iteration]-Lower_Bound[iteration]+1)+Lower_Bound[iteration]);
}

/*Print the total transmitted,blocked,received and statistics per sender*/
void Print_Data(void)
{
	T1avg=T1avg/(Transmitted_Messages_Per_Sender[0]+Blocked_Messages_Per_Sender[0]);
	T2avg=T2avg/(Transmitted_Messages_Per_Sender[1]+Blocked_Messages_Per_Sender[1]);
	T3avg=T3avg/(Transmitted_Messages_Per_Sender[2]+Blocked_Messages_Per_Sender[2]);
	printf("Total Transmitted Data: %lu\n",Total_Transmitted_Messages);
	printf("Total Blocked Data    : %lu\n",Total_Blocked_Messages);
	printf("Total Received Data   : %lu\n",Total_Received_Messages);

	printf("\t Sender 1\nTransmitted : %lu\nBlocked     : %lu \nAverage Time: %lu \n",Transmitted_Messages_Per_Sender[0],Blocked_Messages_Per_Sender[0],T1avg);
	printf("\t Sender 2\nTransmitted : %lu\nBlocked     : %lu \nAverage Time: %lu \n",Transmitted_Messages_Per_Sender[1],Blocked_Messages_Per_Sender[1],T2avg);
	printf("\t Sender 3(Higher Priority)\nTransmitted : %lu\nBlocked     : %lu \nAverage Time: %lu \n",Transmitted_Messages_Per_Sender[2],Blocked_Messages_Per_Sender[2],T3avg);
}

/*--------------------Reset the counters and the queue------------------------*/
void Reset_Data(void)
{
	Total_Transmitted_Messages = 0;
	Total_Blocked_Messages = 0;
	Total_Received_Messages = 0;
	for(int i=0; i<3; i++)
	{
		Transmitted_Messages_Per_Sender[i] = 0;
		Blocked_Messages_Per_Sender[i] = 0;


	}
	xQueueReset(messageQueue);
}

/*Delete everything and stop the program	*/
void The_End(void)
{

			vTaskSuspendAll();
			vTaskDelete(Sender1);
			vTaskDelete(Sender2);
			vTaskDelete(Sender3);
			vTaskDelete(Receiver);
			vQueueDelete(messageQueue);
			xTimerDelete(xTimer1,0);
			xTimerDelete(xTimer2,0);
			xTimerDelete(xTimer3,0);
			xTimerDelete(xTimer4,0);
			vSemaphoreDelete(Sem_sender_Task_1);
			vSemaphoreDelete(Sem_sender_Task_2);
			vSemaphoreDelete(Sem_sender_Task_3);
			vSemaphoreDelete(Sem_receiver_Task);
			xTaskResumeAll();
			printf("\tGame Over!\n");


			exit(0);
}

void Reset(void)
{
	if(Total_Received_Messages==0)
	{
		Reset_Data();
		iteration = 0;
		printf("-------------------------------------iteration:%d------------------------------\n",iteration+1);
	}
	else{
	Print_Data();
	Reset_Data();
	iteration++; // increment the iteration counter to change upper and lower bounds
	if(iteration<6)
	{
	printf("-------------------------------------iteration:%d------------------------------\n",iteration+1);
	}
	}
	// check if all bounds are use to end the program
	if(iteration==6){
		The_End();
	}
	}

StaticTask_t xIdleTaskTCB CCM_RAM;
StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE] CCM_RAM;




/**********************timer calllBack functions*********************************/
void Sender_1_Timer_CallBack(TimerHandle_t xTimer)
{

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(Sem_sender_Task_1,&xHigherPriorityTaskWoken);
	Sender_1_Period=Random_Time(Task_1_Id);
	xTimerChangePeriod(xTimer1,pdMS_TO_TICKS(Sender_1_Period),0);
	T1avg+=Sender_1_Period;
}

void Sender_2_Timer_CallBack(TimerHandle_t xTimer)
{

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(Sem_sender_Task_2,&xHigherPriorityTaskWoken);
	Sender_2_Period=Random_Time(Task_2_Id);
	xTimerChangePeriod(xTimer2,pdMS_TO_TICKS(Sender_2_Period),0);
	T2avg+=Sender_2_Period;
}

void Sender_3_Timer_CallBack(TimerHandle_t xTimer)
{

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(Sem_sender_Task_3,&xHigherPriorityTaskWoken);
	Sender_3_Period=Random_Time(Task_3_Id);
	xTimerChangePeriod(xTimer3,pdMS_TO_TICKS(Sender_3_Period),0);
	T3avg+=Sender_3_Period;
}

void Receiver_CallBack(TimerHandle_t xTimer)
{
	BaseType_t xHigherPriorityTaskWoken = pdTRUE;
	xSemaphoreGiveFromISR(Sem_receiver_Task,&xHigherPriorityTaskWoken);
	if(Total_Received_Messages >=Max_Messages)
	{
		Reset();
	}

}
/*Function to create and intialize the timers  */
BaseType_t Timer_init()
{
	BaseType_t timer_1_Started=0;
	BaseType_t timer_2_Started=0;
	BaseType_t timer_3_Started=0;
	BaseType_t timer_4_Started=0;


	Sender_1_Period=Random_Time(Task_1_Id);
	Sender_2_Period=Random_Time(Task_2_Id);
	Sender_3_Period=Random_Time(Task_3_Id);

	T1avg+=Sender_1_Period;
	T2avg+=Sender_2_Period;
	T3avg+=Sender_3_Period;

	xTimer1 = xTimerCreate("Sender Timer 1",pdMS_TO_TICKS(Sender_1_Period),pdTRUE,(void*)1,Sender_1_Timer_CallBack);
	if (xTimer1 == NULL)
	{
		// Error handling for timer creation failure
		printf("Error creating Timer 1");
		while (1)
		{
		}
	}

	xTimer2 = xTimerCreate("Sender Timer 2",pdMS_TO_TICKS(Sender_2_Period),pdTRUE,(void*)2,Sender_2_Timer_CallBack);
	if (xTimer2 == NULL)
	{
		// Error handling for timer creation failure
		printf("Error creating Timer 2");
		while (1)
		{
		}
	}


	xTimer3 = xTimerCreate("Sender Timer 3",pdMS_TO_TICKS(Sender_3_Period),pdTRUE,(void*)3,Sender_3_Timer_CallBack);
	if (xTimer3 == NULL)
	{
		// Error handling for timer creation failure
		printf("Error creating Timer 3");
		while (1)
		{
		}
	}

	xTimer4 = xTimerCreate("Receiver Timer",pdMS_TO_TICKS(T_Receiver_ms),pdTRUE,(void*)3,Receiver_CallBack);
	if (xTimer4 == NULL)
	{
		// Error handling for timer creation failure
		printf("Error creating Timer 4");
		while (1)
		{
		}
	}

	if( xTimer1 != NULL && xTimer2!= NULL && xTimer3!= NULL && xTimer4!= NULL)
	{
		timer_1_Started = xTimerStart( xTimer1, 0 );
		timer_2_Started = xTimerStart( xTimer2, 0 );
		timer_3_Started = xTimerStart( xTimer3, 0 );
		timer_4_Started = xTimerStart( xTimer4, 0 );

	}

	if( timer_1_Started == pdPASS && timer_2_Started== pdPASS && timer_3_Started == pdPASS && timer_4_Started == pdPASS)
	{
		return 1;
	}
	else
	{
		return 0;
	}



}
/*----------------------------------Sender Tasks Callbacks-------------------------------------------------------------*/

void Sender_1_Task(void *pvParameters)
{

	for(;;)
	{

		BaseType_t xStatus;

		if( xSemaphoreTake(Sem_sender_Task_1 , LONG_TIME ) == pdTRUE )
		{
			TickType_t Time_Now = xTaskGetTickCount(); // get time now in ticks

			char message [20];

			snprintf(message, 20, "Time is %lu", Time_Now);
			xStatus = xQueueSend(messageQueue, message, 0);

			if (xStatus == pdPASS)
			{
				Transmitted_Messages_Per_Sender[(int)pvParameters-1]++;
				Total_Transmitted_Messages++;


			}
			else
			{
				Blocked_Messages_Per_Sender[(int)pvParameters-1]++;
				Total_Blocked_Messages++;


			}

		}

	}
}


void Sender_2_Task(void *pvParameters)
{

	for(;;)
	{

		BaseType_t xStatus;

		if( xSemaphoreTake(Sem_sender_Task_2 , LONG_TIME ) == pdTRUE )
		{
			TickType_t Time_Now = xTaskGetTickCount();

			char message [20];

			snprintf(message, 20, "Time is %lu", Time_Now);
			xStatus = xQueueSend(messageQueue, message, 0);

			if (xStatus == pdPASS)
			{
				Transmitted_Messages_Per_Sender[(int)pvParameters-1]++;
				Total_Transmitted_Messages++;


			}
			else
			{
				Blocked_Messages_Per_Sender[(int)pvParameters-1]++;
				Total_Blocked_Messages++;


			}

		}

	}
}

void Sender_3_Task(void *pvParameters)
{

	for(;;)
	{

		BaseType_t xStatus;

		if( xSemaphoreTake(Sem_sender_Task_3 , LONG_TIME ) == pdTRUE )
		{
			TickType_t Time_Now = xTaskGetTickCount();

			char message [20];

			snprintf(message, 20, "Time is %lu", Time_Now);
			xStatus = xQueueSend(messageQueue, message, 0);

			if (xStatus == pdPASS)
			{
				Transmitted_Messages_Per_Sender[(int)pvParameters-1]++;
				Total_Transmitted_Messages++;


			}
			else
			{
				Blocked_Messages_Per_Sender[(int)pvParameters-1]++;
				Total_Blocked_Messages++;



			}

		}

	}
}
/*-----------------Receiver Task Callback----------------------------*/
void ReceiverTask(void *pvParameters)
{

	for (;;)
	{
		if( xSemaphoreTake(Sem_receiver_Task , LONG_TIME ) == pdTRUE )
		{
			char Received_Text[20];
			BaseType_t xStatus;
			xStatus = xQueueReceive(messageQueue, Received_Text, pdMS_TO_TICKS(T_Receiver_ms));
			if (xStatus == pdPASS)
			{
				Total_Received_Messages++;
				printf("Received Message is:%s\n",Received_Text);

			}
		}
	}
}
/*Function to create all tasks*/
BaseType_t Sender_Receiver_tasks_init()
{
		BaseType_t xStatus_Sender_1=0;
		BaseType_t xStatus_Sender_2=0;
		BaseType_t xStatus_Sender_3=0;
		BaseType_t xStatus_Receiver=0;

		xStatus_Sender_1 = xTaskCreate(Sender_1_Task, "SenderTask 1", 1000, (void *)1, SENDER_1_PRIORITY, &Sender1);
		if (xStatus_Sender_1 != pdPASS)
		{
			// Error handling for task creation failure
			printf("Error creating sender task");
			return 0;
		}

		xStatus_Sender_2 = xTaskCreate(Sender_2_Task, "SenderTask 2", 1000, (void *)2, SENDER_2_PRIORITY, &Sender2);
		if (xStatus_Sender_2 != pdPASS)
		{
			// Error handling for task creation failure
			printf("Error creating sender task 2");
			return 0;
		}

		/* task 3 has higher priority */
		xStatus_Sender_3 = xTaskCreate(Sender_3_Task, "SenderTask 3", 1000, (void *)3, SENDER_3_PRIORITY, &Sender3);
		if (xStatus_Sender_3 != pdPASS)
		{
			// Error handling for task creation failure
			printf("Error creating sender task");
			return 0;
		}


		xStatus_Receiver = xTaskCreate(ReceiverTask, "ReceiverTask", 1000, NULL, RECEIVER_PRIORITY, &Receiver);
		if (xStatus_Receiver != pdPASS)
		{
			// Error handling for task creation failure
			printf("Error creating receiver task");
			return 0;
		}

		return 1;

}
/*Function to create all semaphores*/
void create_semaphore()
{
	Sem_sender_Task_1= xSemaphoreCreateBinary();

	if( Sem_sender_Task_1 != NULL )
	{
		xSemaphoreTake(Sem_sender_Task_1,0);
	}

	Sem_sender_Task_2= xSemaphoreCreateBinary();
	if( Sem_sender_Task_2 != NULL )
	{
		xSemaphoreTake(Sem_sender_Task_2,0);
	}

	Sem_sender_Task_3= xSemaphoreCreateBinary();
	if( Sem_sender_Task_3 != NULL )
	{
		xSemaphoreTake(Sem_sender_Task_3,0);
	}

	Sem_receiver_Task= xSemaphoreCreateBinary();
		if( Sem_receiver_Task != NULL )
		{
			xSemaphoreTake(Sem_receiver_Task,0);
		}

}

int main(int argc, char *argv[])
{
	/* Timers initialization */
	BaseType_t timers_started;
	timers_started = Timer_init();

	/* create sender and receiver tasks */
	BaseType_t task_created;
	task_created = Sender_Receiver_tasks_init();


	/* create queue */
	messageQueue = xQueueCreate(Queue_Size, sizeof(char)*20);
	if (messageQueue == NULL)
	{
		// Error handling for queue creation failure
		printf("Error creating message queue");
		while (1)
		{
		}
	}

	/* create semaphore for tasks blocking */
	create_semaphore();


	Reset();	/* Call Reset for the first time to make
	 	 	 	 sure that Queue is empty and counters=0*/


	if(timers_started == pdTRUE && task_created == pdTRUE)
	{
		vTaskStartScheduler();
		while(1)
		{

		}
	}

	return 0;
}


#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------






void vApplicationMallocFailedHook( void )
{
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	volatile size_t xFreeStackSpace;

	/* This function is called on each cycle of the idle task.  In this case it
	does nothing useful, other than report the amout of FreeRTOS heap that
	remains unallocated. */
	xFreeStackSpace = xPortGetFreeHeapSize();

	if( xFreeStackSpace > 100 )
	{
		/* By now, the kernel has allocated everything it is going to, so
		if there is a lot of heap remaining unallocated then
		the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
		reduced accordingly. */
	}
}

void vApplicationTickHook(void) {
}



void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
	/* Pass out a pointer to the StaticTask_t structure in which the Idle task's
  state will be stored. */
	*ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

	/* Pass out the array that will be used as the Idle task's stack. */
	*ppxIdleTaskStackBuffer = uxIdleTaskStack;

	/* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
  Note that, as the array is necessarily of type StackType_t,
  configMINIMAL_STACK_SIZE is specified in words, not bytes. */
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t xTimerTaskTCB CCM_RAM;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH] CCM_RAM;

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize) {
	*ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
	*ppxTimerTaskStackBuffer = uxTimerTaskStack;
	*pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

