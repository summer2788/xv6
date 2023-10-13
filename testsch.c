#include "types.h"
#include "stat.h"
#include "user.h"

int main(void){

    int parent = getpid();
    int child;

    printf(1, "====================Project 3 Test====================\n\n");
   
    printf(1, "=================== Initial table ====================\n");
    ps(0);
    printf(1,"\n");

    setnice(1,5);
    setnice(2,5);
    setnice(parent,5);

    child = fork();
    if (child < 0) {
        printf(1,"fork error!");
    }
    if(child==0){
        setnice(parent,39);
	printf(1,"child ! \n\n");
        ps(0);
        printf(1,"\n");
        exit();
    }
    else{
        setnice(child,39);
	printf(1,"parent ! \n\n");
        ps(0);
        printf(1,"\n");
        wait();
    }
    
    printf(1,"\n");

    printf(1, "===================== After fork ====================\n");
    
    
    setnice(4,0);
    ps(0);

    exit();
}
