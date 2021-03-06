#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <mpi.h>
#include <math.h>

//Define variáveis globais
int k, quantidade, total;
int np, meu_rank;


/** Funcoes auxiliares */

float calculaDistancia(float pontoX,float pontoY, float centroideX, float centroideY){
  return sqrt(pow(centroideX - pontoX, 2) + pow(centroideY- pontoY, 2));
}


int* somaVetores(int* vetor1, int* vetor2){
  int i;
  for(i=0; i<k; i++)
    vetor1[i]+= vetor2[i];
  return vetor1;
}

float* somaVetoresFloat(float* vetor1, float* vetor2){
  int i;
  for(i=0; i<k*2; i++)
    vetor1[i]+= vetor2[i];
  return vetor1;
}

void imprimeCentroides(float* centroides){
    int i;
    printf("Centroides encontrados, k = %d \n", k);
    for (i = 0; i < k*2; i++)
      printf("dentro antes\n");
      printf("%f \n", centroides[i]);
      printf("dentro depois\n");
}

/** Fim funcoes auxiliares*/

//Funcao que acha o centroide mais proximo e retona seu indice
int achaCentroide(float pontoX, float pontoY, float* centroides){
  int i, indice;
  float distanciaAtual = INT_MAX;
  float distancia;
  for (i = 0; i < k*2; i+=2) {
    distancia = calculaDistancia(pontoX, pontoY, centroides[i], centroides[i+1]);
    if(distancia < distanciaAtual){
      distanciaAtual = distancia;
      indice = i;
    }
  }
  return indice/2;
}

//Funcao que obtem os novos centroides
//A partir da soma dos pontos pertencentes ao centroide (x,y) dividido pela quantidade
//De pontos pertencentes aquele grupo.
float* calculaCentroide(int* quantidade, float* somas, float* centroides){
  int i;
  for (i = 0; i < k*2; i+=2) {
    centroides[i] = somas[i]/quantidade[i/2];
    centroides[i+1] = somas[i+1]/quantidade[i/2];
  }

  return centroides;
}

void kmeans(float* pontos, float* centroides, int inicio){
  int i,j, mudancas, mudancasTemp,index;
  int* totalCentroide = (int*) malloc(sizeof(int)*k);
  int indices[quantidade];
  float* somaCentroide = (float*)calloc(k*2,sizeof(float));
  int* totalTemp = (int*)calloc(k,sizeof(int));
  float* somaTemp = (float*)malloc(k*2*sizeof(float));
  float limite = 0.00001;
  int max = 100;
  MPI_Status status;
  int iteracoes = 10;
  int termina = 0;

  int contador = 0;

  do{
    printf("contador1: %d, rank: %d\n", contador, meu_rank);
    for(i=0; i<k; i++) {
      totalCentroide[i] = 0;
      somaCentroide[i]= 0;
      somaCentroide[i+1] = 0;
    }

    iteracoes++;
    mudancas = 0;
    printf("meu rank: %d, inicio: %d, quantidade: %d, inicio %d\n", meu_rank, inicio, quantidade, inicio);
    for(i=inicio; i<inicio + quantidade*2-1; i+=2){
        index = achaCentroide(pontos[i], pontos[i+1], centroides);
        if(indices[i] != index){
            indices[i] = index;
            mudancas++;
        }
        somaCentroide[index*2] += pontos[i];
        somaCentroide[index*2+1] += pontos[i+1];
        totalCentroide[index]++;
        // printf("rank %d indice %d\n", meu_rank, index);
    }

//!!!!! Arrumar daqui pra baixo, verificar se send e recv tem & na frente
    if(meu_rank != 0){

       MPI_Send(&mudancas, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

       MPI_Send(totalCentroide, k, MPI_INT, 0, 2, MPI_COMM_WORLD);
       MPI_Send(somaCentroide, k*2, MPI_FLOAT, 0, 1, MPI_COMM_WORLD);



    }else{
      int j;

       for(i=1; i< np; i++){
        MPI_Recv(totalTemp, k, MPI_INT, MPI_ANY_SOURCE, 2, MPI_COMM_WORLD, &status);
        totalCentroide = somaVetores(totalCentroide, totalTemp);

        MPI_Recv(somaTemp, k*2, MPI_FLOAT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);

        somaCentroide = somaVetoresFloat(somaCentroide, somaTemp);

          int j;
          for(j=0; j<k*2; j++) printf("soma %f\n", somaCentroide[j]);

        centroides = calculaCentroide(totalCentroide, somaCentroide, centroides);
        MPI_Recv(&mudancasTemp, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
        mudancas+=mudancasTemp;
       }




      //!!MUDAR SEND PARA BROADCAST QUANDO MEU_RANK = 0
      if(iteracoes == max && mudancas/total <= limite){
        termina = 1;
      }

    }
    printf("aaaaaaaaa\n");
    printf("termina: %d, rank: %d\n", termina, meu_rank);
    MPI_Bcast(&termina, 1, MPI_INT, 0, MPI_COMM_WORLD);

    printf("termina: %d\n", termina);
    printf("aqui, meu rank: %d\n", meu_rank);
    imprimeCentroides(centroides);
    MPI_Bcast(centroides, k, MPI_FLOAT, 0, MPI_COMM_WORLD);
    printf("abc\n");
    printf("contador: %d, rank: %d\n", contador, meu_rank);
    contador++;
  } while (!termina);
  printf("acabou1");
  free(totalCentroide);
  free(somaCentroide);
  free(totalTemp);
  free(somaTemp);
  printf("acabou");
  //imprimeCentroides(centroides);
}



int main(int argc, char** argv){
  //Inicializa MPI
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &meu_rank);
  MPI_Comm_size(MPI_COMM_WORLD,&np);

  //Se for processo master, lê o arquivo para então distribuir os pontos entre os processos

  char nomeArquivo[255];
  total = 0;
  FILE* arquivo;

  if(meu_rank == 0){
    printf("******************\n");
    printf("Algoritmo kmeans\n");
    printf("Insira o nome do arquivo no qual serao lidos os pontos.\n");
    printf("\tE esperado um arquivo no formato: Numero inteiro k de grupos;\n");
    printf("\tNumero inteiro com a quantidade de pontos a serem lidos; \n");
    printf("\tPontos no plano.\n");

    scanf("%s", &nomeArquivo);
    arquivo = fopen(nomeArquivo, "r");

    printf("\nLendo arquivo...\n");

    fscanf(arquivo, "%d", &k);
    fscanf(arquivo, "%d", &total);
  }

    MPI_Bcast(&total, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&k, 1, MPI_INT, 0, MPI_COMM_WORLD);

    float *centro = (float*) malloc(k * 2 * sizeof(float));
    float *pontos = (float*) malloc(total * 2 * sizeof(float));

    if(meu_rank == 0){
      int i;
      for(i = 0; i < total * 2; i++){
        //Lendo os pontos
        fscanf(arquivo, "%f", &pontos[i]);
        // Usando os k primeiros pontos como centroides iniciais
        if(i < k*2){
          centro[i] = pontos[i];
        }
      }

      fclose(arquivo);
  }


  MPI_Bcast(pontos, total*2, MPI_FLOAT, 0, MPI_COMM_WORLD);
  MPI_Bcast(centro, k*2, MPI_FLOAT, 0, MPI_COMM_WORLD);



  //Distribui os pontos igualmente entre os processos
  quantidade = total/np;
  int resto = total % np;
  int inicio;
  if(meu_rank < resto){
    quantidade++;

    inicio = quantidade * meu_rank*2;
  } else {
    inicio = quantidade * meu_rank *2 + resto*2;
  }



  //Chama o procedimento
  kmeans(pontos, centro, inicio);


  printf("done\n");
  free(pontos);
  free(centro);
  MPI_Finalize();
}
