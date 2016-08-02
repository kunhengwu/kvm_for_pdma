// #include <linux/init.h>
#include <linux/kernel.h>
#include <stdbool.h>
typedef struct kvm_queue_ele{
	void *prev;
	void *next;
}kvm_queue_element;

typedef kvm_queue_element kvm_queue_head;
 
//queue head init
void kvm_queue_init(kvm_queue_head * head);

//queue init
void  kvm_queue_element_init(kvm_queue_element * queue_element);

//queue push
void kvm_queue_push(kvm_queue_head *head, kvm_queue_element *queue_element);

//queue pop
kvm_queue_element* kvm_queue_pop(kvm_queue_head *head);

//judge whether queue is empty
bool kvm_queue_is_empty(kvm_queue_head *head);

//queue front
kvm_queue_element* kvm_queue_front(kvm_queue_head* head);

//queue length
int kvm_queue_length(kvm_queue_head *head);
