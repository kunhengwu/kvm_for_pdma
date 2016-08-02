#define _GNU_SOURCE
#include <malloc.h> 
#include <linux/wait.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <sched.h>
#include <pthread.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <string.h>
#include "pdma-ioctl.h"
#include "queue.h"
#include "host.h"
#include <sys/socket.h>
#include <sys/un.h>

// #define DEBUG
// #define TEST_T
// #define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member)({const typeof(((type *)0)->member) *__mptr = (ptr); (type *)((char *)__mptr - offsetof(type,member));})

struct kvm_host_pdma host_pdma;
struct kvm_host_pdma_control host_pdma_control; 

// struct pollfd pollfd[FDSIZE];

/*
* queue lock for request and ready
*/
pthread_spinlock_t native_queue_lock;
pthread_spinlock_t readyread_queue_lock;
struct timeval start, stop;
int epollfd ;
struct epoll_event events[FDSIZE];

void check_request_from_guest(){
	int i = 0;
	int fd;
	// int buffer_length = 0;
	int per_read_len = 0;
	int guest_all_number = host_pdma_control.guest_number;
	struct guest_request *req;

	/**
	* Start check the request. Ret is the number of events;
	* Push requst into the host_pdma.native_queue
	*/
#ifdef TEST_T
	gettimeofday(&start, 0);
#endif
	int ret = epoll_wait(epollfd, events, EPOLLLEVENTS, 0);
	
	for (i = 0; i < ret; ++i){
		req = (struct guest_request *)malloc(sizeof(struct guest_request));
		req->fd = events[i].data.fd;
		req->buffer_length = 0;
		memset(req->buffer, 0, 1024 * 28);
		if(events[i].events & EPOLLIN){
			
#ifdef DEBUG
			printf("The number of triggle events is %d\n", ret);
#endif		
			while(1){
				per_read_len = read(req->fd, req->buffer + req->buffer_length, PACKET_SIZE);
				
				req->buffer_length += per_read_len;
				// if(per_read_len == 0){
				// 	printf("per_read_len: %d\n", per_read_len);
				// 	break;
				// }
				// else if(per_read_len < 0 && (errno == EINTR || errno == EAGAIN)){
				// 	printf("FD read failed in %d EINTR\n", i);
				// 	break;
				// }else 
				if(per_read_len < 0){
					printf("read error in %d\n", i);
					break;
				}
				if(per_read_len > 0 && per_read_len < PACKET_SIZE){
					per_read_len = 0;
					break;
				}
				
			}
#ifdef DEBUG
			// printf("The all buffer_length is %d\n", req->buffer_length);
#endif		

			pthread_spin_lock(&native_queue_lock);
			kvm_queue_push(host_pdma.native_queue, &(req->queue_element));		
			pthread_spin_unlock(&native_queue_lock);	
		}		
	}
#ifdef TEST_T
	gettimeofday(&stop, 0);
	printf("The time is: %d\n", stop.tv_usec - start.tv_usec);
#endif
}

void convert_req_to_resp(struct guest_request *task){
	int length = 0;
	length = task->buffer_length;
	task->buffer_length = ((length >> 2) - 4) >> 3;
}

void deal_request(){
	unsigned long flags;
	struct guest_request *task;
	kvm_queue_element *request_element;

	// wait_event_interruptible(host_pdma.native_wq_request, 
	// 			!kvm_queue_is_empty(host_pdma.native_queue));
	while(1){
		if(!kvm_queue_is_empty(host_pdma.native_queue))
			break;
	}

	pthread_spin_lock(&native_queue_lock);
	request_element = kvm_queue_pop(host_pdma.native_queue);
	pthread_spin_unlock(&native_queue_lock);
	
	task = container_of(request_element, struct guest_request, queue_element);
	pdma_write_all(task->buffer, task->buffer_length);
	
	convert_req_to_resp(task);
	// printf("task->buffer is %s, task->length is %d\n", task->buffer, task->buffer_length);
	pthread_spin_lock(&native_queue_lock);
	kvm_queue_push(host_pdma.ready_queue, &(task->queue_element));
	pthread_spin_unlock(&native_queue_lock);

	// wake_up(&(host_pdma.ready_wq_response));
}

void ready_response_to_guest(){
	struct guest_request *task;
	unsigned long flags;
	kvm_queue_element *response_element;
	// wait_event_interruptible(host_pdma.ready_wq_response, 
	// 			!kvm_queue_is_empty(host_pdma.ready_queue));

	while(1){
		if(!kvm_queue_is_empty(host_pdma.ready_queue)){
			// printf("host_pdma.ready_queue is not empty\n");
			break;
		}
			
	}

	pthread_spin_lock(&native_queue_lock);
	response_element = kvm_queue_pop(host_pdma.ready_queue);
	pthread_spin_unlock(&native_queue_lock);

	task = container_of(response_element, struct guest_request, queue_element);
	
	pdma_read_all(task->fd, task->buffer, task->buffer_length);
	free(task);

}

void *get_request_thread(void *cpu_index){
	// if(set_cpu(*(int *)cpu_index)){
	// 	printf("get_request_thread set_cpu failed\n");
	// }
	
	while(1){
		check_request_from_guest();
	}
	return 0;
}

void *deal_request_thread(void *cpu_index){
	// if(set_cpu(*(int *)cpu_index)){
	// 	printf("deal_request_thread set_cpu failed\n");
	// }

	while(1){
		deal_request();
	}
	return 0;
}

void *ready_response_thread(void *cpu_index){
	// if(set_cpu(*(int *)cpu_index)){
	// 	printf("ready_response_thread set_cpu failed\n");
	// }
	
	while(1){
		ready_response_to_guest();
	}
}

void host_exit(int argc){
	int i = 0;
	for(i = 0; i < argc; i++){
		close(host_pdma_control.socket_fds[i]);
	}
}

void init_queue(){
	host_pdma.native_queue = (kvm_queue_head *)malloc(sizeof(kvm_queue_head));
	host_pdma.ready_queue = (kvm_queue_head *)malloc(sizeof(kvm_queue_head));
	kvm_queue_init(host_pdma.native_queue);
	kvm_queue_init(host_pdma.ready_queue);
	// init_waitqueue_head(host_pdma.native_wq_request);
	// init_waitqueue_head(host_pdma.ready_wq_response);
}

int init_pdma(){
	int ret;
	int fd;
	struct pdma_rw_reg ctrl;
	ret = access(DEVICE_PATH, F_OK);
	if(ret){
		printf("The pdma driver is not loaded in host\n");
		close(fd);
		return ret;
	}
	fd = open(DEVICE_PATH, O_RDWR);
	if(fd == -1){
		printf("Open pdma fail\n");
		return -1;
	}else {
		host_pdma.pdma_fd = fd;
	}
	/*reset*/
	ctrl.type = 1;
	ctrl.addr = 0;
	ctrl.val = 1;
	ret = ioctl(fd, PDMA_IOC_RW_REG, (unsigned long)&ctrl);
	if(ret == -1){
		printf("PDMA_IOC_RW_REG reset error\n");
		return -1;
	}
	return 0;
}

int get_pdma_info(int fd){
	int ret;	
	ret = ioctl(fd, PDMA_IOC_INFO, (unsigned long)&(host_pdma.pdma_info));
	return ret;
}

void add_event(int epollfd, int fd, int state){
	struct epoll_event ev;
	ev.events = state;
	ev.data.fd = fd;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
}

void host_connect_chardev(int argc, char *argv[]){
	struct sockaddr_un sock;
	int i = 0;	
	int ret ;
	for (i = 1; i < argc ; ++i){
		ret = access(argv[i], F_OK);
		if(ret){
			printf("There is not %s\n", argv[i]);
		}
		host_pdma_control.socket_fds[i - 1] = socket(AF_UNIX, SOCK_STREAM, 0);
		if(host_pdma_control.socket_fds[i - 1] == -1){
			printf("Socket %s failed\n", argv[i]);
			return ;
		}
		sock.sun_family = AF_UNIX;
		memcpy(&sock.sun_path, argv[i], sizeof(sock.sun_path));
		ret = connect(host_pdma_control.socket_fds[i-1],  (struct sockaddr *)&sock, sizeof(sock));
		if(ret < 0){
			printf("fail connect %s\n", argv[i]);
		}
	}
	host_pdma_control.guest_number = argc - 1;
}

int pdma_write_all(unsigned char *buffer_addr, int buffer_length){
	int wt_block_sz = host_pdma.pdma_info.wt_block_sz;
	int wt_count = buffer_length;
	int i, ret;
	int code_bit;
	struct pdma_rw_reg ctrl;
	
	wt_count = (buffer_length + 1) / wt_block_sz + 1;
	/*the code bit len that write into FPGA
	* code_bit = buffer_length >> 2 - 4 (buffer_length is use byte as unit)
	* before encode, data lenght is x bit
	* after encode, data length is (x + 4) << 2 Byte
	* In this function buffer_length is after encode.
	*/
	code_bit = buffer_length >> 2 ;
	ctrl.type = 1;
	ctrl.addr = 0;
	ctrl.val = code_bit  << 19;
#ifdef DEBUG
	printf("code_bit: %d\n", code_bit);
	printf("wt_count: %d\n", wt_count);
	printf("buffer_length is %d\n", buffer_length);
	FILE *tt = fopen("guest_buffer.txt","w");
	for(i = 0; i < buffer_length; i++){
		if(i % 8 == 0)
			fprintf(tt, "\n");
		fprintf(tt, "%x ", *(buffer_addr + i));
	}
#endif
	ret = ioctl(host_pdma.pdma_fd, PDMA_IOC_RW_REG, (unsigned long)&ctrl);
	if(ret == -1){
		printf("PDMA_IOC_RW_REG error\n");
		return ret;
	}
	for(i = 0; i < wt_count; i++){
		ret = write(host_pdma.pdma_fd, buffer_addr + i * wt_block_sz, wt_block_sz);
		if(ret != wt_block_sz){
			printf("write failed\n");
			return ret;
		}
	}
#ifdef DEBUG
	printf("pdma_write_all success\n");
#endif
	return 0;
}

/**
* send buffer to corresponding socket_fd
*/
int pdma_read_all(int socket_fd, unsigned char *buffer_addr, int buffer_length){
	int ret, i;
	int rd_block_sz = host_pdma.pdma_info.rd_block_sz;
#ifdef DEBUG
	printf("Send buffer_length is %d, rd_block_sz: %d\n", buffer_length, rd_block_sz);
#endif
	ret = read(host_pdma.pdma_fd, buffer_addr, rd_block_sz);
	
#ifdef DEBUG
	// if(ret != buffer_length){
	// 	printf("Read pdma device error\n");
	// 	return 0;
	// }
	printf("All send buffer_length is %d\n", buffer_length);
#endif
	if(ret <= 0){
		printf("Read pdma_device failed\n");
		return -1;
	}
	// printf("Send guest data is %s\n", buffer_addr);

	ret = write(socket_fd, buffer_addr, buffer_length);
#ifdef DEBUG
	printf("send guest data ok, data number is %d, buffer_length: %d\n"
			, ret, buffer_length);
#endif
	return ret;
}

int main(int argc, char *argv[]){
	int i = 0, ret, fd, cpu_index = 14;
	char c;
	
	if(argc <= 1){
		printf("You should input some of socket file path\n");
		return 0;
	}
	for(i = 1; i < argc; i++)
		printf("socket file is %s\n", argv[i]);
	/**
	* init all of epoll and open per socket file
	*/
	if(init_pdma() < 0){
		printf("PDMA failed in main\n");
		return 0;
	}

	host_connect_chardev(argc, argv);
	epollfd = epoll_create(FDSIZE);
	for(i = 0; i < argc - 1; i++){
		add_event(epollfd, host_pdma_control.socket_fds[i], EPOLLIN);
	}
	// for(i = 0; i < argc - 1; i++){
	// 	pollfd[i].fd = host_pdma_control.socket_fds[i];
	// 	pollfd[i].events = POLLIN;
	// }
	if(get_pdma_info(host_pdma.pdma_fd)){
		printf("Cannot get pdma_info\n");
		return 0;
	}

	pthread_spin_init(&native_queue_lock, PTHREAD_PROCESS_SHARED);
	pthread_spin_init(&readyread_queue_lock, PTHREAD_PROCESS_SHARED);
	/**
	* init queue
	*/
	init_queue();

	pthread_create(&(host_pdma_control.native_request_thread), NULL,
			get_request_thread, &cpu_index);
	cpu_index += 1;
	pthread_create(&(host_pdma.send_request2pdma_thread), NULL,
			deal_request_thread, &(cpu_index));
	cpu_index += 1;
	pthread_create(&(host_pdma.ready_thread), NULL,
			ready_response_thread, &(cpu_index));

	while(1){
		printf("Host has start. If you want to stop it, you can input \'c\' \n");
		printf("input>");
		c = getchar();
		getchar();
		if(c == 's')
			break;
		else
			break;
	}
	host_exit(argc-1);
	exit(0);
}

inline int set_cpu(int i){
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(i, &mask);
	if(-1 == pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask)){
		printf("pthread_setaffinity_np failed\n");
		return -1;
	}
	return 0;
}





