#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

//Define variáveis globais
int k, quantidade;

float calculaDistancia(float* ponto, float* centroide){
  return sqrt(pow(centroide[0] - ponto[0], 2) + pow(centroide[0]- ponto[0], 2));
}

int achaCentroide(float* ponto, float** centroides){
  int i, indice;
  float distanciaAtual = INT_MAX;
  float distancia;
  for (i = 0; i < k; i++) {
    distancia = calculaDistancia(ponto, centroides[i]);
    if(distancia < distanciaAtual){
      distanciaAtual = distancia;
      indice = i;
    }
  }
  return indice;
}

void imprimeCentroides(float** centroides){
    int i;
    printf("Centroides encontrados:\n");
    for (i = 0; i < k; i++)
      printf("%f %f\n", centroides[i][0], centroides[i][1]);
}

float** calculaCentroide(float** pontos, int* indices, float** centroides){
  int i, j, geral;
  float totalx, totaly;
  for (i = 0; i < k; i++) {
    totalx = 0;
    totaly = 0;
    geral = 0;
    for(j =0; j<quantidade; j++){
      if(indices[j] == i){
        totalx += pontos[j][0];
        totaly += pontos[j][1];
        geral++;
      }
    }

    centroides[i][0] = totalx/geral;
    centroides[i][1] = totaly/geral;
  }
  return centroides;
}

void kmeans(float** pontos, float** centroides){
  int i,j, mudancas, index;
  float distancia, distanciaAtual;
  int indices[quantidade];
  float limite = 0.01;
  int max = 100;

  int iteracoes = 0;
  for(i=0; i<quantidade; i++) indices[i] = -1;

  do{
    iteracoes++;
    mudancas = 0;

    for(i=0; i<quantidade; i++){
        index = achaCentroide(pontos[i], centroides);
        if(indices[i] != index){
            indices[i] = index;
            mudancas++;
        }
    }

    centroides = calculaCentroide(pontos, indices, centroides);
  } while (mudancas!=0> limite && iteracoes < max);

  imprimeCentroides(centroides);
}



int main(){
  char nomeArquivo[255];
  printf("Insira o nome do arquivo no qual serao lidos os pontos.\n");
  printf("\tE esperado um arquivo no formato: Numero inteiro k de grupos;\n");
  printf("\tNumero inteiro com a quantidade de pontos a serem lidos; \n");
  printf("\tPontos no plano.\n");

  scanf("%s", &nomeArquivo);
  FILE* arquivo = fopen(nomeArquivo, "r");

  printf("\nLendo arquivo...\n");

  fscanf(arquivo, "%d", &k);
  fscanf(arquivo, "%d", &quantidade);

  float** pontos =(float**) malloc(sizeof(float*) * quantidade);
  float** centro =(float**) malloc(sizeof(float*) * k);

  int i;
  for(i =0; i< quantidade; i++){
    //Lendo os pontos
    pontos[i] = (float*) malloc(sizeof(float)* 2);
    fscanf(arquivo, "%f %f", &pontos[i][0], &pontos[i][1]);

    // Usando os k primeiros pontos como centroides iniciais
    if(i < k){
      centro[i] = (float*) malloc(sizeof(float)* 2);
      centro[i][0] = pontos[i][0];
      centro[i][1] = pontos[i][1];
    }
  }

  kmeans(pontos, centro);

  fclose(arquivo);
  printf("done\n");
  return 0;
}
