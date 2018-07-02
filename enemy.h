#include <stdint.h>
#include <stdio.h>
typedef struct enemy{
	uint8_t xLoc, yLoc;
} enemy_t;

typedef struct node{
	enemy_t data;
	struct node *next;
} node_t;

void spawnEnemy(node_t** );



void deleteEnemy(uint8_t, node_t**);
