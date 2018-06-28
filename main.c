#include <lpc17xx.h>
#include <stdio.h>
#include <stdint.h>
#include <RTL.h>
#include "GLCD.h"
#define up    0
#define right 1
#define down  2
#define left  3

OS_SEM move;

uint32_t moveDelay = 1000;

unsigned short field[48][64] = {White};
 
typedef struct{
	int xPos, yPos ,ammo, aimDir;
}  myTank;
myTank tank ; 
void drawPixel(int x, int y, unsigned short colour){
	int i;
	GLCD_SetTextColor(colour);
	for(i = 0; i <5; i++)
			{
				GLCD_PutPixel( x*5+i, y*5);
				GLCD_PutPixel( x*5+i, y*5+1);
				GLCD_PutPixel( x*5+i, y*5+2);
				GLCD_PutPixel( x*5+i, y*5+3);
				GLCD_PutPixel( x*5+i, y*5+4);
			}
}

void drawTank(void){
// 	drawPixel((tank.xPos)-1, (tank.yPos)-1, Green);
// 	drawPixel((tank.xPos)-1, (tank.yPos)+1, Green);
// 	drawPixel((tank.xPos)+1, (tank.yPos)-1, Green);
// 	drawPixel((tank.xPos)+1, (tank.yPos)+1, Green);
// 	if(tank.aimDir != up)
// 		drawPixel(tank.xPos, (tank.yPos)-1, Green);
// 	if(tank.aimDir != down)
// 		drawPixel(tank.xPos, (tank.yPos)+1, Green);
// 	if(tank.aimDir != right)
// 		drawPixel((tank.xPos)+1, tank.yPos, Green);
// 	if(tank.aimDir != left)
// 		drawPixel((tank.xPos)-1, tank.yPos, Green);
// 	
 	
// 	
// 	if(tank.aimDir == up){
// 		drawPixel(tank.xPos, (tank.yPos)-1, Black);
// 		drawPixel(tank.xPos, (tank.yPos)-2, Black);
// 	}
// 	else if(tank.aimDir == right){
// 		drawPixel((tank.xPos)+1, tank.yPos, Black);
// 		drawPixel((tank.xPos)+2, tank.yPos, Black);
// 	}
// 	else if(tank.aimDir == down){
// 		drawPixel(tank.xPos, (tank.yPos)+1, Black);
// 		drawPixel(tank.xPos, (tank.yPos)+2, Black);
// 	}
// 	else {
// 		drawPixel((tank.xPos)-1, tank.yPos, Black);
// 		drawPixel((tank.xPos)-2, tank.yPos, Black);
// 	}
	int8_t i,j, canX, canY;
	drawPixel(tank.xPos, tank.yPos, Black);
	if(tank.aimDir == up){
		canX = 0;
		canY = -1;
		}
	else if(tank.aimDir == right
		){
		canX = 1;
		canY = 0;
		}
	else if(tank.aimDir == down){
		canX = 0;
		canY = 1;
		}
	else{
		//left
		canX = -1;
		canY = 0;
	}
	for (i = (tank.xPos)-1; i <= tank.xPos+1; i++){
		for (j = (tank.yPos)-1; j <=(tank.yPos)+1; j++){
			if (i == tank.xPos  && j == tank.yPos)
				drawPixel(i,j,Black);
			else if (i == (tank.xPos)+canX && j == (tank.yPos)+canY){
				drawPixel(i,j,Black);
				drawPixel(i+canX,j+canY,Black);
			}
			else
				drawPixel(i,j,Green);
		}
	}
	drawPixel((tank.xPos)+canX, (tank.yPos)+canY,Black);
}
void clearTank(void){
	drawPixel((tank.xPos)-1, (tank.yPos)-1, White);
	drawPixel((tank.xPos)-1, (tank.yPos)+1, White);
	drawPixel((tank.xPos)+1, (tank.yPos)-1, White);
	drawPixel((tank.xPos)+1, (tank.yPos)+1, White);
	if(tank.aimDir != up)
		drawPixel(tank.xPos, (tank.yPos)-1, White);
	if(tank.aimDir != down)
		drawPixel(tank.xPos, (tank.yPos)+1, White);
	if(tank.aimDir != right)
		drawPixel((tank.xPos)+1, tank.yPos, White);
	if(tank.aimDir != left)
		drawPixel((tank.xPos)-1, tank.yPos, White);
	
	drawPixel(tank.xPos, tank.yPos, White);
	
	if(tank.aimDir == up){
		drawPixel(tank.xPos, (tank.yPos)-1, White);
		drawPixel(tank.xPos, (tank.yPos)-2, field[tank.xPos][(tank.yPos)-2]);
	}
	else if(tank.aimDir == right){
		drawPixel((tank.xPos)+1, tank.yPos, White);
		drawPixel((tank.xPos)+2, tank.yPos, field[(tank.xPos)+2][tank.yPos]);
	}
	else if(tank.aimDir == down){
		drawPixel(tank.xPos, (tank.yPos)+1, White);
		drawPixel(tank.xPos, (tank.yPos)+2, field[tank.xPos][(tank.yPos)+2]);
	}
	else {
		drawPixel((tank.xPos)-1, tank.yPos, White);
		drawPixel((tank.xPos)-2, tank.yPos, field[(tank.xPos)-2][tank.yPos]);
	}

}

void myleds(int num){

}

__task void moveTank(void)
{
	while(1){
		int button;
		if((~LPC_GPIO1->FIOPIN & 0x07900000)){
			button = (~LPC_GPIO1->FIOPIN);
			//while((~LPC_GPIO1->FIOPIN & 0x07900000));
			clearTank();
				if(button & 0x04000000)
					(tank.yPos)++; //down
				if(button & 0x02000000)
					(tank.xPos)++;	//right
				if(button & 0x00800000)
					(tank.xPos)--; //left
				if(button & 0x01000000)
					(tank.yPos)--; //up
				
			

		}
		drawTank();
	}
}

__task void start_tasks(void){
	os_sem_init(&move, 1);
	os_tsk_create(moveTank,1);
	os_tsk_delete_self();
}
int main(void){
	int i, j, x, y;
	
	tank.xPos = 5;
	tank.yPos = 5;
	tank.aimDir = up;
	tank.ammo = 8;

	printf(" df \n");
	GLCD_Init();
	GLCD_Clear(White);
	GLCD_SetTextColor(0xFFFF);  
	GLCD_SetBackColor(White);
	GLCD_SetTextColor(Red);
	

	
	for(x = 0; x < 48; x++)
	{
		for(y = 0; y < 64; y++)
		{
			field[x][y] = White;
			drawPixel(x,y,field[x][y]);
		}
	}
	for (i = 0; i < 48; i ++){
		for(j = 0; j < 64; j++){
			field[i][0] = Red;
			field[0][j] = Red;
			field[i][63] = Red;
			field[47][j] = Red;
			drawPixel(i,j,field[i][j]);

		}
	}
	os_sys_init(start_tasks);
 
}



