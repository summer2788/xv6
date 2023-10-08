#include "types.h"
#include "stat.h"
#include "user.h"


int main(void) {
    int pid;

    printf(1, "Testing the getnice, setnice, and ps system calls\n\n");

    printf(1, "Before changing nice value:\n");
    ps(0);

    pid = getpid();

    printf(1, "\nSetting the nice value of current process (PID: %d) to 10\n", pid);
    if(setnice(pid, 10) == 0) {
        printf(1, "Set nice value successfully.\n");
    } else {
        printf(1, "Failed to set nice value.\n");
        exit();
    }

    printf(1, "\nAfter changing nice value:\n");
    ps(0);

    printf(1, "\nGetting the nice value of current process (PID: %d)\n", pid);
    int value = getnice(pid);
    if(value >= 0) {
        printf(1, "Nice value of PID %d: %d\n", pid, value);
    } else {
        printf(1, "Failed to get nice value.\n");
    }  


    printf(1, "call the ps given the pid 5(it should print nothing):\n");
    ps(5);

    printf(1, "getnice of pid 0 process(it should return -1):\n");
    value=getnice(0);
    printf(1,"return value is %d\n", value);	

    exit();
} 
