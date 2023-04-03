#include <stdio.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <limits.h>

#define liczba_filozofow 5

struct Filozof
{
    int id, zjadl;
    bool gotownosc;
};

static struct sembuf buf;

void signal(int semid, int semnum)
{
    buf.sem_num = semnum;
    buf.sem_op = 1;
    buf.sem_flg = 0;
    if (semop(semid, &buf, 1) == -1)
    {
        perror("\nBlad podnoszenia semafora\n");
        exit(1);
    }
}

void wait(int semid, int semnum)
{
    buf.sem_num = semnum;
    buf.sem_op = -1;
    buf.sem_flg = 0;
    if (semop(semid, &buf, 1) == -1)
    {
        perror("\nBlad opuszczania semafora\n");
        exit(1);
    }
}

void Filozof_prog(int id, int pozwolenie_sem, int filozofy_sem, int widelec_gotownosc_sem, struct Filozof filozofy[], bool widelce[])
{
    srand(getpid());
    const int lewy_widelec = id;
    const int prawy_widelec = (id + 1) % liczba_filozofow;
    int sleepTime;

    while (true)
    {
        sleepTime = 1 + (rand() % 15);
        printf("%i: Zaczal myslec: %is\n", id, sleepTime);
        sleep(sleepTime);
        wait(filozofy_sem, id);

        filozofy[id].gotownosc = true;
        signal(filozofy_sem, id);

        printf("%i: Czeka na pozwolenie\n", id);
        wait(pozwolenie_sem, id);

        sleepTime = 1 + (rand() % 15);
        printf("%i: Zaczal jesc %is\n", id, sleepTime);
        sleep(sleepTime);

        wait(widelec_gotownosc_sem, 0);
        widelce[lewy_widelec] = true;
        widelce[prawy_widelec] = true;
        signal(widelec_gotownosc_sem, 0);

        wait(filozofy_sem, id);
        filozofy[id].zjadl += 1;
        signal(filozofy_sem, id);

        printf("%i: Skaczyl jesc\n", id);
    }
}

void sort(struct Filozof sorted_kolej[])
{
    bool zamiana = false;
    int sorting = liczba_filozofow;
    do
    {
        zamiana = false;
        for (int i = 0; i < sorting - 1; i++)
        {
            if (sorted_kolej[i].zjadl > sorted_kolej[i + 1].zjadl)
            {
                struct Filozof fil = sorted_kolej[i];
                sorted_kolej[i] = sorted_kolej[i + 1];
                sorted_kolej[i + 1] = fil;
                zamiana = true;
            }
        }
        sorting--;
    } while (zamiana == true);
}

void dodac_do_kolejki(struct Filozof sorted_kolej[], struct Filozof newFilozof)
{
    bool check_kolej = false;
    for (int i = 0; i < liczba_filozofow; i++)
    {
        if (sorted_kolej[i].id == -1)
        {
            sorted_kolej[i] = newFilozof;
            check_kolej = true;
            break;
        }
    }
    if (check_kolej == false)
    {
        perror("\nW kolejce nie ma miejsca!\n");
    }
    sort(sorted_kolej);
}

struct Filozof usunac_z_kolejki(struct Filozof sorted_kolej[])
{
    if (sorted_kolej[0].id == -1)
    {
        perror("\nW kolejce nie ma filozofow!\n");
    }
    struct Filozof first = sorted_kolej[0];

    for (int i = 0; i < liczba_filozofow - 1; i++)
    {
        sorted_kolej[i] = sorted_kolej[i + 1];
    }

    struct Filozof null_Filozof = {-1, INT_MAX, false};
    sorted_kolej[liczba_filozofow - 1] = null_Filozof;

    return first;
}

void print_kolej(int widelec_num, struct Filozof *kolej)
{
    int i = 0;
    int filozof_id = kolej[i].id;
    printf("Widelec %i: ", widelec_num);
    while (filozof_id != -1)
    {
        printf("%i (%i), ", filozof_id, kolej[i].zjadl);
        i++;
        filozof_id = kolej[i].id;
    }
    printf("\n");
}

void kelner(int pozwolenie_sem, int filozofy_sem, int widelec_gotownosc_sem, struct Filozof filozofy[], bool widelce[])
{
    struct Filozof *widelec_kolej[liczba_filozofow];

    for (int i = 0; i < liczba_filozofow; i++)
    {
        widelec_kolej[i] = (struct Filozof *)malloc(liczba_filozofow * sizeof(struct Filozof));
        for (int j = 0; j < liczba_filozofow; j++)
        {
            widelec_kolej[i][j].id = -1;
            widelec_kolej[i][j].zjadl = INT_MAX;
            widelec_kolej[i][j].gotownosc = false;
        }
    }

    while (true)
    {
        for (int id = 0; id < liczba_filozofow; id++)
        {
            wait(filozofy_sem, id);

            if (filozofy[id].gotownosc)
            {
                const int lewy_widelec = id;
                const int prawy_widelec = (id + 1) % liczba_filozofow;

                dodac_do_kolejki(widelec_kolej[lewy_widelec], filozofy[id]);
                dodac_do_kolejki(widelec_kolej[prawy_widelec], filozofy[id]);

                filozofy[id].gotownosc = false;
                printf("Filozof %i dodany do kolejki\n", id);
                print_kolej(lewy_widelec, widelec_kolej[lewy_widelec]);
                print_kolej(prawy_widelec, widelec_kolej[prawy_widelec]);
            }

            signal(filozofy_sem, id);
        }

        for (int id = 0; id < liczba_filozofow; id++)
        {
            const int lewy_widelec = id;
            const int prawy_widelec = (id + 1) % liczba_filozofow;

            wait(widelec_gotownosc_sem, 0);
            if (widelec_kolej[lewy_widelec][0].id == id && widelec_kolej[prawy_widelec][0].id == id && widelce[lewy_widelec] && widelce[prawy_widelec])
            {
                usunac_z_kolejki(widelec_kolej[lewy_widelec]);
                usunac_z_kolejki(widelec_kolej[prawy_widelec]);

                widelce[lewy_widelec] = false;
                widelce[prawy_widelec] = false;

                signal(pozwolenie_sem, id);
                printf("Kelner pozwala jesc filozofu: %i\n", id);
            }
            signal(widelec_gotownosc_sem, 0);
        }
    }
}

int main()
{
    //tworzymy współdzieloną pamięć dla filozofów
    int filozofy_shmid = shmget(IPC_PRIVATE, liczba_filozofow * sizeof(struct Filozof), IPC_CREAT | 0600);
    //przydzielamy filozofów do segmentu pamięci
    struct Filozof *filozofy = (struct Filozof *)shmat(filozofy_shmid, NULL, 0);

    //tworzymy współdzieloną pamięć dla widelców
    int widelce_shmid = shmget(IPC_PRIVATE, liczba_filozofow * sizeof(bool), IPC_CREAT | 0600);
    //przydzielamy widelce do segmentu pamięci
    bool *widelce = (bool *)shmat(widelce_shmid, NULL, 0);

    //tworzenie semaforów
    int widelec_gotownosc_sem = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    //prypisanie gotowności
    semctl(widelec_gotownosc_sem, 0, SETVAL, 1);
    int pozwolenie_sem = semget(IPC_PRIVATE, liczba_filozofow, IPC_CREAT | 0600);
    int filozofy_sem = semget(IPC_PRIVATE, liczba_filozofow, IPC_CREAT | 0600);


    for (int i = 0; i < liczba_filozofow; i++)
    {
        struct Filozof f = {
            i, 0, false};
        filozofy[i] = f;
        widelce[i] = true;
        semctl(pozwolenie_sem, i, SETVAL, 0);
        semctl(filozofy_sem, i, SETVAL, 1);
    }

    for (int i = 0; i < liczba_filozofow; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            Filozof_prog(i, pozwolenie_sem, filozofy_sem, widelec_gotownosc_sem, filozofy, widelce);
            exit(0);
        }
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        kelner(pozwolenie_sem, filozofy_sem, widelec_gotownosc_sem, filozofy, widelce);
        exit(0);
    }

    return 0;
}
