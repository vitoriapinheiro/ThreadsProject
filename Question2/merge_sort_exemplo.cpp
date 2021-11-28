#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

int * vetor;   // array
int N;         // tamanho do array

typedef struct Par
{
     int first, second;
} Par;
Par createPar(int l, int r){  // argumentos da função sort
     Par p;
     p.first = l;
     p.second = r;
     return p;
}

void merge(int l, int m, int r){
     int i1=l, i2=m+1, i3=0;
     int tam = r-l+1;
     int vetorAux[tam];

     while (i1 <= m && i2 <= r){
          if(vetor[i2] < vetor[i1]) {
               vetorAux[i3] = vetor[i2];
               i3++;
               i2++;
          }else{
               vetorAux[i3] = vetor[i1];
               i3++;
               i1++;
          }
     }

     while(i1 <= m) vetorAux[i3++] = vetor[i1++];
     while(i2 <= r) vetorAux[i3++] = vetor[i2++];

     for(int i = 0; i < tam; i++){
          vetor[l+i] = vetorAux[i];
     }
          
     
}

pthread_mutex_t mutex_limite_threads;   // instancia o mutex;
int N_operantes;         // número de threads operando
int N_threadsMAX = 10;        // número de threads máximo

void * sort(void * args){
     int l = ((Par*)args)->first;
     int r = ((Par*)args)->second;

     if(l == r) return NULL;

     int m = (l+r)/2;

     Par Par_l = createPar(l,m);
     Par Par_r = createPar(m+1, r);

     pthread_mutex_lock(&mutex_limite_threads);
     if(N_operantes < N_threadsMAX){ // região crítica
          
          N_operantes++;      //regiao crítica
          pthread_mutex_unlock(&mutex_limite_threads);

          pthread_t thread_l;

          pthread_create(&thread_l, NULL, sort, &Par_l);

          sort(&Par_r);  // ao invés de chamar duas threads, podemos chamar uma e a própria thread
                         // atual ficar encarregada de chamar a função sort para a direita (menos threads)
          pthread_join(thread_l, NULL);
          
     }
     else{
          pthread_mutex_unlock(&mutex_limite_threads);

          sort(&Par_l);       // não é uma região crítica
          sort(&Par_r);
     }


     


     merge(l, m, r);

     pthread_mutex_lock(&mutex_limite_threads);
     N_operantes--;    // região crítica - a gente isola
     pthread_mutex_unlock(&mutex_limite_threads);

     return NULL;
}

void multi_thread_merge_sort(int * vetor, int N){
     pthread_t t0;                      //instanciando a thread inicial
     Par P0 = createPar(0, N-1);        //argumentos da função
     pthread_create(&t0,NULL,sort,&P0); //criando uma thread inicial para sort
     pthread_join(t0,NULL);             //esperando a thread acabar

}

void print(int * vetor, int start, int end){
     for(int i = start; i < end; i++){
          printf("%d ", vetor[i]);
     }
     printf("\n");
}

int main(){
     N = 10;
     vetor = (int*) malloc (sizeof(int)*N);

     int array[] =  {12,15,2,423,5,98,13,4,6,78};
     for(int i = 0; i < N; i++){
          vetor[i] = array[i];
     }
     multi_thread_merge_sort(vetor, N);

     
     
     printf("\narray final: ");
     print(vetor, 0, N);

     free(vetor);
     return 0;
}