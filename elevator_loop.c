#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main () {

	int dir = 1;

	int i = 0;

	while( i < 10) {
		sleep(1);    
		
		printf("%d\n", i);
		
		if (dir == 1) {
			if(i != 9) {
				i++;
			} else { 
				dir = 0; 
				i--; }	
		} else {
			if(i != 0) {
				i--;
			} else {
				dir = 1;
				i++;
			}
		}		
	}

}
