#include <stdio.h>
#include <sys/queue.h>
    

typedef struct node {
    TAILQ_ENTRY(node) next;
    int c;
} node_t;

TAILQ_HEAD(node_list, node);

int main(int argc, char **argv) {
    node_t n; 
    struct node_list list;
    TAILQ_INIT(&list);
        do { 
            if (((&n)->next.tqe_next = (&list)->tqh_first) != ((void *)0)) 
                (&list)->tqh_first->next.tqe_prev = &(&n)->next.tqe_next; 
            else 
                (&list)->tqh_last = &(&n)->next.tqe_next; 
            (&list)->tqh_first = (&n); 
            (&n)->next.tqe_prev = &(&list)->tqh_first; 
        } while ( 0);
        

    //TAILQ_INSERT_HEAD(&list, &n, next);  
    return 0;
}




