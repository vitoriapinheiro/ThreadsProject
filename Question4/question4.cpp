#include <pthread.h>
#include "bits/stdc++.h"

#define endl '\n'

using namespace std;

int n, m;                                                       // linha e coluna da matriz

struct Pixel{                                                   // Estrutura de argumentos, neste caso as cores de cada pixel
    int r, g, b;
    Pixel() : r(-1), g(-1), b(-1) {}
    Pixel(int i, int j, int k) : r(i), g(j), b(k) {}
};

void *conversor(void *args){                                    // Função de cada thread, converte seu respectivo pixel para escala de cinza
    int r = ((Pixel*)args)->r;
    int g = ((Pixel*)args)->g;
    int b = ((Pixel*)args)->b;

    int c = r*0.3 + g*0.59 + b*0.11;

    Pixel *ret = new Pixel(c,c,c);
    pthread_exit((void *) ret);
}

int main(){
    ifstream imagem;
    imagem.open("image.ppm");                                   // abre o arquivo de entrada e lê os valores
    if(!imagem){
        cout << "Ocorreu um erro ao abrir a imagem" << endl;
    }
    vector<vector<Pixel> > matriz_pixel,matriz_pixel_saida;     // matrizes de entrada e saída
    string tipo;
    int valor_max;
    imagem >> tipo >> m >> n >> valor_max;                      // Leitura dos atributos da imagem
    
    matriz_pixel.resize(n);
    matriz_pixel_saida.resize(n);
    
    for(int i = 0; i < n; i++){
        matriz_pixel[i].resize(m);
        for(int j = 0; j < m; j++){
            int r, g, b;
            imagem >> r >> g >> b;
            matriz_pixel[i][j] = Pixel(r, g, b);                // Leitura dos pixels
        }
    }

    pthread_t threads_pixel[n][m];
    
    for(int i = 0; i < n; i++){                                 // Cria as threads
        for(int j = 0; j < m; j++){
            pthread_create(&threads_pixel[i][j], NULL, conversor, &matriz_pixel[i][j]); 
        }
    }
    
    for(int i = 0; i < n; i++){                                 // Espera as threads e o valor de retorno
        matriz_pixel_saida[i].resize(m);
        for(int j = 0; j < m; j++){
            Pixel *ret;
            pthread_join(threads_pixel[i][j],(void **) &ret);
            matriz_pixel_saida[i][j] = *(ret);
        }
    }
    
    ofstream imagem_saida;
    imagem_saida.open("imagem_saida.ppm");                      // Gera o arquivo de saida
    imagem_saida << tipo << endl;
    imagem_saida << m << " " << n << endl;
    imagem_saida << valor_max << endl;
    for(int i = 0; i < n; i++){
        for(int j = 0; j < m; j++){
            imagem_saida << matriz_pixel_saida[i][j].r << " " <<  matriz_pixel_saida[i][j].g << " " << matriz_pixel_saida[i][j].b << endl;
        }
    }

    return 0;
}