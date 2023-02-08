/*
 * Code for basic C skills diagnostic.
 * Developed for courses 15-213/18-213/15-513 by R. E. Bryant, 2017
 * Modified to store strings, 2018
 */

/*
 * This program implements a queue supporting both FIFO and LIFO
 * operations.
 *
 * It uses a singly-linked list to represent the set of queue elements
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "harness.h"
#include "queue.h"

/*
  Create empty queue.
  Return NULL if could not allocate space.
*/
queue_t *q_new()
{
    queue_t *q = malloc(sizeof(queue_t));
    /* 
        What if malloc returned NULL? 
        So I'll check to see if the q returned is null.
        If it is not, we'll set the q.head ptr to NULL
        and return the q ptr. In this way, we only 
        return once.
    */

    if (q != NULL)
    {
        q->head = q->tail = NULL;
        q->size = 0;
    }
    
    return q ? q : NULL;
}

/* Free all storage used by queue */
void q_free(queue_t *q) // ~q (destructor)
{
    //list_ele_t *tmp, *kewl;
    /* How about freeing the list elements and the strings? */

    if (q == NULL || q->size == 0)
    {
        if (q == NULL)
            return;
        else
        {
            free(q);
            return;
        }
    }

    while(q->size > 0)
    {
        q_remove_head(q, NULL, 0);
    }
    free(q);

}

/*
  Attempt to insert element at head of queue.
  Return true if successful.
  Return false if q is NULL or could not allocate space.
  Argument s points to the string to be stored.
  The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_head(queue_t *q, char *s) // this is a stack function
{
    list_ele_t *newh; // new header element
    char * news; // new string element
    bool flag = false;
    int newsLen;

    // printf("len: %lu malloc size: %lu\n", strlen(s), strlen(s) * sizeof(char) +1);

    /* Bad input values */
    if (q == NULL || s == NULL)  
        return flag; // this will jump to the end and return False

    newsLen = strlen(s) + 1;
    newh = malloc(sizeof(list_ele_t *)); // make a new element for the list
    news = malloc(newsLen); // allocate space for new string to exactly the length of 's' PLUS the null terminator
    
    /* What if either call to malloc returns NULL? */
    if (newh == NULL || news == NULL)
    {
        // if one succeeds but the other doesn't
        if (newh != NULL) free(newh);
        if (news != NULL) free(news);
        return flag;
    }

    strncpy(news, s, newsLen); // copy the value of input s into the news string
    newh->value = news; // place the new string into the new header's value property
    
    if (q->size == 0) // queue was empty
    {
        q->head = q->tail = newh;
        q->head->next = NULL;
        q->tail->next = NULL;
        flag = true;
        q->size++;
    }
    else if (q->size == 1)
    {
        newh->next = q->head;
        q->tail = q->head;
        q->head = newh;
        flag = true;
        q->size++;        
    }
    else
    {
        newh->next = q->head; // take the current header and store it in newh.next
        q->head = newh; // set the header to the newh
        flag = true; // set return flag to true since we succeeded
        q->size++;
    }

    //free(newh);
    //free(news);
    return flag;
}

/*
  Attempt to insert element at tail of queue.
  Return true if successful.
  Return false if q is NULL or could not allocate space.
  Argument s points to the string to be stored.
  The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_tail(queue_t *q, char *s) // this is a queue function
{
    list_ele_t *newt; // create new tail element
    char *news; // create new string element
    int newsLen;

    /* Handle bad inputs */
    if (q == NULL || s == NULL)
    {
        return false; // return False
    }

    newsLen = strlen(s) + 1;
    newt = malloc(sizeof(list_ele_t)); // allocate space for new tail
    news = malloc(newsLen); // allocate space for new string to exactly the length of 's' + null char

    // if either malloc returns null, return will == false
    if (newt == NULL || news == NULL)
    {
        if (newt != NULL) free(newt);
        if (news != NULL) free(news);
        return false;
    }

    strncpy(news, s, newsLen); // copy string but include space for the null character
    newt->value = news;

    /*
        Basically, if the list is empty, both the head and the tail
        should point to the same element.
    */
    if (q->size == 0)
    {
        q->head = q->tail = newt;
        q->size++;
        return true;
    }

    q->tail->next = newt; // current tail 'next' value is set to the new tail
    q->tail = newt; // the new tail is then stored in q.tail
    q->size++; // increment the size to accommodate the new item

    return true;
}

/*
    Attempt to remove element from head of queue.
    Return true if successful.
    Return false if queue is NULL or empty.
    If sp is non-NULL and an element is removed, copy the removed string to *sp
    (up to a maximum of bufsize-1 characters, plus a null terminator.)
    The space used by the list element and the string should be freed.
*/
bool q_remove_head(queue_t *q, char *sp, size_t bufsize)
{
    list_ele_t *tmp;
    bool flag = false;
    if (q == NULL || q->size == 0)
    {
        return flag; // return False
    }

    if (sp != NULL)
    {
        tmp = q->head; // put the value of q.head into tmp
        q->head = q->head->next; // set the value of q.head to q.head.next
        strncpy(sp, tmp->value, bufsize - 1); // copy the string to the sp variable
        free(tmp->value); // free the old header value memory
        free(tmp); // free the old header memory
        flag = true;
        q->size--; // decrement the size of the queue
    }
    else
    {
        tmp = q->head; // put the value of q.head into tmp
        q->head = q->head->next; // set the value of q.head to q.head.next
        free(tmp->value); // free the old header value memory
        free(tmp); // free the old header memory
        flag = true;
        q->size--; // decrement the size of the queue

    }

    return flag;
}

/*
    Return number of elements in queue.
    Return 0 if q is NULL or empty
    You need to write the code for this function
    Remember: It should operate in O(1) time
 */
int q_size(queue_t *q)
{
    int z = 0; // this will be our return value
    if (q != NULL && q->size > 0)
        z = q->size; // queue is not empty, put its size in z
    
    return z;
}

/*
  Reverse elements in queue
  No effect if q is NULL or empty
  This function should not allocate or free any list elements
  (e.g., by calling q_insert_head, q_insert_tail, or q_remove_head).
  It should rearrange the existing ones.
 */
void q_reverse(queue_t *q)
{
    if (q == NULL || q->size == 0 || q->size == 1)
        return;

    list_ele_t *prv = NULL;
    list_ele_t *cur = q->head; // start by storing the head of the list
    list_ele_t *nxt = NULL;
    list_ele_t *tmp = q->head;

    if (q->size == 2) // just do a simple swap
    {
        cur = q->head;
        q->head = q->tail;
        q->tail = cur;
    }
    else
    {
    // while the list hasn't reached the end
        while (cur != NULL)
        {
            nxt = cur->next; // store the current value of the current node
            cur->next = prv; // change the link of the current node to the previous node
            
            /*
                Now that we've reordered the link for the current node, we will move to the
                next node in the list.
            */
            prv = cur; // store the pointer to the current node in the previous variable
            cur = nxt; // store the pointer to the next node in the current variable
        }
        q->head = prv; // the list is now reversed so store the pointer to prv in q.head
        q->tail = tmp; // set the value of q.tail to the old header
    }
}
