#include "queue.h"

// • Každý zákazník je jednoznačně identifikován číslem idZ, 0<idZ<=NZ
//• Po spuštění vypíše: A: Z idZ: started
//• Následně čeká pomocí volání usleep náhodný čas v intervalu <0,TZ>
//• Pokud je pošta uzavřena
//◦ Vypíše: A: Z idZ: going home
//◦ Proces končí
//• Pokud je pošta otevřená, náhodně vybere činnost X---číslo z intervalu <1,3>
//◦ Vypíše: A: Z idZ: entering office for a service X
//◦ Zařadí se do fronty X a čeká na zavolání úředníkem.
//◦ Vypíše: Z idZ: called by office worker
//◦ Následně čeká pomocí volání usleep náhodný čas v intervalu <0,10> (synchronizace s
//úředníkem na dokončení žádosti není vyžadována).
//◦ Vypíše: A: Z idZ: going home
//◦ Proces končí
void customer_process(int id, int TZ) {

    print_msg("Z %d: started", id);

    if (*isClosed) {
        print_msg("Z %d: going home", id);
        fclose(f_out);
        exit(0);
    }

    srand(getpid());
    int X = rand() % 3 + 1;

    if (TZ > 0){

        srand(INT_MIN);
        usleep((rand() % TZ) * 1000);

    }

    print_msg("Z %d: entering office for a service %d", id, X);

    sem_wait(mutex_customer);
    enqueue(queues[X], id);
    sem_post(mutex_customer);

    sem_wait(mutex_clerk);
    print_msg("Z %d: called by office worker", id);
    usleep((rand() % 10) * 1000);
    sem_post(mutex_clerk);

    print_msg("Z %d: going home", id);
    fclose(f_out);
    exit(0);

}

//Každý úředník je jednoznačně identifikován číslem idU, 0<idU<=NU
//• Po spuštění vypíše: A: U idU: started
//[začátek cyklu]
//• Úředník jde obsloužit zákazníka z fronty X (vybere náhodně libovolnou neprázdnou).
//◦ Vypíše: A: U idU: serving a service of type X
//◦ Následně čeká pomocí volání usleep náhodný čas v intervalu <0,10>
//◦ Vypíše: A: U idU: service finished
//◦ Pokračuje na [začátek cyklu]
//• Pokud v žádné frontě nečeká zákazník a pošta je otevřená vypíše
//◦ Vypíše: A: U idU: taking break
//◦ Následně čeká pomocí volání usleep náhodný čas v intervalu <0,TU>
//◦ Vypíše: A: U idU: break finished
//◦ Pokračuje na [začátek cyklu]
//• Pokud v žádné frontě nečeká zákazník a pošta je zavřená
//◦ Vypíše: A: U idU: going home
//◦ Proces končí
void clerk_process(int id, int TU) {
    print_msg("U %d: started", id);
    srand(getpid());
    while (true) {

        int X = -1;

        sem_wait(mutex_customer);

        for (int i = 1; i <= 3; i++) {
            if (!isEmpty(queues[i])) {
                X = i;
                break;
            }
        }

        if (X != -1) {

            dequeue(queues[X]);
            sem_post(mutex_customer);
            print_msg("U %d: serving a service of type %d", id, X);
            usleep((rand() % 10) * 1000);
            print_msg("U %d: service finished", id);

        } else {

            if (*isClosed) {

                sem_post(mutex_customer);
                print_msg("U %d: going home", id);
                fclose(f_out);
                exit(0);

            } else {

                sem_post(mutex_customer);
                print_msg("U %d: taking break", id);

                if (TU == 0) {

                    print_msg("U %d: break finished", id);

                } else {

                    usleep((rand() % TU) * 1000);
                    print_msg("U %d: break finished", id);
                }

            }
        }
    }
}

//Hlavní proces vytváří ihned po spuštění NZ procesů zákazníků a NU procesů úředníků.
//• Čeká pomocí volání usleep náhodný čas v intervalu <F/2,F>
//• Vypíše: A: closing
//• Poté čeká na ukončení všech procesů, které aplikace vytváří. Jakmile jsou tyto procesy
//ukončeny, ukončí se i hlavní proces s kódem (exit code) 0.
// /proj2 NZ NU TZ TU F
//• NZ: počet zákazníků
//• NU: počet úředníků
//• TZ: Maximální čas v milisekundách, po který zákazník po svém vytvoření čeká, než vejde na
//poštu (eventuálně odchází s nepořízenou). 0<=TZ<=10000
//• TU: Maximální délka přestávky úředníka v milisekundách. 0<=TU<=100
//• F: Maximální čas v milisekundách, po kterém je uzavřena pošta pro nově příchozí.
//0<=F<=10000
int main(int argc, char *argv[]) {

    if (argc != 6) {
        fprintf(stderr, "Wrong number of arguments (expected 5, got %d)\n", argc - 1);
        return 1;
    }

    int NZ = atoi(argv[1]);
    int NU = atoi(argv[2]);
    int TZ = atoi(argv[3]);
    int TU = atoi(argv[4]);
    int F = atoi(argv[5]);

    if (NZ <= 0 || NU <= 0 || TZ < 0 || TU < 0 || F < 0 || TZ > 10000 || TU > 100 || F > 10000) {
        fprintf(stderr, "Wrong arguments\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        for (int j = 0; argv[i][j] != '\0'; j++) {
            if (argv[i][j] < '0' || argv[i][j] > '9') {
                fprintf(stderr, "Wrong arguments\n");
                return 1;
            }
        }
    }

    init_sem();

    for (int i = 0; i < 4; i++) {
        queues[i] = initQueue(NZ);
    }

    f_out = fopen("proj2.out", "w");

    if (f_out == NULL) {
        perror("Error opening file");
        cleanup();
        return 1;
    }

    // Create customer processes
    for (int i = 1; i <= NZ; i++) {
        int pid = fork();
        if (pid == 0) {
            customer_process(i, TZ);
            exit(0);
        } else if (pid < 0) {
            perror("Error creating customer process");
            cleanup();
            exit(1);
        }
    }

    // Create clerk processes
    for (int i = 1; i <= NU; i++) {
        int pid = fork();
        if (pid == 0) {
            clerk_process(i, TU);
            exit(0);
        } else if (pid < 0) {
            perror("Error creating clerk process");
            cleanup();
            exit(1);
        }
    }

    if (F > 0){

        srand(INT_MAX);
        usleep((rand() % (F / 2)) * 1000 + (F / 2) * 1000);
        print_msg("closing");
        *isClosed = true;

    } else {

        print_msg("closing");
        *isClosed = true;

    }

    // Wait for all child processes to finish
    int status;
    while (wait(&status) > 0);

    cleanup();
    return 0;

}
