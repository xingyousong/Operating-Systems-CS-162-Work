/*
 * mm_alloc.c
 *
 * Stub implementations of the mm_* routines.
 */

#include "mm_alloc.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
//trolll
mem_ptr head = NULL;
mem_ptr end = NULL; 

mem_ptr increase_mem(mem_ptr start, size_t size){
	void * current = sbrk(0);

	int error_test = (int)sbrk( (int*) (META_SIZE + size) );
	if (error_test < 0){
		return NULL; 
	}	

    mem_ptr new_block = (mem_ptr) current;

	new_block -> size = size; 
	new_block -> prev = NULL; 
	new_block -> free = 0;
    new_block -> next = NULL;

    if (head == NULL) {
        head = new_block;
        end = new_block; 
    } else {
        new_block -> prev = end; 
        end -> next = new_block; 
        end = new_block; 
    }
	return new_block; 
}

void split(mem_ptr start, size_t s){
    mem_ptr middle = (mem_ptr)(start ->actual_memory + s );
    start -> size = s; 
    middle -> next = start-> next; 
    middle -> prev = start; 
    start -> next = middle; 
    middle -> free = 1; 
    middle -> size = start -> size - s - 2*META_SIZE; 
}

mem_ptr combine(mem_ptr temp){

   if (temp -> prev != NULL){
    if (temp -> prev -> free == 1){
        temp -> prev -> size = temp -> size + META_SIZE + temp -> prev -> size;
        temp -> next -> prev = temp -> prev; 
        temp -> prev -> next = temp -> next; 
    }
   }

    if (temp -> next != NULL){
        if (temp -> next -> free == 1){
            temp -> next -> size = temp -> size + META_SIZE + temp -> next -> size;
            temp -> prev -> next = temp -> next;
            temp -> next -> prev = temp -> prev; 
            
            temp -> next = temp->next -> next;
        }
    
    }
    
    return temp; 
}


void *mm_malloc(size_t size) {
    //return calloc(1, size);
    
   
    
    if (size == 0){
    	return NULL; 
    }

    mem_ptr temp; 
    mem_ptr before_temp; 


    if(head == NULL){
    	temp = increase_mem(NULL, size);
    	if(temp == NULL){
    		return NULL; 
    	}
        return temp -> actual_memory;
    }
    else {
    	temp = head; 
        before_temp = head; 
    	while (temp != NULL){
            if (temp -> free == 1 && temp -> size >= size){
                
                printf("good \n");
                break; 
            }
            before_temp = temp; 
            temp = temp -> next; 
        }

		if(temp != NULL){
            if(temp ->size > size + 2*META_SIZE){
                split(temp, size);
                return temp -> actual_memory; 
            }
            else{
                temp -> free = 0;

                return temp -> actual_memory; 
            }
        }
        else{
        	temp = increase_mem(before_temp, size);
        	if(!temp){
        		return NULL;
        	}
        	
            return temp -> actual_memory;

        }

    }
    
}

void *mm_realloc(void *ptr, size_t size) {
    /* YOUR CODE HERE */
    
    //return realloc(ptr, size);

    if (size == 0){
        mm_free(ptr); 
        return NULL;
    }
    if (ptr == NULL && size !=0){
        mm_malloc(size);
        return; 
    }
    if (ptr == NULL && size == 0){
        mm_malloc(0);
        return; 
    }


    

    mem_ptr temp; 
   
    temp = (mem_ptr) ( ((unsigned char*) ptr) - META_SIZE); 
    

    //combine(temp); 
    temp -> free = 1; 



    void *new_ptr = mm_malloc(size);

    if (new_ptr == NULL){
        return NULL; 
    }

    mem_ptr new_temp = (mem_ptr) ( ((unsigned char*) new_ptr) - META_SIZE); 

    new_temp -> prev = temp -> prev;
    new_temp -> next = temp -> next;

    if(temp -> prev != NULL){
        temp -> prev -> next = new_temp;
    }
    if (temp -> next != NULL){
        temp -> next -> prev = new_temp;
    }


    memset(new_ptr, 0, size);

    if(new_temp -> size >= temp -> size){
        memcpy(new_ptr, ptr, temp -> size);
    }
    else{
        memcpy(new_ptr, ptr, new_temp -> size);
    }
    
    memset(temp -> actual_memory, 0, temp -> size);
    //brk(temp + META_SIZE);
    

    return new_ptr; 

    
}

void mm_free(void *ptr) {
    /* YOUR CODE HERE */
    //free(ptr);
    
    mem_ptr temp; 
    if (ptr == NULL){
        return;
    }
    else{
        temp = (mem_ptr) ( ((unsigned char*) ptr) - META_SIZE); 
        //temp = (mem_ptr) ((unsigned char*) ptr);
        
        //combine(temp); 
        temp -> free = 1; 
        memset(temp -> actual_memory, 0, temp -> size);
        //brk(temp + META_SIZE);
    }
    
}
