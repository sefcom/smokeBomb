
#ifndef _COMMON_H
#define _COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <mqueue.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>

typedef unsigned char U8;
typedef unsigned int U32;

/* msg */
#define MSG_SIZE_MAX (100000)
#define REPLY_MSG "reply"
#define END_MSG "endend"

/* permission */
#define PERM_FILE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

/* fifo */
#define FIFO_SERVER_NAME "fifo_server"
#define FIFO_CLIENT_NAME "fifo_client"

/* msg queue */
#define MQUEUE_NAME "/test_mqueue"
#define MQUEUE_MAX_MSG_SIZE (1024 * 1024)

struct mqueue_msg {
	long type;
	mqd_t id;
	char msg[MQUEUE_MAX_MSG_SIZE - 64];
};

/* shared memory */
#define SHM_NAME "/test_shm"
#define SHM_SERVER_BUF_IDX (0)
#define SHM_CLIENT_BUF_IDX (MSG_SIZE_MAX / 2)

struct shm_msg {
	int status;
	size_t len;
	char msg[SHM_CLIENT_BUF_IDX - 1024];
};

/* Interprocess Variables by shm */
struct shm_mutex_cond {
	pthread_mutex_t mutex;
	pthread_mutexattr_t mutex_attr;
	pthread_cond_t cond;
	pthread_condattr_t cond_attr;
};

#define shm_send_signal(smc) do{\
	pthread_mutex_lock(&smc->mutex);\
	pthread_cond_signal(&smc->cond);\
	pthread_mutex_unlock(&smc->mutex);\
}while(0)

#define shm_wait_signal(smc) do{\
	pthread_mutex_lock(&smc->mutex);\
	pthread_cond_wait(&smc->cond, &smc->mutex);\
	pthread_mutex_unlock(&smc->mutex);\
}while(0)

/* socket */
#define SOCKET_SERVER_NAME "test_socket_server"
#define SOCKET_CLIENT_NAME "test_socket_client"

/* macros */
#define measure_start() \
	long bt, at; \
	struct timeval tv; \
	gettimeofday(&tv, NULL); \
	bt = (tv.tv_sec * 1000 + tv.tv_usec / 1000)

#define measure_end() \
	gettimeofday(&tv, NULL); \
	at = (tv.tv_sec * 1000 + tv.tv_usec / 1000); \
	printf("%ld\n", at - bt)

struct arr_type {
    unsigned int val1;
    unsigned int val2;
    unsigned int val3;
    unsigned int val4;
    char value[48];
};

unsigned int get_probe_array_val1(int idx);

extern const unsigned int arr_bound1;
extern const unsigned int arr_bound2;

#endif
