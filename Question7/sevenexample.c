#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

    /*
    Nesta questão serão necessários dois buffers e, consequentemente, dois Mutexes.
    Serão necessários dois buffers pois, em um precisamos ter uma fila de execuções que ainda serão realizadas e
    em outro precisamos ter uma lista de todos os resultados das execuções que já foram concluídas.
    Tendo estes dois buffers, precisamos agora de dois Mutexes, um para cada buffer,
    a fim de evitar que duas ou mais threads tenham acesso a uma região crítica (os buffers).
    */

    #define N 5 // Número de processadores/núcleos do sistema, portanto, também representa a quantidade de threads do programa
    #define BUFFER_SIZE 10 // Tamanho máximo do Buffer

    typedef struct param { // Struct pedida pela questão que irá servir como parâmetro de uma função em uma execução
    int a, b, c;
    int execucaoId; // ID da execução a qual esse parâmetro está vinculado (será utilizado para colocar o resultado das funções no buffer de resultados)
    int threadId;
    } Param;

    typedef struct execucao { // Para cada execução agendada na função agendarExecucao, será criada uma instância desta Struct
    void *genericFunc;
    Param funcParams;
    } Execucao;

    // Inicialização dos Mutexes e suas variáveis de condição.
    pthread_mutex_t mutexBufferExecucao = PTHREAD_MUTEX_INITIALIZER; // Mutex relacionado ao buffer de execuções
    pthread_mutex_t mutexBufferResult = PTHREAD_MUTEX_INITIALIZER; // Mutex relacionado ao buffer de resultados
    pthread_cond_t condExecucao = PTHREAD_COND_INITIALIZER; // Variavél de condição para o buffer de execução
    pthread_cond_t condResult = PTHREAD_COND_INITIALIZER; // Variavél de condição para o buffer de resultados

    // Variáveis globais que serão utilizadas no decorrer da questão
    Execucao *bufferExecucao;
    int statusBufferExecucoes = 0; // Tamanho atual do buffer de execucoes
    int *bufferResult;
    int statusBufferResult = 0; // Tamanho atual do buffer de resultados

    int qtdExecucoes = 0; // Irá funcionar como o ID das execuções, além disso, usaremos essa variável para comparar com o tamanho máximo do buffer e conferir se chegamos no limite
    int execucaoDaVez = 0;

    pthread_t threads[N];
    int statusThreads[N];

    int *idResultados;

    // Utilizaremos essas cores para printar as respostas das execuções
    char *cores[3] = {
    "\033[48;2;255;0;0", // Vermelho
    "\033[48;2;255;255;0", // Amarelo
    "\033[48;2;0;255;0", // Verde
    };
    char *preto = ";30m";
    char *branco = "m";
    char *frases[3] = {
    " ",
    "funexecMod10 acabou de lançar um novo resultado no nosso sistema!\n",
    "O resultado de ",
    };


    // Início das funções
int agendarExecucao(void *funexec, Param funcParams) {  // Primeira função pedida pela questão, aqui colocamos uma nova execução (tarefa) no buffer de execuções e retornamos o ID dessa execução
    Execucao request;
    request.funcParams = funcParams;
    request.genericFunc = funexec;

    pthread_mutex_lock(&mutexBufferExecucao); // Travando o mutex para que outras threads não tenham acesso ao recurso compartilhado (buffer e variáveis globais) enquanto mexemos nele

    // printf("%s%d...\033[m\n", cores[0], qtdExecucoes);
    printf("%s%s Por favor, aguarde um instante enquanto agendamos a execução %d no buffer.\033[m\n", cores[0], branco, qtdExecucoes);
    request.funcParams.execucaoId = qtdExecucoes;
    bufferExecucao[qtdExecucoes] = request;

    if(qtdExecucoes == BUFFER_SIZE) qtdExecucoes = 0; // O buffer funciona de forma parecida a um array circular, portanto, caso cheguemos ao seu final, devemos retornar ao início
    if(statusBufferExecucoes == 1) pthread_cond_broadcast(&condExecucao); // Caso essa seja a primeira execução dentro do buffer, devemos acordar as threads consumidoras
    
    qtdExecucoes++;
    statusBufferExecucoes++;

    if(qtdExecucoes == BUFFER_SIZE) qtdExecucoes = 0;
    if(statusBufferExecucoes == 1) pthread_cond_broadcast(&condExecucao); // Aqui conferimos novamente para atestar que as condições não foram atingidas após o incremento das variáveis

    pthread_mutex_unlock(&mutexBufferExecucao);
    return request.funcParams.execucaoId;
}

int pegarResultadoExecucao(int execucaoId) { // Segunda função pedida pela questão, aqui checamos se há algum resultado de execuções no buffer de resultados, se sim, retornamos esse resultado, se não, adormecemos as threads até termos um resultado
    int result;

    pthread_mutex_lock(&mutexBufferResult); // Travando o mutex para que outras threads não tenham acesso ao recurso compartilhado (buffer e variáveis globais) enquanto mexemos nele

    while(statusBufferResult == 0 || bufferResult[execucaoId] < 0) { // Enquanto o buffer estiver vazio ou então o resultado dessa execução em específico não estiver disponível, esperamos
        pthread_cond_wait(&condResult, &mutexBufferResult);
    }

    printf("%s%s Por favor, aguarde um instante enquanto coletamos o resultado da execução %d.\033[m\n", cores[1], preto, execucaoId);
    result = bufferResult[execucaoId];
    bufferResult[execucaoId] = 0;
    statusBufferResult--;

    pthread_mutex_unlock(&mutexBufferResult);
    return result;
}

    void *funexecAddMod10(void* params) {
    Param *aux = (Param *) params;
    int result = (aux->a + aux->b + aux->c) % 10;

    pthread_mutex_lock(&mutexBufferResult);

    bufferResult[aux->execucaoId] = result;
    statusBufferResult++;

    pthread_cond_signal(&condResult); // Avisa às threads que um resultado foi adicionado ao buffer de resultados
    pthread_mutex_unlock(&mutexBufferResult);

    statusThreads[aux->threadId] = -1;
    pthread_exit(NULL);
}

void *funcThreadDespachante() { // Esta função será chamada quando a thread despachante for criada
    int i;
    pthread_mutex_lock(&mutexBufferExecucao);

    while(statusBufferExecucoes == 0) { // Aqui sinalizamos às threads do buffer de execução que ainda não existe nenhuma execução pendente, fazemos algo parecido na sexta questão, com o put da fila bloqueante
        pthread_cond_wait(&condExecucao, &mutexBufferExecucao);
    }

    while(statusBufferExecucoes > 0) { // Aqui rodamos um laço de repetição que só irá terminar quando não houver mais nenhuma execução pendente no buffer
        for(i = 0; i < N && bufferExecucao[execucaoDaVez].genericFunc != NULL; i++) {
            if(statusThreads[i] == -1) {
                statusThreads[i] = 1;
                bufferExecucao[execucaoDaVez].funcParams.threadId = i;
                pthread_create(&threads[i], NULL, bufferExecucao[execucaoDaVez].genericFunc, (void *) &bufferExecucao[execucaoDaVez].funcParams);
            }
            bufferExecucao[execucaoDaVez].genericFunc = NULL;

            printf("A thread despachante acabou de despachar a execucao: %d\n", execucaoDaVez);
            if (execucaoDaVez == BUFFER_SIZE) execucaoDaVez = 0; // Assim como em um array circular, aqui voltamos a checar o início do buffer de execuções
            if (statusBufferExecucoes == 0) break;
            statusBufferExecucoes--;
            execucaoDaVez++;
            if (execucaoDaVez == BUFFER_SIZE) execucaoDaVez = 0; // Aqui checamos novamente para garantir que as condições não foram atingidas desde o último incremento
            if (statusBufferExecucoes == 0) break;
        }

        if (statusBufferExecucoes == 0) pthread_cond_wait(&condExecucao, &mutexBufferExecucao);
    }

    pthread_mutex_unlock(&mutexBufferExecucao);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    int i, result;
    Param aux;

    bufferExecucao = (Execucao *) malloc(BUFFER_SIZE * sizeof(Execucao));
    bufferResult = (int *) malloc(BUFFER_SIZE * sizeof(int));
    idResultados = (int *) malloc(BUFFER_SIZE * sizeof(int));

    for(i = 0; i < BUFFER_SIZE; i++) bufferResult[i] = -1; // Iniciando todos os resultados do buffer de resultados como -1 para que saibamos que ainda não temos um resultado válido nesta posição
    for(i = 0; i < N; i++) statusThreads[i] = -1; // Iniciando todas as threads com o status -1 para indicar que elas não estão ocupadas, portanto pode receber outra funexec

    pthread_t despachante; // Thread pedida pela questão, ela irá pegar as requisições do buffer e gerenciar as N threads para saber qual vai ficar responsável por cada requisição
    pthread_create(&despachante, NULL, funcThreadDespachante, NULL);

    for(i = 0; i < BUFFER_SIZE; i++) {
        // Essa série de IF's abaixo são apenas para diversificar mais os resultados encontrados
        if (i == 1) {
            aux.a = 1;
            aux.b = 2;
            aux.c = 3;
        } 
        else if (i == 2) {
            aux.a = 1;
            aux.b = 2;
            aux.c = 3;
        } else if (i % 3 == 0) {
            aux.a = 3;
            aux.b = 6;
            aux.c = 9;
        } 
        else if (i == 9) {
            aux.a = 9;
            aux.b = 8;
            aux.c = 7;
        } 
        else {
            aux.a = 94;
            aux.b = 88;
            aux.c = 75;
        }

        idResultados[i] = agendarExecucao((void *) funexecAddMod10, aux);
    }

    for(int i = 0; i < BUFFER_SIZE; i++) {
        result = pegarResultadoExecucao(idResultados[i]);
        printf("%s%s Veja, conseguimos o resultado da execução %d: %d.\033[m\n", cores[2], preto, idResultados[i], result);
    }

    for(i = 0; i < N; i++) pthread_join(threads[i], NULL);

    pthread_join(despachante, NULL);
    printf("Fim do programa.\n");

    free(bufferExecucao);
    free(bufferResult);
    pthread_exit(NULL);

    return 0;
}