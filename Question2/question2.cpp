#include <pthread.h>
#include "bits/stdc++.h"

using namespace std;

int tam_nums;
int NUM_THREADS;
vector<int> nums;
bool ordenado = true;

struct Param{ // Estrutura de argumentos, neste caso os limites de cada thread
    int thread_id;
    int low, high;
    Param(): thread_id(-1),low(-1),high(-1) {}
    Param(int id,int l, int r): thread_id(id), low(l), high(r) {}
};

void *check_sort(void *args){
    int id = ((Param*)args)->thread_id;
    int l  = ((Param*)args)->low;
    int r  = ((Param*)args)->high;
    
    if(!ordenado){                                          // Se já estiver desordenado, a thread retorna
        pthread_exit(NULL);
    }
    for(int i = l; i <= r && ordenado; i++){                // Cada thread realiza um "bubble-check" no seu intervalo designado
        for(int j = i + 1; j <= r && ordenado; j++){
            if(!ordenado){
                pthread_exit(NULL);
            }
            if(nums[i] > nums[j]){ 
                ordenado = false;
                pthread_exit(NULL);
            }
        }
    }
    if(l && nums[l] < nums[l-1]) ordenado = false;          //Checa se o primeiro numero desse intervalo eh maior que o ultimo do intervalo anterior
    pthread_exit(NULL);
}

int main(){
    freopen("input.txt", "r", stdin);
    freopen("output.txt", "w", stdout);

    int t;
    cin >> t;
    for(int cs = 1; cs <= t; cs++){
        scanf("%d %d", &NUM_THREADS, &tam_nums);

        nums.resize(tam_nums);

        for(int i = 0; i < tam_nums; i++)
            scanf("%d", &nums[i]);

        ordenado = true;

        Param threads_args[NUM_THREADS];
        pthread_t threads_exec[NUM_THREADS];
        int intervalo = (tam_nums + NUM_THREADS - 1)/NUM_THREADS;       // Tamanho do intervalo (teto)
        
        int l = 0, r = intervalo - 1;                                   // Inicializamos variáveis para manter controle do intervalo de cada thread
        for(int i = 0; i < NUM_THREADS; i++){
            Param args(i, l, r);
            pthread_create(&threads_exec[i], NULL, check_sort, &args);  // Inicializa a thread com os argumentos do seu intervalo
                
                r += intervalo;                                         // Seta o intervalo R aumentando a cada iteracao o tamanho do intervalo definido
                if(r >= tam_nums) r = tam_nums - 1;                     // Caso r (o limite superior) seja maior que o tamanho do array, limita de volta para o último elemento
                l = r - intervalo + 1;                                  // Seta o intervalo inferior com uma distância "intervalo" para r
        }
        for(int i = 0; i < NUM_THREADS; i++){
            pthread_join(threads_exec[i], NULL);                        // Espera todas as threads terminarem sua execução
        }

        if(ordenado) printf("O ARRAY %d ESTA ORDENADO\n", cs);
        else printf("O ARRAY %d NAO ESTA ORDENADO\n", cs);
    }

    return 0;
}

