#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <limits.h>

#define N 4
#define D 5
#define K 3

pthread_mutex_t mutex;
pthread_cond_t owca;
pthread_cond_t pasterz;
pthread_t tid[N];

int krok_pasterza = 0;
int kroki[N];
__thread int id;

int ostatni()
{
 int ostat = INT_MAX;
 for (int i = 0; i < N; i++)
 {
		if (kroki[i] < ostat)
			ostat = kroki[i];
 }
 return ostat;
}

void *krokOwcy(void *arg)
{
 for (int i = 0; i < D; i++)
 {
		pthread_mutex_lock(&mutex);

		while (kroki[id] - K >= krok_pasterza)
			pthread_cond_wait(&owca, &mutex);

		kroki[id] += 1;

		printf("Owca %d robi krok\nJest na kroku: %d\nOstatnia owca jest na kroku: %d\nPasterz jest na kroku: %d\n\n", id + 1, kroki[id], ostatni(), krok_pasterza);

		if (krok_pasterza < ostatni() + K)
			pthread_cond_signal(&pasterz);

		pthread_mutex_unlock(&mutex);
 }
}

void krokPasterza()
{
 for (int i = 0; i < D; i++)
 {
		pthread_mutex_lock(&mutex);

		krok_pasterza += 1;
		pthread_cond_broadcast(&owca);
		printf("Pasterz robi krok\nJest na kroku: %d\nOstatnia owca jest na %d kroku\n\n", krok_pasterza, ostatni());

		pthread_mutex_unlock(&mutex);
 }
}

int main()
{
 int temp;
 for (int i = 0; i < N; i++)
		kroki[i] = 0;

 if (pthread_mutex_init(&mutex, NULL) != 0)
 {
		perror("Blad mutexa");
		exit(1);
 }

 for (int i = 0; i < N; i++)
 {
		temp = pthread_create(&(tid[i]), NULL, &krokOwcy, NULL);
		if (temp != 0)
		{
			perror("Blad watku");
			exit(1);
		}
 }

 krokPasterza();

 for (int i = 0; i < N; i++)
		pthread_join(tid[i], NULL);
 pthread_mutex_destroy(&mutex);
}
