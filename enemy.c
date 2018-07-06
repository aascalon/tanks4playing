#include "enemy.h"
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

void spawnEnemy(uint32_t startX, uint32_t startY,node_t **L ){
	//make a new enemy struct
	enemy_t *newEnemy ;
	node_t *newNode;
	//creates a new enemy struct
	newEnemy =(enemy_t*) malloc(sizeof(enemy_t));
	
	//randomly generated numbers are passed in as the starting coordinate
	newEnemy->xLoc = startX;
	newEnemy->yLoc = startY;
	newNode = (node_t*) malloc(sizeof(node_t));
	newNode->data = *newEnemy;
	//puts the new node at the front of the list 
	newNode->next = *L;
	*L = newNode;

}




void deleteEnemy(int x, int y, node_t **head){
 
	node_t *temp = *head;
	node_t *prev;
	
	if(temp != NULL && abs(temp->data.xLoc - x) <= 1 && abs(temp->data.yLoc - y) <= 1){
		*head = temp->next;
		free(temp);
		printf("deleted head\n");
		return;
	}

	while(temp != NULL && (abs(temp->data.xLoc - x) > 1 || abs(temp->data.yLoc - y) > 1)){
		prev = temp;
		temp = temp->next;
	}
	
	if(temp == NULL)
		return;
	
	prev->next = temp->next;
	
	free(temp);
}

