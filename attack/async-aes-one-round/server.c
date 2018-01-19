/**
 *	shared memory server (POSIX)
 *
 *	Copyright (C) 2016  Jinbum Park <jinb.park7@gmail.com> Haehyun Cho <haehyun@asu.edu>
*/

#include "common.h"
#include <openssl/aes.h>
#include <unistd.h>

/* AES T-tables */
static const unsigned int AES_Te0[256] = {
        0xc66363a5U, 0xf87c7c84U, 0xee777799U, 0xf67b7b8dU,
        0xfff2f20dU, 0xd66b6bbdU, 0xde6f6fb1U, 0x91c5c554U,
        0x60303050U, 0x02010103U, 0xce6767a9U, 0x562b2b7dU,
        0xe7fefe19U, 0xb5d7d762U, 0x4dababe6U, 0xec76769aU,
        0x8fcaca45U, 0x1f82829dU, 0x89c9c940U, 0xfa7d7d87U,
        0xeffafa15U, 0xb25959ebU, 0x8e4747c9U, 0xfbf0f00bU,
        0x41adadecU, 0xb3d4d467U, 0x5fa2a2fdU, 0x45afafeaU,
        0x239c9cbfU, 0x53a4a4f7U, 0xe4727296U, 0x9bc0c05bU,
        0x75b7b7c2U, 0xe1fdfd1cU, 0x3d9393aeU, 0x4c26266aU,
        0x6c36365aU, 0x7e3f3f41U, 0xf5f7f702U, 0x83cccc4fU,
        0x6834345cU, 0x51a5a5f4U, 0xd1e5e534U, 0xf9f1f108U,
        0xe2717193U, 0xabd8d873U, 0x62313153U, 0x2a15153fU,
        0x0804040cU, 0x95c7c752U, 0x46232365U, 0x9dc3c35eU,
        0x30181828U, 0x379696a1U, 0x0a05050fU, 0x2f9a9ab5U,
        0x0e070709U, 0x24121236U, 0x1b80809bU, 0xdfe2e23dU,
        0xcdebeb26U, 0x4e272769U, 0x7fb2b2cdU, 0xea75759fU,
        0x1209091bU, 0x1d83839eU, 0x582c2c74U, 0x341a1a2eU,
        0x361b1b2dU, 0xdc6e6eb2U, 0xb45a5aeeU, 0x5ba0a0fbU,
        0xa45252f6U, 0x763b3b4dU, 0xb7d6d661U, 0x7db3b3ceU,
        0x5229297bU, 0xdde3e33eU, 0x5e2f2f71U, 0x13848497U,
        0xa65353f5U, 0xb9d1d168U, 0x00000000U, 0xc1eded2cU,
        0x40202060U, 0xe3fcfc1fU, 0x79b1b1c8U, 0xb65b5bedU,
        0xd46a6abeU, 0x8dcbcb46U, 0x67bebed9U, 0x7239394bU,
        0x944a4adeU, 0x984c4cd4U, 0xb05858e8U, 0x85cfcf4aU,
        0xbbd0d06bU, 0xc5efef2aU, 0x4faaaae5U, 0xedfbfb16U,
        0x864343c5U, 0x9a4d4dd7U, 0x66333355U, 0x11858594U,
        0x8a4545cfU, 0xe9f9f910U, 0x04020206U, 0xfe7f7f81U,
        0xa05050f0U, 0x783c3c44U, 0x259f9fbaU, 0x4ba8a8e3U,
        0xa25151f3U, 0x5da3a3feU, 0x804040c0U, 0x058f8f8aU,
        0x3f9292adU, 0x219d9dbcU, 0x70383848U, 0xf1f5f504U,
        0x63bcbcdfU, 0x77b6b6c1U, 0xafdada75U, 0x42212163U,
        0x20101030U, 0xe5ffff1aU, 0xfdf3f30eU, 0xbfd2d26dU,
        0x81cdcd4cU, 0x180c0c14U, 0x26131335U, 0xc3ecec2fU,
        0xbe5f5fe1U, 0x359797a2U, 0x884444ccU, 0x2e171739U,
        0x93c4c457U, 0x55a7a7f2U, 0xfc7e7e82U, 0x7a3d3d47U,
        0xc86464acU, 0xba5d5de7U, 0x3219192bU, 0xe6737395U,
        0xc06060a0U, 0x19818198U, 0x9e4f4fd1U, 0xa3dcdc7fU,
        0x44222266U, 0x542a2a7eU, 0x3b9090abU, 0x0b888883U,
        0x8c4646caU, 0xc7eeee29U, 0x6bb8b8d3U, 0x2814143cU,
        0xa7dede79U, 0xbc5e5ee2U, 0x160b0b1dU, 0xaddbdb76U,
        0xdbe0e03bU, 0x64323256U, 0x743a3a4eU, 0x140a0a1eU,
        0x924949dbU, 0x0c06060aU, 0x4824246cU, 0xb85c5ce4U,
        0x9fc2c25dU, 0xbdd3d36eU, 0x43acacefU, 0xc46262a6U,
        0x399191a8U, 0x319595a4U, 0xd3e4e437U, 0xf279798bU,
        0xd5e7e732U, 0x8bc8c843U, 0x6e373759U, 0xda6d6db7U,
        0x018d8d8cU, 0xb1d5d564U, 0x9c4e4ed2U, 0x49a9a9e0U,
        0xd86c6cb4U, 0xac5656faU, 0xf3f4f407U, 0xcfeaea25U,
        0xca6565afU, 0xf47a7a8eU, 0x47aeaee9U, 0x10080818U,
        0x6fbabad5U, 0xf0787888U, 0x4a25256fU, 0x5c2e2e72U,
        0x381c1c24U, 0x57a6a6f1U, 0x73b4b4c7U, 0x97c6c651U,
        0xcbe8e823U, 0xa1dddd7cU, 0xe874749cU, 0x3e1f1f21U,
        0x964b4bddU, 0x61bdbddcU, 0x0d8b8b86U, 0x0f8a8a85U,
        0xe0707090U, 0x7c3e3e42U, 0x71b5b5c4U, 0xcc6666aaU,
        0x904848d8U, 0x06030305U, 0xf7f6f601U, 0x1c0e0e12U,
        0xc26161a3U, 0x6a35355fU, 0xae5757f9U, 0x69b9b9d0U,
        0x17868691U, 0x99c1c158U, 0x3a1d1d27U, 0x279e9eb9U,
        0xd9e1e138U, 0xebf8f813U, 0x2b9898b3U, 0x22111133U,
        0xd26969bbU, 0xa9d9d970U, 0x078e8e89U, 0x339494a7U,
        0x2d9b9bb6U, 0x3c1e1e22U, 0x15878792U, 0xc9e9e920U,
        0x87cece49U, 0xaa5555ffU, 0x50282878U, 0xa5dfdf7aU,
        0x038c8c8fU, 0x59a1a1f8U, 0x09898980U, 0x1a0d0d17U,
        0x65bfbfdaU, 0xd7e6e631U, 0x844242c6U, 0xd06868b8U,
        0x824141c3U, 0x299999b0U, 0x5a2d2d77U, 0x1e0f0f11U,
        0x7bb0b0cbU, 0xa85454fcU, 0x6dbbbbd6U, 0x2c16163aU,
};


U8 plain[16] = {0x00,};
U8 enc[16] = {0x10,0x20,};
U8 real_key[16] = {0x9e,0x41,0x00,0x5f,0xd9,0xa0,0xc2,0x28,0xe7,0xb4,0x31,0x29,0x94,0xaa,0x4d,0xc7};
AES_KEY aes_key;
unsigned int *addr_te0 = NULL;

static inline int is_rx(char *flags)
{
    if(flags[0] == 'r' && flags[1] == '-' && flags[2] == 'x')
        return 1;
    return 0;
}

static inline int is_ro(char *flags)
{
    if(flags[0] == 'r' && flags[1] == '-' && flags[2] == '-')
        return 1;
    return 0;
}

static unsigned char* server_memmem(unsigned char *p_src,  int src_len, unsigned char *p_trg, int trg_len)
{
	unsigned char *p = NULL;
	int i;
	int j;

	p = p_src;
	for (i = 0; i < src_len - trg_len + 1; i++)
	{
		if (*p == *p_trg)
		{
			for (j = 1; j < trg_len; j++)
			{
				if (p[j] != p_trg[j])
				{
					break;
				}
			}
			if (j == trg_len)
			{
				return p;
			}
		}
		p++;
	}
	
	return NULL;
}

static void get_te_addr(unsigned long sva, unsigned long eva, unsigned long *te0)
{
    unsigned long size;
    void *ptr;

    size = eva - sva;	
    if(*te0 == 0) {
        *te0 = (unsigned long) server_memmem((unsigned char *)sva, size, (unsigned char*)AES_Te0, sizeof(AES_Te0));
    }
}

static int get_libcrypto_map(void) {
    char line[1024] = {0,};
    char tmp[256] = {0,};
    FILE* fp = NULL;
    unsigned long sva, eva;
    unsigned long te0 = 0;
    char flags[5] = {0,};
    int i = 0;

    fp = fopen("/proc/self/maps", "r");
    if(!fp) {
        return -1;
    }

    while(fgets(line, sizeof(line), fp)) {
        if(strstr(line, "libcrypto")) {
            sscanf(line, "%lx-%lx %5s", &sva, &eva, flags);

            if(is_rx(flags)) {
                get_te_addr(sva, eva, &te0);
                break;
            }
        }
    }

	addr_te0 = (unsigned int *)te0;
    fclose(fp);
    return 0;
}

unsigned int preload_te0(void)
{
	int i;
	unsigned int val, ret;

	ret = 0x00;
	for(i=0; i<256; i++) {
		val = *(addr_te0 + i);
		ret |= val;
	}
	return ret;
}

void bind_cpu(int id)
{
	int r;
	unsigned long mask = 0;

	if(id == 0) mask = 1;
	else if(id == 1) mask = 2;
	else if(id == 2) mask = 4;
	else if(id == 3) mask = 8;

	r = sched_setaffinity(0, sizeof(mask), &mask);
	if(r) {
		printf("sched_setaffinity error\n");
	}
}

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

static int first_try = 3;
int main(int argc, char **argv)
{
	char msg[MSG_SIZE_MAX];
	struct shm_msg *client_msg;
	struct shm_msg *server_msg;

	void *addr;
	int fd;
	ssize_t nread;
	unsigned int sleep_cycle, i;
	
	set_aes_key();
	get_libcrypto_map();
	bind_cpu(3);

	if(argc == 2)
		sleep_cycle = atoi(argv[1]);
	else
		sleep_cycle = 0;

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
	//bind_cpu(2);
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

		/* Pre-load sensitive data!! */
		first_try--;
		if (first_try == 0)
			printf("pre preload\n");
		if(preload_te0() == 0)
			break;
		if (first_try == 0)
			printf("after preload\n");

		for(i=0; i<sleep_cycle; i++)
			asm volatile ("nop");
			
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
