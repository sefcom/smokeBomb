#include <stdio.h>
#include <libflush.h>
#include "common.h"

const int secret = 3;
const int arr[10] = {0,1,2,3,4,5,6,7,8,9,};
const int secret2 = 2;
libflush_session_t *session;

void bind_cpu(int cpuid)
{
    unsigned long mask = 0;

    if(cpuid == 0) mask = 1;
    else if(cpuid == 1) mask = 2;
    else if(cpuid == 2) mask = 4;
    else if(cpuid == 3) mask = 8;

    if(sched_setaffinity(0, sizeof(mask), (cpu_set_t*)&mask) < 0)
    {
        printf("sched_setaffinity error\n");
        return;
    }
}

static inline void flush_arr_bound(void)
{
	libflush_flush(session, &arr_bound1);
	libflush_flush(session, &arr_bound2);
}

void init_libflush(void)
{
	bool r;

	r = libflush_init(&session, NULL);
	if(r == false) {
		printf("libflush_init error\n");
		exit(-1);
	}
}

void finalize_libflush(void)
{
	libflush_terminate(session);
}

void victim_func(int idx)
{
	unsigned int val;
	
	if (idx >= arr_bound1 && idx < arr_bound2) {
		val = get_probe_array_val1(arr[idx]);
	}

	if (val == 999)
		printf("val 999\n");
}

int main(int argc, char **argv)
{
	int input_idx;
	unsigned int val;
	
	char reply_msg[] = "reply!!";
	struct shm_msg *client_msg;
	struct shm_msg *server_msg;
	void *addr;
	int fd;

	init_libflush();
	bind_cpu(2);

	/* create shm */
	if((fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, PERM_FILE)) == -1) {
		printf("shm_open error : %s\n", strerror(errno));
		return -1;
	}

	/* set size */
	if(ftruncate(fd, MSG_SIZE_MAX) == -1) {
		printf("ftruncate error : %s\n", strerror(errno));
		goto out;
	}

	/* mmap */
	addr = mmap(NULL, MSG_SIZE_MAX, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(addr == MAP_FAILED) {
		printf("mmap error : %s\n", strerror(errno));
		goto out;
	}
	memset(addr, 0, MSG_SIZE_MAX);

	client_msg = (struct shm_msg *)((char*)addr + SHM_CLIENT_BUF_IDX);
	server_msg = (struct shm_msg *)((char*)addr + SHM_SERVER_BUF_IDX);

	printf("server is running\n");
	while(1) {		
		/* read msg */
		while(1) {
			if(client_msg->status == 1) {
				memcpy(&input_idx, client_msg->msg, sizeof(input_idx));
				client_msg->status = 0;
				break;
			}
		}

		flush_arr_bound();
		if(client_msg->len == sizeof(END_MSG)) {
			printf("end msg : %s\n", client_msg->msg);
			break;
		}

		/* test!! */
		victim_func(input_idx);

		/* prepare msg */
		server_msg->status = 0;
		server_msg->len = sizeof(reply_msg);

		/* send reply */
		memcpy(server_msg->msg, &val, sizeof(val));
		server_msg->status = 1;
	}

out:
	/* destroy shm */
	if(munmap(addr, MSG_SIZE_MAX) == -1) {
		printf("munmap error : %s\n", strerror(errno));
	}

	if(close(fd) == -1) {
		printf("close error : %s\n", strerror(errno));
	}

	if(shm_unlink(SHM_NAME) == -1) {
		printf("shm_unlink error : %s\n", strerror(errno));
	}

	finalize_libflush();
	return 0;
}

