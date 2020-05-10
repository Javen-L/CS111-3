// NAME: Anh Mac
// EMAIL: anhmvc@gmail.com
// UID: 905111606

#include "SortedList.h"
#include <stdio.h>
#include <string.h>
#include <sched.h>

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
    SortedListElement_t *curr = list->next;
    if (!list || !element)
        return;
    // finding position for the element to be inserted
    while (curr != list && strcmp(curr->key, element->key) <= 0)
        curr = curr->next;
    if (opt_yield & INSERT_YIELD)
        sched_yield();
    
    element->next = curr;
    element->prev = curr->prev;
    curr->prev->next = element;
    curr->prev = element;
}

int SortedList_delete(SortedListElement_t *element) {
    if (element->next->prev != element || element->prev->next != element)
        return 1;
    
    if (!element->key) return 1; // empty list

    element->prev->next = element->next;
    if (opt_yield & DELETE_YIELD)
        sched_yield();
    element->next->prev = element->prev;
    return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
    SortedListElement_t *curr = list->next;
    if (!key) return NULL;
    
    while (curr != list && strcmp(curr->key,key) <= 0) {
        if (opt_yield & LOOKUP_YIELD)
            sched_yield();
        if (strcmp(curr->key,key) == 0)
            return curr;
        curr = curr->next;
    }
    return NULL;
}

int SortedList_length(SortedList_t *list) {
    int count = 0; 
    SortedListElement_t *curr = list->next;
    
    while (curr != list) {
        if (curr->next->prev != curr || curr->prev->next != curr)
            return -1;
        count += 1;
        curr = curr->next;
        if (opt_yield & LOOKUP_YIELD)
            sched_yield();
    }
    return count;
}