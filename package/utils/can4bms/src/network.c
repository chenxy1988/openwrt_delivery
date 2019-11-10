#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <libgen.h>
#include <time.h>
#include <errno.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <pthread.h>
#include "network.h"
#include "protocol.h"

static void network_interface_init()
{
    system("ip link set can0 down");
    system("ip link set can0 type can bitrate 250000");
    system("ip link set can0 up");
}

int network_send_data(int fd,unsigned short int can_id,unsigned char *data,unsigned char len,unsigned int frame_type)
{
	struct can_frame frame;

	if(frame_type == NORMAL_FRAME)
		frame.can_id = can_id;
	else if(frame_type == EXT_FRAME)
		frame.can_id = CAN_EFF_FLAG | can_id;
	else{
		printf("network_send_data, error type of frame_type");
		return NET_RET_FAILED;
	}

    	memcpy(frame.data,data,len);
	frame.can_dlc = len;
	if( write(fd, &frame, sizeof(frame)) == sizeof(frame))
		return NET_RET_OK;
	else
		return NET_RET_FAILED;
}

int network_read_data(int fd, unsigned short int *can_id,unsigned char *len,unsigned char *data)
{
	struct can_frame frame;
    int rx;

	if(!can_id || !len || !data){
		printf("network_read_data : error arg type \n");
		return NET_RET_FAILED;
	}

    rx = read(fd,&frame,sizeof(frame));
    if(rx != sizeof(frame)){
		perror("Faild to read data \n");
		return NET_RET_FAILED;
	}
	*can_id = frame.can_id;
	*len = frame.can_dlc;
	memcpy(data,frame.data,frame.can_dlc);

	return NET_RET_OK;
}

int network_create_socket(char *can_if)
{
	struct sockaddr_can addr;
	struct ifreq ifr;
	int fd;
	int ret = NET_RET_FAILED;

	fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);//Create socket fd
	if(fd <= 0){
		perror("network_create_socket \n");
		return ret;
	}

	//For debug
	strcpy(ifr.ifr_name,"can0");
	if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0){
		perror("network_create_socket, failed to set interface name \n");
		return ret;
	}

	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0)// Bind socket fd with CAN interface
	{
		perror("network_create_socket,failed to bind can interface with socket fd \n");
		return ret;
	}

	return fd;
}

static void *task_network_rx(void *arg)
{
    unsigned short canid;
    unsigned char len;
    int fd;
    printf("[CAN_MODULE]: RX TASK START fd = %d...\n",(int)arg);
    fd = (int)arg;
    while(1)
    {
        unsigned char rx[8];
        memset(rx,0x0,sizeof(rx));


        network_read_data(fd,&canid,&len,rx);

        printf("Recv data from CAN: canid = 0x%x,len = %d,",canid,len);
        int i;
        for(i=0;i<len;i++)
            printf("0x%02x,",rx[i]);
        printf("\n");
	
	can_decompose(fd,canid,rx,len);
    }
}

int network_terminate(int fd)
{
	return close(fd);
}

int network_test()
{
    int fd;
    unsigned char tx[4] = {'T','E','S','T'};

    network_interface_init();

    fd = network_create_socket("can0");
    pthread_t pid;
    pthread_create(&pid,NULL,task_network_rx,(void *)fd);
    network_send_data(fd,0x4200,tx,4,EXT_FRAME);

    pthread_join(pid,NULL);

    close(fd);
}
