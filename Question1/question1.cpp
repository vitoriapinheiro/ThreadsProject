#include <pthread.h>
#include "bits/stdc++.h"

using namespace std;

#define NUM_THREADS 10 
#define LIMITE 1000000

int contador;
pthread_mutex_t contador_mutex = PTHREAD_MUTEX_INITIALIZER;

void *inc(void *threadid){
    while(true){
        pthread_mutex_lock(&contador_mutex); 
        if(contador >= LIMITE){                     // Evitar que threads bloqueadas antes que o contador alcance o limite incrementem.
            pthread_mutex_unlock(&contador_mutex);
            break;
        }
        contador++;
        if(contador == LIMITE){                    // Verifica a thread que incrementou o contador pela ultima vez e ela imprime o valor.
            printf("Valor alcancado = %d pela thread %d\n", contador, *(int*) threadid);
        }
        pthread_mutex_unlock(&contador_mutex);
    }
    pthread_exit(NULL);
}

int main(){
    contador = 0;
    int threads_id[NUM_THREADS];
    pthread_t threads_exec[NUM_THREADS];
    
    for(int i = 0; i < NUM_THREADS; i++){
        threads_id[i] = i;
        pthread_create(&threads_exec[i], NULL, inc, &threads_id[i]);
    }
    for(int i = 0; i < NUM_THREADS; i++){
        pthread_join(threads_exec[i], NULL);    // Espera todas as threads terminarem sua execução
    }

    return 0;
}