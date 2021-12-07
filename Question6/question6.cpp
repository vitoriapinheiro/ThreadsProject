#include <bits/stdc++.h>
#include <pthread.h>

using namespace std;

#define N 10
#define M 10
#define BANCO_TAM 100
#define endl '\n'

mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());

struct Param{
    int indice;
    Param() : indice(-1){}
    Param(int i) : indice(i) {}
};

vector<int> banco_dados;                                                  // banco de dados 
vector<bool> status_thread_leitora;
vector<bool> status_thread_escritora;                                     // verifica o uso das threads | 0 = disponivel / 1 = sendo usada

pthread_mutex_t banco_mutex = PTHREAD_MUTEX_INITIALIZER;                  // mutex para uso do banco de dados
pthread_mutex_t saida_mutex = PTHREAD_MUTEX_INITIALIZER;                  // mutex para a escrita no terminal

pthread_cond_t lendo_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t escrevendo_cond = PTHREAD_COND_INITIALIZER;                // variáveis de condição

bool lendo = false;
bool escrevendo = false;

void *leitura(void *args){                      
    int indice = ((Param*)args)->indice;  
    status_thread_leitora[indice] = true;

    int i = (rng() % BANCO_TAM);                                           // índice aleatório que sera lido

    pthread_mutex_lock(&banco_mutex);
    while(escrevendo) pthread_cond_wait(&escrevendo_cond, &banco_mutex);   // se uma thread estiver escrevendo, a thread de leitura dorme
    pthread_mutex_unlock(&banco_mutex);                                    // libera o mutex depois de acordar
    lendo = true;

    int j = banco_dados[i];                                                // le um indice do banco de dados
    pthread_mutex_lock(&saida_mutex);
    cout << "Valor " << j << " foi lido" << endl;
    pthread_mutex_unlock(&saida_mutex);

    status_thread_leitora[indice] = false;
    lendo = false;
    pthread_cond_broadcast(&lendo_cond);                                  // acorda todas as threads escritoras
    pthread_exit(NULL);
}

void *escrita(void *args){
    int indice = ((Param*)args)->indice;
    status_thread_escritora[indice] = true;

    int i = (rng() % BANCO_TAM);                                          // escolhe um indice e um valor aleatório para escrever

    int j = (rng() % 1000000);

    pthread_mutex_lock(&banco_mutex);                                     // usa o mutex para escrever no banco de dados (região crítica)
    while(lendo) pthread_cond_wait(&lendo_cond, &banco_mutex);            // dorme caso a thread de leitura estiver lendo

    escrevendo = true;
    banco_dados[i] = j;

    pthread_mutex_lock(&saida_mutex);
    cout << "O valor " << j << " foi escrito no indice " << i << endl;
    pthread_mutex_unlock(&saida_mutex);

    escrevendo = false;
    pthread_mutex_unlock(&banco_mutex);

    pthread_cond_broadcast(&escrevendo_cond);                             // acorda as threads de leitura que estão dormindo
    

    status_thread_escritora[indice] = false;
    pthread_exit(NULL);
}

int main(){
    freopen("output.txt", "w", stdout);

    banco_dados.resize(BANCO_TAM);
    banco_dados.assign(BANCO_TAM, 0);

    status_thread_leitora.resize(N);
    status_thread_escritora.resize(M);

    status_thread_leitora.assign(N, 0);
    status_thread_escritora.assign(M, 0);

    pthread_t thread_escritora[M], thread_leitora[N];                        

    for(int i = 0; i < 100; i++){                                           // o loop pode ser um while(true), porém, para efeitos de teste deixei limitado
        Param j(i % M);
        Param k(i % N);

        if(!status_thread_escritora[j.indice])                                // verifica se a thread escritora está sendo usada antes de criar 
            pthread_create(&thread_escritora[j.indice], NULL, escrita, &j);

        if(!status_thread_leitora[k.indice])                                  // verifica se a thread leitora está sendo usada antes de criar
            pthread_create(&thread_leitora[k.indice], NULL, leitura, &k);
    }

    for(int i = 0; i < N; i++){      
        pthread_join(thread_leitora[i], NULL);                           
    }

    for(int i = 0; i < M; i++){  
        pthread_join(thread_escritora[i], NULL);
    }

    return 0;
}