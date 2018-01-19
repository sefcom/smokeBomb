/*
 *	sync one round attack
 *
 *	Copyright (C) 2017  Jinbum Park <jinb.park@samsung.com> Haehyun Cho <haehyun@asu.edu>
*/

#include "common.h"
#include <id3.h>

char *testset1[] = 
{
	/*
	 * outlook, temperature, humidity, wind, play ball (class)
	 */
	"SUNNY", 	"MILD",  	"HIGH",    	"WEAK",		"NO",
};


int main(int argc, char **argv)
{
	char msg[MSG_SIZE_MAX];
	char reply_msg[] = "reply!!";
	struct shm_msg *client_msg;
	struct shm_msg *server_msg;
	void *addr;
	int fd;

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
				memcpy(msg, client_msg->msg, client_msg->len);
				client_msg->status = 0;
				break;
			}
		}

		if(client_msg->len == sizeof(END_MSG)) {
			printf("end msg : %s\n", client_msg->msg);
			break;
		}

		/* test decision tree!! */
		test_prestored_tree_with_data(testset1);

		/* prepare msg */
		server_msg->status = 0;
		server_msg->len = sizeof(reply_msg);

		/* send reply */
		memcpy(server_msg->msg, reply_msg, sizeof(reply_msg));
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

	return 0;
}
