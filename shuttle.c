#include<stdio.h>
#include<stdlib.h>
#include<time.h>

// Passenger Node Structure
struct Passenger_Struct{
    char passenger_type;
  
    int init_terminal;
    int dest_terminal;
    
    int seatSize;

    struct Passenger_Struct * next;
};

typedef struct Passenger_Struct Passenger;

// struct Terminal_Struct {
//     Passenger * head;
//     Passenger * tail;
// } t_default = {.head = NULL, .tail = NULL};

// typedef struct Terminal_Struct Terminal;

Passenger * terminalHead[5] = {NULL, NULL, NULL, NULL, NULL};
Passenger * terminalTail[5] = {NULL, NULL, NULL, NULL, NULL};

// Terminal * terminalHead[5] = {t_default, t_default, t_default, t_default, t_default};

Passenger * create_list(Passenger newPass, int terminal) {

    Passenger * ptr = (Passenger*)malloc(sizeof(Passenger));
    
    if(NULL == ptr) {
        printf("\n Node creation failed \n");
        return NULL;
    }

    ptr->passenger_type = newPass.passenger_type;
    ptr->init_terminal = newPass.init_terminal;
    ptr->dest_terminal = newPass.dest_terminal;
    ptr->seatSize = newPass.seatSize;

    ptr->next = NULL;

    terminalHead[terminal] = terminalTail[terminal] = ptr;

    return ptr;
}

Passenger * add_to_list(Passenger newPass, int terminal) {

    if (terminalHead[terminal] == NULL) {
        return create_list(newPass, terminal);
    }
    

    Passenger *ptr = (Passenger*)malloc(sizeof(Passenger));
    
    if(NULL == ptr) { printf("\n Node creation failed \n"); return NULL; }
    
    ptr->passenger_type = newPass.passenger_type;
    ptr->init_terminal = newPass.init_terminal;
    ptr->dest_terminal = newPass.dest_terminal;
    ptr->seatSize = newPass.seatSize;

    ptr->next = NULL;

    terminalTail[terminal]->next = ptr;
    terminalTail[terminal] = ptr;

    return ptr;
}

int delete_first(int terminal) {
    terminalHead[terminal] = terminalHead[terminal]->next;

    return 0;
}

void print_list(int terminal) {
    
    Passenger * ptr = terminalHead[terminal];

    printf("\n ------- Terminal %d ------- \n", terminal);

    while(ptr != NULL) {

        printf("\n [%c] [%d] [%d] [%d] \n", 
                ptr->passenger_type, 
                ptr->init_terminal, 
                ptr->dest_terminal, 
                ptr->seatSize);
        
        ptr = ptr->next;
    }

    printf("\n -------------------------- \n");

    return;
}

int main() {
    Passenger Shuttle[25];

    int terminal;

    while (1) {
        sleep(1);
        printf("HEY\n");
        // travels between Terminals



    }
    // unsigned int time_to_sleep = 10; // sleep 10 seconds
    // int count = 0;
    // int end = 50;

    // while(count != end) {
    //     time_to_sleep = sleep(time_to_sleep);
    //     printf("%d\n", count);
    //     count++;
    // }

    // char passType;
    // int initTerm, destTerm, seatSize;

    // // prompt user add passenger
    // printf(" ---- Add Passenger ---- ");
    // printf("Type : "); scanf("%c", passType);

    // if (passType == 'C') {
    //     seatSize = 
    // }

    // printf("Initial Terminal : "); scanf("%d", );
    // printf("Destination Terminal : "); scanf("%d", );


    // Passenger person1 = {.passenger_type = 'C', 
    //                     .init_terminal = 1, 
    //                     .dest_terminal = 2, 
    //                     .seatSize = 1,
    //                     .next = NULL };
   
    // Passenger person2 = {.passenger_type = 'A', 
    //                     .init_terminal = 1, 
    //                     .dest_terminal = 4, 
    //                     .seatSize = 2,
    //                     .next = NULL };

    // Passenger person3 = {.passenger_type = 'L', 
    //                     .init_terminal = 2, 
    //                     .dest_terminal = 1, 
    //                     .seatSize = 4,
    //                     .next = NULL };                        
    
    // add_to_list(person3, 2);
    // add_to_list(person1, 2);
    // add_to_list(person2, 2);
    
    // add_to_list(person3, 2);
    // add_to_list(person1, 3);
    // add_to_list(person2, 3);

    // add_to_list(person3, 5);
    // add_to_list(person1, 1);
    // add_to_list(person2, 2);

    // print_list(1);
    // print_list(2);
    // print_list(3);
    // print_list(4);
    // print_list(5);

    // delete_first(2);
    
    // print_list(2);

    return 0;
}