#ifndef __NETWORK_H__
#define __NETWORK_H__

#define NETWORK_IF_NAME	"can0"

#define NORMAL_FRAME	1
#define EXT_FRAME	2

#define NET_RET_OK	0
#define NET_RET_FAILED	-1
int network_test()
;
int network_send_data(int fd,unsigned short int can_id,unsigned char *data,unsigned char len,unsigned int frame_type);
#endif
