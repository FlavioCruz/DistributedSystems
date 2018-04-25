#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <mpi.h>
#include <math.h>

//Define variáveis globais
int k, quantidade, total;

/** Funcoes auxiliares */

float calculaDistancia(float* ponto, float* centroide){
  return sqrt(pow(centroide[0] - ponto[0], 2) + pow(centroide[0]- ponto[0], 2));
}


int* somaVetores(int* vetor1, int* vetor2){
  int i;
  for(i=0; i<k; i++)
    vetor1+= vetor2
  return vetor1;
}

float*  somaVetores(float* vetor1, float* vetor2){
  int i;
  for(i=0; i<k; i++)
    vetor1+= vetor2
  return vetor1;
}

void imprimeCentroides(float** centroides){
    int i;
    printf("Centroides encontrados:\n");
    for (i = 0; i < k; i++)
      printf("%f %f\n", centroides[i][0], centroides[i][1]);
}

/** Fim funcoes auxiliares*/

//Funcao que acha o centroide mais proximo e retona seu indice
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

//Funcao que obtem os novos centroides
//A partir da soma dos pontos pertencentes ao centroide (x,y) dividido pela quantidade
//De pontos pertencentes aquele grupo.
float** calculaCentroide(int* quantidade, float** somas, float** centroides){
  int i;
  for (i = 0; i < k; i++) {
    centroides[i][0] = somas[i][0]/quantidade[i];
    centroides[i][1] = somas[i][1]/quantidade[i];
  }

  return centroides;
}

void kmeans(float** pontos, float** centroides, int inicio){
  int i,j, mudancas, index;
  int totalCentroide[k];
  int indices[quantidade];
  float somaCentroide[k][2];
  float limite = 0.00001;
  int max = 100;
  int iteracoes = 0;
  for(i=0; i<k; i++) {
    totalCentroide[i] = 0;
    somaCentroide[i] = 0;
  }

  do{
    iteracoes++;
    mudancas = 0;

    for(i=inicio; i<quantidade; i++){
        index = achaCentroide(pontos[i], centroides);
        if(indices[i] != index){
            indices[i] = index;
            totalCentroide[index]++;
            somaCentroide[i][0] += pontos[i][0];
            somaCentroide[i][1] += pontos[i][1];
            mudancas++;
        }
    }

//!!!!! Arrumar daqui pra baixo, verificar se send e recv tem & na frente
    if(meu_rank != 0){
      MPI_Send(&mudancas, 1, MPI_INT, i, 1, MPI_COMM_WORLD);
      MPI_Send(&totalCentroide[0], k, MPI_INT, i, 0, MPI_COMM_WORLD);
      MPI_Send(&somaCentroide[0][0], k*2, MPI_INT, i, 1, MPI_COMM_WORLD);
      MPI_Recv(termina, 1, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);
      if(!termina)
        MPI_Recv(centroides, k, MPI_INT, MPI_ANY_SOURCE, 2, MPI_COMM_WORLD, &status);
    }
    else{
      int totalTemp[k];
      int mudancasTemp;
      for(i=0; i< np; i++){
        MPI_Recv(&vetTemp, k, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
        somaCentroide = somaVetores(somaCentroide, vetTemp);
        MPI_Recv(&vetTemp, k, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);
        totalCentroide = somaVetores(totalCentroide, vetTemp);
        MPI_Recv(mudancasTemp, 1, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);
        mudancas+=mudancasTemp;
      }

      //!!MUDAR SEND PARA BROADSCAST QUANDO MEU_RANK = 0
      if(mudancas/total < limite || iteracoes == max)
        termina = 1;

      MPI_Send(termina, 1, MPI_INT, i, 1, MPI_COMM_WORLD);
      if(!termina){
      centroides = calculaCentroide(pontos, somaCentroide, totalCentroide);

      MPI_Send(centroides, k, MPI_FLOAT, i, 1, MPI_COMM_WORLD);
      }

    }
  } while (!termina);

  imprimeCentroides(centroides);
}



int main(){
  //Inicializa MPI
  int meu_rank, np, origem, destino, tag=0;
  MPI_Status status;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &meu_rank);
  MPI_Comm_size(MPI_COMM_WORLD,&np);

  //Se for processo master, lê o ar]quivo para então distribuir os pontos entre os processos

  char nomeArquivo[255];
  int total;

  printf("Insira o nome do arquivo no qual serao lidos os pontos.\n");
  printf("\tE esperado um arquivo no formato: Numero inteiro k de grupos;\n");
  printf("\tNumero inteiro com a quantidade de pontos a serem lidos; \n");
  printf("\tPontos no plano.\n");

  scanf("%s", &nomeArquivo);
  FILE* arquivo = fopen(nomeArquivo, "r");

  printf("\nLendo arquivo...\n");

  fscanf(arquivo, "%d", &k);
  fscanf(arquivo, "%d", &total);

  float** pontosTemp = (float**) malloc(sizeof(float*) * total);
  float** centro = (float**) malloc(sizeof(float*) * k);

  int i;
  for(i =0; i< total; i++){
    //Lendo os pontos
    pontosTemp[i] = (float*) malloc(sizeof(float)* 2);
    fscanf(arquivo, "%f %f", &pontos[i][0], &pontos[i][1]);

    // Usando os k primeiros pontos como centroides iniciais
    if(i < k){
      centro[i] = (float*) malloc(sizeof(float)* 2);
      centro[i][0] = pontosTemp[i][0];
      centro[i][1] = pontosTemp[i][1];
    }
  }

  fclose(arquivo);


  //Distribui os pontos igualmente entre os processos
  quantidade = total/np;
  int resto = total % np;

  if(meu_rank < resto){
    quantidade++;
  }
  //Chama o procedimento
  kmeans(pontos, centro, inicio);


  printf("done\n");
  MPI_Finalize();
}
