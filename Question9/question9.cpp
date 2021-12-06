#include "bits/stdc++.h"
#include <pthread.h>

using namespace std;

int N;
bool acabou = false;

queue<int> primos, limites;                                             // a fila de limites serve para impedir que o crivo siga para um intervalo que ainda não foi iterado

vector<int> primos_output;                                              // vetor de primos
vector<bool> eh_primo, ja_foi;

pthread_mutex_t mutex_primos = PTHREAD_MUTEX_INITIALIZER;               // controla a fila de primos
pthread_mutex_t mutex_ja_foi = PTHREAD_MUTEX_INITIALIZER;               // controla o vetor que sinaliza os limites
pthread_mutex_t mutex_eh_primo = PTHREAD_MUTEX_INITIALIZER;             // controla o vetor que sinaliza se é ou não primo

pthread_cond_t tem_primo = PTHREAD_COND_INITIALIZER;                    // avisa que tem algum primo na fila    
pthread_cond_t espera_primo = PTHREAD_COND_INITIALIZER;                 // avisa que o respectivo limite pode ser ultrapassado

void func_crivo(){
    pthread_mutex_lock(&mutex_eh_primo);
    eh_primo[0] = eh_primo[1] = false;                                  // seta 0 e 1 como não primos
    pthread_mutex_unlock(&mutex_eh_primo);

    for(int i = 2; i < N; i++){
        pthread_mutex_lock(&mutex_ja_foi);
        if(!limites.empty() && i >= limites.front() && !ja_foi[i]){     // se o crivo estiver numa posição que não pode seguir em frente 
            pthread_cond_wait(&espera_primo, &mutex_ja_foi);            // espera o limite ser liberado        
            limites.pop();
        }

        pthread_mutex_unlock(&mutex_ja_foi);
        pthread_mutex_lock(&mutex_eh_primo);

        if(eh_primo[i]){                           
            pthread_mutex_unlock(&mutex_eh_primo);
            pthread_mutex_lock(&mutex_primos);

            primos.push(i);
            primos_output.push_back(i);
            
            int lim = i * 2;                                            // Define o limite como o dobro do primo uma vez que
            limites.push(lim);                                          // Não existem multiplos do primo nesse intervalo

            pthread_mutex_unlock(&mutex_primos);
            pthread_cond_broadcast(&tem_primo);                         // Avisa que tem alguém na fila de primos
        }
        else pthread_mutex_unlock(&mutex_eh_primo);
    }

    acabou = true;
    pthread_cond_broadcast(&tem_primo);
}

void *marca_crivo(void *args){
    while(!acabou){
        pthread_mutex_lock(&mutex_primos);

        while(!acabou && primos.empty()){
            pthread_cond_wait(&tem_primo, &mutex_primos);
        }

        if(acabou){                                                     // se acabou, pode sair
            pthread_mutex_unlock(&mutex_primos);
            break;
        }

        int primo = primos.front();                                     // pega o primeiro primo na fila
        primos.pop();

        pthread_mutex_unlock(&mutex_primos);

        pthread_mutex_lock(&mutex_eh_primo);
        for(int j = primo * 2; j < N; j += primo){                      // seta os multiplos do primo como nao-primos
            eh_primo[j] = false;
        }
        pthread_mutex_lock(&mutex_ja_foi);

        for(int j = primo; j <= 2 * primo; j++) ja_foi[j] = true;       // seta todos os numeros do intervalo como true

        pthread_mutex_unlock(&mutex_ja_foi);
        pthread_cond_broadcast(&espera_primo);
        pthread_mutex_unlock(&mutex_eh_primo);
    }
    pthread_exit(NULL);
}

int main(){
    freopen("input.txt", "r", stdin);
    freopen("output.txt", "w", stdout);

    int num_threads;                                                
    cin >> num_threads >> N;

    eh_primo.assign(N, true);
    ja_foi.assign(2*N + 2, false);

    pthread_t threads_crivo[num_threads];
    for(int i = 0; i < num_threads; i++){
        pthread_create(&threads_crivo[i], NULL, marca_crivo, NULL);   // Cria as threads que iteram pelos primos
    }

    func_crivo();

    for(int i = 0; i < num_threads; i++){
        pthread_join(threads_crivo[i], NULL);                         // espera todas as threads terminarem sua execucao
    }

    int cnt = 0;
    for(auto p : primos_output){                                      // printa os numeros primos
        cout << p << ' ';
        cnt++;                                                        // divide os primos em linhas  
        if(cnt % 10 == 0) cout << '\n';
    }
    cout << '\n';

    return 0;
}