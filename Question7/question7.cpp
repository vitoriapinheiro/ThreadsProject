//contrabando
/*
    Nessa questão o uso  concorrente de  memória não foi uma  preocupação por conta de cada  pixel da
    imagem  trabalhar de  forma  isolada, ou  seja, foi possível  isolar  a  conversão  dos pixels em
    diferentes Threads. Desse modo, a implementação se deu de forma que a leitura e escrita dos dados
    é feita em uma única oportunidade e cada Thread fica responsável apenas por converter um conjunto
    específico de pixels. 

    Sendo  assim,  após  feita a leitura  dos dados do arquivo de imagem, uma  estrutura de array de 2
    dimensões é  criada  com  esses dados. Essa estrutura  foi construida  para ser um array de struct
    com os dados  RGB  de  cada pixel. Em contrapartida, há  um  array  bi-dimensional  que  guarda os
    dados  após  a  conversão. Para  fins  de  facilitar  o  desenvolvimento, essas  estruturas  forma
    declaradas de forma global.
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> 

// Quantidade de Threads a ser usada na execução do código 
#define THREADS 3

// Struct com os elemtnos RGB
typedef struct Elem{
    int R, G, B;
}Elem;

// Array que guarda os valores convertidos
int **mat;
// Array que guarda os valores lidos do arquivo
Elem **pixel;
// Variáveis de uso em laços (i, j) | Quantidade de Linhas de Colunas (LIN, COL)
int i, j, LIN, COL;

// Função que será executada dentro de cada Thread, seu obejtivo consiste em converter os
// dados  RBG em Tons de cinza 
void *conversao(void * info) {
    // Recebe o id da thread, esse dado consiste em um número que identifica qual a thread
    // que executará naquele instante.
    int* id = (int*) info;
    // Esse laço tem por objetivo varrer o array bi-dimensional de uma forma que a Thread
    // atual leia somente os pixels que outras threads não lerão
    for(int i = *id; i < (LIN * COL); i+=THREADS){
        int k = i/COL, j = i%COL;
        // Converte usando os dados fornecidos na questão
        mat[k][j] = (int) (pixel[k][j].R*0.30 + pixel[k][j].G*0.59 + pixel[k][j].B*0.11);
    }
    pthread_exit(NULL);
}

int main () {
    pthread_t thread[THREADS];
    FILE *img;
    char texto[20];
    int status, *threadID, max;

    // abrindo a imagem
    img = fopen("img.ppm", "r");
    if(img == NULL) {
        printf("Erro na abertura do arquivo!");
        return 1;
    }

    // Leitura das dimensões e valor maximo
    fgets(texto, 20, img);
    fscanf(img, "%d %d", &COL, &LIN);
    fscanf(img, "%d", &max);

    // Alocaçao do array
    mat = (int **) malloc(LIN * sizeof(int *));
    for (i=0; i < LIN; i++)
        mat[i] = (int *) malloc(COL * sizeof(int));

    
    // Alocaçao do array de id
    threadID = (int *) malloc(THREADS * sizeof(int));

    pixel = (Elem **) malloc(LIN * sizeof(Elem));
    for (i=0; i < LIN; i++)
        pixel[i] = (Elem*) malloc(COL * sizeof(Elem));
    
    // Leitura dos pixels da imagem
    for(i = 0; i < LIN; i++) {
        for(j = 0; j < COL; j++){
            fscanf(img, "%d %d %d", &pixel[i][j].R, &pixel[i][j].G, &pixel[i][j].B);
        }
    }

    // Criação das threads
    for (i = 0; i < THREADS; i++){
        threadID[i] = i; 
        status = pthread_create(&thread[i],NULL,conversao,(void *) &threadID[i]); 
        if(status){
            printf("Erro ao criar Thread!");
            exit(-1);
        }
    }

    // Aguarda a execução das Threads
    for(i = 0; i < THREADS; i++) {
        pthread_join(thread[i],NULL);
    }

    // Gerando nova imagem
    img = fopen("new-image.ppm", "w");
    if(img == NULL) {
        printf("Erro na criação do arquivo!");
        return 1;
    }

    // Escrita dos dados convertidos
    fprintf(img, "P3\n%d %d\n%d\n", COL, LIN, max);
    for(i = 0; i < LIN; i++) {
        for(j = 0; j < COL; j++){
            int aux = mat[i][j];
            fprintf(img, "%d %d %d\n", aux, aux, aux); 
        }
    }

    // Desalocando os arrays    
    for(i = 0; i < LIN; i++){
        free(mat[i]);
        free(pixel[i]);
    }
    free(mat);
    free(pixel);

    fclose(img);
    return 0;
}