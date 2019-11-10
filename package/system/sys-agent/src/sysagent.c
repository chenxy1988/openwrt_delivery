#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>


#define MEMORY_PATH "/dev/mem"
#define FPGA_BASE_ADDR	0x40000000
#define FPGA_REG_SIZE	64

#define MAGIC_HEAD 0xff0055aa
#define MAGIC_TAIL 0xff55aa00

#define LISTEN_PORT	9000

typedef struct prefix_s{
        unsigned int magic_head;
        unsigned int debug_mode;
        unsigned int counter;
        unsigned int reboot_reason;
        unsigned int magic_tail;
}PREFIX_T;

typedef struct bank_s{
        unsigned int magic_head;
        unsigned int current;
        unsigned int bank_1_version;
        unsigned int bank_2_version;
        unsigned int magic_tail;
}BANK_T;

static volatile void * baseaddr = NULL;
static int memfd = -1;

static int fpga_addr_mapping(void)
{
	memfd = open(MEMORY_PATH, O_RDWR | O_SYNC);
	if(memfd < 0){
		perror("fpga_addr_mapping: failed to open /dev/mem \n");
		return -1;
	}

	baseaddr = mmap(NULL,FPGA_REG_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED, memfd,FPGA_BASE_ADDR);
	if(baseaddr == NULL){
		perror("fpga_addr_mapping: failed to map the address \n");
		close(memfd);
		return -1;
	}

	return memfd;
}

static void fpga_addr_unmapping(void)
{
	munmap((void *)baseaddr,FPGA_REG_SIZE);
	close(memfd);
	baseaddr = NULL;
	memfd = -1;

}

static void fpga_sync(void)
{
	msync((void *)baseaddr,FPGA_REG_SIZE,MS_SYNC);
}

static unsigned int fpga_read(unsigned char reg)
{
	unsigned char addr = reg * 4;
	unsigned int *p;

	fpga_sync();
	p = (unsigned int *)(baseaddr+addr);
	return *p;
}

static void fpga_write(unsigned char reg,unsigned int val)
{
	unsigned char addr = reg * 4;
	unsigned int *p;

	p = (unsigned int *)(baseaddr+addr);
	*p = val;
	fpga_sync();

}

static int fpga_selftest(void)
{
	unsigned int val;
	fpga_write(0,0xffaa55dd);
	val = fpga_read(0);

	if(val != 0xffaa55dd){
		printf("FPGA Selftest failed,val=%x ,should be 0xffaa55dd, seems wrong FPGA image \n",val);
		return -1;
	}

	return 0;
}

static unsigned char bin2bcd(unsigned val)
{
	return ((val / 10) << 4) + val % 10;
}

static unsigned bcd2bin(unsigned char val)
{
	return (val & 0x0f) + (val >> 4) * 10;
}

static int fpga_rtc_set(void)
{
	time_t timep;
	struct tm *p;

	time(&timep);
	p = localtime(&timep);
	printf("%d-%d-%d %d:%d:%d\n", (1900 + p->tm_year), ( 1 + p->tm_mon), p->tm_mday,
                               (p->tm_hour), p->tm_min, p->tm_sec);


	fpga_write(0x8,0);
	fpga_write(0x9,2);
	fpga_write(10,bin2bcd(p->tm_sec));
	fpga_write(10,bin2bcd(p->tm_min));	
	fpga_write(10,bin2bcd(p->tm_hour));	
	fpga_write(10,bin2bcd(p->tm_mday));	
	fpga_write(10,p->tm_wday % 0x7);	
	fpga_write(10,bin2bcd(p->tm_mon+1));	
	fpga_write(10,bin2bcd((p->tm_year)));
	fpga_write(12,1);
	fpga_sync();

	return 0;
}

static int fpga_rtc_get(void)
{
	struct tm _tm;
	struct timeval tv;
	time_t timep;

	fpga_write(0x8,1);
	fpga_write(0x9,2);
	fpga_write(10,163);
	fpga_write(12,1);
	fpga_sync();
	sleep(1);

	while(fpga_read(11) != 2){
	        fpga_write(0x8,1);
	        fpga_write(0x9,2);
	        fpga_write(10,163);
	        fpga_write(12,1);
	        fpga_sync();
	        sleep(1);
	}


	_tm.tm_sec = bcd2bin((unsigned char)fpga_read(13) & 0x7f);
	_tm.tm_min = bcd2bin((unsigned char)fpga_read(14) & 0x7f);
	_tm.tm_hour = bcd2bin((unsigned char)fpga_read(15) & 0x3f);
	_tm.tm_mday = bcd2bin((unsigned char)fpga_read(16) & 0x3f);
	fpga_read(17);
	_tm.tm_mon = bcd2bin((unsigned char)fpga_read(18) & 0x1f) -1;
	_tm.tm_year = bcd2bin((unsigned char)fpga_read(19));

	timep = mktime(&_tm);  
	tv.tv_sec = timep;  
	tv.tv_usec = 0;  
	if(settimeofday (&tv, (struct timezone *) 0) < 0)  
	{  
		printf("Set system datatime error!/n");  
		return -1;  
	}  
	return 0;
}



static void dual_bank_init(void)
{
	system("mkdir -p /tmp/tmp-master");
	system("mount /dev/mmcblk0p1 /tmp/tmp-master");

}

static void dual_bank_terminate(void)
{
	system("umount /tmp/tmp-master");
	system("rm -fr /tmp/tmp-master");
}

static void set_boot_ok_flag(void)
{
	int fd = -1;
	unsigned char prefix_buf[64];

	memset(prefix_buf,0x0,sizeof(prefix_buf));
	PREFIX_T *prefix = (PREFIX_T *)prefix_buf;

	fd = open("/tmp/tmp-master/prefix.bin",O_RDWR | O_SYNC);
	if(fd < 0){
		printf("Failed to open prefix.bin \n");
		return;
	}

	if((read(fd,prefix_buf,sizeof(PREFIX_T))) != sizeof(PREFIX_T)){
		printf("Error in read the prefix flags, \n");
		close(fd);
		return ;
	}

	if((prefix->magic_head != MAGIC_HEAD) || (prefix->magic_tail != MAGIC_TAIL)){
		printf("No available prefix header and tail find \n");
		printf("prefix->magic_head = %x,prefix->magic_tail = %x \n",prefix->magic_head,prefix->magic_tail);
		close(fd);
		return;
	}

	prefix->counter = 1;

	lseek(fd,0x0,SEEK_SET);

	if((write(fd,prefix_buf,sizeof(PREFIX_T))) != sizeof(PREFIX_T)){
		printf("Failed to set boot flags to prefix.bin\n");
	}else{
		printf("Update bootflags successfully \n");
	}

	close(fd);
}

static int get_current_bank(void)
{
	int fd = -1;
	unsigned char bank_buf[32];
	memset(bank_buf,0x0,sizeof(bank_buf));
	BANK_T *bank =(BANK_T *)bank_buf;
	unsigned int curbank;

	fd = open("/tmp/tmp-master/bank.bin",O_RDWR | O_SYNC);
	if(fd < 0){
		printf("Failed to open bank.bin \n");
		return -1;
	}

	if((read(fd,bank_buf,sizeof(BANK_T))) != sizeof(BANK_T)){
		printf("Error in read the bank flags, \n");
		close(fd);
		return -1;
	}

	if((bank->magic_head != MAGIC_HEAD) || (bank->magic_tail != MAGIC_TAIL)){
		printf("No available bank header and tail find \n");
		printf("bank->magic_head = %x,bank->magic_tail = %x \n",bank->magic_head,bank->magic_tail);
		close(fd);
		return -1;
	}

	curbank = bank->current;
	printf("Current bank = %d \n",curbank);
	if((curbank != 0) && (curbank != 1))
		curbank = 0;

	close(fd);
	return curbank;

}

static void set_current_bank(int b)
{
	int fd = -1;
	unsigned char bank_buf[32];
	memset(bank_buf,0x0,sizeof(bank_buf));
	BANK_T *bank =(BANK_T *)bank_buf;

	printf("set_current_bank : b=%d \n");
	fd = open("/tmp/tmp-master/bank.bin",O_RDWR | O_SYNC);
	if(fd < 0){
		printf("Failed to open bank.bin \n");
		return;
	}

	if((read(fd,bank_buf,sizeof(BANK_T))) != sizeof(BANK_T)){
		printf("Error in read the bank flags, \n");
		close(fd);
		return;
	}

	if((bank->magic_head != MAGIC_HEAD) || (bank->magic_tail != MAGIC_TAIL)){
		printf("No available bank header and tail find \n");
		printf("bank->magic_head = %x,bank->magic_tail = %x \n",bank->magic_head,bank->magic_tail);
		close(fd);
		return;
	}

	if((b!=0) && (b!=1))
		b = 0;

	printf("Update Current bank from %d to %d \n",bank->current,b);
	bank->current = b;

	lseek(fd,0x0,SEEK_SET);

	if((write(fd,bank_buf,sizeof(BANK_T))) != sizeof(BANK_T)){
		printf("Failed to update BANK flag to bank.bin");
	}else{
		printf("Update BANK successfully ... \n");
	}

	close(fd);
}

static int verify_upgrade_image(char *img)
{
	char cmd[256];
	char *cmd2 = &cmd[128];
	int fd;int ret =100;

	memset(cmd,'\0',sizeof(cmd));
	system("mkdir -p /tmp/image");
	system("rm -fr /tmp/image/*");
	sprintf(cmd,"tar xzvf %s -C /tmp/image\n",img);
	system(cmd);
	system("md5sum /tmp/image/devicetree.dtb > /tmp/dts.md5");
	system("md5sum /tmp/image/uimage > /tmp/uimg.md5");
	system("md5sum /tmp/image/rootfs.bin > /tmp/rfs.md5");

//Device tree md5 checkusm
	fd = open("/tmp/dts.md5",O_RDWR | O_SYNC);
	if(fd < 0){
		printf("verify_upgrade_image : failed to open /tmp/dts.md5\n");
		return -1;
	}

	memset(cmd,0x0,sizeof(cmd));
	if(read(fd,cmd,32) != 32)
	{
		printf("verify_upgrade_image : failed to read 32 byte from dts.md5 \n");
		close(fd);
		return -1;
	}

	close(fd);

	fd = open("/tmp/image/devicetree.dtb.md5",O_RDWR | O_SYNC);
	if(fd < 0){
		printf("verify_upgrade_image : open /tmp/image/devicetree.dtb.md5  failed \n");
		return -1;
	}

	if(read(fd,cmd2,32) != 32)
	{
		printf("verify_upgrade_image : failed to read 32 byte from devicetree.dtb.md5 \n");
		close(fd);
		return -1;
	}

	close(fd);

	if(strncmp(cmd,cmd2,32) != 0){
		printf("verify_upgrade_image : compare dtb md5 failed! \n");
		printf("target MD5 sum is %s \n",cmd2);
		printf("current MD5 sum is %s \n",cmd);
		ret = -1;
	}

//Kernel md5 checksum

	fd = open("/tmp/uimg.md5",O_RDWR | O_SYNC);
	if(fd < 0){
		printf("verify_upgrade_image : failed to open /tmp/uimg.md5\n");
		return -1;
	}

	memset(cmd,0x0,sizeof(cmd));
	if(read(fd,cmd,32) != 32)
	{
		printf("verify_upgrade_image : failed to read 32 byte from uimg.md5 \n");
		close(fd);
		return -1;
	}

	close(fd);

	fd = open("/tmp/image/uimage.md5",O_RDWR | O_SYNC);
	if(fd < 0){
		printf("verify_upgrade_image : open /tmp/image/uimage.md5  failed \n");
		return -1;
	}

	if(read(fd,cmd2,32) != 32)
	{
		printf("verify_upgrade_image : failed to read 32 byte from uimage.md5 \n");
		close(fd);
		return -1;
	}

	close(fd);

	if(strncmp(cmd,cmd2,32) != 0){
		printf("verify_upgrade_image : compare uImage md5 failed! \n");
		printf("target MD5 sum is %s \n",cmd2);
		printf("current MD5 sum is %s \n",cmd);
		ret = -1;
	}

//Rootfs checksum

	fd = open("/tmp/rfs.md5",O_RDWR | O_SYNC);
	if(fd < 0){
		printf("verify_upgrade_image : failed to open /tmp/rfs.md5\n");
		return -1;
	}

	memset(cmd,0x0,sizeof(cmd));
	if(read(fd,cmd,32) != 32)
	{
		printf("verify_upgrade_image : failed to read 32 byte from rfs.md5 \n");
		close(fd);
		return -1;
	}

	close(fd);

	fd = open("/tmp/image/rootfs.bin.md5",O_RDWR | O_SYNC);
	if(fd < 0){
		printf("verify_upgrade_image : open /tmp/image/rootfs.bin.md5  failed \n");
		return -1;
	}

	if(read(fd,cmd2,32) != 32)
	{
		printf("verify_upgrade_image : failed to read 32 byte from rootfs.bin.md5 \n");
		close(fd);
		return -1;
	}

	close(fd);

	if(strncmp(cmd,cmd2,32) != 0){
		printf("verify_upgrade_image : compare rootfs md5 failed! \n");
		printf("target MD5 sum is %s \n",cmd2);
		printf("current MD5 sum is %s \n",cmd);
		ret = -1;
	}

	if(ret < 0){
		printf("verify_upgrade_image : invalid upgrade image, stop updating \n");
	}

	return ret;


}

static void do_system_upgrade(void)
{
	char cmd_buf[256];
	int curbank;

	curbank = get_current_bank();
	printf("Current BANK is %d \n",curbank);

	if(curbank == 0){
		system("mkdir -p /tmp/bank1");
		system("mount /dev/mmcblk0p2 /tmp/bank1");
		system("mv /tmp/image/uimage /tmp/bank1/uimage1");
		system("mv /tmp/image/devicetree.dtb /tmp/bank1/devicetree1.dtb");
		system("dd if=/tmp/image/rootfs.bin of=/dev/mmcblk0p4");
		system("sync");
		system("umount /tmp/bank1");
		set_current_bank(curbank+1);
	}else{
		curbank = 1;
		system("mkdir -p /tmp/bank1");
		system("mount /dev/mmcblk0p2 /tmp/bank1");
		system("mv /tmp/image/uimage /tmp/bank1/uimage0");
		system("mv /tmp/image/devicetree.dtb /tmp/bank1/devicetree0.dtb");
		system("dd if=/tmp/image/rootfs.bin of=/dev/mmcblk0p3");
		system("umount /tmp/bank1");
		set_current_bank(curbank-1);
		system("sync");
	}

	printf("Switch BANK to %d \n",get_current_bank());

}

/*
 * Usage: sysagent -u <filepath> : upgrade system
 *        sysagent -t sync time from current system to FPGA
 *        sysagent -s sync time from FPGA RTC to current system
 *        sysagent -r <reg> read data from FPGA special register
 *        sysagent -o <reg> <val> set val output to FPGA special register
 *        sysagent -b update bootflag
 *        sysagent -g get current active bank
 */


int main(int argc,char **argv)
{
	int opt;
	unsigned int reg,val;
	char *endptr;

	opterr = 0;
	fpga_addr_mapping();
	dual_bank_init();

	if (argc == 1){
		printf("usage wrong ... \n");
		goto exit;
	}

	while ((opt = getopt(argc, argv, "utsrobg")) > 0) {
		switch(opt){
			case 'u':
				printf("Start upgrade system ..... image = %s\n",argv[2]);
				if(verify_upgrade_image(argv[2]) < 0){
					goto exit;
				}
				do_system_upgrade();
				break;
			case 't':
				fpga_rtc_set();
				printf("Sync time from current system to FPGA \n");
				break;
			case 's':
				fpga_rtc_get();
				printf("Sync time from FPGA to current system \n");
				break;
			case 'r':
				printf("Read data from FPGA special register\n");
				reg = strtoul(argv[2], &endptr, 0);
				printf("Reg = %x, value = %x \n",reg,fpga_read(reg));
				break;
			case 'o':
				printf("Output data to FPGA special register\n");
				reg = strtoul(argv[2], &endptr, 0);
				val = strtoul(argv[3], &endptr, 0);
				fpga_write(reg,val);
				printf("Output reg=%x,val = %x \n",reg,val);
				break;
			case 'b':
				printf("Set bootflag \n");
				set_boot_ok_flag();
				break;
			case 'g':
				printf("BANK=%d\n",get_current_bank());
			default:
				goto exit;
		}
	}

exit:
	fpga_addr_unmapping();
	dual_bank_terminate();

	return 0;
}
