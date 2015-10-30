#ifndef __OBJ_H
#define __OBJ_H

#include <linux/list.h>

#define MAX_WEIGHT 16
#define MAX_PASS 8
#define MIN_FLOOR 0
#define MAX_FLOOR 9
#define FLOOR_MOVE_TIME 2
#define LOAD_TIME 1

typedef struct Person {
	int type;	// 0 for child
				// 1 for adult
				// 2 for bellhop
				// 3 for room service
	int start;
	int dest;
	struct list_head list;
} Person;

typedef struct {
	int pass;	// Number of passengers
	int weight; // Total weight
	int dir;	// 0 FOR DOWN
				// 1 FOR UP
	int curr;	// Current floor
	int next;	// Next floor
	int status;	// 0 for idle,
				// 1 for UP
				// 2 for DOWN
				// 3 for LOADING
				// 4 for STOPPED
	int active;	// 0 for inactive
				// 1 for active
} Elevator;

Elevator g_elevator;

static struct list_head Bldg[10];
static struct list_head Passengers;

#endif
