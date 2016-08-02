#ifndef _HOST_H_
#define _HOST_H_

#define DEVICE_PATH "/dev/pdma"
#define FDSIZE 1024
#define EPOLLLEVENTS 100
#define PACKET_SIZE 4096
struct kvm_host_pdma_control{
	int socket_fds[FDSIZE];
	int guest_number;
	int pdma_dev_number;
	pthread_t native_request_thread;
};
struct  kvm_host_pdma{
	
	int pdma_fd;
	/**
	* wait_queue is used in waiting interrupt of request and ready. 
	*/
	// wait_queue_head_t native_wq_request;
	// wait_queue_head_t ready_wq_response;
	/**
	* kvm_queue is used in save request and ready.
	*/
	kvm_queue_head *native_queue;
	kvm_queue_head *ready_queue;

	pthread_t send_request2pdma_thread;
	pthread_t ready_thread;
 	
	struct  pdma_info pdma_info;
};

struct guest_request{
	int fd;
	int buffer_length;
	unsigned char buffer[1024 * 28];
	kvm_queue_element queue_element;
};
void check_request_from_guest();
void convert_req_to_resp(struct guest_request *task);
void deal_request();
void ready_response_to_guest();
void host_exit(int argc);
void init_queue();
int init_pdma();
int get_pdma_info(int fd);
void add_event(int epollfd, int fd, int state);
void host_connect_chardev(int argc, char *argv[]);
int pdma_ioctl_op(unsigned int cmd, unsigned long data_addr);

int pdma_write_all(unsigned char* buffer_addr, int buffer_length);
int pdma_read_all(int socket_fd, unsigned char* buffer_addr, int buffer_length);
inline int set_cpu(int i);
#endif
