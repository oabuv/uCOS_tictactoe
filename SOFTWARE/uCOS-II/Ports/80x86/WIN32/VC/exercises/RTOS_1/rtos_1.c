/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*
*					WIN32 PORT & LINUX PORT
*                          (c) Copyright 2004, Werner.Zimmermann@fht-esslingen.de
*                 (Similar to Example 1 of the 80x86 Real Mode port by Jean J. Labrosse)
*                                           All Rights Reserved
** *****************************************************************************************************
*		Examination file
* *****************************************************************************************************
*/

#include "includes.h"

/*
*********************************************************************************************************
*                                               CONSTANTS
*********************************************************************************************************
*/

#define  TASK_STK_SIZE                 512       /* Size of each task's stacks (# of WORDs)            */
#define  N_TASKS                        5       /* Number of identical tasks                          */
#define PLAYER_1 0x01
#define PLAYER_2 0x02
#define END 0x08
#define COL 35
#define ROW 5
/*
*********************************************************************************************************
*                                               VARIABLES
*********************************************************************************************************
*/

OS_STK        TaskStk[N_TASKS][TASK_STK_SIZE];        /* Tasks stacks                                  */
OS_STK        TaskStartStk[TASK_STK_SIZE];

/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/
void TaskStart(void* data);
void drawboard(void* data);
void player_1(void* data);
void player_2(void* data);
void Check(void* data);


// Globale var

OS_EVENT* TaskSem1;
OS_EVENT* TaskSem2;
INT8U err;

OS_FLAG_GRP* GameLogic;
INT8U errFlag;


INT8U board[3][3];
INT8U  Box[5][11] = {
{0xDA, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xBF, 0x00},
{0xB3, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xB3, 0x00},
{0xB3, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xB3, 0x00},
{0xB3, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xB3, 0x00},
{0xC0, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xD9, 0x00},
};
/*
*********************************************************************************************************
*                                                MAIN
*********************************************************************************************************
*/

int  main(void)
{
	PC_DispClrScr(DISP_FGND_WHITE + DISP_BGND_BLACK);      /* Clear the screen                         */

	OSInit();                                              /* Initialize uC/OS-II                      */

	OSTaskCreate(TaskStart, (void*)0, &TaskStartStk[TASK_STK_SIZE - 1], 0);

	GameLogic = OSFlagCreate(0, &errFlag);

	TaskSem1 = OSSemCreate(1);
	TaskSem2 = OSSemCreate(1);
	OSStart();                                             /* Start multitasking                       */

	return 0;
}


/*
*********************************************************************************************************
*                                              STARTUP TASK
*********************************************************************************************************
*/
void  TaskStart(void* pdata)
{
	INT16U key;


	pdata = pdata;                                         /* Prevent compiler warning                 */
	OSTaskCreate(drawboard, (void*)0, (void*)& TaskStk[0][TASK_STK_SIZE - 1], 1);
	OSTaskCreate(player_1, (void*)0, (void*)& TaskStk[1][TASK_STK_SIZE - 1], 2);		
	OSTaskCreate(player_2, (void*)0, (void*)& TaskStk[2][TASK_STK_SIZE - 1], 4);
	OSTaskCreate(Check, (void*)0, (void*)& TaskStk[3][TASK_STK_SIZE - 1], 3);
	for (;;) {

		if (PC_GetKey(&key) == TRUE) {                     /* See if key has been pressed              */
			if (key == 0x1B) {                             /* Yes, see if it's the ESCAPE key          */
				exit(0);  	                           /* End program                              */
			}
		}
		OSTimeDlyHMSM(0, 0, 1, 0);                         /* Wait one second                          */
	}
}

/*
*********************************************************************************************************
*                                                  TASKS
*********************************************************************************************************
*/

void drawboard(void* data) {
	INT8U c, j, i;
	INT8U x = 35;
	INT8U y = 5;
	INT8U check = 1;
	OS_FLAGS mess = 0;

	
	// Draw game board
	
	for (i = 0; i < 3; i++) {
		for (c = 0; c < 3; c++) {
			for (j = 0; j < 5; j++) {
				PC_DispStr(x, j + y, Box[j], DISP_FGND_WHITE);
			}
			x = x + 9;
		}
		x = 35;
		y = y + 4;
		
	}
	
	
	// Give the turns for players
	for (;;) {
		
		mess = OSFlagQuery(GameLogic, &errFlag);

		if(mess==0) OSFlagPost(GameLogic, PLAYER_1, OS_FLAG_SET, &errFlag);

		if (mess == PLAYER_1)
		{
			OSFlagPost(GameLogic, PLAYER_1, OS_FLAG_CLR, &errFlag);
			OSFlagPost(GameLogic, PLAYER_2, OS_FLAG_SET, &errFlag);
		}
		else
		{
			OSFlagPost(GameLogic, PLAYER_2, OS_FLAG_CLR, &errFlag);
			OSFlagPost(GameLogic, PLAYER_1, OS_FLAG_SET, &errFlag);
		}
		OSTimeDly(1);
		
	}
	
	OSTaskDel(1);
	
}

void player_1(void* data) {
	INT8U  x;
	INT8U  y;
	INT8U mark = 'X';

	srand(GetCurrentThreadId());
	for (;;) {
		
		OSFlagPend(GameLogic, PLAYER_1, OS_FLAG_WAIT_SET_ALL, 0, &errFlag);		
		OSSemPend(TaskSem2, 0, &err);
		do {
			x = rand() % 3;		                  
			y = rand() % 3;
			
		} while(board[x][y] != 0);
		
		board[x][y] = mark;
		
		PC_DispChar(COL + 3 + x * 10, ROW + 2 + y * 4, mark, DISP_FGND_WHITE + DISP_BGND_RED);
		OSTimeDlyHMSM(0, 0, 1, 0);
		err = OSSemPost(TaskSem2);
		
		
		OSFlagPost(GameLogic, PLAYER_1, OS_FLAG_CLR, &errFlag);
		OSFlagPost(GameLogic, END, OS_FLAG_SET, &errFlag);
		OSFlagPost(GameLogic, PLAYER_2, OS_FLAG_SET, &errFlag);
		
		

	}
	
}
void player_2(void* data) {
	INT8U  x;
	INT8U  y;
	INT8U mark = 'O';
	
	srand(GetCurrentThreadId());
	for (;;) {
		
		OSFlagPend(GameLogic, PLAYER_2, OS_FLAG_WAIT_SET_ALL, 0, &errFlag);
		OSSemPend(TaskSem2, 0, &err);
		do {
			x = rand() % 3;		                 
			y = rand() % 3;
			
		} while (board[x][y] != 0);

		
		board[x][y] = mark;
		
		PC_DispChar(COL + 3 + x * 10, ROW + 2 + y * 4, mark, DISP_FGND_WHITE + DISP_BGND_BLUE);
		OSTimeDlyHMSM(0, 0, 1, 0);
		err = OSSemPost(TaskSem2);
		
		
		OSFlagPost(GameLogic, PLAYER_2, OS_FLAG_CLR, &errFlag);
		OSFlagPost(GameLogic, END, OS_FLAG_SET, &errFlag);
		OSFlagPost(GameLogic, PLAYER_1, OS_FLAG_SET, &errFlag);


	}
	
}

void Check(void* data) {

	INT8U i, j;
	INT8U msg_player_1[] = "Player 1 win!";
	INT8U msg_player_2[] = "Player 2 win!";
	INT8U msg_draw[] = "Tie!";
	INT8U win = 0;

	for (;;) {
		
		OSFlagPend(GameLogic, END, OS_FLAG_WAIT_SET_ANY, 0, &errFlag);
		
		OSSemPend(TaskSem1, 0, &err);
		PC_DispStr(35, 2, msg_draw, DISP_FGND_WHITE + DISP_BGND_GREEN);
	
		// check columns
		for (j = 0; j < 3; j++) {
			if (board[0][i] == board[1][i] && board[0][i] == board[2][i] && board[0][i] == 'X') {
				PC_DispStr(35, 2, msg_player_1, DISP_FGND_WHITE + DISP_BGND_RED);
				win = 1;
			}
				
			if (board[0][i] == board[1][i] && board[0][i] == board[2][i] && board[0][i] == 'O') {
				PC_DispStr(35, 2, msg_player_2, DISP_FGND_WHITE + DISP_BGND_BLUE);
				win = 1;
			}
				
		}
		// check rows (still have problem with row win checking without any idea!!)
		for (i = 0; i < 3; i++) {
			if (board[i][0] == board[i][1] && board[i][0] == board[i][2] && board[i][0] == 'X') {
				PC_DispStr(35, 2, msg_player_1, DISP_FGND_WHITE + DISP_BGND_RED);
				win = 1;
			}

			if (board[i][0] == board[i][1] && board[i][0] == board[i][2] && board[i][0] == 'O') {
				PC_DispStr(35, 2, msg_player_2, DISP_FGND_WHITE + DISP_BGND_BLUE);
				win = 1;
			}
		}
		if (win) break;
		// check diagonals
		if (board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] == 'X') {
			PC_DispStr(35, 2, msg_player_1, DISP_FGND_WHITE + DISP_BGND_RED); break;
		}
			
		if (board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] == 'O') {
			PC_DispStr(35, 2, msg_player_2, DISP_FGND_WHITE + DISP_BGND_BLUE); break;
		}
			

		if (board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] == 'X') {
			PC_DispStr(35, 2, msg_player_1, DISP_FGND_WHITE + DISP_BGND_RED); break;
		}
			
		if (board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] == 'O') {
			PC_DispStr(35, 2, msg_player_2, DISP_FGND_WHITE + DISP_BGND_BLUE); break;
		}	
		err = OSSemPost(TaskSem1);
		OSTimeDly(1);
		
	}
	
	// Delete Tasks. Game is done!!
	OSTaskDel(2);
	OSTaskDel(3);
	OSTaskDel(4);
}


/*
*********************************************************************************************************
*                                      NON-TASK FUNCTIONS
*********************************************************************************************************
*/
