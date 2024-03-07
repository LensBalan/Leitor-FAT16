# Leitor-FAT16
Leitor de imagens FAT16 implementado em C

**Problema:**  
Fazer um programa (linguagem de livre escolha), que faça a leitura de uma imagem em FAT16, e apresente as seguintes informações:
* Dados do boot record: bytes por setor, setores reservados, setores por cluster, numero de fat, numero de setores por fat e número de entradas no diretório raiz.
* Apresentar as entradas (somente 8.3) válidas do diretório raiz apresentando as seguintes informações: nome, extensão, tipo, first cluster e tamanho.
* Mostrar o conteúdo de um arquivo (o primeiro arquivo encontrado no diretório raiz).

**Readme do código:**

Programa implementado em C, 'lens_fat16.c'.

- Compilar no terminal Linux: gcc -o lens_fat16 lens_fat16.c -lm

- Compilando no Windows: Exibindo resultados inconsistentes e rodando incorretamente. Usar Linux.

- Alterar imagem FAT 16 a ser aberta: Linha 124, ' char img[] = "nome_da_img.img"; '.

- Linha 148 a 157, fórmulas comentadas que peguei do FAT OS Dev WIKI, porém não foram necessárias
na execução final do programa.
