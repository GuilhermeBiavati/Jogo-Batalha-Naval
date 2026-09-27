#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Valores usados dentro das matrizes do jogo. */
#define AGUA 0
#define NAVIO_ACERTADO 2
#define MARCA_AGUA 3
#define MARCA_IMPACTO 4
#define MARCA_AFUNDOU 5
#define DESCONHECIDO 7
#define NAVIO_BASE 10

/* Regras minimas do trabalho. */
#define TAMANHO_MINIMO 8
#define NAVIOS_MINIMOS 4
#define ARQUIVO_DADOS "batalha_naval.bin"

/* Registro que guarda as configuracoes do jogo. */
typedef struct
{
    int tamanho;
    int quantidadeNavios;
} Configuracao;

/* Registro que guarda as duas partes de um navio. */
typedef struct
{
    int linha1;
    int coluna1;
    int linha2;
    int coluna2;
    int acertos;
} Navio;

/* Registro que guarda tudo que pertence a um jogador. */
typedef struct
{
    int **campo;
    int **visaoAdversario;
    Navio *navios;
    int totalNavios;
    int naviosAfundados;
} Jogador;

/* Registro que guarda uma partida inteira. */
typedef struct
{
    Configuracao config;
    Jogador humano;
    Jogador computador;
    int turno;                  /* 0 = humano, 1 = computador */
    int emAndamento;            /* 1 = jogo ativo, 0 = jogo parado */
    int bombardeiosHumanoFeitos; /* disparos ja feitos no turno atual */
} Jogo;

/* Prototipos das funcoes. */
int **criarMatriz(int tamanho);
void liberarMatriz(int **matriz, int tamanho);
void preencherMatriz(int **matriz, int tamanho, int valor);
void imprimirCampoProprio(int **campo, int tamanho);
void imprimirCampoAdversario(int **campo, int tamanho);
void configuracaoPadrao(Configuracao *config);
int configuracoesIguais(Configuracao primeira, Configuracao segunda);
int salvarConfiguracao(Configuracao *config);
int carregarConfiguracao(Configuracao *config);
int salvarJogo(Jogo *jogo);
int carregarJogo(Jogo *jogo);
void inicializarJogo(Jogo *jogo, Configuracao config);
void liberarJogo(Jogo *jogo);
void executarPartida(Jogo *jogo);
void configurarJogo(Configuracao *config);
int contarNaviosRestantes(Navio *navios, int total, int indice);
int contarBombardeiosRealizados(int **visaoAdversario, int tamanho);

/* Limpa o restante da linha quando o usuario digita algo invalido. */
void limparEntrada(void)
{
    int c;

    do
    {
        c = getchar();
    } while (c != '\n' && c != EOF);
}

/* Le um numero inteiro do teclado. */
int lerInteiro(char mensagem[])
{
    int valor;

    printf("%s", mensagem);
    while (scanf("%d", &valor) != 1)
    {
        limparEntrada();
        printf("Digite um numero inteiro: ");
    }

    limparEntrada();
    return valor;
}

/* Pausa a tela ate o usuario confirmar que leu as mensagens. */
void aguardarEnter(void)
{
    printf("Pressione ENTER para continuar...");
    getchar();
}

/* Aloca uma matriz quadrada de inteiros usando ponteiros. */
int **criarMatriz(int tamanho)
{
    int i;
    int **matriz = (int **)malloc(tamanho * sizeof(int *));

    if (matriz == NULL)
    {
        return NULL;
    }

    for (i = 0; i < tamanho; i++)
    {
        matriz[i] = (int *)malloc(tamanho * sizeof(int));

        if (matriz[i] == NULL)
        {
            liberarMatriz(matriz, i);
            return NULL;
        }
    }

    return matriz;
}

/* Libera a memoria da matriz linha por linha. */
void liberarMatriz(int **matriz, int tamanho)
{
    int i;

    if (matriz == NULL)
    {
        return;
    }

    for (i = 0; i < tamanho; i++)
    {
        free(matriz[i]);
    }

    free(matriz);
}

/* Preenche todas as posicoes de uma matriz com o mesmo valor. */
void preencherMatriz(int **matriz, int tamanho, int valor)
{
    int i;
    int j;

    for (i = 0; i < tamanho; i++)
    {
        for (j = 0; j < tamanho; j++)
        {
            matriz[i][j] = valor;
        }
    }
}

/* Imprime os indices das colunas e as linhas separadoras. */
void imprimirCabecalho(int tamanho)
{
    int j;

    printf("     ");
    for (j = 0; j < tamanho; j++)
    {
        printf("%3d", j + 1);
    }
    printf("\n");

    printf("    +");
    for (j = 0; j < tamanho; j++)
    {
        printf("---");
    }
    printf("\n");
}

/* Imprime o campo do jogador humano, mostrando os seus navios. */
void imprimirCampoProprio(int **campo, int tamanho)
{
    int i;
    int j;
    int valor;

    imprimirCabecalho(tamanho);

    for (i = 0; i < tamanho; i++)
    {
        printf("%3d |", i + 1);
        for (j = 0; j < tamanho; j++)
        {
            valor = campo[i][j];

            if (valor == AGUA)
            {
                printf("%3d", AGUA);
            }
            else if (valor == NAVIO_ACERTADO)
            {
                printf("%3d", NAVIO_ACERTADO);
            }
            else if (valor >= NAVIO_BASE)
            {
                printf("%3d", 1);
            }
            else
            {
                printf("%3d", valor);
            }
        }
        printf("\n");
    }
}

/* Imprime o que o jogador humano conhece do campo adversario. */
void imprimirCampoAdversario(int **campo, int tamanho)
{
    int i;
    int j;

    imprimirCabecalho(tamanho);

    for (i = 0; i < tamanho; i++)
    {
        printf("%3d |", i + 1);
        for (j = 0; j < tamanho; j++)
        {
            printf("%3d", campo[i][j]);
        }
        printf("\n");
    }
}

/* Converte o valor interno do campo proprio para o numero que aparece na tela. */
int valorVisivelCampoProprio(int valor)
{
    if (valor == AGUA)
    {
        return AGUA;
    }

    if (valor == NAVIO_ACERTADO)
    {
        return NAVIO_ACERTADO;
    }

    if (valor >= NAVIO_BASE)
    {
        return 1;
    }

    return valor;
}

/* Imprime uma linha com os indices das colunas para dois campos juntos. */
void imprimirCabecalhoDuplo(int tamanho)
{
    int j;

    printf("     ");
    for (j = 0; j < tamanho; j++)
    {
        printf("%3d", j + 1);
    }

    printf(" | ");

    printf("     ");
    for (j = 0; j < tamanho; j++)
    {
        printf("%3d", j + 1);
    }
    printf("\n");

    printf("    +");
    for (j = 0; j < tamanho; j++)
    {
        printf("---");
    }

    printf(" | ");

    printf("    +");
    for (j = 0; j < tamanho; j++)
    {
        printf("---");
    }
    printf("\n");
}

/* Imprime o campo do jogador e o campo conhecido do computador juntos. */
void imprimirCamposLadoALado(int **campoHumano, int **campoComputador, int tamanho)
{
    int i;
    int j;
    int valor;

    imprimirCabecalhoDuplo(tamanho);

    for (i = 0; i < tamanho; i++)
    {
        printf("%3d |", i + 1);
        for (j = 0; j < tamanho; j++)
        {
            valor = valorVisivelCampoProprio(campoHumano[i][j]);
            printf("%3d", valor);
        }

        printf(" | ");

        printf("%3d |", i + 1);
        for (j = 0; j < tamanho; j++)
        {
            printf("%3d", campoComputador[i][j]);
        }

        printf("\n");
    }
}

/* Define a configuracao minima caso nao exista arquivo salvo. */
void configuracaoPadrao(Configuracao *config)
{
    config->tamanho = TAMANHO_MINIMO;
    config->quantidadeNavios = NAVIOS_MINIMOS;
}

/* Verifica se duas configuracoes possuem os mesmos valores. */
int configuracoesIguais(Configuracao primeira, Configuracao segunda)
{
    return primeira.tamanho == segunda.tamanho &&
           primeira.quantidadeNavios == segunda.quantidadeNavios;
}

/* Salva somente a configuracao em arquivo binario. */
int salvarConfiguracao(Configuracao *config)
{
    FILE *arquivo = fopen(ARQUIVO_DADOS, "wb");
    int temJogo = 0;

    if (arquivo == NULL)
    {
        return 0;
    }

    fwrite(config, sizeof(Configuracao), 1, arquivo);
    fwrite(&temJogo, sizeof(int), 1, arquivo);

    fclose(arquivo);
    return 1;
}

/* Carrega a configuracao do arquivo binario. */
int carregarConfiguracao(Configuracao *config)
{
    FILE *arquivo = fopen(ARQUIVO_DADOS, "rb");

    if (arquivo == NULL)
    {
        configuracaoPadrao(config);
        return 0;
    }

    fread(config, sizeof(Configuracao), 1, arquivo);
    fclose(arquivo);
    return 1;
}

/* Escreve uma matriz no arquivo binario, linha por linha. */
void escreverMatriz(FILE *arquivo, int **matriz, int tamanho)
{
    int i;

    for (i = 0; i < tamanho; i++)
    {
        fwrite(matriz[i], sizeof(int), tamanho, arquivo);
    }
}

/* Le uma matriz do arquivo binario, linha por linha. */
void lerMatriz(FILE *arquivo, int **matriz, int tamanho)
{
    int i;

    for (i = 0; i < tamanho; i++)
    {
        fread(matriz[i], sizeof(int), tamanho, arquivo);
    }
}

/* Salva uma partida completa em arquivo binario. */
int salvarJogo(Jogo *jogo)
{
    FILE *arquivo = fopen(ARQUIVO_DADOS, "wb");
    int temJogo = 1;
    int tamanho = jogo->config.tamanho;

    if (arquivo == NULL)
    {
        return 0;
    }

    fwrite(&jogo->config, sizeof(Configuracao), 1, arquivo);
    fwrite(&temJogo, sizeof(int), 1, arquivo);
    fwrite(&jogo->turno, sizeof(int), 1, arquivo);
    fwrite(&jogo->emAndamento, sizeof(int), 1, arquivo);

    fwrite(&jogo->humano.totalNavios, sizeof(int), 1, arquivo);
    fwrite(&jogo->humano.naviosAfundados, sizeof(int), 1, arquivo);
    fwrite(jogo->humano.navios, sizeof(Navio), jogo->humano.totalNavios, arquivo);
    escreverMatriz(arquivo, jogo->humano.campo, tamanho);
    escreverMatriz(arquivo, jogo->humano.visaoAdversario, tamanho);

    fwrite(&jogo->computador.totalNavios, sizeof(int), 1, arquivo);
    fwrite(&jogo->computador.naviosAfundados, sizeof(int), 1, arquivo);
    fwrite(jogo->computador.navios, sizeof(Navio), jogo->computador.totalNavios, arquivo);
    escreverMatriz(arquivo, jogo->computador.campo, tamanho);
    escreverMatriz(arquivo, jogo->computador.visaoAdversario, tamanho);

    fclose(arquivo);
    return 1;
}

/* Carrega uma partida completa do arquivo binario. */
int carregarJogo(Jogo *jogo)
{
    FILE *arquivo = fopen(ARQUIVO_DADOS, "rb");
    int temJogo;
    int tamanho;

    if (arquivo == NULL)
    {
        return 0;
    }

    fread(&jogo->config, sizeof(Configuracao), 1, arquivo);
    fread(&temJogo, sizeof(int), 1, arquivo);

    if (temJogo == 0)
    {
        fclose(arquivo);
        return 0;
    }

    tamanho = jogo->config.tamanho;

    fread(&jogo->turno, sizeof(int), 1, arquivo);
    fread(&jogo->emAndamento, sizeof(int), 1, arquivo);

    fread(&jogo->humano.totalNavios, sizeof(int), 1, arquivo);
    fread(&jogo->humano.naviosAfundados, sizeof(int), 1, arquivo);
    jogo->humano.navios = (Navio *)malloc(jogo->humano.totalNavios * sizeof(Navio));
    jogo->humano.campo = criarMatriz(tamanho);
    jogo->humano.visaoAdversario = criarMatriz(tamanho);
    fread(jogo->humano.navios, sizeof(Navio), jogo->humano.totalNavios, arquivo);
    lerMatriz(arquivo, jogo->humano.campo, tamanho);
    lerMatriz(arquivo, jogo->humano.visaoAdversario, tamanho);

    fread(&jogo->computador.totalNavios, sizeof(int), 1, arquivo);
    fread(&jogo->computador.naviosAfundados, sizeof(int), 1, arquivo);
    jogo->computador.navios = (Navio *)malloc(jogo->computador.totalNavios * sizeof(Navio));
    jogo->computador.campo = criarMatriz(tamanho);
    jogo->computador.visaoAdversario = criarMatriz(tamanho);
    fread(jogo->computador.navios, sizeof(Navio), jogo->computador.totalNavios, arquivo);
    lerMatriz(arquivo, jogo->computador.campo, tamanho);
    lerMatriz(arquivo, jogo->computador.visaoAdversario, tamanho);

    if (jogo->turno == 0)
    {
        jogo->bombardeiosHumanoFeitos = contarBombardeiosRealizados(jogo->humano.visaoAdversario, tamanho) % 3;
    }
    else
    {
        jogo->bombardeiosHumanoFeitos = 0;
    }

    fclose(arquivo);
    return 1;
}

/* Verifica se duas posicoes estao livres para colocar um navio. */
int posicaoLivre(int **campo, int linha1, int coluna1, int linha2, int coluna2)
{
    return campo[linha1][coluna1] == AGUA && campo[linha2][coluna2] == AGUA;
}

/* Sorteia todos os navios de um jogador sem deixar sobrepor. */
void posicionarNavios(Jogador *jogador, int tamanho)
{
    int i;
    int linha;
    int coluna;
    int direcao;
    int linha2;
    int coluna2;

    for (i = 0; i < jogador->totalNavios; i++)
    {
        do
        {
            linha = rand() % tamanho;
            coluna = rand() % tamanho;
            direcao = rand() % 2;
            linha2 = linha;
            coluna2 = coluna;

            if (direcao == 0)
            {
                coluna2 = coluna + 1;
            }
            else
            {
                linha2 = linha + 1;
            }
        } while (linha2 >= tamanho || coluna2 >= tamanho ||
                 !posicaoLivre(jogador->campo, linha, coluna, linha2, coluna2));

        jogador->navios[i].linha1 = linha;
        jogador->navios[i].coluna1 = coluna;
        jogador->navios[i].linha2 = linha2;
        jogador->navios[i].coluna2 = coluna2;
        jogador->navios[i].acertos = 0;

        jogador->campo[linha][coluna] = NAVIO_BASE + i;
        jogador->campo[linha2][coluna2] = NAVIO_BASE + i;
    }
}

/* Aloca e prepara as estruturas de um jogador. */
void inicializarJogador(Jogador *jogador, int tamanho, int totalNavios)
{
    jogador->campo = criarMatriz(tamanho);
    jogador->visaoAdversario = criarMatriz(tamanho);
    jogador->navios = (Navio *)malloc(totalNavios * sizeof(Navio));
    jogador->totalNavios = totalNavios;
    jogador->naviosAfundados = 0;

    preencherMatriz(jogador->campo, tamanho, AGUA);
    preencherMatriz(jogador->visaoAdversario, tamanho, DESCONHECIDO);
    posicionarNavios(jogador, tamanho);
}

/* Libera a memoria usada por um jogador. */
void liberarJogador(Jogador *jogador, int tamanho)
{
    liberarMatriz(jogador->campo, tamanho);
    liberarMatriz(jogador->visaoAdversario, tamanho);
    free(jogador->navios);

    jogador->campo = NULL;
    jogador->visaoAdversario = NULL;
    jogador->navios = NULL;
}

/* Cria uma nova partida. */
void inicializarJogo(Jogo *jogo, Configuracao config)
{
    jogo->config = config;
    jogo->turno = 0;
    jogo->emAndamento = 1;
    jogo->bombardeiosHumanoFeitos = 0;

    inicializarJogador(&jogo->humano, config.tamanho, config.quantidadeNavios);
    inicializarJogador(&jogo->computador, config.tamanho, config.quantidadeNavios);
}

/* Libera a memoria usada pela partida. */
void liberarJogo(Jogo *jogo)
{
    int tamanho = jogo->config.tamanho;

    liberarJogador(&jogo->humano, tamanho);
    liberarJogador(&jogo->computador, tamanho);
}

/* Funcao recursiva: conta navios que ainda nao foram afundados. */
int contarNaviosRestantes(Navio *navios, int total, int indice)
{
    if (indice == total)
    {
        return 0;
    }

    if (navios[indice].acertos < 2)
    {
        return 1 + contarNaviosRestantes(navios, total, indice + 1);
    }

    return contarNaviosRestantes(navios, total, indice + 1);
}

/* Conta quantas posicoes o jogador ja bombardeou no campo adversario. */
int contarBombardeiosRealizados(int **visaoAdversario, int tamanho)
{
    int i;
    int j;
    int total = 0;

    for (i = 0; i < tamanho; i++)
    {
        for (j = 0; j < tamanho; j++)
        {
            if (visaoAdversario[i][j] != DESCONHECIDO)
            {
                total++;
            }
        }
    }

    return total;
}

/* Mostra o significado dos numeros do tabuleiro. */
void mostrarLegenda(void)
{
    printf("\n===== LEGENDA =====\n");
    printf("0 = Agua no seu campo\n");
    printf("1 = Seu navio\n");
    printf("2 = Parte do seu navio acertada\n");
    printf("3 = Agua encontrada no campo adversario\n");
    printf("4 = Impacto em navio adversario\n");
    printf("5 = Navio adversario afundado\n");
    printf("7 = Posicao desconhecida no campo adversario\n");
}

/* Limpa a tela usando sequencia ANSI, sem depender do comando clear/cls. */
void limparTela(void)
{
    printf("\033[2J\033[H");
    fflush(stdout);
}

/* Mostra o campo do jogador e o campo conhecido do computador. */
void mostrarCampos(Jogo *jogo)
{
    int restantesHumano;
    int restantesComputador;
    int larguraCampo = 5 + jogo->config.tamanho * 3;
    char textoHumano[80];
    char textoComputador[80];

    restantesHumano = contarNaviosRestantes(jogo->humano.navios, jogo->humano.totalNavios, 0);
    restantesComputador = contarNaviosRestantes(jogo->computador.navios, jogo->computador.totalNavios, 0);
    sprintf(textoHumano, "Seus navios restantes: %d", restantesHumano);
    sprintf(textoComputador, "Navios restantes do computador: %d", restantesComputador);

    printf("\n%-*s | %s\n", larguraCampo, "===== SEU CAMPO =====",
           "===== CAMPO DO COMPUTADOR CONHECIDO =====");
    printf("%-*s | %s\n", larguraCampo, textoHumano, textoComputador);

    imprimirCamposLadoALado(jogo->humano.campo,
                            jogo->humano.visaoAdversario,
                            jogo->config.tamanho);

    mostrarLegenda();
    printf("\n");
}

/* Procura o indice do navio que esta em uma posicao. */
int indiceNavioPorPosicao(Navio *navios, int total, int linha, int coluna)
{
    int i;

    for (i = 0; i < total; i++)
    {
        if ((navios[i].linha1 == linha && navios[i].coluna1 == coluna) ||
            (navios[i].linha2 == linha && navios[i].coluna2 == coluna))
        {
            return i;
        }
    }

    return -1;
}

/* Marca as duas partes de um navio afundado. */
void marcarNavioAfundado(int **visao, Navio navio)
{
    visao[navio.linha1][navio.coluna1] = MARCA_AFUNDOU;
    visao[navio.linha2][navio.coluna2] = MARCA_AFUNDOU;
}

/* Faz um bombardeio e devolve Agua, Impacto ou Afundou. */
int bombardear(Jogador *atacante, Jogador *defensor, int linha, int coluna)
{
    int indice;
    int valor = defensor->campo[linha][coluna];

    if (valor == AGUA)
    {
        atacante->visaoAdversario[linha][coluna] = MARCA_AGUA;
        return MARCA_AGUA;
    }

    if (valor >= NAVIO_BASE)
    {
        indice = indiceNavioPorPosicao(defensor->navios, defensor->totalNavios, linha, coluna);
        defensor->campo[linha][coluna] = NAVIO_ACERTADO;
        defensor->navios[indice].acertos++;

        if (defensor->navios[indice].acertos == 2)
        {
            defensor->naviosAfundados++;
            marcarNavioAfundado(atacante->visaoAdversario, defensor->navios[indice]);
            return MARCA_AFUNDOU;
        }
        else
        {
            atacante->visaoAdversario[linha][coluna] = MARCA_IMPACTO;
            return MARCA_IMPACTO;
        }
    }

    return MARCA_AGUA;
}

/* Mostra o texto do resultado de um bombardeio. */
void mostrarResultadoBombardeio(int resultado)
{
    if (resultado == MARCA_AGUA)
    {
        printf("Agua!\n");
    }
    else if (resultado == MARCA_IMPACTO)
    {
        printf("Impacto!\n");
    }
    else
    {
        printf("Afundou!\n");
    }
}

/* Verifica se o jogador ja bombardeou uma posicao. */
int posicaoJaBombardeada(Jogador *atacante, int linha, int coluna)
{
    return atacante->visaoAdversario[linha][coluna] != DESCONHECIDO;
}

/* Executa os tres bombardeios do jogador humano. */
int turnoHumano(Jogo *jogo)
{
    int i;
    int linha;
    int coluna;
    int linhaEscolhida;
    int colunaEscolhida;
    int tamanho = jogo->config.tamanho;
    int entradaValida;

    for (i = jogo->bombardeiosHumanoFeitos; i < 3; i++)
    {
        mostrarCampos(jogo);
        printf("Bombardeio %d de 3\n", i + 1);
        printf("Digite 0 na linha para voltar ao menu sem salvar.\n");
        printf("Digite -1 na linha para salvar e voltar ao menu.\n");

        do
        {
            entradaValida = 1;
            linhaEscolhida = lerInteiro("Escolha a linha do campo adversario: ");

            if (linhaEscolhida == 0)
            {
                jogo->emAndamento = 0;
                return 0;
            }

            if (linhaEscolhida == -1)
            {
                salvarJogo(jogo);
                printf("Jogo salvo.\n");
                return 0;
            }

            if (linhaEscolhida < 1 || linhaEscolhida > tamanho)
            {
                printf("Linha invalida. Use valores de 1 ate %d.\n", tamanho);
                entradaValida = 0;
            }
            else
            {
                colunaEscolhida = lerInteiro("Escolha a coluna do campo adversario: ");

                if (colunaEscolhida < 1 || colunaEscolhida > tamanho)
                {
                    printf("Coluna invalida. Use valores de 1 ate %d.\n", tamanho);
                    entradaValida = 0;
                }
                else
                {
                    linha = linhaEscolhida - 1;
                    coluna = colunaEscolhida - 1;
                }

                if (entradaValida && posicaoJaBombardeada(&jogo->humano, linha, coluna))
                {
                    printf("Essa posicao ja foi bombardeada.\n");
                    entradaValida = 0;
                }
            }
        } while (!entradaValida);

        printf("\nResultado do bombardeio: ");
        mostrarResultadoBombardeio(bombardear(&jogo->humano, &jogo->computador, linha, coluna));
        jogo->bombardeiosHumanoFeitos = i + 1;

        if (jogo->computador.naviosAfundados == jogo->computador.totalNavios)
        {
            return 1;
        }
    }

    jogo->bombardeiosHumanoFeitos = 0;
    jogo->turno = 1;
    printf("\nFim do seu turno. Agora e a vez da maquina bombardear.\n");
    return 1;
}

/* Converte uma posicao sorteada em linha e coluna. */
void posicaoParaCoordenada(int posicao, int tamanho, int *linha, int *coluna)
{
    posicao--;
    *linha = posicao / tamanho;
    *coluna = posicao % tamanho;
}

/* Sorteia uma posicao valida para o computador. */
void sortearPosicaoComputador(Jogo *jogo, int *linha, int *coluna)
{
    int posicao;
    int limite = jogo->config.tamanho * jogo->config.tamanho;

    do
    {
        posicao = (rand() % limite) + 1;
        posicaoParaCoordenada(posicao, jogo->config.tamanho, linha, coluna);
    } while (posicaoJaBombardeada(&jogo->computador, *linha, *coluna));
}

/* Executa os tres bombardeios do computador. */
void turnoComputador(Jogo *jogo)
{
    int i;
    int j;
    int linha;
    int coluna;
    int totalBombardeios = 0;
    int linhas[3];
    int colunas[3];
    int resultados[3];

    for (i = 0; i < 3; i++)
    {
        sortearPosicaoComputador(jogo, &linha, &coluna);
        linhas[i] = linha + 1;
        colunas[i] = coluna + 1;
        resultados[i] = bombardear(&jogo->computador, &jogo->humano, linha, coluna);
        totalBombardeios++;

        limparTela();
        mostrarCampos(jogo);
        printf("===== BOMBARDEIOS DA MAQUINA =====\n");

        for (j = 0; j < totalBombardeios; j++)
        {
            printf("Bombardeio %d de 3\n", j + 1);
            printf("A maquina bombardeou a linha %d e coluna %d.\n", linhas[j], colunas[j]);
            printf("\nResultado do bombardeio: ");
            mostrarResultadoBombardeio(resultados[j]);
            printf("\n");
        }

        if (jogo->humano.naviosAfundados == jogo->humano.totalNavios)
        {
            break;
        }
    }

    if (jogo->humano.naviosAfundados == jogo->humano.totalNavios)
    {
        return;
    }

    aguardarEnter();
    jogo->bombardeiosHumanoFeitos = 0;
    jogo->turno = 0;
}

/* Menu que aparece depois de vitoria ou derrota. */
int menuFimDeJogo(void)
{
    printf("\n1 - Jogar novamente\n");
    printf("2 - Voltar ao menu principal\n");
    printf("3 - Sair\n");
    return lerInteiro("Escolha: ");
}

/* Controla a partida ate alguem vencer ou o jogador voltar ao menu. */
void executarPartida(Jogo *jogo)
{
    int opcao;
    int terminou = 0;

    while (jogo->emAndamento && !terminou)
    {
        if (jogo->turno == 0)
        {
            if (!turnoHumano(jogo))
            {
                return;
            }
        }
        else
        {
            turnoComputador(jogo);
        }

        if (jogo->computador.naviosAfundados == jogo->computador.totalNavios)
        {
            printf("\nVoce venceu!\n");
            terminou = 1;
        }
        else if (jogo->humano.naviosAfundados == jogo->humano.totalNavios)
        {
            printf("\nVoce perdeu!\n");
            terminou = 1;
        }
    }

    while (terminou)
    {
        opcao = menuFimDeJogo();

        if (opcao == 1)
        {
            liberarJogo(jogo);
            inicializarJogo(jogo, jogo->config);
            executarPartida(jogo);
            terminou = 0;
        }
        else if (opcao == 2)
        {
            terminou = 0;
        }
        else if (opcao == 3)
        {
            jogo->emAndamento = 0;
            terminou = 0;
            exit(0);
        }
        else
        {
            printf("Opcao invalida.\n");
        }
    }
}

/* Permite mudar tamanho do campo e quantidade de navios. */
void configurarJogo(Configuracao *config)
{
    Configuracao novaConfig;
    int tamanho;
    int navios;
    int maximoNavios;

    tamanho = lerInteiro("Tamanho do campo quadrado (minimo 8): ");
    if (tamanho < TAMANHO_MINIMO)
    {
        tamanho = TAMANHO_MINIMO;
    }

    maximoNavios = (tamanho * tamanho) / 2;
    navios = lerInteiro("Quantidade de navios de duas celulas (minimo 4): ");
    if (navios < NAVIOS_MINIMOS)
    {
        navios = NAVIOS_MINIMOS;
    }
    if (navios > maximoNavios)
    {
        navios = maximoNavios;
    }

    novaConfig.tamanho = tamanho;
    novaConfig.quantidadeNavios = navios;

    if (configuracoesIguais(*config, novaConfig))
    {
        printf("Configuracao mantida. O jogo salvo foi preservado.\n");
        return;
    }

    *config = novaConfig;

    if (salvarConfiguracao(config))
    {
        printf("Configuracao salva.\n");
    }
    else
    {
        printf("Nao foi possivel salvar a configuracao.\n");
    }
}

/* Le a opcao escolhida no menu principal. */
int lerOpcaoMenu(void)
{
    return lerInteiro("Escolha: ");
}

/* Mostra o menu principal do jogo. */
void mostrarMenu(Configuracao config)
{
    printf("\n===== BATALHA NAVAL =====\n");
    printf("Configuracao atual: campo %dx%d, %d navios\n",
           config.tamanho, config.tamanho, config.quantidadeNavios);
    printf("1 - Novo Jogo\n");
    printf("2 - Configurar\n");
    printf("3 - Carregar\n");
    printf("4 - Sair\n");
}

/* Funcao principal: o programa sempre comeca por aqui. */
int main(void)
{
    Configuracao config;
    Jogo jogo;
    int opcao;
    int executando = 1;

    srand((unsigned int)time(NULL));
    carregarConfiguracao(&config);

    while (executando)
    {
        mostrarMenu(config);
        opcao = lerOpcaoMenu();

        if (opcao == 1)
        {
            inicializarJogo(&jogo, config);
            executarPartida(&jogo);
            liberarJogo(&jogo);
        }
        else if (opcao == 2)
        {
            configurarJogo(&config);
        }
        else if (opcao == 3)
        {
            if (carregarJogo(&jogo))
            {
                executarPartida(&jogo);
                liberarJogo(&jogo);
                carregarConfiguracao(&config);
            }
            else
            {
                printf("Nenhum jogo salvo encontrado.\n");
            }
        }
        else if (opcao == 4)
        {
            executando = 0;
        }
        else
        {
            printf("Opcao invalida.\n");
        }
    }

    printf("Programa encerrado.\n");
    return 0;
}
