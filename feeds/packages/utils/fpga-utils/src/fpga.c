/*
 * Simple app. to do memory accesses via /dev/mem.
 *
 *
 * Copyright (c) Richard Hirst <rhirst@linuxcare.com>
 * Copyright (c) Thomas Langer <thomas.langer@infineon.com>
 * Copyright (c) Thomas Langer <xiangyu.chen@aol.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define MEM_READ  0
#define MEM_WRITE 1
#define MEM_AND   2
#define MEM_OR    3

static void
usage (char *argv0)
{
	fprintf(stderr,
"    -1|2|4     Sets memory access size in bytes (default byte)\n"
"    -l <len>   Length in bytes of area to access (defaults to\n"
"               one access, or whole file length)\n"
"    -r|w|a|o   Read from or Write to memory (default read)\n"
"               optional write with modify (and/or)\n"
"    <addr>     The memory address to access\n"
"    <val>      The value to write (implies -w)\n\n"
"    Xilinx ZYNQ FPGA base address is 0x40000000 \n"
"Examples:\n"
#ifdef CONFIG_XILINX_BMU_PROJECT
"   Read data from a FPGA address:\n"
"   %s -r -4 0x5           Reads 4 bytes from 0x40000014\n"
"   %s -r -l 4 0x5         Reads 4 bytes from 0x40000014, show in per byte\n"
"   %s -r 0x5	        Read one byte from 0x40000041\n"
"   Write data to FPGA address: \n"
"   %s -w -4 0x1 0xaaffaaff     Write 0xaaffaaff to 0x40000004 \n"
#else
"    Read data from special addrss :\n"
"    %s 0x40000000              Reads one byte from 0x40000000\n"
"    %s -l 4 0x40000000         Reads 4 bytes from 0x40000000,show in per byte\n"
"    %s -4 0x40000000           Reads 4 bytes from 0x40000000\n"
"    Write data to speical address: \n"
"    %s -4 0x40000000  0xa1a2a3a4 Write 0xa1a2a3a4 to 0x40000000\n"
#endif
"\n",
		argv0, argv0, argv0, argv0);
	exit(1);
}


static void
memread_memory(unsigned long phys_addr, void *addr, int len, int iosize)
{
	int i;

	while (len) {
		printf("%08lx: ", phys_addr);
		i = 0;
		while (i < 16 && len) {
			switch(iosize) {
			case 1:
				printf(" %02x", *(unsigned char *)addr);
				break;
			case 2:
				printf(" %04x", *(unsigned short *)addr);
				break;
			case 4:
				printf(" %08lx", *(unsigned long *)addr);
				break;
			}
			i += iosize;
			addr += iosize;
			len -= iosize;
		}
		phys_addr += 16;
		printf("\n");
	}
}


static void
write_memory(unsigned long phys_addr, void *addr, int len, int iosize, unsigned long value)
{
	switch(iosize) {
	case 1:
		while (len) {
			*(unsigned char *)addr = value;
			len -= iosize;
			addr += iosize;
		}
		break;
	case 2:
		while (len) {
			*(unsigned short *)addr = value;
			len -= iosize;
			addr += iosize;
		}
		break;
	case 4:
		while (len) {
			*(unsigned long *)addr = value;
			len -= iosize;
			addr += iosize;
		}
		break;
	}
}


static void
and_write_memory(unsigned long phys_addr, void *addr, int len, int iosize, unsigned long value)
{
	switch(iosize) {
	case 1:
		while (len) {
			*(unsigned char *)addr &= value;
			len -= iosize;
			addr += iosize;
		}
		break;
	case 2:
		while (len) {
			*(unsigned short *)addr &= value;
			len -= iosize;
			addr += iosize;
		}
		break;
	case 4:
		while (len) {
			*(unsigned long *)addr &= value;
			len -= iosize;
			addr += iosize;
		}
		break;
	}
}


static void
or_write_memory(unsigned long phys_addr, void *addr, int len, int iosize, unsigned long value)
{
	switch(iosize) {
	case 1:
		while (len) {
			*(unsigned char *)addr |= value;
			len -= iosize;
			addr += iosize;
		}
		break;
	case 2:
		while (len) {
			*(unsigned short *)addr |= value;
			len -= iosize;
			addr += iosize;
		}
		break;
	case 4:
		while (len) {
			*(unsigned long *)addr |= value;
			len -= iosize;
			addr += iosize;
		}
		break;
	}
}


int
main (int argc, char **argv)
{
	int mfd, ffd = 0, req_len = 0, opt;
	void *real_io;
	unsigned long real_len, real_addr, req_addr, req_value = 0, offset;
	char *endptr;
	int memfunc = MEM_READ;
	int iosize = 1;
	char *filename = NULL;
	int verbose = 0;

	opterr = 0;
	if (argc == 1)
		usage(argv[0]);

	while ((opt = getopt(argc, argv, "hv124rwaol:f:")) > 0) {
		switch (opt) {
		case 'h':
			usage(argv[0]);
		case 'v':
			verbose = 1;
			break;
		case '1':
		case '2':
		case '4':
			iosize = opt - '0';
			break;
		case 'r':
			memfunc = MEM_READ;
			break;
		case 'a':
			memfunc = MEM_AND;
			break;
		case 'o':
			memfunc = MEM_OR;
			break;
		case 'w':
			memfunc = MEM_WRITE;
			break;
		case 'l':
			req_len = strtoul(optarg, &endptr, 0);
			if (*endptr) {
				fprintf(stderr, "Bad <size> value '%s'\n", optarg);
				exit(1);
			}
			break;
		case 'f':
			filename = strdup(optarg);
			break;
		default:
			fprintf(stderr, "Unknown option: %c\n", opt);
			usage(argv[0]);
		}
	}

	if (optind == argc) {
		fprintf(stderr, "No address given\n");
		exit(1);
	}
	req_addr = strtoul(argv[optind], &endptr, 0);
#ifdef CONFIG_XILINX_BMU_PROJECT
#define ZYNQ_BASE_ADDR 0x40000000
	req_addr = ZYNQ_BASE_ADDR + (req_addr * 4);
	if (verbose)
		fprintf(stderr,"Current request address is 0x%x \n",req_addr);
#endif
	if (*endptr) {
		fprintf(stderr, "Bad <addr> value '%s'\n", argv[optind]);
		exit(1);
	}
	optind++;
	if (!filename && (memfunc == MEM_READ) && optind < argc) {
		memfunc = MEM_WRITE;
	}
	if (filename && optind > argc) {
		fprintf(stderr, "Filename AND value given\n");
		exit(1);
	}
	if (!filename && (memfunc != MEM_READ) && optind == argc) {
		fprintf(stderr, "No value given for WRITE\n");
		exit(1);
	}
	if (!filename && (memfunc != MEM_READ)) {
		req_value = strtoul(argv[optind], &endptr, 0);
		if (*endptr) {
			fprintf(stderr, "Bad <value> value '%s'\n", argv[optind]);
			exit(1);
		}
		if ((iosize == 1 && (req_value & 0xffffff00)) ||
				(iosize == 2 && (req_value & 0xffff0000))) {
			fprintf(stderr, "<value> too large\n");
			exit(1);
		}
		optind++;
	}
	if (filename && (memfunc == MEM_READ) && !req_len) {
		fprintf(stderr, "No size given for file memread\n");
		exit(1);
	}
	if (optind < argc) {
		fprintf(stderr, "Too many arguments '%s'...\n", argv[optind]);
		exit(1);
	}
	if (filename && (memfunc == MEM_READ)) {
		ffd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
		if (ffd < 0) {
			fprintf(stderr, "Failed to open destination file '%s': %s\n", filename, strerror(errno));
			exit(1);
		}
	}
	if (filename && (memfunc != MEM_READ)) {
		ffd = open(filename, O_RDONLY);
		if (ffd < 0) {
			fprintf(stderr, "Failed to open source file '%s': %s\n", filename, strerror(errno));
			exit(1);
		}
	}

	if (filename && !req_len) {
		req_len = lseek(ffd, 0, SEEK_END);
		if (req_len < 0) {
			fprintf(stderr, "Failed to seek on '%s': %s\n",
					filename, strerror(errno));
			exit(1);
		}
		if (lseek(ffd, 0, SEEK_SET)) {
			fprintf(stderr, "Failed to seek on '%s': %s\n",
					filename, strerror(errno));
			exit(1);
		}
	}
	if (!req_len)
		req_len = iosize;

	if ((iosize == 2 && (req_addr & 1)) ||
			(iosize == 4 && (req_addr & 3))) {
		fprintf(stderr, "Badly aligned <addr> for access size\n");
		exit(1);
	}
	if ((iosize == 2 && (req_len & 1)) ||
			(iosize == 4 && (req_len & 3))) {
		fprintf(stderr, "Badly aligned <size> for access size\n");
		exit(1);
	}

	if (!verbose)
		/* Nothing */;
	else if (filename && (memfunc == MEM_READ))
		printf("Request to read 0x%x bytes from address 0x%08lx\n"
			"\tto file %s, using %d byte accesses\n",
			req_len, req_addr, filename, iosize);
	else if (filename)
		printf("Request to write 0x%x bytes to address 0x%08lx\n"
			"\tfrom file %s, using %d byte accesses\n",
			req_len, req_addr, filename, iosize);
	else if (memfunc == MEM_READ)
		printf("Request to read 0x%x bytes from address 0x%08lx\n"
			"\tusing %d byte accesses\n",
			req_len, req_addr, iosize);
	else
		printf("Request to write 0x%x bytes to address 0x%08lx\n"
			"\tusing %d byte accesses of value 0x%0*lx\n",
			req_len, req_addr, iosize, iosize*2, req_value);

	real_addr = req_addr & ~4095;
	if (real_addr == 0xfffff000) {
		fprintf(stderr, "Sorry, cannot map the top 4K page\n");
		exit(1);
	}
	offset = req_addr - real_addr;
	real_len = req_len + offset;
	real_len = (real_len + 4095) & ~ 4095;
	if (real_addr + real_len < real_addr) {
		fprintf(stderr, "Aligned addr+len exceeds top of address space\n");
		exit(1);
	}
	if (verbose)
		printf("Attempting to map 0x%lx bytes at address 0x%08lx\n",
			real_len, real_addr);

	mfd = open("/dev/mem", (memfunc == MEM_READ) ? O_RDONLY : O_RDWR);
	if (mfd == -1) {
		perror("open /dev/mem");
		fprintf(stderr, "Is CONFIG_DEVMEM activated?\n");
		exit(1);
	}
	if (verbose)
		printf("open(/dev/mem) ok\n");
	real_io = mmap(NULL, real_len,
			(memfunc == MEM_READ) ? PROT_READ:PROT_READ|PROT_WRITE,
			MAP_SHARED, mfd, real_addr);
	if (real_io == (void *)(-1)) {
		fprintf(stderr, "mmap() failed: %s\n", strerror(errno));
		exit(1);
	}
	if (verbose)
		printf("mmap() ok\n");

	if (verbose) {
		int c;

		printf("OK? ");
		fflush(stdout);
		c = getchar();
		if (c != 'y' && c != 'Y') {
			printf("Aborted\n");
			exit(1);
		}
	}

	if (filename && (memfunc == MEM_READ)) {
		int n = write(ffd, real_io + offset, req_len);

		if (n < 0) {
			fprintf(stderr, "File write failed: %s\n", strerror(errno));
			exit(1);
		}
		else if (n != req_len) {
			fprintf(stderr, "Only wrote %d of %d bytes to file\n",
					n, req_len);
			exit(1);
		}
	}
	else if (filename) {
		int n = read(ffd, real_io + offset, req_len);

		if (n < 0) {
			fprintf(stderr, "File read failed: %s\n", strerror(errno));
			exit(1);
		}
		else if (n != req_len) {
			fprintf(stderr, "Only read %d of %d bytes from file\n",
					n, req_len);
			exit(1);
		}
	}
	else {
		switch (memfunc)
		{
		case MEM_READ:
			memread_memory(req_addr, real_io + offset, req_len, iosize);
			break;
		case MEM_WRITE:
			write_memory(req_addr, real_io + offset, req_len, iosize, req_value);
			break;
		case MEM_AND:
			and_write_memory(req_addr, real_io + offset, req_len, iosize, req_value);
			break;
		case MEM_OR:
			or_write_memory(req_addr, real_io + offset, req_len, iosize, req_value);
			break;
		}
	}

	munmap(real_io, real_len);

	if (filename)
		close(ffd);
	close (mfd);

	return 0;
}

