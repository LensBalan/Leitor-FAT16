// Autor: Leonardo Balan
// Trabalho de SO - Leitor FAT 16

// Compilar no terminal Linux: gcc -o lens_fat16 lens_fat16.c -lm

// Compilando no Windows: Exibindo resultados inconsistentes
// e rodando incorretamente. Usar Linux.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

// Struct para armazenar o Boot Record
typedef struct fat_BS {
  unsigned char bootjmp[3];
  unsigned char oem_name[8];
  unsigned short bytes_per_sector;
  unsigned char sectors_per_cluster;
  unsigned short reserved_sector_count;
  unsigned char table_count;
  unsigned short root_entry_count;
  unsigned short total_sectors_16;
  unsigned char media_type;
  unsigned short table_size_16;
  unsigned short sectors_per_track;
  unsigned short head_side_count;
  unsigned int hidden_sector_count;
  unsigned int total_sectors_32;
  // this will be cast to it's specific type once the driver actually knows what
  // type of FAT this is.
  unsigned char extended_section[54];

} __attribute__((packed)) fat_BS_t;

// Struct para armazenar cada entrada do Diretorio Raiz
typedef struct {
  unsigned char filename[8];
  unsigned char extension[3];
  unsigned char attributes;
  unsigned char reservedWindowsNT;
  unsigned char CreationTimeisTenths;
  unsigned short CreatedTime;
  unsigned short CretedDate;
  unsigned short LastAcessDate;
  unsigned short bits16high; // Para FAT 12 e FAT 16 é sempre zero
  unsigned short LastModificationTime;
  unsigned short LastModificationDate;
  unsigned short Bits16low; // Numero para localizar o primeiro cluster
  unsigned int FileSize;

} __attribute__((packed)) Diretorio;

// Func para calcular as posicoes das particoes na FAT
void calcularPosicoes(fat_BS_t boot_record, int *fat, int *fat2, int *root_dir, int *dados) {
    *fat = boot_record.reserved_sector_count * boot_record.bytes_per_sector;
    *fat2 = (boot_record.root_entry_count * boot_record.table_size_16) + *fat;
    *root_dir = (boot_record.root_entry_count * boot_record.table_size_16) + *fat2;
    *dados = (boot_record.bytes_per_sector * 32) + *root_dir;
}

// Func para printar as posicoes iniciais das particoes da FAT
void printarPosicoes(fat_BS_t boot_record, int fat, int fat2, int root_dir, int dados){
  printf("-----------------------------------------------\n\n");
  printf("Onde cada Partição começa\n\n");
  printf("FAT: %d\n", fat);
  printf("FAT2: %d\n", fat2);
  printf("Diretório Raiz: %d\n", root_dir);
  printf("Dados: %d\n", dados);
  printf("-----------------------------------------------\n\n");
}

// Func para obter o proximo cluster do arq
unsigned short obterProxCluster(FILE *ptarq, fat_BS_t boot_record, unsigned short cluster_atual, int fat) {
    // Calcular a posicao do cluster na FAT. Achar a posicao direta em bytes
    // Cada entrada tem 2 Bytes
    int posicao_fat = fat + (cluster_atual * 2); 

    // Posicionando o ponteiro do arq na entrada FAT correspondente ao cluster atual
    fseek(ptarq, posicao_fat, SEEK_SET);

    // Lendo o numero do proximo cluster
    unsigned short proximo_cluster; //2 bytes
    fread(&proximo_cluster, sizeof(unsigned short), 1, ptarq);
    return proximo_cluster;
}

// Func para ler o conteudo do arq a partir dos clusters
void lerConteudoArquivo(FILE *ptarq, fat_BS_t boot_record, unsigned short first_cluster1, unsigned int size1, int dados, int fat) {

    int clusters_lidos = 0; // qnts ja foram lidos
    int cluster_atual = first_cluster1; //começar pelo 1º cluster do arq

    // variavel para armazenar o conteudo dos clusters
    unsigned char conteudo_arquivo[boot_record.bytes_per_sector * boot_record.sectors_per_cluster];

  //ceil arredonda pra cima o valor sempre. Formula para saber a qnt de clusters do arq
    while (clusters_lidos < ceil((float)size1 / (boot_record.bytes_per_sector * boot_record.sectors_per_cluster))) {

        // Calcular a posicao do cluster no arq
        // Formulas passadas em aula
        int setor_dados = dados / boot_record.bytes_per_sector;
        int setor_cluster = setor_dados + ((cluster_atual - 2) * boot_record.sectors_per_cluster);
        int posicao_cluster = (setor_cluster * boot_record.bytes_per_sector);

        // Ponteiro do arq no inicio do cluster
        fseek(ptarq, posicao_cluster, SEEK_SET);
        // Lendo o conteudo do cluster
        fread(conteudo_arquivo, sizeof(unsigned char), boot_record.bytes_per_sector * boot_record.sectors_per_cluster, ptarq);

        // Printando o conteudo do cluster atual
        printf("%s", conteudo_arquivo);

        // Obter o proximo cluster do arq
        cluster_atual = obterProxCluster(ptarq, boot_record, cluster_atual, fat);

        clusters_lidos++; // incrementando o nº de clusters lidos para o while
    }
}

///////////////////// Função Principal /////////////////////////////
int main() {

  char img[] = "test.img"; // Imagem FAT16 a ser aberta
  FILE *fp, *dp;
  fat_BS_t boot_record;
  Diretorio DirRaiz;
  // Variaveis para calcular informacoes da FAT 
  int root_dir_sectors, fat_size, first_data_sector, first_root_dir_sector, first_sector_of_cluster, dir_raiz_posicao;
  // Variaveis para armazenar as posicoes das particoes FAT
  int fat, fat2, root_dir, dados;

  // Salvando as informacoes do Boot Record na Struct
  fp = fopen(img, "rb");
  fseek(fp, 0, SEEK_SET);
  fread(&boot_record, sizeof(fat_BS_t), 1, fp);
  fclose(fp);

  // Dados do Boot Record para calculo
  printf("Dados Principais do Boot Record\n\n");
  printf("Bytes por Setor: %hd \n", boot_record.bytes_per_sector);
  printf("Setores por Cluster: %x \n", boot_record.sectors_per_cluster);
  printf("Nº Fat: %x \n", boot_record.table_count);
  printf("Setores Reservados: %x \n", boot_record.reserved_sector_count);
  printf("Nº de Entradas no Dir. Raiz: %d \n", boot_record.root_entry_count); // printando em decimal, em hexa "%x"
  printf("Nº de Setores por Fat: %d \n", boot_record.table_size_16); // printando em decimal, em hexa "%x"

  //------- Formulas do FAT OS Dev Wiki que não utilizei comentadas !!: --------

  //total_sectors = (boot_record.total_sectors_16 == 0) ?boot_record.total_sectors_32 : boot_record.total_sectors_16;
  //printf("Nº total de setores: %d \n", total_sectors);
  //first_fat_sector = boot_record.reserved_sector_count;
  //printf("Firt Fat Sector: %d \n", first_fat_sector);
  //data_sectors = total_sectors - (boot_record.reserved_sector_count + (boot_record.table_count * fat_size) + root_dir_sectors);
  //printf("Data Sectors: %d \n", data_sectors);
  //total_clusters = data_sectors / boot_record.sectors_per_cluster;
  //printf("Total Clusters: %d \n", total_sectors);

  // ------------- Formulas do FAT OS Dev WIKI !!!! -----------------
  fat_size = (boot_record.table_size_16 == 0) ? boot_record.table_size_16 : boot_record.table_size_16; // Setores por FAT
 // printf("Fat Size: %d \n", fat_size);
  root_dir_sectors = ((boot_record.root_entry_count * 32) +(boot_record.bytes_per_sector - 1)) / boot_record.bytes_per_sector;
  //printf("Tamanho do Diretório Raiz em Setores: %d \n", root_dir_sectors);
  first_data_sector = boot_record.reserved_sector_count +(boot_record.table_count * fat_size) + root_dir_sectors;
  //printf("Primeiro Setor de Dados: %d \n", first_data_sector);
  first_root_dir_sector = first_data_sector - root_dir_sectors;
  //printf("Primeiro Setor do Dir Raiz: %d \n", first_root_dir_sector);
  // 311 x 512 = 159232, posição no arquivo fat
  dir_raiz_posicao = first_root_dir_sector * boot_record.bytes_per_sector;
  //printf("Posição do Inicio do Dir Raiz: %d\n", dir_raiz_posicao);

  // Chamando a func para calcular as posicoes na imagem FAT e printar o resul
  calcularPosicoes(boot_record, &fat, &fat2, &root_dir, &dados);
  printarPosicoes(boot_record, fat, fat2, root_dir, dados);

  //Infos para o primeiro arq encontrado
  unsigned short first_cluster1; 
  unsigned int size1;  
  unsigned char *nome1[8];
  unsigned char *extension1[3];
  int acessou = 0; // flag para o primeiro arq

  int n_entrada = 0; // contador para printar a ordem das entradas

  printf("Processando Dir Raiz\n\n");
  while (1) {
    // Abrindo a Imagem para visitar o Dir Raiz
    // Nova posição +32 a cada iteracao
    dp = fopen(img, "rb");
    fseek(dp, dir_raiz_posicao, SEEK_SET);
    fread(&DirRaiz, sizeof(DirRaiz), 1, dp);
    fclose(dp);

    // Ver se é o fim do Dir Raiz
    if (DirRaiz.CreatedTime == 0 && DirRaiz.attributes == 0 &&
        DirRaiz.Bits16low == 0 && DirRaiz.FileSize == 0 &&
        DirRaiz.CreationTimeisTenths == 0) {
      printf("Fim do Diretório Raiz!\n\n");
      break;
    }
    //Armazenar as infos do primeiro arquivo achado
    if(DirRaiz.attributes == 0x20 && DirRaiz.filename[0] != 0xE5 && acessou == 0){
      //copiando as infos 
      strcpy((char*)nome1, (char*)DirRaiz.filename);
      strcpy((char*)extension1, (char*)DirRaiz.extension);
      first_cluster1 = DirRaiz.Bits16low;
      size1 = DirRaiz.FileSize;
      acessou = 1; // já achou o primeiro arq
    }
    printf("ENTRADA %d\n", n_entrada);
    // Ver se é entrada excluida
    if (DirRaiz.filename[0] == 0xE5) {
      printf("Entrada Excluida, indo para próxima...\n\n");
      dir_raiz_posicao = dir_raiz_posicao + 32;
    } else if (DirRaiz.attributes == 0x0F) { // ver se é LNF
      printf("A entrada é LNF, indo para a próxima...\n\n");
      dir_raiz_posicao = dir_raiz_posicao + 32;
    } else { //Senão, imprimir dados da entrada
      printf("File Name: %s \n", DirRaiz.filename);
      printf("Extension: %s \n", DirRaiz.extension);
      printf("Tipo: 0x%X \n", DirRaiz.attributes);
      printf("First Cluster: 0x%X \n", DirRaiz.Bits16low);
      printf("File Size: 0x%X \n", DirRaiz.FileSize);
      printf("\n");
      dir_raiz_posicao = dir_raiz_posicao + 32;
    }
    n_entrada ++;
  }

  // Processando o primeiro arquivo (0x20) encontrado
  FILE *ptarq = fopen(img, "rb");
  // Ler o conteudo do arquivo
  printf("-----------------------------------------------\n\n");
  printf("Processando o Primeiro Arquivo Encontrado \n\n Arquivo: %s\n Extensão: .%s\n\n",  (char*)nome1,  (char*)extension1);
  printf("Conteúdo do Arquivo:\n\n");
  lerConteudoArquivo(ptarq, boot_record, first_cluster1, size1, dados, fat);
  fclose(ptarq);

  printf("\n");

  return 0;
}