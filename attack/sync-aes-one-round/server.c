/*
 *	sync one round attack
 *
 *	Copyright (C) 2017  Jinbum Park <jinb.park@samsung.com> Haehyun Cho <haehyun@asu.edu>
*/

#include "common.h"
#include <openssl/aes.h>
#include <unistd.h>

U8 plain[16] = {0x00,};
U8 enc[16] = {0x10,0x20,};
U8 real_key[16] = {0x9e,0x41,0x00,0x5f,0xd9,0xa0,0xc2,0x28,0xe7,0xb4,0x31,0x29,0x94,0xaa,0x4d,0xc7};
AES_KEY aes_key;

void set_aes_key(void)
{
	if(AES_set_encrypt_key(real_key, 128, &aes_key) < 0) {
		printf("set_aes_key error\n");
		exit(-1);
	}
}
void aes_encrypt(U8 *in, U8 *out)
{
	AES_ecb_encrypt(in, out, &aes_key, AES_ENCRYPT);
}

int main(int argc, char **argv)
{
	char msg[MSG_SIZE_MAX];
	struct shm_msg *client_msg;
	struct shm_msg *server_msg;
	void *addr;
	int fd;
	
	set_aes_key();

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
				memcpy(plain, client_msg->msg, client_msg->len);
				client_msg->status = 0;
				break;
			}
		}

		if(client_msg->len == sizeof(END_MSG)) {
			printf("end msg : %s\n", client_msg->msg);
			break;
		}

		/* encrypt */
		aes_encrypt(plain, enc);

		/* prepare msg */
		server_msg->status = 0;
		server_msg->len = sizeof(enc);

		/* send reply */
		memcpy(server_msg->msg, enc, sizeof(enc));
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
