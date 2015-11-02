#ifndef __OBJ_H
#define __OBJ_H

#include <linux/list.h>

#define MAX_WEIGHT 16
#define MAX_PASS 8
#define MIN_FLOOR 0
#define MAX_FLOOR 9
#define FLOOR_MOVE_TIME 2
#define LOAD_TIME 1

// Floor structure
typedef struct {
	int num;
	int weight;
	int serviced;
} Floor;

// Person structure
typedef struct {
	int type;	// 0 for child
				// 1 for adult
				// 2 for bellhop
				// 3 for room service
	int start;	// Start floor
	int dest;	// End floor
	struct list_head list;	// List_head for the Person list
} Person;

// Elevator structure
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

//Array of 10 floors
static struct list_head Bldg[10];
static struct list_head Passengers;

//Holds passenger value, weight, and number serviced
static Floor floor_data[10];

#endif
