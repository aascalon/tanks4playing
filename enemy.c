#include "enemy.h"
#include <stdint.h>
#include <stdlib.h>

void spawnEnemy( node_t **L ){
	//make a new enemy struct
	enemy_t *newEnemy ;
	node_t *newNode;
	newEnemy =(enemy_t*) malloc(sizeof(enemy_t));
	
	newEnemy->xLoc = 20;
	newEnemy->yLoc = 20;
	newNode = (node_t*) malloc(sizeof(node_t));
	newNode->data = *newEnemy;
	//adds this enemy to the end of the linked list
	if (*L == NULL){
		*L = newNode;
	}
	else{
		node_t* last = (*L);
		while(last->next != NULL){
			last = last->next;
			}
		last->next = newNode;
	}
}




void deleteEnemy(uint8_t searchID, node_t **L){
}
