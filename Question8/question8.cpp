#include "bits/stdc++.h"
#include <pthread.h>
#include <unistd.h>

using namespace std;

#define endl '\n'
#define MAX_VOLTAS 10
#define QNTD_PASSAGEIROS 20
#define CAPACIDADE_CARRINHO 10

pthread_cond_t carrinho_cheio = PTHREAD_COND_INITIALIZER;   // sinaliza que o carrinho está cheio
pthread_cond_t acabou_passeio = PTHREAD_COND_INITIALIZER;   // sinaliza que a montanha russa terminou
pthread_cond_t fila_nao_vazia = PTHREAD_COND_INITIALIZER;   // sinaliza que tem alguem na fila

pthread_mutex_t mutex_fila = PTHREAD_MUTEX_INITIALIZER;     // para controle da fila
pthread_mutex_t mutex_carrinho = PTHREAD_MUTEX_INITIALIZER; // para controle do carrinho

pthread_mutex_t mutex_terminal = PTHREAD_MUTEX_INITIALIZER; // para controle da saída

bool terminou = false;                                      // avisa que não tem mais passeio
bool comecou_passeio = false;                               // flag para avisar que o passeio está rodando


queue<int> fila_de_espera;

vector<int> carrinho;
vector<bool> foi_na_ultima;

// montanha russa
    // viaja com o carrinho
    // tira os passageiros do carrinho no final do passeio
// carrinho 
    // colocar os passageiros no carrinho
    // esperar encher os assentos
    // acordar a montanha russa
// passageiros
    // entrar na fila
    
void printRollercoaster(){                             // Funcao para desenhar a montanha russa
    double y;   
    int m, x;
    
    // Carrinho
    for(int i = 0; i < 14; i++) printf(" ");
    printf("___\n");
    for(int i = 0; i < 8; i++) printf(" ");
    printf("_-_- |___|\n");
    for(int i = 0; i < 13; i++) printf(" ");
    printf("O   O\n");
    
    // Trilho da montanha russa
    for (y = 1; y >= 0; y -= 0.1) {
        m = asin(y) * 10;
        for (x = 1; x < m; x++)
            printf(" ");
        printf("#");
        for (; x < 31 - m; x++)
            printf(" ");
        printf("#\n");
    }
    for(y = 0; y <= 1; y += 0.1){
            m = 31 + asin(y)*10;
            for(x = 1; x < m; x++) printf(" ");
            printf("  #");
            for( ; x < 93 - m; x++) printf(" ");
            printf(" #\n");
    }  
}
    
void *func_passageiro(void *args){
    int id = *((int*)args);
    
    pthread_mutex_lock(&mutex_fila);        
    fila_de_espera.push(id);                                     // Coloca o passageiro na fila pelo id
    pthread_mutex_unlock(&mutex_fila);
    
    pthread_cond_signal(&fila_nao_vazia);                       // Manda sinal de list não vazia

    pthread_exit(NULL);
}
//Supondo que o carro e cada passageiro sejam representados por uma thread diferente, escreva um programa, usando pthreads, que simule o sistema descrito. Após as 10 voltas, o programa deve ser encerrado );

void *func_montanha_russa(void *args){
    
    for(int i = 0; i < MAX_VOLTAS; i++){                       // Iteracao com o numero de voltas diarias
        pthread_mutex_lock(&mutex_carrinho);
        pthread_cond_wait(&carrinho_cheio, &mutex_carrinho);   // Espera o carrinho ficar cheio para iniciar o passeio
        comecou_passeio = true;
        pthread_mutex_lock(&mutex_terminal);
        cout << "PASSEANDO..." << endl;
        printRollercoaster();                                  // Desenha a montanha russa no terminal
        usleep(5000000);                                       // Dorme por 5 segundos
        pthread_mutex_unlock(&mutex_terminal);

        pthread_t thread_passageiro[CAPACIDADE_CARRINHO];
        int j = 0, id_thread[CAPACIDADE_CARRINHO];
        
        comecou_passeio = false;                               // Terminou o passeio

        while(carrinho.size()){                                // Enquanto tiver passageiros no carrinho
            pthread_mutex_lock(&mutex_terminal);
            cout << "Retirando as pessoas do carrinho para o proximo passeio\n";
            pthread_mutex_unlock(&mutex_terminal);
            id_thread[j] = carrinho.back();                    // Tira os passageiros do carrinho
            carrinho.pop_back();
            pthread_create(&thread_passageiro[j], NULL, func_passageiro, &id_thread[j]);    // "Chama" os passageiros novamente
            j++;
        }

        pthread_mutex_unlock(&mutex_carrinho);

        pthread_cond_broadcast(&acabou_passeio);               // Manda o sinal que o passeio terminou
        pthread_mutex_lock(&mutex_terminal);
        cout << "Acabou o passeio" << endl;
        pthread_mutex_unlock(&mutex_terminal);
    }

    terminou = true;
    cout << "Acabou por hoje, pessoal.\n";

    pthread_exit(NULL);
}

void *func_carrinho(void *args){
    while(!terminou){

        pthread_mutex_lock(&mutex_fila);

        
        while(fila_de_espera.size() == 0){

            pthread_cond_wait(&fila_nao_vazia, &mutex_fila);    // Espera a fila ter algum passageiro

        }
        // Esperar ter alguem na fila, botar no carrinho
        pthread_mutex_lock(&mutex_carrinho);

        if(carrinho.size() == CAPACIDADE_CARRINHO){
            // carrinho vai sair
            pthread_cond_broadcast(&carrinho_cheio);           // sinaliza que o carrinho está cheio
            pthread_mutex_unlock(&mutex_carrinho);             // Desbloqueia o carrinho
            pthread_mutex_unlock(&mutex_fila);
            
            pthread_mutex_lock(&mutex_terminal);
            cout << "Carrinho cheio" << endl;
            pthread_mutex_unlock(&mutex_terminal);

            pthread_mutex_lock(&mutex_carrinho);
            while(comecou_passeio){
                pthread_cond_wait(&acabou_passeio, &mutex_carrinho); // Espera acabar o passeio 
            }
            pthread_mutex_unlock(&mutex_carrinho);
        }
        else{
            int prox = fila_de_espera.front();
            fila_de_espera.pop();

            if(foi_na_ultima[prox]){                    // se a pessoa na frente já foi na ultima vez
                foi_na_ultima[prox] = false;            // sinaliza que não foi na ultima
                pthread_mutex_lock(&mutex_terminal);    
                cout << prox << " foi na ultima.\n";
                pthread_mutex_unlock(&mutex_terminal);
                fila_de_espera.push(prox);              // coloca no final da fila
            }
            else{
                carrinho.push_back(prox);               // coloca no carrinho
                foi_na_ultima[prox] = true;             // avisa que o passageiro foi na ultima
                pthread_mutex_lock(&mutex_terminal);
                cout << "Colocando " << prox << " no carrinho\n";
                pthread_mutex_unlock(&mutex_terminal);
            }
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
    pthread_create(&thread_montanha, NULL, func_montanha_russa, NULL);
    pthread_create(&thread_carro, NULL, func_carrinho, NULL);

    pthread_join(thread_carro, NULL);
    pthread_join(thread_montanha, NULL);

    return 0;
}