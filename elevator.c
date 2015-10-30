#include <linux/init.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/kernel.h> 
#include <asm-generic/uaccess.h>
#include "obj.h"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Evan Lee");
MODULE_AUTHOR("John Cyr");
MODULE_AUTHOR("Abe Omari");
MODULE_DESCRIPTION("A kernel module representing a simulated elevator system.");

#define KFLAGS (__GFP_WAIT | __GFP_IO | __GFP_FS)
#define ENTRY_NAME "elevator"
#define PERMS 0644
#define PARENT NULL

static struct file_operations fops;
static struct mutex m_elevator;
static struct mutex m_floor[MAX_FLOOR+1];
static char *message;
static int read_p;

extern long (*STUB_test_call)(int test_int);
extern long (*STUB_start_elevator)(void);
extern long (*STUB_issue_request)(int type, int start, int dest);
extern long (*STUB_stop_elevator)(void);

//////////////////////////////////////////////////////////////////////////////

int check_if_want_off(int currFloor){
	struct list_head *ptr;
	Person *person;

	list_for_each(ptr, &Passengers){
		person = list_entry(ptr, Person, list);
		if(person->dest == currFloor)
			return 1;
	}

	return 0;
}

// Assumes elevator only moves up or down one floor
int move_elevator(void){
	if(g_elevator.curr == 0 || g_elevator.curr == 9)
		g_elevator.dir = !g_elevator.dir;

	// MOVING UP
	if(g_elevator.dir){
		g_elevator.next++;
		g_elevator.status = 1;
	}
	else{
		g_elevator.next--;
		g_elevator.status = 2;
	}

	ssleep(FLOOR_MOVE_TIME);

	g_elevator.curr = g_elevator.next;
	g_elevator.status = 4;

	return 0;
}

//////////////////////////////////////////////////////////////////////////////

// Function returns a string for status of elevator
char* print_elevator_status(void) {
	char* msg;
	char value[3];
	char load[40]; 
	int unit1, unit2;
	Person *person;
	struct list_head *ptr;

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

	// Checks the Elevator Status variable and concatenates string
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
	if(g_elevator.status)
		strcat(msg, "ACTIVE\n");
	else if(!g_elevator.status && g_elevator.pass != 0)
		strcat(msg, "DEACTIVATING\n");
	else
		strcat(msg, "INACTIVE\n");

	strcat(msg, "CURRENT FLOOR:\t");
	sprintf(value, "%i\n", g_elevator.curr);
	strcat(msg, value);
	strcat(msg, "NEXT FLOOR:\t");
	sprintf(value, "%i\n", g_elevator.next);
	strcat(msg, value);
	strcat(msg, "LOAD:\t\t");

	sprintf(load, "%i passengers | %i.%i weight units\n", g_elevator.pass, unit1, unit2);
	strcat(msg, load);

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

// This function should not only print the status 
// of the elevator but also the state of each list
// for each floor of the building
char* print_building_status(void){
	int i;
	char *msg, *num, *header, el;
	Person *person;
	struct list_head *ptr;

	msg = kmalloc(sizeof(char) * 1024, KFLAGS);
	if (msg == NULL) {
		return NULL;
	}

	num = kmalloc(sizeof(char) * 5, KFLAGS);

	header = kmalloc(sizeof(char) * 20, KFLAGS);

	strcpy(msg, "== BUILDING ==\n");

	for(i = MAX_FLOOR; i >= 0; i--){
		if(g_elevator.curr == i)
			el = '*';
		else
			el = ' ';

		sprintf(header, "|%c|[%i]: ", el, i+1);
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

int elevator_open(struct inode *sp_inode, struct file *sp_file){
	read_p = 1;
	message = kmalloc(sizeof(char) * 5096, KFLAGS);
	if (message == NULL) {
		return -ENOMEM;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////

ssize_t elevator_read(struct file *sp_file, char __user *buf, size_t size, loff_t *offset){
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

int elevator_release(struct inode *sp_inode, struct file *sp_file){
	kfree(message);
	return 0;
}

long my_test_call(int test) {
	printk("%s: Your int is %i\n", __FUNCTION__, test);
	return test;
}

long start_elevator(void) {
	printk("Attemping to activate elevator.\n");
	if(g_elevator.active){
		printk("The elevator is already active.\n");
		return 1;
	}
	else{
		g_elevator.active = 1;
		printk("Elevator activated successfully.");
		return 0;
	}

}

long issue_request(int type, int start, int dest) {
	Person *person;

	--start;
	--dest;

	if(type < 0 || type > 3)
		return 1;

	if(start < 0 || start > 9)
		return 1;

	if(dest < 0 || dest > 9)
		return 1;

	person = kmalloc(sizeof(Person), KFLAGS);
	person->type = type;
	person->start = start;
	person->dest = dest;

	list_add_tail(&person->list, &Bldg[start]);
	
	printk("Passenger Added to Building: %i, %i, %i\n", type, start, dest);

	return 0;
}

long stop_elevator(void) {
	printk("Attemping to stop elevator.\n");
	if(!g_elevator.active && g_elevator.pass != 0){
		printk("The elevator is currently deactiving.\n");
		return 1;
	}
	else{
		g_elevator.active = 0;
		printk("Elevator stopped successfully!.\n");
		return 0;
	}

}

//////////////////////////////////////////////////////////////////////////////

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

	for(i = 0; i <= MAX_FLOOR; i++){
		INIT_LIST_HEAD(&Bldg[i]);
		mutex_init(&m_floor[i]);
	}

	INIT_LIST_HEAD(&Passengers);
	mutex_init(&m_elevator);


	STUB_test_call = &(my_test_call);
	STUB_start_elevator = &(start_elevator);
	STUB_issue_request = &(issue_request);
	STUB_stop_elevator = &(stop_elevator);

	return 0;
}

//////////////////////////////////////////////////////////////////////////////

static void elevator_exit(void){
	int i;
	Person *person;
	struct list_head *ptr, *temp;

	remove_proc_entry(ENTRY_NAME, NULL);

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
