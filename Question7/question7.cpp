#include <pthread.h>
#include "bits/stdc++.h"
#include <iostream>

using namespace std;

#define endl '\n'
#define NUM_THREADS 10
#define MOD 1000000007

mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());    // Gerador de números pseudo-aleatórios
pthread_mutex_t mutex_arquivo_thread = PTHREAD_MUTEX_INITIALIZER;       // Mutex para o output das threads
pthread_mutex_t mutex_buffer = PTHREAD_MUTEX_INITIALIZER;               // Mutex da fila
pthread_mutex_t mutex_acesso = PTHREAD_MUTEX_INITIALIZER;               // Mutex para o buffer de resultado e sinal da thread     
pthread_cond_t update = PTHREAD_COND_INITIALIZER;                       // Variável condicional que verifica se uma thread terminou a execução
pthread_cond_t vazio = PTHREAD_COND_INITIALIZER;                        // Váriavel condicional que verifica que a fila não está mais vazia

struct Param{                                                           // Struct para os parâmetros de cada função
    int id, a, b;
    int thread_id = 0;
    Param() : id(-1), a(-1), b(-1) {}
    Param(int i, int x, int y) : id(i), a(x), b(y) {}
};

struct Exec{                                                            // Struct para cada uma das execuções em agendamento
    int tipo;
    Param args;
    Exec() : tipo(-1), args() {}
    Exec(int t, Param p) : tipo(t), args(p) {}
};

int last = 0;                                                           // Váriavel para salvar o ultimo indice executado pelo despachante            
ofstream arq_saida, arq_threads_kernel;                                 // Arquivos de saída
bool terminou_agendamento = false;                                      // Flags para avisar que o agendamento e o despacho terminaram
bool terminou_despacho = false;
vector<bool> sinal_thread;                                              // Vetor que avisa que a thread específica terminou sua execução
queue<Exec> buffer_funcoes;
unordered_map<int, bool> criou_id;                                      // Garantir que os ID's não se repetem
unordered_map<int, int> buffer_resultados;                              // Resultados calculados pelas threads

int gera_id(){
    int id = rng() % MOD;       
    while(criou_id.count(id)){                                          // Enquanto o ID gerado for repetido (já estiver no mapa)
        id = rng() % MOD;
    }
    criou_id[id] = true;                                                // Atualiza o mapa com o novo ID
    return id;
}

// O Funcionamento das 3 Funções é exatamente igual exceto pelo resultado calculado.
void *soma_int(void *args){ 
    Param p_args = *((Param *)args);                // Salva numa váriavel local o ponteiro de argumento passado pela thread

    // -------- PARAMETROS ---------
    int id = p_args.id;                 
    long long a = p_args.a;
    long long b = p_args.b;
    int thread_id = p_args.thread_id;

    // -------- RESULTADO ---------
    long long res = (a + b) % MOD;

    pthread_mutex_lock(&mutex_arquivo_thread);      // Bloqueia o acesso ao arquivo de output das threads
    arq_threads_kernel << "Soma_Int de ID " << id << " executado pela thread " << thread_id << endl;
    pthread_mutex_unlock(&mutex_arquivo_thread);

    pthread_mutex_lock(&mutex_acesso);              // Bloqueia o mutex para modificar o buffer de resultados e o array de sinal

    buffer_resultados[id] = (int)res;               // Escreve o resultado da operação no buffer de resultados
    sinal_thread[thread_id] = false;                // Envia o sinal que a thread já terminou

    pthread_mutex_unlock(&mutex_acesso);            
    pthread_cond_signal(&update);                   // Acorda uma das threads (despachante ou pega_resultado)

    
    pthread_exit(NULL);
}

void *diff_quadrados(void *args){
    Param p_args = *((Param *)args);

    int id = p_args.id;
    long long a = p_args.a;
    long long b = p_args.b;
    int thread_id = p_args.thread_id;

    long long res = (((a + b) % MOD) * ((a - b) % MOD)) % MOD;

    pthread_mutex_lock(&mutex_arquivo_thread);
    arq_threads_kernel << "Diff_Quadrados de ID " << id << " executado pela thread " << thread_id << endl;
    pthread_mutex_unlock(&mutex_arquivo_thread);

    pthread_mutex_lock(&mutex_acesso);

    buffer_resultados[id] = (int)res;
    sinal_thread[thread_id] = false;

    pthread_mutex_unlock(&mutex_acesso);
    pthread_cond_signal(&update);


    pthread_exit(NULL);
}

void *mult_int(void *args){
    Param p_args = *((Param *)args);

    int id = p_args.id;
    long long a = p_args.a;
    long long b = p_args.b;
    
    int thread_id = p_args.thread_id % MOD;

    long long res = (a * b) % MOD;

    pthread_mutex_lock(&mutex_arquivo_thread);
    arq_threads_kernel << "Mult_Int de ID " << id << " executado pela thread " << thread_id << endl;
    pthread_mutex_unlock(&mutex_arquivo_thread);

    pthread_mutex_lock(&mutex_acesso);

    buffer_resultados[id] = (int)res;
    sinal_thread[thread_id] = false;

    pthread_mutex_unlock(&mutex_acesso);
    pthread_cond_signal(&update);

    pthread_exit(NULL);
}

void *despachante(void * args){                             // Thread que despacha as funções para as threads que as executam
    Param param_thread[NUM_THREADS];                        // Vetor que guarda os parâmetros utilizados por cada thread
    pthread_t thread_exec[NUM_THREADS];                     // Vetor com as threads possíveis
    
    while(true){
        pthread_mutex_lock(&mutex_buffer);                  // Bloqueia o uso da fila

        while(buffer_funcoes.size() == 0){                  // Enquanto a fila estiver vazia
            if(terminou_agendamento){                       // Se estiver vazia porque a execução terminou, termine o despacho
                pthread_mutex_unlock(&mutex_buffer);        // Desbloqueia o uso da fila
                for(int i = 0; i < NUM_THREADS; i++){       // Espera todas as threads terminarem
                    pthread_join(thread_exec[i], NULL);
                    pthread_cond_signal(&update);           // Avisa que "alguma thread" terminou sua execução
                }
                terminou_despacho = true;                   // Avisa que o despacho acabou
                pthread_cond_broadcast(&update);            // Redundância por garantia
                pthread_exit(NULL);
            }
            else pthread_cond_wait(&vazio, &mutex_buffer);  // Se o agendamento ainda estiver ocorrendo, espere o sinal "vazio"
        }
        
        Exec atual = buffer_funcoes.front();                // Pega o primeiro elemento da fila para executar
        buffer_funcoes.pop();

        pthread_mutex_unlock(&mutex_buffer);                // Desbloqueia o mutex, pois o despachante ja terminou a modificação do buffer

        int i = last;                                       // Inicializa com o ultimo valor    
        bool executou = false;                              // Flag para avisa que a função será executada por alguma thread

        while(!executou){
            pthread_mutex_lock(&mutex_acesso);              // Bloqueia o mutex para verificar o array sinal_thread
            if(!sinal_thread[i]){                           // Caso a thread sinalize que já terminou suas contas, ela está livre
                pthread_mutex_unlock(&mutex_acesso);        // Desbloqueia o mutex para esperar o fim da thread, caso ela não tenha terminado
                pthread_join(thread_exec[i], NULL);         // Espera a thread terminar
                executou = true;

                pthread_mutex_lock(&mutex_acesso);          // Bloqueia o mutex para modificar o array sinal_thread
                sinal_thread[i] = true;
                pthread_mutex_unlock(&mutex_acesso);        

                param_thread[i] = atual.args;               // Pega os parâmetros da thread para passar para a função
                param_thread[i].thread_id = i;

                if(atual.tipo == 0) pthread_create(&thread_exec[i], NULL, soma_int, &param_thread[i]);              // Designa a thread para a função respectiva
                else if(atual.tipo == 1) pthread_create(&thread_exec[i], NULL, diff_quadrados, &param_thread[i]);
                else pthread_create(&thread_exec[i], NULL, mult_int, &param_thread[i]);
                
                break;
            }
            

            i = (i + 1) % NUM_THREADS;
            pthread_mutex_unlock(&mutex_acesso);
        }
        last = i;                                           // Seta o ultimo valor
    }
    
    pthread_exit(NULL);
}

int agendar_execucao(int tipo, Param args){     // Coloca na fila as funções e seus parâmetros pela struct "Exec"
    Exec buffer_params(tipo, args);

    pthread_mutex_lock(&mutex_buffer);          // Bloqueia o acesso a fila
    
    buffer_funcoes.push(buffer_params);         // Coloca a função na fila 

    pthread_mutex_unlock(&mutex_buffer);
    pthread_cond_signal(&vazio);                // Avisa que "há alguém na fila", caso o despachante esteja esperando a condição

    return args.id;                             // Retorna o ID da função
}

int pegar_resultado_execucao(int id){
    int res = -1;                                                               // Variável de retorno do valor calculado pela função
    while(true){
        bool foi = false;                                                       // Flag para avisar que achou o valor
        pthread_mutex_lock(&mutex_acesso);                                      // Bloqueia o acesso à váriavel "buffer_resultados"

        if(buffer_resultados.count(id)) foi = true;                             // Verifica se a thread do respectivo ID terminou sua execução
        else if(!terminou_despacho) pthread_cond_wait(&update, &mutex_acesso);  // Se não, se o despacho não tiver terminado, ele espera alguma thread sinalizar que terminou sua execução
        else{
            pthread_mutex_unlock(&mutex_acesso);                                // Caso o despacho tenha terminado, mas a thread com o id do parâmetro não terminou a execução (ERRO!)
            break;
        }

        if(foi){
            res = buffer_resultados[id];                                        // Caso tenha achado o ID no buffer, retorna o resultado calculado pelas threads
            pthread_mutex_unlock(&mutex_acesso);
            break;
        }
        else pthread_mutex_unlock(&mutex_acesso);
    } 

    return res;
}

int main(){
    sinal_thread.resize(NUM_THREADS  + 1);                          // Aloca array de sinais
    fill(sinal_thread.begin(),sinal_thread.end(),false);

    pthread_t thread_despachante;
    pthread_create(&thread_despachante, NULL, despachante, NULL);   // Thread que despacha as funções

    arq_saida.open("saida.txt");                                    // Inicia os arquivos de saída
    arq_threads_kernel.open("saida_threads.txt");

    if(!arq_saida){
        cout << "Ocorreu um erro ao ler o arquivo de saída";
        return 1;
    }

    vector<int> id_exec;
    int num_func = (rng() % (MOD / 100000)) + 100;                 // Gera um número aleatório de funções
    cout << "Criando arquivo de saída..." << endl;

    for(int i = 0; i < num_func; i++){                             
        int id = gera_id();                                        // Gera os casos de teste
        int tipo = (rng() % 3);
        int a = (rng() % MOD), b = (rng() % MOD);                  

        Param p(id, a, b);                                         // Struct com os parâmetros da função

        arq_saida << "Gerou função com ID " << id << " do tipo " << tipo << " de parametros " << a << " e " << b << endl;
            
        agendar_execucao(tipo, p);

        id_exec.push_back(id);
    }

    terminou_agendamento = true;                                   // Flag para avisar que terminou de agendar todas as funções do usuário
    pthread_cond_broadcast(&vazio);                                // Caso o despachante tenha ficado preso na condição de vazio, reanima para que ele saia
    
    //Checa todas os id executados, se já houve execução
    for (auto &id : id_exec){
        int res = pegar_resultado_execucao(id);
        arq_saida << "Resultado para o id: " << id << " foi " << res << endl;
    }

    pthread_join(thread_despachante,NULL);
    cout << "O arquivo de saída já está pronto!" << endl;

    return 0;
}
