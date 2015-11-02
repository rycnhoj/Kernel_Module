/**
	ELEVATOR.C
	COP4610
	Project 2
	Group 22

	Members:
	Evan Lee
	Abe Omari
	John Cyr

	This source file provides most of the driving
	functionality for the simulated elevator kernel
	module.
*/

#include <linux/delay.h>	// ssleep()
#include <linux/init.h>		// initialization
#include <linux/kernel.h> 	// kernel functions
#include <linux/kthread.h>	// kthreading
#include <linux/list.h>		// linked lists
#include <linux/module.h>	// module linking
#include <linux/mutex.h>	// mutual exclusion
#include <linux/proc_fs.h>	// proc writing
#include <linux/slab.h>		// kmalloc/kfree
#include <linux/string.h>	// string functions
#include <asm-generic/uaccess.h>
#include "obj.h"			// holds data types and declarations

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Evan Lee");
MODULE_AUTHOR("John Cyr");
MODULE_AUTHOR("Abe Omari");
MODULE_DESCRIPTION("A kernel module representing a simulated elevator system.");

#define KFLAGS (__GFP_WAIT | __GFP_IO | __GFP_FS)	// kmalloc flags
#define ENTRY_NAME "elevator"						// Module name
#define PERMS 0644									// File permissions
#define PARENT NULL			

static struct task_struct *op_elevator;		// Creates a kthread for the 
											// elevator
static struct file_operations fops;			// A struct for file operations
static struct mutex m_elevator;				// Mutex lock for elevator
static struct mutex m_floor[MAX_FLOOR+1];	// Mutex lock for each floor
static char *message;						// Global message containers
static int read_p;

//Function pointers
extern long (*STUB_start_elevator)(void);
extern long (*STUB_issue_request)(int type, int start, int dest);
extern long (*STUB_stop_elevator)(void);

//////////////////////////////////////////////////////////////////////////////

/* CHECK IF VALID ROOM */
// Return 1 if there is room, 0 otherwise
int check_if_valid_room(void){
	if(!g_elevator.active)
		return 0;
	if(g_elevator.pass >= MAX_PASS)
		return 0;
	if(g_elevator.weight >= MAX_WEIGHT)
		return 0;
	return 1;
}

/* CHECK IF WANT OFF*/
//Returns 1 if someone wants to get off, 0 if not
int check_if_want_off(void){
	struct list_head *ptr;
	Person *person;

	if(list_empty(&Passengers))
		return 0;

	list_for_each(ptr, &Passengers){
		person = list_entry(ptr, Person, list);
		if(person->dest == g_elevator.curr)
			return 1;
	}

	return 0;
}

/* CHECK IF WANT ON */
// Returns 1 if someone wants on the elevator
// 0 otherwise
int check_if_want_on(void){
	if(list_empty(&Bldg[g_elevator.curr]))
		return 0;
	return 1;
}

/* UNLOAD PASSENGERS */
//Removes passengers from elevator when the elevator reaches their desired floor
void unload_passengers(void){
	Person *person;
	struct list_head *ptr, *temp;
	int w;
	int p;

	if(!list_empty(&Passengers)){	
		w = 0;
		p = 0;
		//Checks if anyone on elevator wants to get off,
		// and if they do it removes them from the elevator list and deletes 
		// them
		list_for_each_safe(ptr, temp, &Passengers){
			person = list_entry(ptr, Person, list);
			if(person->dest == g_elevator.curr){
				printk("Removing a passenger! From %i, to floor: %i\n", person
					->dest, g_elevator.curr);
				switch(person->type){
				case 0: 
					p = 1;
					w = 1;
					break;
				case 1:
					p = 1;
					w = 2;
					break;
				case 2:
					p = 2;
					w = 4;
					break;
				case 3:
					p = 1;
					w = 4;
					break;
				}
				list_del(ptr);
				kfree(person);
				g_elevator.pass -= p;
				g_elevator.weight -= w;
				floor_data[g_elevator.curr].serviced += p;
			}
		}
	}
}

/* LOAD PASSENGERS */
// If there is room on the elevator, checks each passenger on the floor
// against the weight limit, and lets them on if valid
void load_passengers(void){
	Person *person;
	struct list_head *ptr, *tmp;
	int p, w;

	// Iterates safely through the list of Persons on the floor
	list_for_each_safe(ptr, tmp, &Bldg[g_elevator.curr]){
		p = 0;
		w = 0;
		// If there is no more valid room, return
		if (!check_if_valid_room())
			return;
		person = list_entry(ptr, Person, list);
		switch(person->type){
		case 0: 
			p = 1;
			w = 1;
			break;
		case 1:
			p = 1;
			w = 2;
			break;
		case 2:
			p = 2;
			w = 4;
			break;
		case 3:
			p = 1;
			w = 4;
			break;
		}
		// Weight checker: checks if adding this passenger
		// would make the elevator go over its weight limit
		if(MAX_WEIGHT >= (w + g_elevator.weight)){
			g_elevator.pass += p;
			g_elevator.weight += w;
			floor_data[g_elevator.curr].num -= p;
			floor_data[g_elevator.curr].weight -= w;
			printk("About to add passenger...\n");
			list_move_tail(&person->list, &Passengers);
			printk("Added passenger!\n");
		}
	}
}

/* MOVE ELEVATOR */
//Unloads passengers, loads new passengers, reverses the direction if needed, 
//and moves the elevator to the next floor 
int move_elevator(void *data){
	while(!kthread_should_stop()){

		//Checks if passengers want to get off, locks the elevator, and 
		//unloads them
		mutex_lock_interruptible(&m_elevator);
		g_elevator.status = 4;
		if(check_if_want_off()){
			g_elevator.status = 3;
			unload_passengers();
			ssleep(LOAD_TIME);
		}
		mutex_unlock(&m_elevator);
		//Unlock elevator

		//Lock elevator and floor, check if we have room and/or people want to 
		//get on the levator, load them
		mutex_lock_interruptible(&m_elevator);
		mutex_lock_interruptible(&m_floor[g_elevator.curr]);
		if(check_if_valid_room() && !list_empty(&Bldg[g_elevator.curr])){;
			ssleep(LOAD_TIME);
			g_elevator.status = 3;
			load_passengers();
		}
		else if (!check_if_valid_room()){
			printk("No room!\n");
		}
		else if(list_empty(&Bldg[g_elevator.curr])){
			printk("Empty floor!\n");
		}
		mutex_unlock(&m_floor[g_elevator.curr]);
		mutex_unlock(&m_elevator);
		//Unlock elevator and floor

		//Changes direction elevator is traveling in
		mutex_lock_interruptible(&m_elevator);
		if(g_elevator.curr == 0 || g_elevator.curr == 9)
		{
			printk("Changing Direction!\n");
			g_elevator.dir = !g_elevator.dir;
		}

		if(g_elevator.dir){
			g_elevator.next++;
			g_elevator.status = 1;
		}
		else{
			g_elevator.next--;
			g_elevator.status = 2; 
		}

		//Unlock the elevator, move it, Lock it
		printk("Moving from %i to %i...\n", g_elevator.curr, g_elevator.next);
		mutex_unlock(&m_elevator);
		ssleep(FLOOR_MOVE_TIME);
		mutex_lock_interruptible(&m_elevator);
		printk("Moved from %i to %i...\n", g_elevator.curr, g_elevator.next);

		//Update current floor
		g_elevator.curr = g_elevator.next;
		g_elevator.status = 4; 
		mutex_unlock(&m_elevator);
	}


	//Very similar to above code, except this is called when we want to stop 
	//the elevator
	mutex_lock_interruptible(&m_elevator);
	g_elevator.active = 0;
	mutex_unlock(&m_elevator);

	while(g_elevator.pass > 0) {
		mutex_unlock(&m_elevator);
		mutex_lock_interruptible(&m_elevator);
		g_elevator.status = 4; 

		if(check_if_want_off()){
			g_elevator.status = 3;
			unload_passengers();
			ssleep(LOAD_TIME);
		}
		mutex_unlock(&m_elevator);

		if(g_elevator.pass > 0){
			mutex_lock_interruptible(&m_elevator);
			if(g_elevator.curr == 0 || g_elevator.curr == 9) {
				printk("Changing Direction!\n");
				g_elevator.dir = !g_elevator.dir;
			}
			
			if(g_elevator.dir){
				g_elevator.next++;
				g_elevator.status = 1; 
			}
			else{
				g_elevator.next--;
				g_elevator.status = 2; 
			}
			mutex_unlock(&m_elevator);
			printk("Moving from %i to %i...\n", g_elevator.curr, g_elevator.
				next);
			mutex_unlock(&m_elevator);
			ssleep(FLOOR_MOVE_TIME);
			mutex_lock_interruptible(&m_elevator);
			printk("Moved from %i to %i...\n", g_elevator.curr, g_elevator.next
				);
			g_elevator.curr = g_elevator.next;
			g_elevator.status = 4; 
			mutex_unlock(&m_elevator);
		}
	}
	printk("Elevator stopped successfully!.\n");
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

/* PRINT ELEVATOR STATUS */
// Returns a string for status of elevator
// Message contains
//	STATE
//	CURRENT FLOOR
//	NEXT FLOOR
//	LOAD (number of passengers | weight units)
//	LIST OF PASSENGERS
char* print_elevator_status(void) {
	char* msg;
	char value[3];
	char load[40]; 
	int unit1, unit2;
	Person *person;
	struct list_head *ptr;

	// Calculates the weight using integer division and remainder
	unit1 = g_elevator.weight/2;
	unit2 = 0;

	if(g_elevator.weight % 2 != 0)
		unit2 = 5;

	msg = kmalloc(sizeof(char) * 1024, KFLAGS);
	
	if (msg == NULL) { 
		return NULL;
	}
	
	strcpy(msg, "== ELEVATOR ==\n");
	strcat(msg, "STATE:\t\t");

	// Changes the status message based on underlying data
	switch(g_elevator.status){
		case 0:
			strcat(msg, "IDLE|");
			break;
		case 1:
			strcat(msg, "UP|");
			break;
		case 2:
			strcat(msg, "DOWN|");
			break;
		case 3:
			strcat(msg, "LOADING|");
			break;
		case 4:
			strcat(msg, "STOPPED|");
			break;
		default:
			return NULL;
	}

	// Changes the activity message based on underlying data
	if(g_elevator.active)
		strcat(msg, "ACTIVE\n");
	else if(!g_elevator.active && g_elevator.pass != 0)
		strcat(msg, "DEACTIVATING\n");
	else
		strcat(msg, "INACTIVE\n");

	strcat(msg, "CURRENT FLOOR:\t");
	sprintf(value, "%i\n", g_elevator.curr+1);
	strcat(msg, value);
	strcat(msg, "NEXT FLOOR:\t");
	sprintf(value, "%i\n", g_elevator.next+1);
	strcat(msg, value);
	strcat(msg, "LOAD:\t\t");

	sprintf(load, "%i passengers | %i.%i weight units\n", g_elevator.pass, 
		unit1, unit2);
	strcat(msg, load);

	// Prints out the list of passengers based on type
	strcat(msg, "PASSENGERS:\t");
	if(list_empty(&Passengers)){
		strcat(msg, "EMPTY\n");
	}
	else{
		list_for_each(ptr, &Passengers){
			person = list_entry(ptr, Person, list);
			sprintf(value, "%i ", person->type);
			strcat(msg, value);
		}
		strcat(msg, "\n");
	}

	return msg;
}

//////////////////////////////////////////////////////////////////////////////

/* PRINT BUILDING STATUS */
// This function prints the status of the elevator but also the state of each 
// list for each floor of the building
//
// Outputs Floor number, number of passenger value currently on the floor, the 
// the weight value of the people on the floor, and the total value of the 
// passengers who have been serviced to date

char* print_building_status(void){
	int i, unit1, unit2;
	char *msg, *num, *header, el;
	Person *person;
	struct list_head *ptr;

	//Allocates space for message
	msg = kmalloc(sizeof(char) * 1024, KFLAGS);
	if (msg == NULL) {
		return NULL;
	}
	num = kmalloc(sizeof(char) * 5, KFLAGS);
	header = kmalloc(sizeof(char) * 40, KFLAGS);

	strcpy(msg, "== BUILDING ==\n");

	//Iterates through each floor of the building and prints the people on it
	for(i = MAX_FLOOR; i >= 0; i--){
		unit1 = floor_data[i].weight/2;
		unit2 = 0;
		if(floor_data[i].weight % 2 != 0)
			unit2 = 5;

		if(g_elevator.curr == i)
			el = '*';
		else
			el = ' ';

		if(i == MAX_FLOOR)
			sprintf(header, "|%c|[%i] #-%i | W-%i.%i | S-%i: ",
				el, i+1, floor_data[i].num,
				unit1, unit2, floor_data[i].serviced);
		else
			sprintf(header, "|%c|[%i]  #-%i | W-%i.%i | S-%i: ",
				el, i+1, floor_data[i].num,
				unit1, unit2, floor_data[i].serviced);

		strcat(msg, header);
		list_for_each(ptr, &Bldg[i]){
			person = list_entry(ptr, Person, list);
			sprintf(num, "%i ", person->type);
			strcat(msg, num);
		}
		strcat(msg, "\n");
	}

	return msg;
}

//////////////////////////////////////////////////////////////////////////////

/* ELEVATOR OPEN */
// Run upon creating the proc file
// Allocates memory to the global message container
int elevator_open(struct inode *sp_inode, struct file *sp_file){
	read_p = 1;
	message = kmalloc(sizeof(char) * 5096, KFLAGS);
	if (message == NULL) {
		return -ENOMEM;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////

/* ELEVATOR READ */
// Called whenever the /proc/elevator file is accessed
// Prints statuses of the elevator and building

ssize_t elevator_read(struct file *sp_file, char __user *buf, size_t size, 
	loff_t *offset){
	int len;

	message = kmalloc(sizeof(char) * 1024, KFLAGS);
	if (message == NULL) {
		return -ENOMEM;
	}
	read_p = !read_p;
	if (read_p){
		return 0;
	}

	strcpy(message, print_elevator_status());
	strcat(message, print_building_status());
	len = strlen(message);
	copy_to_user(buf, message, len);
	kfree(message);

	return len;

}

//////////////////////////////////////////////////////////////////////////////

/* ELEVATOR RELEASE */
// Called upon releasing proc file
int elevator_release(struct inode *sp_inode, struct file *sp_file){
	kfree(message);
	return 0;
}

/* START ELEVATOR */
// Makes the elevator active if it isn't already
// Returns 0 if the elevator was successfully started
// Returns 1 if the elevator was already started
// Returns error if there is an error
long start_elevator(void) {
	printk("Attemping to activate elevator.\n");
	//Locks the elevator
	mutex_lock_interruptible(&m_elevator);
	//If the elevator is already active, it does nothing and unlocks the 
	//elevator
	if(g_elevator.active){
		printk("The elevator is already active.\n");
		mutex_unlock(&m_elevator);
		return 1;
	}
	else{
		//If the elevator isn't already active, it initializes it's values, 
		//and creates and executes a kthread, then unlocks the elevator
		g_elevator.active = 1;
		g_elevator.status = 0;
		g_elevator.pass = 0;
		g_elevator.weight = 0;
		g_elevator.curr = 1;
		g_elevator.next = 1;
		g_elevator.dir = 1;
		op_elevator = kthread_run(move_elevator, NULL, "move");
		if(IS_ERR(op_elevator)){
			printk("ERROR: Elevator operation.\n");
			return PTR_ERR(op_elevator);
		}
		printk("Elevator activated successfully.");
		mutex_unlock(&m_elevator);
		return 0;
	}

}

/* ISSUE REQUEST */
// Accepts a person request from the producer program.
// Return 1 if any parameters are invalid.
// Otherwise, adds the person to the correct floor and returns 0
long issue_request(int type, int start, int dest) {
	Person *person;
	int w, p;

	--start;
	--dest;

	// Bounds checking
	if(type < 0 || type > 3)
		return 1;

	if(start < 0 || start > 9)
		return 1;

	if(dest < 0 || dest > 9)
		return 1;

	// Creates a temporary person container
	person = kmalloc(sizeof(Person), KFLAGS);
	person->type = type;
	person->start = start;
	person->dest = dest;

	if(type == 3)
		p = 2;
	else
		p = 1;

	// Changes the weight value based on the person type
	switch(person->type){
		case 0: w = 1; break;
		case 1: w = 2; break;
		case 2:
		case 3: w = 4; break;
	}

	// Locks the specific floor, adds the person, updates floor data, and 
	// unlocks
	printk("floor lock start!\n");
	mutex_lock_interruptible(&m_floor[start]);
	list_add_tail(&person->list, &Bldg[start]);
	floor_data[start].num+=p;
	floor_data[start].weight+=w;
	mutex_unlock(&m_floor[start]);

	printk("Passenger Added to Building: %i, %i, %i\n", type, start, dest);

	return 0;
}

/* STOP ELEVATOR */
//Stops the kthread running the elevator
long stop_elevator(void) {
	int ret;
	printk("Attemping to stop elevator.\n");

	//Lock elevator, deactivate it, unlock it
	mutex_lock_interruptible(&m_elevator);
	g_elevator.active = 0;
	g_elevator.status = 0;
	mutex_unlock(&m_elevator);

	ret = kthread_stop(op_elevator);
	if(ret != -EINTR)
		printk("Elevator thread has been instructed to stop.\n");
	return 0;

}

//////////////////////////////////////////////////////////////////////////////

/* ELEVATOR INIT */
// Called upon module install
// Initializes the elevator and building to zeroed values
static int elevator_init(void){
	int i;

	fops.open = elevator_open;
	fops.read = elevator_read;
	fops.release = elevator_release;

	if (!proc_create(ENTRY_NAME, PERMS, NULL, &fops)){
		printk("ERROR: proc creation\n");
		remove_proc_entry(ENTRY_NAME, NULL);
		return -ENOMEM;
	}

	// Initializes all floors to empty lists
	// Initializes all floor mutexes
	for(i = 0; i <= MAX_FLOOR; i++){
		INIT_LIST_HEAD(&Bldg[i]);
		mutex_init(&m_floor[i]);
	}

	INIT_LIST_HEAD(&Passengers);
	mutex_init(&m_elevator);

	// Initializes all floor data to 0
	for (i = 0; i <= MAX_FLOOR; i++) {
		floor_data[i].num = 0;
		floor_data[i].weight = 0;
		floor_data[i].serviced = 0;
	}

	// Links syscalls to this module
	STUB_start_elevator = &(start_elevator);
	STUB_issue_request = &(issue_request);
	STUB_stop_elevator = &(stop_elevator);

	return 0;
}

//////////////////////////////////////////////////////////////////////////////

/* ELEVATOR EXIT */
// Removes people from the building and the elevator
static void elevator_exit(void){
	int i;
	Person *person;
	struct list_head *ptr, *temp;

	remove_proc_entry(ENTRY_NAME, NULL);

	// Removes the people each floor of the building
	for(i = 0; i <= MAX_FLOOR; i++){
		list_for_each_safe(ptr, temp, &Bldg[i]){
			person = list_entry(ptr, Person, list);
			list_del(ptr);
			kfree(person);
		}
	}

	// Clears list of elevator passengers
	list_for_each_safe(ptr, temp, &Passengers){
		person = list_entry(ptr, Person, list);
		list_del(ptr);
		kfree(person);
	}
}

//////////////////////////////////////////////////////////////////////////////

module_init(elevator_init);
module_exit(elevator_exit);