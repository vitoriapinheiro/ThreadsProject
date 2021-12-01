#include "bits/stdc++.h"
#include <pthread.h>
#include <unistd.h>

using namespace std;

#define endl '\n'
#define MAX_VOLTAS 10
#define QNTD_PASSAGEIROS 20
#define CAPACIDADE_CARRINHO 10

pthread_cond_t carrinho_cheio = PTHREAD_COND_INITIALIZER;
pthread_cond_t acabou_passeio = PTHREAD_COND_INITIALIZER;
pthread_cond_t fila_nao_vazia = PTHREAD_COND_INITIALIZER;

pthread_mutex_t mutex_fila = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_carrinho = PTHREAD_MUTEX_INITIALIZER;

bool terminou = false;

queue<int> fila_de_espera;

vector<int> carrinho;
vector<bool> foi_na_ultima;

//montanha russa
    // viaja com o carrinho
    // tira os passageiros do carrinho no final do passeio
// carrinho 
    // colocar os passageiros no carrinho
    // esperar encher os assentos
    // acordar a montanha russa
// passageiros
    // entrar na fila
    
void *func_passageiro(void *args){
    int id = *((int*)args);
    
    pthread_mutex_lock(&mutex_fila);
    cout << "Entrou na fila" << endl;
    fila_de_espera.push(id);
    pthread_mutex_unlock(&mutex_fila);
    
    pthread_cond_signal(&fila_nao_vazia);

    pthread_exit(NULL);
}

void *func_montanha_russa(void *args){
    
    for(int i = 0; i < MAX_VOLTAS; i++){
        pthread_mutex_lock(&mutex_carrinho);
        pthread_cond_wait(&carrinho_cheio, &mutex_carrinho);
        //usleep(5000000);
        // desbloquear mutex_carrinho para os passageiros poderem sair
        // chamar passageiros para sair do carro
        pthread_cond_signal(&acabou_passeio);

        pthread_t thread_passageiro[CAPACIDADE_CARRINHO];
        int j = 0, id_thread[CAPACIDADE_CARRINHO];
        
        while(carrinho.size()){
            cout << "Retirando as pessoas do carrinho para o proximo passeio\n";
            id_thread[j] = carrinho.back();
            carrinho.pop_back();
            pthread_create(&thread_passageiro[j], NULL, func_passageiro, &id_thread[j]);
            j++;
        }

        pthread_mutex_unlock(&mutex_carrinho);
    }

    terminou = true;
    cout << "Acabou por hoje, pessoal.\n";

    pthread_exit(NULL);
}

void *func_carrinho(void *args){
    while(!terminou){
        cout << "Carrinho tentando lockar a fila" << endl; 
        pthread_mutex_lock(&mutex_fila);
        pthread_mutex_lock(&mutex_carrinho);
        cout << "Carrinho conseguiu lockar a fila" << endl;
        
        while(fila_de_espera.size() == 0){
            cout << "Alguem na fila" << endl;
            pthread_cond_wait(&fila_nao_vazia, &mutex_fila);
            cout << "Passou da condicao de fila nao vazia" << endl;
        }
        // Esperar ter alguem na fila, botar no carrinho
        if(carrinho.size() == CAPACIDADE_CARRINHO){
            // carrinho vai sair
            pthread_mutex_unlock(&mutex_carrinho);
            pthread_mutex_unlock(&mutex_fila);
            pthread_cond_signal(&carrinho_cheio);
            cout << "Carrinho cheio" << endl;

            pthread_mutex_lock(&mutex_carrinho);
            pthread_cond_wait(&acabou_passeio, &mutex_carrinho);
            pthread_mutex_unlock(&mutex_carrinho);
        }
        else{
            int prox = fila_de_espera.front();
            fila_de_espera.pop();
            carrinho.push_back(prox);
            cout << "Colocando passageiros no carrinho\n";

            pthread_mutex_unlock(&mutex_carrinho);
            pthread_mutex_unlock(&mutex_fila);
        } 
    }
    pthread_exit(NULL);
}

int main(){
    foi_na_ultima.assign(QNTD_PASSAGEIROS, false);

    pthread_t thread_carro;
    pthread_t thread_montanha;
    pthread_t thread_passageiros[QNTD_PASSAGEIROS];
    
    int id_thread[QNTD_PASSAGEIROS];
    for(int i = 0; i < QNTD_PASSAGEIROS; i++){
        id_thread[i] = i;
        pthread_create(&thread_passageiros[i], NULL, func_passageiro, &id_thread[i]);
    }
    
    pthread_create(&thread_carro, NULL, func_carrinho, NULL);
    pthread_create(&thread_montanha, NULL, func_montanha_russa, NULL);

    pthread_join(thread_carro, NULL);
    pthread_join(thread_montanha, NULL);

    return 0;
}