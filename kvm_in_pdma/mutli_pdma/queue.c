#include "queue.h"
#include <stddef.h>

//queue head init
void kvm_queue_init(kvm_queue_head * head){
	head->next = head;
	head->prev = head;
}


//queue init
void  kvm_queue_element_init(kvm_queue_element * queue_element){
	queue_element->next = queue_element;
	queue_element->prev = queue_element;
}

//queue push
void kvm_queue_push(kvm_queue_head *head, kvm_queue_element *queue_element){
	kvm_queue_element *last = (kvm_queue_element *)head->prev;
	queue_element->next = head;
	queue_element->prev = last;
	last->next = queue_element;
	head->prev = queue_element;	
}

//queue pop
kvm_queue_element* kvm_queue_pop(kvm_queue_head *head){
	kvm_queue_element *first = (kvm_queue_element*) head->next;
	if(first == head){
		return NULL;
	}else{
		head->next = first->next;
		((kvm_queue_element *)first->next)->prev = head;
		first->next = first;
		first->prev = first;
		return first;
	}
}

//judge whether queue is empty
bool kvm_queue_is_empty(kvm_queue_head *head){
	return (head->next == head);
}

//queue front
kvm_queue_element* kvm_queue_front(kvm_queue_head* head){
	if (kvm_queue_is_empty(head))
	{
		return NULL;
	}else{
		return (kvm_queue_element*)head->next;
	}
}

//queue length
 int kvm_queue_length(kvm_queue_head *head){
	int length = 0;
	kvm_queue_element *element;
	if(kvm_queue_is_empty(head))
		return 0;
	element = kvm_queue_front(head);

	length++;
	while((kvm_queue_element*)element->next != head){
			length++;
	}

	return length;
}