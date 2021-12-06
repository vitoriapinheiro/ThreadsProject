#include "bits/stdc++.h"
#include <pthread.h>
using namespace std;

// N = numero de linhas, M = numero de colunas
// vamos transformar a coordenada em pontos de tal forma que a coordenada(x, y) = x*M + y
// as transicoes são de pos para:
// {pos - M - 1, pos - M, pos - M + 1, pos - 1, pos + 1, pos + M - 1, pos + M, pos + M + 1}

int N, M, num_threads = 0;

vector<bool> vis;
vector<int> sz, root, matriz;
vector<int> min_thread, max_thread;

pthread_mutex_t mutex_vis = PTHREAD_MUTEX_INITIALIZER;      // para o acesso ao vetor de visitados
pthread_mutex_t mutex_dsu = PTHREAD_MUTEX_INITIALIZER;      // para o acesso ao dsu
pthread_mutex_t mutex_matriz = PTHREAD_MUTEX_INITIALIZER;   // para o acesso a matriz


int find(int a){                                        // funcao para achar a raiz do grupo do A
    if(a == root[a]) return a;
    return root[a] = find(root[a]);
}

void join(int a, int b){                                // funcao para juntar o A e B no mesmo grupo
    a = find(a);
    b = find(b);
    if(a != b){
        if(sz[a] < sz[b])
            root[b] = a;  
        else root[a] = b;
        sz[a] += sz[b];
        sz[b] = sz[a];
    }
}

bool in_range(int i, int j){                                // Checar se a nova posicao esta dentro do grid
    return (i >= 0 && i < N && j >= 0 && j < M);
}

bool regiao_thread(int pos, int id_thread){                 // Checar se a nova posicao esta no range daquela thread
    return (pos >= min_thread[id_thread] && pos <= max_thread[id_thread]);
}

void dfs(int pos, int id_thread){
    pthread_mutex_lock(&mutex_vis);
    pthread_mutex_lock(&mutex_matriz);

    if(vis[pos] || !matriz[pos]){                           // Se já foi visitado ou não é agua no mapa, return
        pthread_mutex_unlock(&mutex_matriz);
        pthread_mutex_unlock(&mutex_vis);
        return;
    }

    vis[pos] = true;

    pthread_mutex_unlock(&mutex_matriz);
    pthread_mutex_unlock(&mutex_vis);
    
    int i = pos / M, j = pos % M;
    for(int dx = -1; dx <= 1; dx++){                        // Itera sobre todos os possiveis movimentos 
        for(int dy = -1; dy <= 1; dy++){
            int new_i = i + dx, new_j = j + dy;
            int new_pos = new_i * M + new_j;
            bool pode = in_range(new_i, new_j);             // Chama a funcao in_range para ver se pode ir para aquela posicao
            
            if(new_pos != pos && pode){
                pthread_mutex_lock(&mutex_matriz);
                if(matriz[new_pos]){                        // Se for um pedaco de terra, junta ao grupo terra que forma a ilha
                    pthread_mutex_unlock(&mutex_matriz);
                    pthread_mutex_lock(&mutex_dsu);
                    join(pos, new_pos);                     // Junta os pedacos de terra
                    pthread_mutex_unlock(&mutex_dsu);
                }
                else pthread_mutex_unlock(&mutex_matriz);

                if(regiao_thread(new_pos, id_thread)){      // Checa se a nova posicao esta no intervalo da thread atual
                    dfs(new_pos, id_thread);
                }
            }
        }
    }
}

void init(int Q){                                           // Inicia cada posicao tendo a si propria como raiz e seu grupo tendo tamanho 1
    for(int i = 0; i < Q; i++){
        root[i] = i, sz[i] = 1;
    }
}

int check(){                                                // Para todas as posicoes, se for uma terra, joga a raiz em um set
    set<int> representantes;
    for(int i = 0; i < N; i++){
        for(int j = 0; j < M; j++){
            if(!matriz[i*M + j]) continue;                  // Se for agua, segue
            find(i*M + j);                                  // Garantir que a raiz esta atualizada
            representantes.insert(root[i*M + j]);
        }
    }
    return representantes.size();                           // A quantidade de ilhas eh o numero de raizes diferentes (tamanho do set)
}

void *func_thread(void *args){
    int i = *((int*)args);
    for(int pos = min_thread[i]; pos <= max_thread[i]; pos++){  // itera sobre os limites de sua região
        pthread_mutex_lock(&mutex_vis);                         
        if(!vis[pos]){                                          // se a posição não foi visitada
            pthread_mutex_unlock(&mutex_vis);
            dfs(pos, i);                                        // visita todas as posições da região
        }
        else pthread_mutex_unlock(&mutex_vis);
    }
    pthread_exit(NULL);
}

int main(){
    freopen("mapa.txt", "r", stdin);                        // Iniciando arquivo do mapa

    cin >> num_threads;
    cin >> N >> M;

    sz.resize(N*M + 5);
    root.resize(N*M + 5);
    matriz.resize(N*M + 5);
    vis.assign(N*M + 5, false);

    init(N*M + 5);                                          // inicializa todos os pixels

    for(int i = 0; i < N; i++){                             // Le a matriz
        for(int j = 0; j < M; j++){
            cin >> matriz[i*M + j];
        }
    }

    min_thread.resize(num_threads);
    max_thread.resize(num_threads);

    //Cada thread checara um intervalo
    int L = 0;
    int intervalo = (N*M)/num_threads;

    for(int i = 0; i < num_threads; i++){                   // Definindo o intervalo que a thread i sera responsavel
        min_thread[i] = L;
        max_thread[i] = L + intervalo;

        L = L + intervalo + 1;
        if(i == num_threads - 1) max_thread[i] = N*M - 1;
    }

    vector<int> id_threads(num_threads);
    pthread_t threads_regiao[num_threads];
    
    for(int i = 0; i < num_threads; i++){
        id_threads[i] = i;
        pthread_create(&threads_regiao[i], NULL, func_thread, &id_threads[i]);  // cria uma thread para cada região
    }
    
    for(int i = 0; i < num_threads; i++){
        pthread_join(threads_regiao[i], NULL);
    }

    int qnt_ilhas = check();
    cout << qnt_ilhas << '\n';
}