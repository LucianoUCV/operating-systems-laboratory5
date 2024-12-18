#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <sys/wait.h>

#define SHM_NAME "/shm_counter"
#define MAX_COUNT 1000

struct shared_mem {
	int counter;
	sem_t sem;
};

void count(struct shared_mem* shm_ptr, pid_t pid) {
	for (int i = 0; i < MAX_COUNT; i++) {
		sem_wait(&shm_ptr->sem);
		if (rand() % 2 == 0) {
			if(!pid) printf("Child");
			else printf("Parent");
			printf(" process incremented counter by 1. (Current value: %d)\n", ++shm_ptr->counter);
		}
		sem_post(&shm_ptr->sem);
		usleep(10000);
	}
}

int main() {
	srand(time(NULL));

	int shm_fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0600);
	if (shm_fd == -1) {
		perror("shm_open");
		exit(EXIT_FAILURE);
	}

	if (ftruncate(shm_fd, sizeof(struct shared_mem)) == -1) {
		perror("ftruncate");
		exit(EXIT_FAILURE);
	}

	struct shared_mem* shm_ptr = mmap(NULL, sizeof(*shm_ptr), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (shm_ptr == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}

	shm_ptr->counter = 0;
	if (sem_init(&shm_ptr->sem, 1, 1) == -1 ) {
		perror("sem_init");
		exit(EXIT_FAILURE);
	}

	pid_t pid = fork();
	if (pid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if (pid == 0) {
		count(shm_ptr, pid);
	}
	else {
		count(shm_ptr, pid);
		wait(NULL);
		printf("Final counter value: %d\n", shm_ptr->counter);
	}

	munmap(shm_ptr, sizeof(*shm_ptr));
	close(shm_fd);
	shm_unlink(SHM_NAME);

	return 0;
}

