#include <stdio.h>
#include "usl.h"


struct usl_timer *setup_timer[256];
static int counter = 0;

void expired(void* arg)
{
    int i = *((int *)arg);
    counter++;
    printf("Calling expired %d\n",i);
}

int main(void)
{
    int i;

    printf("Hello World!\n");

//[1] Timer system init
    usl_signal_init();
    usl_fd_init();
    if(usl_timer_init()< 0){
        printf("Failed to init timer \n");
    }

//[2] Create Timers for testing
    for(i=0;i<128;i++)
    {
        setup_timer[i] = usl_timer_create(USL_TIMER_TICKS(1),0,expired,(void *)(&i),NULL,TIMER_LOOP_ENABLE);
        usl_timer_is_running(setup_timer[i]);
    }
    for(i=128;i<256;i++)
    {
        setup_timer[i] = usl_timer_create(USL_TIMER_TICKS(1),0,expired,(void *)(&i),NULL,TIMER_LOOP_DISABLE);
        usl_timer_is_running(setup_timer[i]);
    }
    while((counter/256) < 10)
    {
        usl_main_loop();
    }
//[3] Terminate all timers
    for(i=0;i<256;i++)
    {
        usl_timer_delete(setup_timer[i]);
    }

//[4] Stop timer system
    usl_timer_cleanup();

    return 0;
}

