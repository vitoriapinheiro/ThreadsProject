#include <pthread.h>
#include "bits/stdc++.h"

using namespace std;


int main(){
    double y;
    int m, x;
    //Carrinho
    for(int i=0; i<14; i++) printf(" ");
    printf("___\n");
    for(int i=0; i<8; i++) printf(" ");
    printf("_-_- |___|\n");
    for(int i=0; i<13; i++) printf(" ");
    printf("O   O\n");
    //trilho da montanha russa
    for (y = 1; y >= 0; y -= 0.1) {
        m = asin(y) * 10;
        for (x = 1; x < m; x++)
            printf(" ");
        printf("#");
        for (; x < 31 - m; x++)
            printf(" ");
        printf("#\n");
    }
    for(y=0;y<=1;y+=0.1){
            m=31+asin(y)*10;
            for(x=1;x<m;x++) printf(" ");
            printf("  #");
            for(;x<93-m;x++) printf(" ");
            printf(" #\n");
    }  
    return 0;
}