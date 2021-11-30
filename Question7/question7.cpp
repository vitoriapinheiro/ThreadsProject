#include <pthread.h>
#include "bits/stdc++.h"

using namespace std;

#define endl '\n'
#define NUM_THREADS 10
#define MOD 1000000007

mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());
pthread_mutex_t mutex_buffer_funcoes = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t vazio = PTHREAD_COND_INITIALIZER;

struct Param{
    int id, a, b;
    int thread_id;
    Param() : id(-1), a(-1), b(-1) {}
    Param(int i, int x, int y) : id(i), a(x), b(y) {}
};

struct Exec{
    int tipo;
    Param args;
    Exec() : tipo(-1), args() {}
    Exec(int t, Param p) : tipo(t), args(p) {}
};

ofstream arq_saida;
bool terminou = false;
vector<bool> sinal_thread;
queue<Exec> buffer_funcoes;
unordered_map<int, bool> criou_id;
unordered_map<int, int> buffer_resultados;

int gera_id(){
    int id = rng() % MOD;
    while (criou_id[id]){
        id = rng() % MOD;
    }
    criou_id[id] = true;
    return id;
}

void *soma_int(void *args){
    cout << "......................................." << endl;
    int a = ((Param *)args)->a;
    int b = ((Param *)args)->b;
    int id = ((Param *)args)->id;
    int thread_id = ((Param *)args)->thread_id;


    int res = (a + b) % MOD;
    buffer_resultados[id] = res;
    sinal_thread[thread_id] = false;
    pthread_exit(NULL);
}

void *diff_quadrados(void *args){
    cout << ",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,," << endl;
    int a = ((Param *)args)->a;
    int b = ((Param *)args)->b;
    int id = ((Param *)args)->id;
    int thread_id = ((Param *)args)->thread_id;
    
    int res = (int)((long long)(a + b) * (a - b) % MOD);
    buffer_resultados[id] = res;
    sinal_thread[thread_id] = false;
    pthread_exit(NULL);
}

void *mult_int(void *args){
    cout << "----------------------------------------" << endl;
    int a = ((Param *)args)->a;
    int b = ((Param *)args)->b;
    int id = ((Param *)args)->id;
    int thread_id = ((Param *)args)->thread_id;

    int res = (int)((long long)(a * b) % MOD);
    buffer_resultados[id] = res;
    sinal_thread[thread_id] = false;

    pthread_exit(NULL);
}

void *despachante(void * args){
    pthread_t thread_exec[NUM_THREADS];
    
    while(true){
        cout << "Entrou no while" << endl;
        while(buffer_funcoes.size() == 0){
            if(terminou) pthread_exit(NULL);
            pthread_cond_wait(&vazio, &mutex_buffer_funcoes);
        }
        cout << "Saiu do while\n";

        Exec atual = buffer_funcoes.front();
        buffer_funcoes.pop();
        
        int i = 0;
        bool executou = false;
        while(!executou){
            cout << "Continua no while" << endl;
            if(!sinal_thread[i]){
                executou = true;
                sinal_thread[i] = true;
                atual.args.thread_id = i;
                switch(atual.tipo){
                    case 0:
                        pthread_create(&thread_exec[i], NULL, soma_int, &(atual.args));
                        break;
                    case 1:
                        pthread_create(&thread_exec[i], NULL, diff_quadrados, &(atual.args));
                        break;
                    case 2:
                        pthread_create(&thread_exec[i], NULL, mult_int, &(atual.args));
                        break;
                    default:
                        cout << "Ocorreu um erro ao selecionar a função" << endl;
                }
            }
            i = (i + 1) % NUM_THREADS;
        }
        cout << "Passou do segundo While\n";
    }

    pthread_exit(NULL);
}

int agendar_execucao(int tipo, Param args){
    Exec buffer_params(tipo, args);

    pthread_mutex_lock(&mutex_buffer_funcoes);
    
    buffer_funcoes.push(buffer_params);
    pthread_cond_broadcast(&vazio);

    cout << "Função de ID: " << args.id << " agendada!" << endl;

    pthread_mutex_unlock(&mutex_buffer_funcoes);
    return args.id;
}

int pegar_resultado_execucao(int id){
    while(!buffer_resultados.count(id)) continue;
    return buffer_resultados[id];
}

int main(){
    sinal_thread.assign(NUM_THREADS, false);

    pthread_t thread_despachante;
    pthread_create(&thread_despachante, NULL, despachante, NULL);

    arq_saida.open("saida.txt");
    if(!arq_saida){
        cout << "Ocorreu um erro ao ler o arquivo de saída";
        return 1;
    }

    vector<int> id_exec;
    int num_func = (rng() % (MOD / 1000000)) + 100;
    cout << "Criando arquivo de saída..." << endl;

    for(int i = 0; i < num_func; i++){
        int id = gera_id();
        int tipo = (rng() % 3);
        int a = (rng() % MOD), b = (rng() % MOD);

        Param p(id, a, b);

        arq_saida << "Gerou função com ID " << id << " do tipo " << tipo << " de parametros " << a << " e " << b << endl;

        switch (tipo){
        case 0:
            agendar_execucao(tipo, p);
            break;
        case 1:
            agendar_execucao(tipo, p);
            break;
        case 2:
            agendar_execucao(tipo, p);
            break;
        default:
            cout << "Essa função não foi prevista pelo sistema" << endl;
        }
        id_exec.push_back(id);
    }
    
    terminou = true;
    //Checa todas os id executados, se já houve execução
    for (auto &id : id_exec){
        arq_saida << "Resultado para o id: " << id << " foi " << pegar_resultado_execucao(id) << endl;
    }

    cout << "O arquivo de saída já está pronto!" << endl;

    return 0;
}