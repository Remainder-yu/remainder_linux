
#include <pthread.h> 
#include <semaphore.h> 
#include <errno.h>
#include <signal.h> 
#include <unistd.h> 
#include <stdlib.h> 
#include <stdio.h> 
#include <stdarg.h> 

static char do_it = 1; 
static unsigned long count_ope = 0; 
static pthread_mutex_t m; 
/* The following function loops on init/destroy some mutex (with different attributes) *
 it does check that no error code of EINTR is returned */ 
static void *threaded(void *arg) { 	
    int ret; 	
    while (do_it) { 		
        ret = pthread_mutex_lock(&m); 		
        if (ret == EINTR) { 			
            printf("pthread_mutex_lock returned EINTR\n"); 		
            } 		
        if (ret != 0) { 			
            printf( "pthread_mutex_lock failed\n"); 		
        } 
        	
        count_ope++; 	

        ret = pthread_mutex_unlock(&m); 		
        if (ret == EINTR) { 			
            printf("pthread_mutex_unlock returned EINTR\n"); 		
        } 		
        if (ret != 0) { 			
            printf( "pthread_mutex_unlock failed\n"); 		
        } 	
    } 	
    return NULL; 
} 

static void *threaded2(void *arg) { 	
    int ret; 	
    while (do_it) { 		
        ret = pthread_mutex_lock(&m); 		
        if (ret == EINTR) { 			
            printf("pthread_mutex_lock returned EINTR\n"); 		
        } 		
        if (ret != 0) { 			
            printf( "pthread_mutex_lock failed\n"); 		
        } 

        ret = pthread_mutex_unlock(&m); 		
        if (ret == EINTR) { 			
            printf("pthread_mutex_unlock returned EINTR\n"); 		
        } 		
        if (ret != 0) { 			
            printf( "pthread_mutex_unlock failed\n"); 		
            } 	
    } 	
    return NULL; 
} 
                        
int main(void) { 	
    int ret; 	
    pthread_t th_work1, th_work2, th_sig1, th_sig2; 	
    pthread_mutexattr_t mutex_attr; 	 	
    if (pthread_mutexattr_init(&mutex_attr) != 0) { 		
        printf("pthread_mutexattr_init failed\n\n"); 	
    } 	
        
    if (pthread_mutexattr_setprotocol(&mutex_attr, PTHREAD_PRIO_INHERIT) != 0) { 		
        printf("pthread_mutexattr_setprotocol failed\n"); 	
    } 	
            
    ret = pthread_mutex_init(&m, &mutex_attr); 	
    if (ret == EINTR) { 		
        printf("pthread_mutex_init returned EINTR\n"); 	
    } 	
    if (ret != 0) { 		
        printf( "pthread_mutex_init failed\n"); 	
    } 	
    if ((ret = pthread_create(&th_work1, NULL, threaded, NULL))) { 		
        printf( "Worker thread creation failed\n"); 	
    } 	
                        
    if ((ret = pthread_create(&th_work2, NULL, threaded2, NULL))) { 		
        printf( "Worker thread creation failed\n"); 	
    } 	
    
    sleep(1); 	
    do { 		
        do_it = 0; 	
    } 	while (do_it); 	
        
    if ((ret = pthread_join(th_work1, NULL))) {

        printf( "Worker thread join failed\n"); 	
    
    } 	
    if ((ret = pthread_join(th_work2, NULL))) { 		
        printf( "Worker2 thread join failed\n"); 	
    } 	
    
    ret = pthread_mutex_destroy(&m); 	
    if (ret == EINTR) { 		
        printf("pthread_mutex_destroy returned EINTR\n"); 	
    } 	
    if (ret != 0) { 		
        printf( "pthread_mutex_destroy failed\n"); 	
    } 
                                    
    printf("Test executed successfully only mutex_m.\n"); 	
    printf(" %d mutex lock and unlock were done.\n", count_ope); 
    return 0;
}


