#include <lpc17xx.h>
#include <stdio.h>
#include <stdint.h>
#include <RTL.h>
#include "GLCD.h"
#include <stdlib.h>
#include "enemy.h"
#define up    0
#define right 1
#define down  2
#define left  3


uint32_t moveDelay = 1000;

unsigned short field[48][64] = {White};
uint8_t enemyQty = 0;
node_t *head;
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
	if(tank.aimDir == up){
		canX = 0;
		canY = -1;
		}
	else if(tank.aimDir == right){
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
			//draw the pixel in the centre of the tank black
			if (i == tank.xPos  && j == tank.yPos)
				drawPixel(i,j,Black);
			//draw the cannon black
			else if (i == (tank.xPos)+canX && j == (tank.yPos)+canY){
				drawPixel(i,j,Black);
				drawPixel(i+canX,j+canY,Black);
			}
			//draw the body of the tank
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
void updateLEDs(uint8_t ammoCount){
	uint32_t i;
	uint32_t mask = 1;
	uint32_t digit = 0;
	uint32_t output = 0;
	uint32_t gpio1, gpio2;
	ammoCount = (1 << (ammoCount))-1;
	
	//clear all LEDs so that they can be set anew
	
	LPC_GPIO1->FIOCLR |=  0xB0000000 ;
	LPC_GPIO2->FIOCLR |=  0x0000007C ;
	
	for (i= 0; i < 8; i++)
	
	{
		digit = (ammoCount & mask);
		if(i == 0)
			digit = (digit << 6);
		else if(i == 1)
			digit = (digit << 4);
		else if(i == 2)
			digit = (digit << 2);
		else if(i == 4)
			digit = (digit >> 2);
		else if(i == 5)
			digit = (digit << 26);
		else if(i == 6)
			digit = (digit << 23);
		else if (i == 7)
			digit = (digit << 21);
		
		output |= digit; 
		mask = (mask << 1);
		digit = 0;
	}

	gpio1 = (output & 0xB0000000);
	gpio2 = (output & 0x0000007C) ;

	LPC_GPIO1->FIOSET = gpio1;
	LPC_GPIO2->FIOSET = gpio2;
	
}

//TODO IF SPARE TIME: look into interrupts
__task void fire(void){
	int cleanup, x, y, xdir, ydir;
	while(1){
		x = tank.xPos;
		y = tank.yPos;
		ydir = 1;
		xdir = 1;
		
		//check if button is pushed AND the tank has enough ammo
		if( (~LPC_GPIO2->FIOPIN & (1<<10)) && (tank.ammo) > 0 ){
			while(~LPC_GPIO2->FIOPIN & ( 1 << 10 ) )
			{}
			//update ammo count on LEDs
			(tank.ammo)--;
			updateLEDs(tank.ammo);
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
		os_tsk_pass();
	}
}

void drawEnemies(void){
	drawPixel( (head->data).xLoc, (head->data).yLoc, Magenta);
}

__task void moveEnemy(void){
		int dir, xEnemy, yEnemy;
	while(1){
		dir = (rand()%10)%4;
		printf(" I'm soooo random! XD %d\n", (rand()%10)%4);
		xEnemy = (head->data).xLoc;
		yEnemy = (head->data).yLoc;
		drawPixel( (head->data).xLoc, (head->data).yLoc, White);
		if (dir == up && field[xEnemy][yEnemy-1] != Navy)
			((head->data).yLoc)--;
		else if (dir == down && field[xEnemy][yEnemy+1] != Navy)
			((head->data).yLoc)++;
		else if (dir == left && field[xEnemy-1][yEnemy] != Navy)
			((head->data).xLoc)--;
		else if (dir == right && field[xEnemy+1][yEnemy] != Navy)
			((head->data).xLoc)++;
		drawPixel(xEnemy, yEnemy, White);
		drawEnemies();
		os_dly_wait(10);
		os_tsk_pass();
	}
}

__task void start_tasks(void){
	os_tsk_create(moveTank,1);
	os_tsk_create(fire,1);
	os_tsk_create(aimCannon,1);
	os_tsk_create(moveEnemy,1);
	os_tsk_delete_self();
}
int main(void){
	int i, j, x, y;
	uint32_t soRandom = 2000000;
	//initialise first member of linked list
	
	head = NULL;
	//potentiometer setup
	LPC_SC->PCONP |= 0x1000;
 	LPC_PINCON->PINSEL1 &= ~(0x03 <<18);
	LPC_PINCON->PINSEL1 |= (0x01 <<18);
	LPC_ADC->ADCR = (1 << 2) | (4 << 8) | (1 << 21);
	
	//LED setup
	LPC_GPIO1->FIODIR |=0xB0000000;
	LPC_GPIO2->FIODIR |=0x0000007C;

	
	
	tank.xPos = 5;
	tank.yPos = 5;
	tank.aimDir = left;
	tank.ammo = 8;

	GLCD_Init();
	GLCD_Clear(White);
	GLCD_SetTextColor(0xFFFF);  
	GLCD_SetBackColor(White);
	GLCD_SetTextColor(Navy);
	updateLEDs(tank.ammo);
	
	
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
	printf("UART begin\n");
	
	while(LPC_GPIO1->FIOPIN & 0x00100000)
		soRandom++;
	srand(soRandom);
	spawnEnemy(rand()%48, rand()%64,&head);
	os_sys_init(start_tasks);
 
}

