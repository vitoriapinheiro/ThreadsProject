#include <pthread.h>
#include "bits/stdc++.h"
#include <iostream>

using namespace std;

#define endl '\n'
#define NUM_THREADS 10
#define MOD 1000000007

mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());
pthread_mutex_t mutex_buffer = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_acesso = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_arquivo_thread = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t vazio = PTHREAD_COND_INITIALIZER;
pthread_cond_t update = PTHREAD_COND_INITIALIZER;

struct Param{
    int id, a, b;
    int thread_id = 0;
    Param() : id(-1), a(-1), b(-1) {}
    Param(int i, int x, int y) : id(i), a(x), b(y) {}
};

struct Exec{
    int tipo;
    Param args;
    Exec() : tipo(-1), args() {}
    Exec(int t, Param p) : tipo(t), args(p) {}
};

ofstream arq_saida, arq_threads_kernel;
bool terminou_agendamento = false;
bool terminou_despacho = false;
vector<bool> sinal_thread;
queue<Exec> buffer_funcoes;
unordered_map<int, bool> criou_id;
unordered_map<int, int> buffer_resultados;

int gera_id(){
    int id = rng() % MOD;
    while(criou_id.count(id)){
        id = rng() % MOD;
    }
    criou_id[id] = true;
    return id;
}

void *soma_int(void *args){
    Param p_args = *((Param *)args);
    long long a = p_args.a;
    long long b = p_args.b;
    int id = p_args.id;
    int thread_id = p_args.thread_id;

    long long res = (a + b) % MOD;

    pthread_mutex_lock(&mutex_arquivo_thread);
    arq_threads_kernel << "Soma_Int de ID " << id << " executado pela thread " << thread_id << endl;
    pthread_mutex_unlock(&mutex_arquivo_thread);

    pthread_mutex_lock(&mutex_acesso);

    buffer_resultados[id] = (int)res;
    sinal_thread[thread_id] = false;

    pthread_mutex_unlock(&mutex_acesso);
    pthread_cond_signal(&update);


    pthread_exit(NULL);
}

void *diff_quadrados(void *args){
    Param p_args = *((Param *)args);
    long long a = p_args.a;
    long long b = p_args.b;
    int id = p_args.id;
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
    long long a = p_args.a;
    long long b = p_args.b;
    int id = p_args.id;
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

void *despachante(void * args){
    pthread_t thread_exec[NUM_THREADS];
    Param param_thread[NUM_THREADS];
    while(true){
        pthread_mutex_lock(&mutex_buffer);

        while(buffer_funcoes.size() == 0){
            if(terminou_agendamento){
                pthread_mutex_unlock(&mutex_buffer);
                for(int i=0;i<NUM_THREADS;i++){
                    pthread_join(thread_exec[i],NULL);
                    pthread_cond_broadcast(&update);
                }
                terminou_despacho = true;
                pthread_cond_broadcast(&update);
                pthread_exit(NULL);
            }
            else pthread_cond_wait(&vazio, &mutex_buffer);
        }
        Exec atual = buffer_funcoes.front();
        buffer_funcoes.pop();

        pthread_mutex_unlock(&mutex_buffer);

        int i = 0;
        bool executou = false;

        while(!executou){
            pthread_mutex_lock(&mutex_acesso);
            if(!sinal_thread[i]){
                pthread_mutex_unlock(&mutex_acesso);
                pthread_join(thread_exec[i],NULL);
                executou = true;

                pthread_mutex_lock(&mutex_acesso);
                sinal_thread[i] = true;
                pthread_mutex_unlock(&mutex_acesso);
                param_thread[i] = atual.args;
                param_thread[i].thread_id = i;

                if(atual.tipo == 0) pthread_create(&thread_exec[i], NULL, soma_int, &param_thread[i]);
                else if(atual.tipo == 1) pthread_create(&thread_exec[i], NULL, diff_quadrados, &param_thread[i]);
                else pthread_create(&thread_exec[i], NULL, mult_int, &param_thread[i]);
                break;
            }
            if(i == NUM_THREADS - 1 and !executou) pthread_cond_wait(&update,&mutex_acesso);
            i = (i + 1) % NUM_THREADS;
            pthread_mutex_unlock(&mutex_acesso);
        }

    }

    pthread_exit(NULL);
}

int agendar_execucao(int tipo, Param args){
    Exec buffer_params(tipo, args);

    pthread_mutex_lock(&mutex_buffer);
    
    buffer_funcoes.push(buffer_params);

    pthread_mutex_unlock(&mutex_buffer);
    pthread_cond_signal(&vazio);
    

    return args.id;
}

int pegar_resultado_execucao(int id){
    int res = -1;
    while(true){
        bool foi = false;
        pthread_mutex_lock(&mutex_acesso);

        if(buffer_resultados.count(id)) foi = true;
        else if(!terminou_despacho) pthread_cond_wait(&update, &mutex_acesso);
        else{
            pthread_mutex_unlock(&mutex_acesso);
            break;
        }

        if(foi){
            res = buffer_resultados[id];
            pthread_mutex_unlock(&mutex_acesso);
            break;
        }
        pthread_mutex_unlock(&mutex_acesso);
    }
    return res;
}

int main(){
    sinal_thread.resize(NUM_THREADS  + 1);
    fill(sinal_thread.begin(),sinal_thread.end(),false);

    pthread_t thread_despachante;
    pthread_create(&thread_despachante, NULL, despachante, NULL);

    arq_saida.open("saida.txt");
    arq_threads_kernel.open("saida_threads.txt");

    if(!arq_saida){
        cout << "Ocorreu um erro ao ler o arquivo de saída";
        return 1;
    }

    vector<int> id_exec;
    int num_func = (rng() % (MOD / 100000)) + 100;
    cout << "Criando arquivo de saída..." << endl;

    for(int i = 0; i < num_func; i++){
        int id = gera_id();
        int tipo = (rng() % 3);
        int a = (rng() % MOD), b = (rng() % MOD);

        Param p(id, a, b);

        arq_saida << "Gerou função com ID " << id << " do tipo " << tipo << " de parametros " << a << " e " << b << endl;
            
        agendar_execucao(tipo, p);

        id_exec.push_back(id);
    }

    terminou_agendamento = true;
    pthread_cond_broadcast(&vazio);
    //Checa todas os id executados, se já houve execução
    for (auto &id : id_exec){
        int res = pegar_resultado_execucao(id);
        arq_saida << "Resultado para o id: " << id << " foi " << res << endl;
    }
    pthread_join(thread_despachante,NULL);
    cout << "O arquivo de saída já está pronto!" << endl;

    return 0;
}