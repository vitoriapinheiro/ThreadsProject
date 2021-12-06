#include <pthread.h>
#include "bits/stdc++.h"

using namespace std;

int n, m;                   // Os tamanhos da matriz
int xs, ys, xf, yf;         // As coordenadas de entrada e saída

int dx[4] = {1, -1, 0, 0};  // Vetor para esquerda e direita
int dy[4] = {0, 0, -1, 1};  // Vetor para cima e baixo

bool consegui = false;      // Flag para caso encontre saída

vector<vector<bool> > vis;
vector<vector<int> > matriz;

pthread_mutex_t **mutex_matriz, mutex_global = PTHREAD_MUTEX_INITIALIZER;

struct Coord{               // Estrutura de argumentos, neste caso as coordenadas de cada thread
    int id;
    int x, y;
    Coord(): x(-1),y(-1) {}
    Coord(int i, int j): x(i), y(j) {}
};

bool in_range(int i, int j, int n, int m){                  //checar se esta no range do labirinto para nao acessar posicao invalida
    return (i >= 0 && i < n && j >= 0 && j < m);
}

void *dfs(void *args){      // In Depth Search
    int x = ((Coord*)args)->x;
    int y = ((Coord*)args)->y;
    
    pthread_mutex_lock(&mutex_global);  // impede o acesso às variaveis globais
    if(!in_range(x, y, n, m) || matriz[x][y] || vis[x][y] || consegui){
        pthread_mutex_unlock(&mutex_global);
        pthread_exit(NULL);  //Checar as condicoes de retorno, sair do grid, ter 1 naquela posicao, ja ter sido visitada ou ja ter saido do labirinto
    }
    vis[x][y] = true;
    pthread_mutex_unlock(&mutex_global);
    
    pthread_mutex_lock(&mutex_matriz[x][y]);      // bloqueia o elemento da matriz para duas threads não procurarem na mesma posição



    if(x == xf && y == yf){                     // Se achou o destino, retona e desbloqueia o mutex.
        consegui = true;
        pthread_mutex_unlock(&mutex_matriz[x][y]);
        pthread_exit(NULL);
    }
    
    pthread_t next_thread[4];
    for(int i = 0; i < 4; i++){         // cria 4 threads (esquerda, direita, cima, baixo) e chama o dfs 
        Coord pos(x + dx[i], y + dy[i]);
        pthread_create(&next_thread[i], NULL, dfs, &pos);
        pthread_join(next_thread[i], NULL);
    }

    pthread_mutex_unlock(&mutex_matriz[x][y]);
    pthread_exit(NULL);
}

int main(void) {
    printf("Lendo Labirinto no TXT...\n");

    ifstream labirinto;                 
    labirinto.open("labirinto.txt");    // O labirinto se localiza numa entrada em txt, assim como seu tamanho    

    labirinto >> n >> m;

    matriz.resize(n, vector<int>(m));
    vis.assign(n, vector<bool>(m, false));

    mutex_matriz = (pthread_mutex_t**) malloc(n * sizeof(pthread_mutex_t*));        // Aloca as linhas da matriz de mutexes

    for(int i = 0; i < n; i++){
        mutex_matriz[i] = (pthread_mutex_t *) malloc(m * sizeof(pthread_mutex_t));  // Aloca as colunas da matriz de mutexes
        for(int j = 0; j < m; j++){
            mutex_matriz[i][j] = PTHREAD_MUTEX_INITIALIZER;
            labirinto >> matriz[i][j];
        }
    }

    printf("Digite a entrada do labirinto em (x,y): \n");
    scanf("%d %d", &xs, &ys);

    printf("Digite a saida do labirinto em (x,y): \n");
    scanf("%d %d", &xf, &yf);

    pthread_t t_principal;
    
    if(!matriz[xs][ys] && !matriz[xf][yf]){                 // Cria a thread principal que chama o dfs
        Coord pos(xs,ys);
        pthread_create(&t_principal, NULL, dfs, &pos); 
        pthread_join(t_principal, NULL);
    }

    if(consegui) printf("O LABIRINTO TEM SAIDA\n");
    else printf("O LABARINTO NAO TEM SAIDA\n");

    return 0;
}