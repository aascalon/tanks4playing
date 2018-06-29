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

	int8_t i,j, canX, canY;
	drawPixel(tank.xPos, tank.yPos, Black);
	if(tank.aimDir == up ){
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
				if((button & 0x04000000) && field[tank.xPos][(tank.yPos)+2] != Navy)
					(tank.yPos)++; //down
				if((button & 0x02000000) && field[(tank.xPos)+2][tank.yPos] != Navy)
					(tank.xPos)++;	//right
				if((button & 0x00800000) && field[(tank.xPos)-2][tank.yPos] != Navy)
					(tank.xPos)--; //left
				if((button & 0x01000000) && field[tank.xPos][(tank.yPos)-2] != Navy)
					(tank.yPos)--; //up
				
		}
		drawTank();
	}
}

__task void fire(void){
	int cleanup, x, y, xdir, ydir;
	while(1){
		x = tank.xPos;
		y = tank.yPos;
		ydir = 1;
		xdir = 1;
		
		if(~LPC_GPIO2->FIOPIN & (1<<10)){
			while(~LPC_GPIO2->FIOPIN & ( 1 << 10 ) )
			{}
			//if the direction is up or down we are firing in y
			if(tank.aimDir == up || tank.aimDir == down)
			{
				//if the direction is up we fire in the negative y direction
				if(tank.aimDir == up)
					ydir = -1;
				
				y+=3*ydir;
				while(field[x][y] != Navy)
				{
					drawPixel(x,y,Red);
					y+= ydir;
					os_dly_wait(1);
				}
				cleanup = (tank.yPos) + 3*ydir;
				
				while((cleanup - y)*ydir*(-1) > 0){
					drawPixel(x,cleanup, White);
					os_dly_wait(1);
					cleanup += ydir;
				}				
			}
			else{
				//else the tank will fire in the x direction
				//if the direction is left we fire in the negative x direction
				if(tank.aimDir == left)
					xdir = -1;
				
				x+=3*xdir;
				while(field[x][y] != Navy)
				{
					drawPixel(x,y,Red);
					x+= xdir;
					os_dly_wait(1);
				}
				cleanup = (tank.xPos) + 3*xdir;
				
				while((cleanup - x)*xdir*(-1) > 0){
					drawPixel(cleanup,y, White);
					os_dly_wait(1);
					cleanup += xdir;
				}
			}
		}
	}
}

__task void aimCannon(void){
	while(1){
		uint16_t num = 0;
		uint8_t oldDir, newDir;
		LPC_ADC->ADCR |= 0x01000000 ; //start a conversion
		while ( !(LPC_ADC->ADGDR & 0x80000000) ) 
		{}
		oldDir = tank.aimDir;
		num = ((LPC_ADC->ADGDR & 0xFFF0) >> 4);
		// aim up
		if ( ( num < 512) || (num >= 2048 && num < 2560))
		newDir = up;
		//aim left
		else if ( (num >= 512 && num < 1024) || (num >= 2560 && num < 3072))
		newDir = left;

		//aim down
		else if ( (num >= 1024 && num < 1536) || (num >= 3584))
		newDir = down;

		//aim right
		else if ( (num >= 1536 && num < 2048))
		newDir = right;
		
		if(newDir != oldDir){
			clearTank();
			tank.aimDir = newDir;
			drawTank();
			
		}
		os_tsk_pass();
	}
}

__task void start_tasks(void){
	os_sem_init(&move, 1);
	os_tsk_create(moveTank,1);
	os_tsk_create(fire,1);
	os_tsk_create(aimCannon,1);
	os_tsk_delete_self();               
}
int main(void){
	int i, j, x, y;
	
	//potentiometer setup
	LPC_SC->PCONP |= 0x1000;
 	LPC_PINCON->PINSEL1 &= ~(0x03 <<18);
	LPC_PINCON->PINSEL1 |= (0x01 <<18);
	LPC_ADC->ADCR = (1 << 2) | (4 << 8) | (1 << 21);
	
	tank.xPos = 5;
	tank.yPos = 5;
	tank.aimDir = up;
	tank.ammo = 8;

	printf("UART begin\n");
	
	GLCD_Init();
	GLCD_Clear(White);
	GLCD_SetTextColor(0xFFFF);  
	GLCD_SetBackColor(White);
	GLCD_SetTextColor(Navy);
	

	
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
			field[i][0] = Navy;
			field[0][j] = Navy;
			field[i][63] = Navy;
			field[47][j] = Navy;
			drawPixel(i,j,field[i][j]);

		}
	}
	os_sys_init(start_tasks);
 
}



