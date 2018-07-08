#include <lpc17xx.h>
#include <stdio.h>
#include <stdint.h>
#include <RTL.h>
#include "GLCD.h"
#include <stdlib.h>
#include "enemy.h"
#include <math.h>
#define up    0
#define right 1
#define down  2
#define left  3

//a mutex to make sure things don't get drawn at the same time
OS_MUT drawlock;
OS_MUT enemylock;
OS_MUT enemyiskill;
	node_t *enemyList;

uint32_t moveDelay = 1000;
const uint8_t ENEMY_MAX = 5;
unsigned short field[48][64] = {White};
uint8_t enemyQty = 0;
uint8_t fieldAmmo = 0;
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


void drawEnemies(uint8_t xLoc, uint8_t yLoc){
	int i, j;
	for (i = xLoc-1; i <= xLoc+1; i++){
		for (j = yLoc-1; j <= yLoc+1; j++){
			drawPixel( i, j, Magenta);
			field[i][j] = Magenta;
		}
	}
}

void clearEnemies(uint8_t xLoc, uint8_t yLoc){
	int i, j;
	for (i = xLoc-1; i <= xLoc+1; i++){
		for (j = yLoc-1; j <= yLoc+1; j++){
			drawPixel( i, j, White);
			field[i][j] = White;
		}
	}
}

int collisionFree(int x, int y, int dir){
	//return 1 if no collision, 0 if collision
	int i;
	
	if (dir == up){
		for(i = x -1; i <= x +1; i++)
			if(field[i][y-2] == (Navy) || field[i][y-2] == Magenta)
				return 0;
		}
	else if (dir == down){
		for(i = x -1; i <= x +1; i++)
			if(field[i][y+2] == (Navy)  || field[i][y+2] == Magenta)
				return 0;
		}
	else if (dir == left){
		for(i = y -1; i <= y +1; i++)
			if(field[x-2][i] == (Navy)  || field[x-2][i] == Magenta)
				return 0;
		}
	else if (dir == right){
		for(i = y -1; i <= y +1; i++)
			if(field[x+2][i] == (Navy)  || field[x+2][i] == Magenta)
				return 0;
		}
			
	return 1;
}

int wallFreeSpawn(uint8_t xLoc, uint8_t yLoc){
	int i, j;
	for (i = xLoc-1; i <= xLoc+1; i++){
		for (j = yLoc-1; j <= yLoc+1; j++){
			if(field[i][j] == Navy)
				return 0;
		}
	}
	return 1;
}


void drawTank(void){
	int8_t i,j, canX, canY;
	os_mut_wait(&drawlock,0xffff);
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
	//this wait ensures that bits of the tank aren't left behind
	os_dly_wait(1);

	}
	os_mut_release(&drawlock);
	
}
void clearTank(void){
	os_mut_wait(&drawlock,0xffff);
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
	os_mut_release(&drawlock);

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

void ammoPickup(int x, int y){
	int i, j;
	for(i = x - 1; i <= x +1; i++)
		for(j = y - 1; j <= y + 1; j++)
			if(field[i][j] == Maroon){
				fieldAmmo--;
				field[i][j] = White;
				if(x == tank.xPos && y == tank.yPos){
					tank.ammo++;
					updateLEDs(tank.ammo);
				}
			}
}


void killEnemy(int x, int y){
	uint8_t xEnemy, yEnemy;
	node_t *curEnemy;
	curEnemy = enemyList;
		while(curEnemy != NULL){
	//iterate through the enemy linked list and find the enemy that the laser hit

			xEnemy = (curEnemy->data).xLoc;
			yEnemy = (curEnemy->data).yLoc;
			
			if(abs(x - xEnemy) <= 1 && abs(y - yEnemy) <= 1){
				deleteEnemy(xEnemy, yEnemy, &enemyList);
				clearEnemies(xEnemy, yEnemy);
				enemyQty--;
				break;
			}
			curEnemy = curEnemy->next;
		}
		

}

//TODO IF SPARE TIME: look into interrupts
__task void fire(void){
	int cleanup, x, y, xdir, ydir;
	while(1){
		x = tank.xPos;
		y = tank.yPos;
		ydir = 1;
		xdir = 1;
		os_mut_wait(&drawlock,0xffff);
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
				while(field[x][y] != Navy && field[x][y] != Magenta)
				{
					drawPixel(x,y,Red);
					y+= ydir;
					os_dly_wait(1);
				}
				
				if(field[x][y] == Magenta){
					os_mut_wait(&enemylock,0xffff);
					killEnemy(x,y);
					os_mut_release(&enemylock);

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
				while(field[x][y] != Navy && field[x][y] != Magenta)
				{
					drawPixel(x,y,Red);
					x+= xdir;
					os_dly_wait(1);
				}
				
				if(field[x][y] == Magenta){
					os_mut_wait(&enemylock,0xffff);
					killEnemy(x,y);
					os_mut_release(&enemylock);
				}
				
				cleanup = (tank.xPos) + 3*xdir;
				
				while((cleanup - x)*xdir*(-1) > 0){
					drawPixel(cleanup,y, White);
					os_dly_wait(1);
					cleanup += xdir;
				}
			}
			
			
		}		
		os_mut_release(&drawlock);
		os_tsk_pass();	
	}

}




__task void moveTank(void)
{
	while(1){
		int button;
		if((~LPC_GPIO1->FIOPIN & 0x07900000)){
			button = (~LPC_GPIO1->FIOPIN);
			clearTank();
				if((button & 0x04000000) && collisionFree(tank.xPos, tank.yPos, down))
					(tank.yPos)++; //down
				if((button & 0x02000000) && collisionFree(tank.xPos, tank.yPos, right))
					(tank.xPos)++;	//right
				if((button & 0x00800000) && collisionFree(tank.xPos, tank.yPos, left))
					(tank.xPos)--; //left
				if((button & 0x01000000) && collisionFree(tank.xPos, tank.yPos, up))
					(tank.yPos)--; //up
				
		//	printf("tank location change\n");
		}
		ammoPickup(tank.xPos, tank.yPos);
		drawTank();
		os_tsk_pass();
	}
}


__task void moveEnemy(void){
		int dir, xEnemy, yEnemy;
		os_itv_set(30);
		while(1){
			node_t *curEnemy;
			os_itv_wait();
			
			//os_mut_wait(&enemylock,0xffff);
			//printf("bout to loop through dis fucka %d enemies total\n", enemyQty);
			os_mut_wait(&drawlock,0xffff);
			os_mut_wait(&enemylock,0xffff);
			curEnemy = enemyList;

			while(curEnemy != NULL){
				//iterate through the enemy linked list and move them someplace
				
				//the problem happens if you delete the head during the loop3
				
			
			

				if(((curEnemy->data).xLoc != 0)&& ((curEnemy->data).yLoc != 0)){
					dir = (rand()%10)%4;
					xEnemy = (curEnemy->data).xLoc;
					yEnemy = (curEnemy->data).yLoc;
					if ((dir == up) && collisionFree(xEnemy, yEnemy, dir))
						((curEnemy->data).yLoc)--;
					else if ((dir == down) && collisionFree(xEnemy, yEnemy, dir))
						((curEnemy->data).yLoc)++;
					else if ((dir == left) && collisionFree(xEnemy, yEnemy, dir))
						((curEnemy->data).xLoc)--;
					else if ((dir == right) && collisionFree(xEnemy, yEnemy, dir))
						((curEnemy->data).xLoc)++;

					ammoPickup((curEnemy->data).xLoc,(curEnemy->data).yLoc);
					clearEnemies(xEnemy, yEnemy);
					drawEnemies((curEnemy->data).xLoc,(curEnemy->data).yLoc);
					}
					
				
				curEnemy = curEnemy->next;
			}
			os_mut_release(&enemylock);
			os_mut_release(&drawlock);
			os_tsk_pass();
	}
}

__task void spawnNew(void){
	int ammoX, ammoY, enemyX, enemyY;
	os_itv_set(1);
	while(1){
		os_itv_wait();
		if (enemyQty < ENEMY_MAX){
			
				enemyX = rand()%44 + 2;
				enemyY = rand()%60 + 2;
			while(!(wallFreeSpawn(enemyX, enemyY))){
				enemyX = rand()%44 + 2;
				enemyY = rand()%60 + 2;
			}
			os_mut_wait(&enemylock,0xffff);

			spawnEnemy(enemyX, enemyY,&enemyList);
			enemyQty++;
			os_mut_release(&enemylock);

			}
		
		if ((8 - (tank.ammo + fieldAmmo)) > 0){

			ammoX = rand()%46 + 1;
			ammoY = rand()%62 + 1;
			os_mut_wait(&drawlock,0xffff);
			while(field[ammoX][ammoY] != White)
			{
				ammoX = rand()%46 + 1;
				ammoY = rand()%62 + 1;
			}
			fieldAmmo++;
			drawPixel(ammoX, ammoY, Maroon);
			field[ammoX][ammoY] = Maroon;
			os_mut_release(&drawlock);
	
		}
			os_tsk_pass(); 
	}

}




__task void start_tasks(void){
	os_mut_init(&drawlock);
	os_mut_init(&enemylock);

	
	os_tsk_create(moveTank,1);
	os_tsk_create(fire,1);
	os_tsk_create(aimCannon,1);
	os_tsk_create(moveEnemy,1);
	os_tsk_create(spawnNew,1);
// 	os_tsk_create(spawnAmmo,1);
	os_tsk_delete_self();
}
int main(void){
	int i, j, x, y;

	uint32_t soRandom = 2000000;
	//initialise first member of linked list containing all the enemies
	enemyList = NULL;
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
	GLCD_SetBackColor(White);
	GLCD_SetTextColor(Black);  

	while(LPC_GPIO1->FIOPIN & 0x00100000){
		soRandom++;
		GLCD_DisplayString(3,1,1,"Press joystick");
		GLCD_DisplayString(5,1,1,"to begin");

	}
		GLCD_Clear(White);

	srand(soRandom);
	GLCD_SetTextColor(Navy);
	updateLEDs(tank.ammo);

	// field set up
	for(x = 0; x < 48; x++)
	{
		for(y = 0; y < 64; y++)
		{
			field[x][y] = White;
		}
	}
	for (i = 0; i < 48; i++){
		for(j = 0; j < 64; j++){
			field[i][0] = Navy;
			field[0][j] = Navy;
			field[i][63] = Navy;
			field[47][j] = Navy;
		}
	}
	for(x = 1; x < 35; x++)
	{	
		if(x < 9)
			field[x][29] = Navy;
		if(x > 10)
			field[x][10] = Navy;
		if(x > 23 && x < 33)
			field[x][41] = Navy;
		if(x > 20 && x < 33)
			field[x][52] = Navy;
	}
	for(y = 1; y < 64; y++)
	{
		if(y < 11)
			field[10][y] = Navy;
		if(y > 6 && y < 15)
			field[35][y] = Navy;
		if(y > 33 && y < 42)
			field[23][y] = Navy;
		if(y > 40 && y < 53)
			field[33][y] = Navy;
		if(y > 51)
			field[20][y] = Navy;
	}
	for (x = 0; x < 48; x++){
		for(y = 0; y < 64; y++){
			drawPixel(x,y,field[x][y]);
		}
	}
	//end field set up (let's make this a function)
	
	printf("UART begin\n");
	


	//printf("is there another enemy? (%d, %d)\n " ,(enemyList->next->data).xLoc, (enemyList->next->data).yLoc);
	os_sys_init(start_tasks);
 
}

