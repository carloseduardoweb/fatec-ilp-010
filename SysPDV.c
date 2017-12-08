/* 
 * CABEÇALHO DESCRITIVO
 * 
 * Arquivo: SysPDV.c.
 * 
 * Descrição: sistema gerenciador de vendas em pontos de venda (PDVs).
 * 
 */


////////////////////////////////////////////////////////////////////
//                 DIRETIVAS DE PRÉ-PROCESSAMENTO                 //
////////////////////////////////////////////////////////////////////

// Bibliotecas Padrão
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio_ext.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>

// Macros
#define DEBUG(formato, ...) vDebugFormatado(__FILE__, __LINE__, __func__, formato, ##__VA_ARGS__)

#define FALSE     0
#define TRUE      1

#define FAILURE   -1
#define SUCCESS   0

#define MAX_ITENS   999  /* Número máximo de itens por cupom. */
#define MAX_ACESSOS 4    /* Número máximo de tentativas de acesso consecutivas. */

#define LIMPA_TELA            0
#define TELA_MATRIZ           1
#define LIMPA_SECAO_I         2
#define LIMPA_SECAO_II        3
#define LIMPA_SECAO_III       4
#define LIMPA_SECAO_IV        5
#define LIMPA_SECOES          6
#define TELA_LOGO             7
#define TELA_APRESENTACAO     8
#define TELA_ESPERA_USUARIO   9
#define TELA_RESPOSTA_USUARIO 10
#define TELA_FALHA_SISTEMA    11
#define TELA_MANUTENCAO       12
#define TELA_CAIXA_FECHADO    13


////////////////////////////////////////////////////////////////////
//                      DEFINIÇÕES DE TIPOS                       //
////////////////////////////////////////////////////////////////////

typedef struct TIPO_USUARIO
{
  char szTipo  [ 2]; // Tipo de usuário: ADM [1], FISCAL [2], OPERADOR [3].
  char szLogin [16]; // Login do usuário.
  char szSenha [16]; // Senha do usuário.
  char szNome  [50]; // Nome do usuário.
} TIPO_USUARIO;

typedef struct TIPO_LISTA_USUARIOS
{
  TIPO_USUARIO                *pstUsuario;
  struct TIPO_LISTA_USUARIOS  *pstAnterior;
  struct TIPO_LISTA_USUARIOS  *pstProximo;
  struct TIPO_LISTA_USUARIOS  *pstPrimeiro;
  struct TIPO_LISTA_USUARIOS **ppstUltimo;
} TIPO_LISTA_USUARIOS;

typedef struct TIPO_SKU
{
  unsigned long int ulCodigo;         // Código de identificação (por exemplo: EAN, PLU etc.).
  char              szDescricao [50]; // Descrição da mercadoria.
  float             fValorUnitario;   // Valor unitário.
} TIPO_SKU; // Stock Keeping Unit (Unidade Existente de Estoque).

typedef struct TIPO_LISTA_SKUS
{
  TIPO_SKU                *pstSKU;
  struct TIPO_LISTA_SKUS  *pstAnterior;
  struct TIPO_LISTA_SKUS  *pstProximo;
  struct TIPO_LISTA_SKUS  *pstPrimeiro;
  struct TIPO_LISTA_SKUS **ppstUltimo;
} TIPO_LISTA_SKUS;

typedef struct TIPO_ITEM
{
  unsigned int uiSequenciaItem; // Número de sequência do item.
  TIPO_SKU     stSKUItem;       // SKU do item.
  float        fQuantidade;     // Quantidade.
  double       dValorItem;      // Valor do item (= valor unitário x quantidade).
} TIPO_ITEM;

typedef struct TIPO_CUPOM
{
  char              szDataHora [16];     // Data e hora da compra [ddmmaaaahhmmss].
  unsigned long int ulNumeroCupom;       // Número do cupom.
  TIPO_USUARIO      stOperador;          // Operador do PDV.
  unsigned int      uiTotalItens;        // Número de itens.
  TIPO_ITEM         astItem [MAX_ITENS]; // Lista de itens (array de estruturas).
  double            dValorCupom;         // Valor do cupom (= soma dos valores dos itens).
} TIPO_CUPOM;

typedef struct TIPO_LISTA_CUPONS
{
  TIPO_CUPOM                *pstCupom;
  struct TIPO_LISTA_CUPONS  *pstAnterior;
  struct TIPO_LISTA_CUPONS  *pstProximo;
  struct TIPO_LISTA_CUPONS  *pstPrimeiro;
  struct TIPO_LISTA_CUPONS **ppstUltimo;
} TIPO_LISTA_CUPONS;

typedef struct TIPO_DATA_HORA
{
  char szDia  [3];
  char szMes  [3];
  char szAno  [5];
  char szHora [3];
  char szMin  [3];
  char szSeg  [3];
} TIPO_DATA_HORA;


////////////////////////////////////////////////////////////////////
//                          PROTOTIPAÇÃO                          //
////////////////////////////////////////////////////////////////////

// Funções Secundárias
int  bValidarAcesso(char *pszTipoUsuario, char *pszNomeUsuario, char *pszLoginUsuario); 
int  iCadastrarUsuarios(void); 
int  iCadastrarProdutos(void); 
int  iRealizarVendas(char *pszLoginUsuario);

// Funções Terciárias
int  iCarregarUsuarios(const char *pszNomeArquivoUsuarios, TIPO_LISTA_USUARIOS **ppstUsuarioLista); 
int  iCarregarSKUs    (const char *pszNomeArquivoSKUs,     TIPO_LISTA_SKUS     **ppstSKULista); 
int  iCarregarCupons  (const char *pszNomeArquivoCupons,   TIPO_LISTA_CUPONS   **ppstCupomLista); 
void vLiberarUsuarios(TIPO_LISTA_USUARIOS *pstUsuarioLista); 
void vLiberarSKUs    (TIPO_LISTA_SKUS     *pstSKULista); 
void vLiberarCupons  (TIPO_LISTA_CUPONS   *pstCupomLista); 
int  iProcurarUsuario(TIPO_LISTA_USUARIOS *pstUsuarioLista, const char              *pszLogin,      TIPO_USUARIO *pstUsuario); 
int  iProcurarSKU    (TIPO_LISTA_SKUS     *pstSKULista,     const unsigned long int  ulCodigo,      TIPO_SKU     *pstSKU); 
int  iProcurarCupom  (TIPO_LISTA_CUPONS   *pstCupomLista,   const unsigned long int  ulNumeroCupom, TIPO_CUPOM   *ptCupom); 
int  iEscreverUsuario(const TIPO_USUARIO stUsuario); 
int  iEscreverSKU    (const TIPO_SKU     stSKU); 
int  iEscreverCupom  (const TIPO_CUPOM   stCupom); 

// Outras Funções
void vDebugFormatado(const char *pszFonte, const int iLinha, const char *pszFuncao, const char *pszFormato, ...); 
int  iExtrairTokenArquivo(FILE *pfArquivo, const char *szDelimitadores, size_t uTamMax, char *pszToken); 
int  bExibirTela(int iOpcao); 
int  iExibirMensagem(unsigned int uiPosX, unsigned int uiPosY, char* pszFormato, ...); 
void vLimparLinha(unsigned int uiLinha); 
void vLimparBufferPadrao(struct _IO_FILE *pstIOBuffer); 
void vTerminal_GoTo(unsigned int uiPosX, unsigned int uiPosY); 
void vObterDataHora(TIPO_DATA_HORA *pstDataHora); 


////////////////////////////////////////////////////////////////////
//                      VARIÁVEIS GLOBAIS                         //
////////////////////////////////////////////////////////////////////

const char szNomeArquivoUsuarios [] = "USUARIOS.dat";
const char szNomeArquivoSKUs     [] = "SKUS.dat";
const char szNomeArquivoCupons   [] = "CUPONS.dat";
const char szNomeArquivoDebug    [] = "SysPDV.log";


////////////////////////////////////////////////////////////////////
//                       FUNÇÃO PRINCIPAL                         //
////////////////////////////////////////////////////////////////////

int main(void)
{
  int  iTentativasAcesso  = 0;
  int  bFimOperacoes      = FALSE;
  int  bFimSistema        = FALSE;
  char szTipoUsuario [16];
  char szNomeUsuario [50];
  char szLoginUsuario[16];

  memset(szTipoUsuario,  '\0', sizeof (szTipoUsuario));
  memset(szNomeUsuario,  '\0', sizeof (szNomeUsuario));
  memset(szLoginUsuario, '\0', sizeof (szLoginUsuario));
  
  DEBUG("--- Inicio");
  
  bExibirTela(TELA_MATRIZ);
  bExibirTela(LIMPA_SECOES);
  bExibirTela(TELA_LOGO);
  
  // Criação do arquivo de usuários padrão.
  {
    FILE *pfArquivo = NULL;
    
    if ((pfArquivo = fopen(szNomeArquivoUsuarios, "r")) == NULL)
    {
      if ((pfArquivo = fopen(szNomeArquivoUsuarios, "w")) == NULL)
      {
        bExibirTela(TELA_FALHA_SISTEMA);
        bExibirTela(TELA_ESPERA_USUARIO);
        bExibirTela(TELA_MANUTENCAO);
        vTerminal_GoTo(1, 38);
        
        DEBUG("Erro: falha ao criar o arquivo de usuarios padrao");
        DEBUG("--- Fim");
        return FAILURE;
      }
      
      fprintf(pfArquivo, "1|1234|123456|ADMINISTRADOR PADRAO|\n");
    }
    
    fclose(pfArquivo);
    pfArquivo = NULL;
  }
  
  // Laço principal do sistema.
  do
  {
    // Tela de apresentação.
    {
      bExibirTela(TELA_APRESENTACAO);
      
      if (bExibirTela(TELA_RESPOSTA_USUARIO) == FALSE)
      {
        bExibirTela(TELA_CAIXA_FECHADO);
        
        DEBUG("Sistema abortado pelo usuario");
        break;
      }
    }
    
    // Validação de acesso.
    {
      bFimOperacoes = FALSE;
      
      do
      {
        memset(szTipoUsuario,  '\0', sizeof (szTipoUsuario));
        memset(szNomeUsuario,  '\0', sizeof (szNomeUsuario));
        memset(szLoginUsuario, '\0', sizeof (szLoginUsuario));
        
        bExibirTela(LIMPA_SECAO_II);
        iExibirMensagem(5, 12, "LOGIN DE USUARIOS");
        
        // Verifica se não será excedido o número de tentativas de acesso consecutivas.
        if (iTentativasAcesso + 1 > MAX_ACESSOS)
        {
          bExibirTela(TELA_MANUTENCAO);
          vTerminal_GoTo(1, 38);
          
          DEBUG("Excedido o numero de tentativas de acesso");
          DEBUG("--- Fim");
          return FAILURE;
        }
        
        // Exibe a mensagem de acessos restantes.
        if (iTentativasAcesso >= 1)
        {
          iExibirMensagem(5, 19, "%d tentativa(s) restante(s).", MAX_ACESSOS - iTentativasAcesso);
          
          if (bExibirTela(TELA_RESPOSTA_USUARIO) == FALSE)
          {
            bFimOperacoes = TRUE;
            break;
          }
        }
        
        iTentativasAcesso++;                
        
        DEBUG("Tentativa de acesso = [%d/%d]", iTentativasAcesso, MAX_ACESSOS);
      } while (bValidarAcesso(szTipoUsuario, szNomeUsuario, szLoginUsuario) != TRUE);
      
      // Se o acesso foi validado com sucesso, então zera-se o contador de acessos.
      if (bFimOperacoes == FALSE)
      {
        iTentativasAcesso = 0;
      }
    }
    
    // Operações de venda.
    {
      while (bFimOperacoes != TRUE)
      {
        char szOpcao [32];
        
        memset(szOpcao, '\0', sizeof (szOpcao));
        
        // Cabeçalho personalizado.
        {
          TIPO_DATA_HORA stDataHora; 
          char           szMensagem     [128];
          char           szPrimeiroNome [50];                 
          
          memset(&stDataHora,    '\0', sizeof (stDataHora));
          memset(szMensagem,     '\0', sizeof (szMensagem));
          memset(szPrimeiroNome, '\0', sizeof (szPrimeiroNome));
          
          vObterDataHora(&stDataHora);
          strncpy(szPrimeiroNome, szNomeUsuario, sizeof (szNomeUsuario));
          strtok(szPrimeiroNome, " \t\n");
          
          if (atoi(stDataHora.szHora) >= 0 && atoi(stDataHora.szHora) < 12)
          {
            sprintf(szMensagem, "Bom dia, %s!", szPrimeiroNome);
          }
          else if (atoi(stDataHora.szHora) < 18)
          {
            sprintf(szMensagem, "Boa tarde, %s!", szPrimeiroNome);
          }
          else
          {
            sprintf(szMensagem, "Boa noite, %s!", szPrimeiroNome);
          }
          
          bExibirTela(LIMPA_SECAO_I);
          bExibirTela(TELA_LOGO);
          iExibirMensagem(65, 3, "%s/%s/%s  %sh%s", 
                          stDataHora.szDia,
                          stDataHora.szMes,
                          stDataHora.szAno,
                          stDataHora.szHora,
                          stDataHora.szMin);
          vLimparLinha(5);
          iExibirMensagem(5, 5, szMensagem);
        }
        
        // Exibição de menu.
        {
          bExibirTela(LIMPA_SECAO_II);
          iExibirMensagem(5, 12, "MENU PRINCIPAL");
          bExibirTela(LIMPA_SECAO_III);
          bExibirTela(LIMPA_SECAO_IV);
          
          switch (atoi(szTipoUsuario))
          {
            case 1 :                  // ADM
            {
              iExibirMensagem(5, 14, "1) Cadastrar usuarios");
              iExibirMensagem(5, 15, "2) Cadastrar produtos");
              iExibirMensagem(5, 16, "3) Realizar vendas");
              iExibirMensagem(5, 17, "9) Sair");
              break;
            }
            case 2 :                  // FISCAL
            {
              iExibirMensagem(5, 14, "1) Cadastrar produtos");
              iExibirMensagem(5, 15, "2) Realizar vendas");
              iExibirMensagem(5, 16, "9) Sair");
              break;
            }
            case 3 :                  // OPERADOR
            {
              iExibirMensagem(5, 14, "1) Realizar vendas");
              iExibirMensagem(5, 15, "9) Sair");
              break;
            }
            default :
            {
              bExibirTela(TELA_FALHA_SISTEMA);
              bExibirTela(TELA_ESPERA_USUARIO);
              bExibirTela(TELA_MANUTENCAO);
              vTerminal_GoTo(1, 38);
              
              DEBUG("Tipo de usuario inconsistente = [%s]", szTipoUsuario);
              DEBUG("--- Fim");
              return FAILURE;
            }
          }
        }
        
        // Entrada da opção.
        {
          unsigned int  uiPosX = 5;
          unsigned int  uiPosY = 20;          
          
          uiPosX += iExibirMensagem(uiPosX, uiPosY, "Digite uma opcao: ");
          vTerminal_GoTo(uiPosX, uiPosY);
          vLimparBufferPadrao(stdin);
          fgets(szOpcao, sizeof (szOpcao), stdin);
          strtok(szOpcao, "\n");

          DEBUG("Menu principal opcao = [%s])", szOpcao);
          DEBUG("Menu principal opcao (atoi=[%d])", atoi(szOpcao));
        }

        // Tratamento da opção digitada.
        {
          switch (atoi(szTipoUsuario))
          {
            case 1 :                  // ADM
            {
              switch (atoi(szOpcao))
              {
                case 1 :
                {
                  iCadastrarUsuarios();
                  break;
                }
                case 2 :
                {
                  iCadastrarProdutos();
                  break;
                }
                case 3 :
                {
                  iRealizarVendas(szLoginUsuario);
                  break;
                }
                case 9 :
                {
                  bFimOperacoes = TRUE;
                  bExibirTela(TELA_CAIXA_FECHADO);
                  break;
                }
                default :
                {
                  vLimparLinha(25);
                  iExibirMensagem(5, 25, "Opcao inválida!");
                  if (bExibirTela(TELA_RESPOSTA_USUARIO) == FALSE)
                  {
                    bFimOperacoes = TRUE;
                    bExibirTela(TELA_CAIXA_FECHADO);
                  }
                  vLimparLinha(25);
                }
              }
              break;
            }
            case 2 :                  // FISCAL
            {
              switch (atoi(szOpcao))
              {
                case 1 :
                {
                  iCadastrarProdutos();
                  break;
                }
                case 2 :
                {
                  iRealizarVendas(szLoginUsuario);
                  break;
                }
                case 9 :
                {
                  bFimOperacoes = TRUE;
                  bExibirTela(TELA_CAIXA_FECHADO);
                  break;
                }
                default :
                {
                  vLimparLinha(25);
                  iExibirMensagem(5, 25, "Opcao inválida!");
                  if (bExibirTela(TELA_RESPOSTA_USUARIO) == FALSE)
                  {
                    bFimOperacoes = TRUE;
                    bExibirTela(TELA_CAIXA_FECHADO);
                  }
                  vLimparLinha(25);
                }
              }
              break;
            }
            case 3 :                  // OPERADOR
            {
              switch (atoi(szOpcao))
              {
                case 1 :
                {
                  iRealizarVendas(szLoginUsuario);
                  break;
                }
                case 9 :
                {
                  bFimOperacoes = TRUE;
                  bExibirTela(TELA_CAIXA_FECHADO);
                  break;
                }
                default :
                {
                  vLimparLinha(25);
                  iExibirMensagem(5, 25, "Opcao inválida!");
                  if (bExibirTela(TELA_RESPOSTA_USUARIO) == FALSE)
                  {
                    bFimOperacoes = TRUE;
                    bExibirTela(TELA_CAIXA_FECHADO);
                  }
                  vLimparLinha(25);
                }
              }
              break;
            }
            default :
            {
              bExibirTela(TELA_FALHA_SISTEMA);
              bExibirTela(TELA_ESPERA_USUARIO);
              bExibirTela(TELA_MANUTENCAO);
              vTerminal_GoTo(1, 38);
              
              DEBUG("Tipo de usuario inconsistente = [%s]", szTipoUsuario);
              DEBUG("--- Fim");
              return FAILURE;
            }
          }
        
        }
      }

    }

  } while (bFimSistema != TRUE);

  vTerminal_GoTo(1, 38);

  DEBUG("--- Fim");
  return SUCCESS;
}


////////////////////////////////////////////////////////////////////
//                      FUNÇÕES AUXILIARES                        //
////////////////////////////////////////////////////////////////////

// Valida o acesso de usuários.
int bValidarAcesso(char *pszTipoUsuario, char *pszNomeUsuario, char *pszLoginUsuario)
{
  TIPO_LISTA_USUARIOS *pstUsuarioLista = NULL;
  TIPO_USUARIO         stUsuario;
  char                 szLogin[17];
  char                 szSenha[17];
  int                  bLoginValido    = 0;
  int                  bSenhaValida    = 0;
  
  memset(&stUsuario, '\0', sizeof (stUsuario));
  memset(szLogin,    '\0', sizeof (szLogin));
  memset(szSenha,    '\0', sizeof (szSenha));

  DEBUG("--- Inicio");
  
  bExibirTela(TELA_MATRIZ);
  bExibirTela(LIMPA_SECOES);
  bExibirTela(TELA_LOGO);
  
  // Entrada de login e senha.
  {
    unsigned int uiPosX = 0;
    
    iExibirMensagem(5, 12, "LOGIN DE USUARIOS");    
    
    uiPosX = iExibirMensagem(5, 14, "Digite seu login: ");
    vTerminal_GoTo(uiPosX + 5, 14);
    vLimparBufferPadrao(stdin);
    fgets(szLogin, sizeof (szLogin), stdin);
    strtok(szLogin, "\n");
    
    DEBUG("Login digitado = [%s]", szLogin);

    uiPosX = iExibirMensagem(5, 16, "Digite sua senha: ");
    vTerminal_GoTo(uiPosX + 5, 16);
    vLimparBufferPadrao(stdin);
    fgets(szSenha, sizeof (szSenha), stdin);
    strtok(szSenha, "\n");
    
    DEBUG("Senha digitada = [%s]", szSenha);
  }

  // Carregamento em memória do arquivo de usuários.
  {
    if (iCarregarUsuarios(szNomeArquivoUsuarios, &pstUsuarioLista) == FAILURE)
    {
      bExibirTela(TELA_FALHA_SISTEMA);
      bExibirTela(TELA_ESPERA_USUARIO);
      bExibirTela(TELA_MANUTENCAO);
      vTerminal_GoTo(1, 38);
      
      DEBUG("Erro: falha ao carregar o arquivo de usuarios");
      DEBUG("--- Fim");
      exit(EXIT_FAILURE);
    }
  }
  
  // Validação e verificação dos dados digitados.
  {
    if ((strlen(szLogin) + 1) <= (sizeof (pstUsuarioLista->pstUsuario->szLogin)))
    {
      if (iProcurarUsuario(pstUsuarioLista, szLogin, &stUsuario) == SUCCESS)
      {
        bLoginValido = 1;
        if (strcmp(szSenha, stUsuario.szSenha) == 0)
        {
          bSenhaValida = 1;
        }
      }
    }
  
    vLiberarUsuarios(pstUsuarioLista->pstPrimeiro);
    
    if (bLoginValido && bSenhaValida)
    {
      strncpy(pszTipoUsuario,  stUsuario.szTipo,  sizeof (stUsuario.szTipo));
      strncpy(pszNomeUsuario,  stUsuario.szNome,  sizeof (stUsuario.szNome));
      strncpy(pszLoginUsuario, stUsuario.szLogin, sizeof (stUsuario.szLogin));
      
      DEBUG("Usuario VALIDO");
      DEBUG("--- Fim");
      return TRUE;
    }
    else
    {
      bExibirTela(LIMPA_SECAO_II);
      iExibirMensagem(5, 12, "LOGIN DE USUARIOS");      
      
      if (!bLoginValido)
      {
        iExibirMensagem(5, 15, "Login incorreto ou inexistente.");
      }
      else
      {
        iExibirMensagem(5, 15, "Senha incorreta.");
      }

      bExibirTela(TELA_ESPERA_USUARIO);

      DEBUG("Usuario INVALIDO");
      DEBUG("--- Fim");
      return FALSE;
    }
  }
  
}

// Cadastra usuários.
int iCadastrarUsuarios(void)
{
  TIPO_USUARIO stUsuario;
  char         szEntrada    [32];
  int          szTamanhoMsg [4];
  
  memset(&stUsuario,   '\0', sizeof (stUsuario));
  memset(szEntrada,    '\0', sizeof (szEntrada));
  memset(szTamanhoMsg, '\0', sizeof (szTamanhoMsg));
  
  DEBUG("--- Inicio");
  
  bExibirTela(TELA_LOGO);
  bExibirTela(LIMPA_SECAO_II);
  iExibirMensagem(5, 12, "CADASTRO DE USUARIOS");
  szTamanhoMsg[0] = iExibirMensagem(7, 14, "Nome  : []");
  szTamanhoMsg[1] = iExibirMensagem(7, 16, "Tipo  : []");
  szTamanhoMsg[2] = iExibirMensagem(7, 18, "Login : []");
  szTamanhoMsg[3] = iExibirMensagem(7, 20, "Senha : []");

  // Entrada do nome.
  {
    int bEntradaValida = FALSE;
    
    memset(szEntrada,  '\0', sizeof (szEntrada));
    
    iExibirMensagem(5, 14, "*");
    
    do
    {
      bExibirTela(LIMPA_SECAO_III);
      bExibirTela(LIMPA_SECAO_IV);
      iExibirMensagem(5, 31, "Digite o nome:");
      vTerminal_GoTo(5, 35);
      vLimparBufferPadrao(stdin);
      fgets(szEntrada, sizeof (szEntrada), stdin);
      strtok(szEntrada, "\n");
      
      DEBUG("Nome digitado = [%s]", szEntrada);
      
      // Validação da entrada.
      if (strchr(szEntrada, '\n') != NULL)
      {
        memset(szEntrada,  '\0', sizeof (szEntrada));
        vLimparLinha(25);
        iExibirMensagem(5, 25, "\"Nome inválido! O nome não pode ser nulo.\"");
        if (bExibirTela(TELA_RESPOSTA_USUARIO) == FALSE)
        {
          DEBUG("--- Fim");
          return FAILURE;
        }
        vLimparLinha(25);
      }
      else
      {
        bEntradaValida = TRUE;
      }
    } while (bEntradaValida == FALSE);
    
    strncpy(stUsuario.szNome, szEntrada, sizeof (stUsuario.szNome) - 1);
    iExibirMensagem(szTamanhoMsg[0] + 7 - 1, 14, "%s]", stUsuario.szNome);
    iExibirMensagem(5, 14, " ");
  }
 
  // Entrada do tipo de usuário.
  {
    int bEntradaValida = FALSE;
    
    memset(szEntrada, '\0', sizeof (szEntrada));
    
    iExibirMensagem(5, 16, "*");
    
    do
    {
      bExibirTela(LIMPA_SECAO_III);
      bExibirTela(LIMPA_SECAO_IV);
      iExibirMensagem(5, 31, "Digite o tipo de usuário ([1] ADM, [2] FISCAL, [3] OPERADOR):");
      vTerminal_GoTo(5, 35);
      vLimparBufferPadrao(stdin);
      fgets(szEntrada, sizeof (szEntrada), stdin);
      strtok(szEntrada, "\n");

      DEBUG("Tipo de usuario digitado = [%s]", szEntrada);
      
      // Validação da entrada.
      if (atoi(szEntrada) < 1 || atoi(szEntrada) > 3)
      {
        memset(szEntrada, '\0', sizeof (szEntrada));
        vLimparLinha(25);
        iExibirMensagem(5, 25, "\"Tipo de usuário inválido!\"");
        if (bExibirTela(TELA_RESPOSTA_USUARIO) == FALSE)
        {
          DEBUG("--- Fim");
          return FAILURE;
        }
        vLimparLinha(25);
      }
      else
      {
        bEntradaValida = TRUE;
      }
    } while (bEntradaValida == FALSE);
    
    strncpy(stUsuario.szTipo, szEntrada, sizeof (stUsuario.szTipo) - 1);
    iExibirMensagem(szTamanhoMsg[0] + 7 - 1, 16, "%s]", stUsuario.szTipo);
    iExibirMensagem(5, 16, " ");
  }
  
  // Entrada do login.
  {
    TIPO_LISTA_USUARIOS *pstUsuarioLista = NULL;
    TIPO_USUARIO         stTmpUsuario;
    int                  bEntradaValida  = FALSE;
    
    memset(szEntrada,       '\0', sizeof (szEntrada));
    memset(&stTmpUsuario,   '\0', sizeof (stTmpUsuario));
    
    iExibirMensagem(5, 18, "*");
    
    iCarregarUsuarios(szNomeArquivoUsuarios, &pstUsuarioLista);
    
    do
    {
      bExibirTela(LIMPA_SECAO_III);
      bExibirTela(LIMPA_SECAO_IV);
      iExibirMensagem(5, 31, "Digite um login:");
      vTerminal_GoTo(5, 35);
      vLimparBufferPadrao(stdin);
      fgets(szEntrada, sizeof (szEntrada), stdin);
      strtok(szEntrada, "\n");
      
      DEBUG("Login digitado = [%s]", szEntrada);
      
      // Validação da entrada.
      if (strchr(szEntrada, '\n') != NULL || strchr(szEntrada, ' ') != NULL)
      {
        memset(szEntrada, '\0', sizeof (szEntrada));
        vLimparLinha(25);
        iExibirMensagem(5, 25, "\"Login inválido! Não pode ser nulo ou ter espaços em branco.\"");
        if (bExibirTela(TELA_RESPOSTA_USUARIO) == FALSE)
        {
          DEBUG("--- Fim");
          return FAILURE;
        }
        vLimparLinha(25);
      }
      else if (iProcurarUsuario(pstUsuarioLista, szEntrada, &stTmpUsuario) == SUCCESS)
      {
        memset(szEntrada,     '\0', sizeof (szEntrada));
        memset(&stTmpUsuario, '\0', sizeof (stTmpUsuario));
        vLimparLinha(25);
        iExibirMensagem(5, 25, "\"Login existente! Escolha outro...\"");
        if (bExibirTela(TELA_RESPOSTA_USUARIO) == FALSE)
        {
          DEBUG("--- Fim");
          return FAILURE;
        }
        vLimparLinha(25);
      }
      else
      {
        bEntradaValida = TRUE;
      }
    } while (bEntradaValida == FALSE);
    
    vLiberarUsuarios(pstUsuarioLista);
    strncpy(stUsuario.szLogin, szEntrada, sizeof (stUsuario.szLogin) - 1);
    iExibirMensagem(szTamanhoMsg[2] + 7 - 1, 18, "%s]", stUsuario.szLogin);
    iExibirMensagem(5, 18, " ");
  }
    
  // Entrada da senha.
  {
    int bEntradaValida = FALSE;
    
    memset(szEntrada,  '\0', sizeof (szEntrada));
    
    iExibirMensagem(5, 20, "*");
    
    do
    {
      bExibirTela(LIMPA_SECAO_III);
      bExibirTela(LIMPA_SECAO_IV);
      iExibirMensagem(5, 31, "Digite a senha:");
      vTerminal_GoTo(5, 35);
      vLimparBufferPadrao(stdin);
      fgets(szEntrada, sizeof (szEntrada), stdin);
      strtok(szEntrada, "\n");

      DEBUG("Senha digitada = [%s]", szEntrada);
      
      // Validação da entrada.
      if (strchr(szEntrada, '\n') != NULL || strchr(szEntrada, ' ') != NULL)
      {
        memset(szEntrada, '\0', sizeof (szEntrada));
        vLimparLinha(25);
        iExibirMensagem(5, 25, "\"Senha inválida! Não pode ser vazia ou ter espaços em branco.\"");
        if (bExibirTela(TELA_RESPOSTA_USUARIO) == FALSE)
        {
          DEBUG("--- Fim");
          return FAILURE;
        }
        vLimparLinha(25);
      }
      else if ((int) strlen(szEntrada) < 6 || (int) strlen(szEntrada) > 15)
      {
        memset(szEntrada, '\0', sizeof (szEntrada));
        vLimparLinha(25);
        iExibirMensagem(5, 25, "\"Senha inválida! Digite uma senha de 6 até 15 caracteres.\"");
        if (bExibirTela(TELA_RESPOSTA_USUARIO) == FALSE)
        {
          DEBUG("--- Fim");
          return FAILURE;
        }
        vLimparLinha(25);
      }
      else
      {
        bEntradaValida = TRUE;
      }
    } while (bEntradaValida == FALSE);
    
    strncpy(stUsuario.szSenha, szEntrada, sizeof (stUsuario.szSenha) - 1);
    iExibirMensagem(szTamanhoMsg[3] + 7 - 1, 20, "%s]", stUsuario.szSenha);
    iExibirMensagem(5, 20, " ");
  }

  // Confirmação dos dados.
  {
    int bEntradaValida = FALSE;
    
    memset(szEntrada, '\0', sizeof (szEntrada));
    
    do
    {
      bExibirTela(LIMPA_SECAO_III);
      bExibirTela(LIMPA_SECAO_IV);
      iExibirMensagem(5, 31, "Os dados serão escritos em disco. Confirma? (Sim: [ENTER], Não: [ESC]):");
      vTerminal_GoTo(5, 35);
      vLimparBufferPadrao(stdin);
      fgets(szEntrada, sizeof (szEntrada), stdin);
      
      DEBUG("Entrada digitada = [%s]", szEntrada);
      
      switch (szEntrada[0])
      {
        case 10 : // ASCII 10 = ENTER
        {
          bEntradaValida = TRUE;
          
          if (iEscreverUsuario(stUsuario) == FAILURE)
          {
            bExibirTela(LIMPA_SECAO_II);
            iExibirMensagem(5, 12, "CADASTRO DE USUARIOS");
            iExibirMensagem(5, 17, "Falha ao escrever no arquivo de usuários.");
            bExibirTela(TELA_ESPERA_USUARIO);
            
            DEBUG("Erro: falha ao escrever no arquivo de usuarios");
            DEBUG("--- Fim");
            return FAILURE;
          }
          
          bExibirTela(LIMPA_SECAO_II);
          iExibirMensagem(5, 12, "CADASTRO DE USUARIOS");
          iExibirMensagem(5, 17, "Usuário cadastrado com sucesso!");
          bExibirTela(TELA_ESPERA_USUARIO);
          break;
        }
        case 27 : // ASCII 27 = ESC
        {
          if ((int) strlen(szEntrada) == 2)
          {
            bEntradaValida = TRUE;
            bExibirTela(LIMPA_SECAO_II);
            iExibirMensagem(5, 12, "CADASTRO DE USUARIOS");
            iExibirMensagem(5, 17, "Operação não concluída. O arquivo não foi alterado.");
            bExibirTela(TELA_ESPERA_USUARIO);
            break;
          }
        }
        default :
        {
          memset(szEntrada, '\0', sizeof (szEntrada));
          vLimparLinha(25);
          iExibirMensagem(5, 25, "\"Entrada inválida!\"");
          bExibirTela(TELA_ESPERA_USUARIO);
          vLimparLinha(25);
        }
      }
    } while (bEntradaValida == FALSE);
  }
    
  DEBUG("--- Fim");
  return SUCCESS;
}

// Cadastra produtos.
int iCadastrarProdutos(void)
{
  TIPO_SKU stSKU;
  char     szEntrada    [32];
  int      szTamanhoMsg [3];

  memset(&stSKU,       '\0', sizeof (stSKU));
  memset(szEntrada,    '\0', sizeof (szEntrada));
  memset(szTamanhoMsg, '\0', sizeof (szTamanhoMsg));

  DEBUG("--- Inicio");
  
  bExibirTela(TELA_LOGO);
  bExibirTela(LIMPA_SECAO_II);
  iExibirMensagem(5, 12, "CADASTRO DE PRODUTOS");
  szTamanhoMsg[0] = iExibirMensagem(7, 14, "Codigo         : []");
  szTamanhoMsg[1] = iExibirMensagem(7, 16, "Descricao      : []");
  szTamanhoMsg[2] = iExibirMensagem(7, 18, "Valor Unitario : []");

  // Entrada do código.
  {
    TIPO_LISTA_SKUS *pstSKULista    = NULL;
    TIPO_SKU         stTmpSKU;
    int              bEntradaValida = FALSE;
    
    memset(szEntrada, '\0', sizeof (szEntrada));
    memset(&stTmpSKU, '\0', sizeof (stTmpSKU));
    
    iExibirMensagem(5, 14, "*");
    
    iCarregarSKUs(szNomeArquivoSKUs, &pstSKULista);

    do
    {
      bExibirTela(LIMPA_SECAO_III);
      bExibirTela(LIMPA_SECAO_IV);
      iExibirMensagem(5, 31, "Digite o codigo:");
      vTerminal_GoTo(5, 35);
      vLimparBufferPadrao(stdin);
      fgets(szEntrada, sizeof (szEntrada), stdin);
      strtok(szEntrada, "\n");
      
      DEBUG("Codigo digitado = [%s]", szEntrada);
      
      // Validação da entrada.
      if (atoi(szEntrada) <= 0 || strchr(szEntrada, '.' ) != NULL)
      {
        memset(szEntrada, '\0', sizeof (szEntrada));
        vLimparLinha(25);
        iExibirMensagem(5, 25, "\"Código inválido! Deve ser um valor numérico inteiro e maior que zero.\"");
        if (bExibirTela(TELA_RESPOSTA_USUARIO) == FALSE)
        {
          DEBUG("--- Fim");
          return FAILURE;
        }
        vLimparLinha(25);        
      }
      else if (iProcurarSKU(pstSKULista, atol(szEntrada), &stTmpSKU) == SUCCESS)
      {
        memset(szEntrada, '\0', sizeof (szEntrada));
        memset(&stTmpSKU, '\0', sizeof (stTmpSKU));
        vLimparLinha(25);
        iExibirMensagem(5, 25, "\"Código já cadastrado!\"");
        if (bExibirTela(TELA_RESPOSTA_USUARIO) == FALSE)
        {
          DEBUG("--- Fim");
          return FAILURE;
        }
        vLimparLinha(25);
      }
      else
      {
        bEntradaValida = TRUE;
      }
    } while (bEntradaValida == FALSE);
    
    vLiberarSKUs(pstSKULista);
    stSKU.ulCodigo = atol(szEntrada);
    iExibirMensagem(szTamanhoMsg[0] + 7 - 1, 14, "%013lu]", stSKU.ulCodigo);
    iExibirMensagem(5, 14, " ");
  }
  
  // Entrada da descrição.
  {
    int bEntradaValida = FALSE;
    
    memset(szEntrada, '\0', sizeof (szEntrada));
    
    iExibirMensagem(5, 16, "*");
    
    do
    {
      bExibirTela(LIMPA_SECAO_III);
      bExibirTela(LIMPA_SECAO_IV);
      iExibirMensagem(5, 31, "Digite a descrição:");
      vTerminal_GoTo(5, 35);
      vLimparBufferPadrao(stdin);
      fgets(szEntrada, sizeof (szEntrada), stdin);
      strtok(szEntrada, "\n");

      DEBUG("Descricao digitada = [%s]", szEntrada);
      
      // Validação da entrada.
      if (strchr(szEntrada, '\n') != NULL)
      {
        memset(szEntrada, '\0', sizeof (szEntrada));
        vLimparLinha(25);
        iExibirMensagem(5, 25, "\"Descrição inválida! Não pode ser vazia.\"");
        if (bExibirTela(TELA_RESPOSTA_USUARIO) == FALSE)
        {
          DEBUG("--- Fim");
          return FAILURE;
        }
        vLimparLinha(25);
      }
      else if ((int) strlen(szEntrada) > 25)
      {
        memset(szEntrada, '\0', sizeof (szEntrada));
        vLimparLinha(25);
        iExibirMensagem(5, 25, "\"Descrição muito longa!\"");
        if (bExibirTela(TELA_RESPOSTA_USUARIO) == FALSE)
        {
          DEBUG("--- Fim");
          return FAILURE;
        }
        vLimparLinha(25);
      }
      else
      {
        bEntradaValida = TRUE;
      }
    } while (bEntradaValida == FALSE);
    
    // Converte os caracteres para maiúsculo.
    {
      char *pChar = szEntrada;
          
      while (*pChar != '\0')
      {
        *pChar = toupper(*pChar);
        pChar++;
      }
    }
    
    strncpy(stSKU.szDescricao, szEntrada, sizeof (stSKU.szDescricao) - 1);
    iExibirMensagem(szTamanhoMsg[1] + 7 - 1, 16, "%s]", stSKU.szDescricao);
    iExibirMensagem(5, 16, " ");
  }
  
  // Entrada do valor unitário.
  {
    int bEntradaValida = FALSE;
    
    memset(szEntrada, '\0', sizeof (szEntrada));
    
    iExibirMensagem(5, 18, "*");
    
    do
    {
      bExibirTela(LIMPA_SECAO_III);
      bExibirTela(LIMPA_SECAO_IV);
      iExibirMensagem(5, 31, "Digite o valor unitário:");
      vTerminal_GoTo(5, 35);
      vLimparBufferPadrao(stdin);
      fgets(szEntrada, sizeof (szEntrada), stdin);
      strtok(szEntrada, "\n");
      
      DEBUG("Valor unitario digitado = [%s]", szEntrada);
      
      // Validação da entrada.
      if ((float) atof(szEntrada) <= 0.0f)
      {
        memset(szEntrada, '\0', sizeof (szEntrada));
        vLimparLinha(25);
        iExibirMensagem(5, 25, "\"Valor inválido! Deve ser numérico e maior que zero.\"");
        if (bExibirTela(TELA_RESPOSTA_USUARIO) == FALSE)
        {
          DEBUG("--- Fim");
          return FAILURE;
        }
        vLimparLinha(25);
      }
      else if (strchr(szEntrada, ',') != NULL)
      {
        memset(szEntrada, '\0', sizeof (szEntrada));
        vLimparLinha(25);
        iExibirMensagem(5, 25, "\"Valor inválido! Utilize ponto em vez de vírgula.\"");
        if (bExibirTela(TELA_RESPOSTA_USUARIO) == FALSE)
        {
          DEBUG("--- Fim");
          return FAILURE;
        }
        vLimparLinha(25);
      }
      else
      {
        bEntradaValida = TRUE;
      }
    } while (bEntradaValida == FALSE);
    
    stSKU.fValorUnitario = (float) atof(szEntrada);
    iExibirMensagem(szTamanhoMsg[2] + 7 - 1, 18, "%.3f]", stSKU.fValorUnitario);
    iExibirMensagem(5, 18, " ");
  }

  // Confirmação dos dados.
  {
    int bEntradaValida = FALSE;
    
    memset(szEntrada, '\0', sizeof (szEntrada));
    
    do
    {
      bExibirTela(LIMPA_SECAO_III);
      bExibirTela(LIMPA_SECAO_IV);
      iExibirMensagem(5, 31, "Os dados serão escritos em disco. Confirma? (Sim: [ENTER], Não: [ESC]):");
      vTerminal_GoTo(5, 35);
      vLimparBufferPadrao(stdin);
      fgets(szEntrada, sizeof (szEntrada), stdin);
      
      DEBUG("Entrada digitada = [%s]", szEntrada);
      
      switch (szEntrada[0])
      {
        case 10 : // ASCII 10 = ENTER
        {
          bEntradaValida = TRUE;
          
          if (iEscreverSKU(stSKU) == FAILURE)
          {
            bExibirTela(LIMPA_SECAO_II);
            iExibirMensagem(5, 12, "CADASTRO DE PRODUTOS");
            iExibirMensagem(5, 17, "Falha ao escrever no arquivo de SKUs.");
            bExibirTela(TELA_ESPERA_USUARIO);
            
            DEBUG("Erro: falha ao escrever no arquivo de SKUs");
            DEBUG("--- Fim");
            return FAILURE;
          }
          
          bExibirTela(LIMPA_SECAO_II);
          iExibirMensagem(5, 12, "CADASTRO DE PRODUTOS");
          iExibirMensagem(5, 17, "Produto cadastrado com sucesso!");
          bExibirTela(TELA_ESPERA_USUARIO);
          break;
        }
        case 27 : // ASCII 27 = ESC
        {
          if ((int) strlen(szEntrada) == 2)
          {
            bEntradaValida = TRUE;
            bExibirTela(LIMPA_SECAO_II);
            iExibirMensagem(5, 12, "CADASTRO DE PRODUTOS");
            iExibirMensagem(5, 17, "Operação não concluída. O arquivo não foi alterado.");
            bExibirTela(TELA_ESPERA_USUARIO);
            break;
          }
        }
        default :
        {
          memset(szEntrada, '\0', sizeof (szEntrada));
          vLimparLinha(25);
          iExibirMensagem(5, 25, "\"Entrada inválida!\"");
          bExibirTela(TELA_ESPERA_USUARIO);
          vLimparLinha(25);
        }
      }
    } while (bEntradaValida == FALSE);
  }
    
  DEBUG("--- Fim");
  return SUCCESS;
}

// Realiza vendas.
int iRealizarVendas(char *pszLoginUsuario)
{
  TIPO_LISTA_SKUS   *pstSKULista       = NULL;
  TIPO_LISTA_CUPONS *pstCupomLista     = NULL;
  TIPO_SKU           stTmpSKU;
  TIPO_CUPOM         stTmpCupom;
  TIPO_USUARIO       stUsuario;
  unsigned long int  ulUltimoNumCupom  = 0;
  char               szCodigo     [32];
  char               szQuantidade [32];
  char               cEstado           = 'I';
  int                bFimVendaAtual    = FALSE;
  int                bNovaVenda        = FALSE;

  memset(&stTmpSKU,    '\0', sizeof (stTmpSKU));
  memset(&stTmpCupom,  '\0', sizeof (stTmpCupom));
  memset(&stUsuario,   '\0', sizeof (stUsuario));
  memset(szCodigo,     '\0', sizeof (szCodigo));
  memset(szQuantidade, '\0', sizeof (szQuantidade));

  DEBUG("--- Inicio");
  
  // Carregamento do arquivo de usuários e inicialização de variável do usuário atual.
  {
    TIPO_LISTA_USUARIOS *pstUsuarioLista = NULL;
    
    if (pszLoginUsuario == NULL)
    {
      bExibirTela(TELA_FALHA_SISTEMA);
      bExibirTela(TELA_ESPERA_USUARIO);
      bExibirTela(TELA_MANUTENCAO);
      vTerminal_GoTo(1, 38);
      
      DEBUG("Erro: parametro invalido = [NULL]");
      DEBUG("--- Fim");
      exit(EXIT_FAILURE);
    }
    
    if (iCarregarUsuarios(szNomeArquivoUsuarios, &pstUsuarioLista) == FAILURE)
    {
      bExibirTela(TELA_FALHA_SISTEMA);
      bExibirTela(TELA_ESPERA_USUARIO);
      bExibirTela(TELA_MANUTENCAO);
      vTerminal_GoTo(1, 38);
      
      DEBUG("Erro: falha ao carregar o arquivo de usuarios");
      DEBUG("--- Fim");
      exit(EXIT_FAILURE);
    }
    
    if (iProcurarUsuario(pstUsuarioLista, pszLoginUsuario, &stUsuario) == FAILURE)
    {
      vLiberarUsuarios(pstUsuarioLista->pstPrimeiro);
      bExibirTela(TELA_FALHA_SISTEMA);
      bExibirTela(TELA_ESPERA_USUARIO);
      bExibirTela(TELA_MANUTENCAO);
      vTerminal_GoTo(1, 38);
      
      DEBUG("Erro: usuario inconsistente durante a venda");
      DEBUG("--- Fim");
      exit(EXIT_FAILURE);
    }
    
    vLiberarUsuarios(pstUsuarioLista->pstPrimeiro);
  }
  
  // Carregamento do arquivo de SKUs.
  {
    if (iCarregarSKUs(szNomeArquivoSKUs, &pstSKULista) == FAILURE)
    {
      bExibirTela(LIMPA_SECAO_II);
      iExibirMensagem(5, 12, "ERRO");
      iExibirMensagem(5, 16, "Não há produtos cadastrados.");
      iExibirMensagem(5, 19, "Solicite a assistência do fiscal.");
      bExibirTela(TELA_ESPERA_USUARIO);
      
      DEBUG("Erro: falha ao carregar o arquivo de SKUs");
      DEBUG("--- Fim");
      return FAILURE;
    }
  }
  
  // Obtenção do último número de cupom.
  {
    if (iCarregarCupons(szNomeArquivoCupons, &pstCupomLista) == SUCCESS)
    {
      ulUltimoNumCupom = (*pstCupomLista->ppstUltimo)->pstCupom->ulNumeroCupom;
      vLiberarCupons(pstCupomLista->pstPrimeiro);
    }
  }
  
  // Laço principal de vendas.
  do
  {
    bNovaVenda     = FALSE;
    bFimVendaAtual = FALSE;
    
    // Configuração de novo cupom.
    {
      memset(&stTmpCupom, '\0', sizeof (stTmpCupom));
      
      // Data e hora.
      {
        TIPO_DATA_HORA stDataHora;
        memset(&stDataHora, '\0', sizeof (stDataHora));
        
        vObterDataHora(&stDataHora);
        
        strcat(stTmpCupom.szDataHora, stDataHora.szDia);
        strcat(stTmpCupom.szDataHora, stDataHora.szMes);
        strcat(stTmpCupom.szDataHora, stDataHora.szAno);
        strcat(stTmpCupom.szDataHora, stDataHora.szHora);
        strcat(stTmpCupom.szDataHora, stDataHora.szMin);
        strcat(stTmpCupom.szDataHora, stDataHora.szSeg);
      }
      
      // Número do cupom.
      {
        stTmpCupom.ulNumeroCupom = ++ulUltimoNumCupom;
      }
      
      // Operador.
      {
        memcpy(&stTmpCupom.stOperador, &stUsuario, sizeof (stUsuario));
      }
    }
    
    // Venda atual.
    do
    {
      bExibirTela(LIMPA_SECOES);
      bExibirTela(TELA_LOGO);
      
      // Cabeçalho personalizado.
      {
        unsigned int   uiPosX     = 5;
        TIPO_DATA_HORA stDataHora;
      
        memset(&stDataHora, '\0', sizeof (stDataHora));
        
        vObterDataHora(&stDataHora);        
        iExibirMensagem(65, 3, "%s/%s/%s  %sh%s", 
                        stDataHora.szDia,
                        stDataHora.szMes,
                        stDataHora.szAno,
                        stDataHora.szHora,
                        stDataHora.szMin);
        
        vLimparLinha(5);
        uiPosX += iExibirMensagem(uiPosX, 5, "Operador(a): ");
        iExibirMensagem(uiPosX, 5, stUsuario.szNome);
        
        vLimparLinha(6);
        iExibirMensagem(65, 6, "Cupom nº: %07lu", stTmpCupom.ulNumeroCupom);
      }
          
      // Identificação de itens [I].
      {
        if (cEstado == 'I')
        {
          TIPO_ITEM    stTmpItem;
          unsigned int uiPosY            = 0;
          int          bCodigoValido     = FALSE;
          int          bQuantidadeValida = FALSE;
          
          memset(&stTmpItem, '\0', sizeof (stTmpItem));

          // Atualização da exibição (anterior) dos itens.
          {
            bExibirTela(LIMPA_SECAO_II);
            uiPosY        = 12;
            iExibirMensagem(5, uiPosY,      "IDENTIFICACAO DE ITENS");
            iExibirMensagem(5, uiPosY += 2, " # |   CODIGO    |        DESCRICAO        |  QTDE  | VL. UNIT.|  VL. ITEM  ");
            
            int iMax      = (int) (stTmpCupom.uiTotalItens < 10 ? stTmpCupom.uiTotalItens : 9);
            int iContador = 0;
            int iIndice   = (int) (stTmpCupom.uiTotalItens - (unsigned int) iMax);

            for ( ; iContador < iMax; iContador++, iIndice++)
            {
              iExibirMensagem(5, ++uiPosY, "%03u|%013lu|%25.25s|%8.3f|R$ %7.2f|R$ %9.2lf",
                                          stTmpCupom.astItem[iIndice].uiSequenciaItem,
                                          stTmpCupom.astItem[iIndice].stSKUItem.ulCodigo,
                                          stTmpCupom.astItem[iIndice].stSKUItem.szDescricao,
                                          stTmpCupom.astItem[iIndice].fQuantidade,
                                          stTmpCupom.astItem[iIndice].stSKUItem.fValorUnitario,
                                          stTmpCupom.astItem[iIndice].dValorItem);
            }
          }
          
          // Entrada do código do produto.
          {
            bExibirTela(LIMPA_SECAO_III);
            bExibirTela(LIMPA_SECAO_IV);
            iExibirMensagem(5, 31, "Digite o código do produto:");
            vTerminal_GoTo(5, 35);
            vLimparBufferPadrao(stdin);
            memset(szCodigo, '\0', sizeof (szCodigo));
            fgets(szCodigo, sizeof (szCodigo), stdin);
            strtok(szCodigo, "\n");
            
            DEBUG("Codigo digitado = [%s]", szCodigo);
          }
          
          // Validação do código.
          {
            memset(&stTmpSKU, '\0', sizeof (stTmpSKU));
            
            if (atoi(szCodigo) <= 0 || strchr(szCodigo, '.') != NULL)
            {
              vLimparLinha(25);
              iExibirMensagem(5, 25, "\"Código inválido!\"");
              bExibirTela(TELA_ESPERA_USUARIO);
              vLimparLinha(25);
            }
            else if (iProcurarSKU(pstSKULista, atol(szCodigo), &stTmpSKU) == FAILURE)
            {
              vLimparLinha(25);
              iExibirMensagem(5, 25, "\"Produto não cadastrado!\"");
              bExibirTela(TELA_ESPERA_USUARIO);
              vLimparLinha(25);
            }
            else
            {
              bCodigoValido = TRUE;
            }
          }
          
          // Entrada da quantidade.
          {
            if (bCodigoValido)
            {
              bExibirTela(LIMPA_SECAO_III);
              bExibirTela(LIMPA_SECAO_IV);
              iExibirMensagem(5, 31, "Digite a quantidade ou ENTER (para uma unidade):");
              vTerminal_GoTo(5, 35);
              vLimparBufferPadrao(stdin);
              memset(szQuantidade, '\0', sizeof (szQuantidade));
              fgets(szQuantidade, sizeof (szQuantidade), stdin);
              strtok(szQuantidade, "\n");
              
              DEBUG("Quantidade digitada = [%s]", szQuantidade);
            }
          }
          
          // Validação da quantidade.
          {
            if (bCodigoValido)
            {
              if (strcmp(szQuantidade, "\n") == 0)
              {
                sprintf(szQuantidade, "1");
                bQuantidadeValida = TRUE;
              }
              else if ((float) atof(szQuantidade) <= 0.0f)
              {
                vLimparLinha(25);
                iExibirMensagem(5, 25, "\"Quantidade inválida!\"");
                bExibirTela(TELA_ESPERA_USUARIO);
                vLimparLinha(25);
              }
              else
              {
                bQuantidadeValida = TRUE;
              }
            }
          }
          
          // Inserção de item no cupom.
          {
            if (bCodigoValido && bQuantidadeValida)
            {
              stTmpItem.uiSequenciaItem          = ++stTmpCupom.uiTotalItens;
              stTmpItem.stSKUItem.ulCodigo       = (unsigned long) atol(szCodigo);
              strncpy(stTmpItem.stSKUItem.szDescricao, stTmpSKU.szDescricao, sizeof (stTmpSKU.szDescricao));
              stTmpItem.stSKUItem.fValorUnitario = stTmpSKU.fValorUnitario;
              stTmpItem.fQuantidade              = (float) atof(szQuantidade);
              stTmpItem.dValorItem               = (double) (stTmpItem.stSKUItem.fValorUnitario * stTmpItem.fQuantidade);
              memcpy(&stTmpCupom.astItem[stTmpCupom.uiTotalItens - 1], &stTmpItem, sizeof (stTmpItem));
              stTmpCupom.dValorCupom            += stTmpItem.dValorItem;
            }
          }
          
          // Atualização da exibição (posterior) dos itens.
          {
            bExibirTela(LIMPA_SECAO_II);
            uiPosY        = 12;
            iExibirMensagem(5, uiPosY,      "IDENTIFICACAO DE ITENS");
            iExibirMensagem(5, uiPosY += 2, " # |   CODIGO    |        DESCRICAO        |  QTDE  | VL. UNIT.|  VL. ITEM  ");
            
            int iMax      = (int) (stTmpCupom.uiTotalItens < 10 ? stTmpCupom.uiTotalItens : 9);
            int iContador = 0;
            int iIndice   = (int) (stTmpCupom.uiTotalItens - (unsigned int) iMax);

            for ( ; iContador < iMax; iContador++, iIndice++)
            {
              iExibirMensagem(5, ++uiPosY, "%03u|%013lu|%25.25s|%8.3f|R$ %7.2f|R$ %9.2lf",
                                          stTmpCupom.astItem[iIndice].uiSequenciaItem,
                                          stTmpCupom.astItem[iIndice].stSKUItem.ulCodigo,
                                          stTmpCupom.astItem[iIndice].stSKUItem.szDescricao,
                                          stTmpCupom.astItem[iIndice].fQuantidade,
                                          stTmpCupom.astItem[iIndice].stSKUItem.fValorUnitario,
                                          stTmpCupom.astItem[iIndice].dValorItem);
            }
          }
        }
      }          
      
      // Subtotal [S].
      {
        if (cEstado == 'S')
        {
          iExibirMensagem(5, 12, "SUBTOTAL");
          iExibirMensagem(5, 15, "Subtotal: R$ %7.2f", stTmpCupom.dValorCupom);
        }
      }
      
      // Pagamento [P].
      {
        if (cEstado == 'P')
        {
          char   szPagamento [32];
          double dPagamento       = 0;
          int    bEntradaValida   = FALSE;
          
          memset(szPagamento, '\0', sizeof (szPagamento));

          iExibirMensagem(5, 12, "PAGAMENTO");
          iExibirMensagem(5, 15, "Subtotal: R$ %7.2lf", stTmpCupom.dValorCupom);
          
          while (bEntradaValida == FALSE)
          {
            bExibirTela(LIMPA_SECAO_III);
            bExibirTela(LIMPA_SECAO_IV);
            iExibirMensagem(5, 31, "Digite o valor de pagamento:");
            vTerminal_GoTo(5, 35);
            
            memset(szPagamento, '\0', sizeof (szPagamento));
            vLimparBufferPadrao(stdin);
            fgets(szPagamento, sizeof (szPagamento), stdin);
            strtok(szPagamento, "\n");
            
            if (atof(szPagamento) <= 0)
            {
              vLimparLinha(25);
              iExibirMensagem(5, 25, "\"Valor inválido!\"");
              if (bExibirTela(TELA_RESPOSTA_USUARIO) == FALSE)
              {
                vLimparLinha(25);
                iExibirMensagem(5, 25, "\"Atenção: a venda será cancelada! Confirma?\"");
                if (bExibirTela(TELA_RESPOSTA_USUARIO) == TRUE)
                {
                  vLimparLinha(25);
                  bExibirTela(LIMPA_SECAO_II);
                  bExibirTela(LIMPA_SECAO_III);
                  bExibirTela(LIMPA_SECAO_IV);
                  iExibirMensagem(5, 12, "PAGAMENTO");
                  iExibirMensagem(5, 15, "Venda cancelada!");                  

                  bEntradaValida = TRUE;
                  bFimVendaAtual = TRUE;
                  ulUltimoNumCupom--;
                  
                  DEBUG("Venda cancelada pelo operador");
                  
                  // Pergunta-se por nova venda.
                  {
                    iExibirMensagem(5, 25, "Deseja realizar nova venda?");
                    
                    if (bExibirTela(TELA_RESPOSTA_USUARIO) == TRUE)
                    {
                      bNovaVenda = TRUE;
                    }
                    else
                    {
                      bNovaVenda = FALSE;
                    }
                    vLimparLinha(25);
                  }
                }
                vLimparLinha(25);
              }
              vLimparLinha(25);
            }
            else
            {
              double dTroco = 0;

              dPagamento += atof(szPagamento);
              dTroco = dPagamento - stTmpCupom.dValorCupom;
              iExibirMensagem(5, 17, "Dinheiro: R$ %7.2lf", dPagamento);
              
              if (dTroco >= 0)
              {
                bEntradaValida = TRUE;
                iExibirMensagem(5, 19, "Troco:    R$ %7.2lf", dTroco);
                if (iEscreverCupom(stTmpCupom) != SUCCESS)
                {
                  bExibirTela(TELA_FALHA_SISTEMA);
                  bExibirTela(TELA_ESPERA_USUARIO);
                  bExibirTela(TELA_MANUTENCAO);
                  vTerminal_GoTo(1, 38);
                  
                  DEBUG("Erro: falhar ao escrever no arquivo de cupons");
                  DEBUG("--- Fim");
                  exit(EXIT_FAILURE);
                }
                
                vLimparLinha(25);
                iExibirMensagem(5, 25, "Venda realizada com sucesso!");
                bExibirTela(TELA_ESPERA_USUARIO);
                vLimparLinha(25);
                
              }
            }
          }
          
          bExibirTela(LIMPA_SECAO_III);
          bExibirTela(LIMPA_SECAO_IV);
        }                  
      }
      
      // Transição de estados de venda.
      {
        int  bEntradaValida = FALSE;
        char szEntrada [32];
        
        memset(szEntrada, '\0', sizeof (szEntrada));
        
        if (!bFimVendaAtual || bNovaVenda)
        {
          do
          {
            bExibirTela(LIMPA_SECAO_III);
            bExibirTela(LIMPA_SECAO_IV);
            
            switch (cEstado)
            {
              case 'I' :  // Identificação de itens.
              { 
                iExibirMensagem(5, 31, "Digite 'S' para subtotal, ESC para sair ou ENTER para continuar...");
                vTerminal_GoTo(5, 35);
                vLimparBufferPadrao(stdin);
                fgets(szEntrada, sizeof (szEntrada), stdin);
                strtok(szEntrada, "\n");
                
                switch (szEntrada[0])
                {            
                  case 10 : // ASCII 10 = ENTER
                  {
                    if ((int) strlen(szEntrada) == 1)
                    {
                      bEntradaValida = TRUE;
                      break;
                    }
                  }
                  case 27 : // ASCII 27 = ESC
                  {
                    if ((int) strlen(szEntrada) == 1)
                    {
                      vLimparLinha(25);
                      iExibirMensagem(5, 25, "Atenção: a venda será cancelada!");
                      if (bExibirTela(TELA_RESPOSTA_USUARIO) == TRUE)
                      {
                        vLimparLinha(25);
                        bExibirTela(LIMPA_SECAO_II);
                        bExibirTela(LIMPA_SECAO_III);
                        bExibirTela(LIMPA_SECAO_IV);
                        
                        iExibirMensagem(5, 12, "IDENTIFICACAO DE ITENS");
                        iExibirMensagem(5, 15, "Venda cancelada!");
                        
                        bEntradaValida = TRUE;
                        bFimVendaAtual = TRUE;
                        ulUltimoNumCupom--;
                        
                        DEBUG("Venda cancelada pelo operador");
                        
                        // Pergunta-se por nova venda.
                        {
                          iExibirMensagem(5, 25, "Deseja realizar nova venda?");
                          if (bExibirTela(TELA_RESPOSTA_USUARIO) == TRUE)
                          {
                            bNovaVenda = TRUE;
                          }
                          else
                          {
                            bNovaVenda = FALSE;
                          }
                          vLimparLinha(25);
                        }
                      }                    
                      vLimparLinha(25);
                      break;
                    }
                  }
                  case 's' :
                  case 'S' :
                  {
                    if ((int) strlen(szEntrada) == 1)
                    {
                      bEntradaValida = TRUE;
                      cEstado        = 'S';
                      break;
                    }
                  }
                  default :
                  {
                    memset(szEntrada, '\0', sizeof (szEntrada));
                    vLimparLinha(25);
                    iExibirMensagem(5, 25, "\"Entrada inválida!\"");
                    bExibirTela(TELA_ESPERA_USUARIO);
                    vLimparLinha(25);
                  }
                }

                break;
              }
              case 'S' :  // Subtotal.
              {          
                iExibirMensagem(5, 31, "Digite 'P' para pagamento, ESC para sair ou 'I' para entrar mais itens...");
                vTerminal_GoTo(5, 35);
                vLimparBufferPadrao(stdin);
                fgets(szEntrada, sizeof (szEntrada), stdin);
                strtok(szEntrada, "\n");
                
                switch (szEntrada[0])
                {              
                  case 27 : // ASCII 27 = ESC
                  {
                    if ((int) strlen(szEntrada) == 1)
                    {
                      vLimparLinha(25);
                      iExibirMensagem(5, 25, "Atenção: a venda será cancelada!");
                      if (bExibirTela(TELA_RESPOSTA_USUARIO) == TRUE)
                      {
                        vLimparLinha(25);
                        bExibirTela(LIMPA_SECAO_II);
                        bExibirTela(LIMPA_SECAO_III);
                        bExibirTela(LIMPA_SECAO_IV);

                        iExibirMensagem(5, 12, "SUBTOTAL");
                        iExibirMensagem(5, 15, "Venda cancelada!");
                        
                        bEntradaValida = TRUE;
                        bFimVendaAtual = TRUE;
                        ulUltimoNumCupom--;
                        
                        DEBUG("Venda cancelada pelo operador");
                        
                        // Pergunta-se por nova venda.
                        {
                          iExibirMensagem(5, 25, "Deseja realizar nova venda?");
                          if (bExibirTela(TELA_RESPOSTA_USUARIO) == TRUE)
                          {
                            bNovaVenda = TRUE;
                            cEstado    = 'I';
                          }
                          else
                          {
                            bNovaVenda = FALSE;
                          }
                          vLimparLinha(25);
                        }                                            
                      }
                      vLimparLinha(25);
                      break;
                    }
                  }
                  case 'i' :
                  case 'I' :
                  {
                    if ((int) strlen(szEntrada) == 1)
                    {
                      bEntradaValida = TRUE;
                      cEstado        = 'I';
                      break;
                    }
                  }
                  case 'p' :
                  case 'P' :
                  {
                    if ((int) strlen(szEntrada) == 1)
                    {
                      bEntradaValida = TRUE;
                      cEstado        = 'P';
                      break;
                    }
                  }
                  default :
                  {
                    memset(szEntrada, '\0', sizeof (szEntrada));
                    vLimparLinha(25);
                    iExibirMensagem(5, 25, "\"Entrada inválida!\"");
                    bExibirTela(TELA_ESPERA_USUARIO);
                    vLimparLinha(25);
                  }
                }              
                
                break;
              }
              case 'P' :  // Pagamento.
              {
                bEntradaValida = TRUE;
                bFimVendaAtual = TRUE;
                bNovaVenda     = TRUE;
                cEstado        = 'I';
                break;
              }
              default :
              {
                vLiberarSKUs(pstSKULista->pstPrimeiro);
                bExibirTela(TELA_FALHA_SISTEMA);
                bExibirTela(TELA_ESPERA_USUARIO);
                bExibirTela(TELA_MANUTENCAO);
                vTerminal_GoTo(1, 38);
                
                DEBUG("Erro: estado de venda inconsistente (ASCII=[%d])", cEstado);
                DEBUG("--- Fim");
                exit(EXIT_FAILURE);
              }        
            }
            
            bExibirTela(LIMPA_SECAO_III);
            bExibirTela(LIMPA_SECAO_IV);
          } while (bEntradaValida == FALSE);
        }
      }
    } while (!bFimVendaAtual);
  } while (bNovaVenda);
  
  // Liberação da memória de dados carregados de SKUS.
  {
    vLiberarSKUs(pstSKULista->pstPrimeiro);
  }
    
  DEBUG("--- Fim");
  return SUCCESS;
}

// Abre o arquivo de usuários e carrega os dados em memória.
int iCarregarUsuarios(const char *pszNomeArquivoUsuarios, TIPO_LISTA_USUARIOS **ppstUsuarioLista)
{
  FILE                 *pfArquivo               =  NULL;
  TIPO_LISTA_USUARIOS  *pstUsuarioListaAnterior = *ppstUsuarioLista = NULL;
  TIPO_LISTA_USUARIOS  *pstUsuarioListaPrimeiro =  NULL;
  TIPO_LISTA_USUARIOS **ppstUsuarioListaUltimo  =  NULL;
  char                  szDelimitadores []      =  "|\n";
  char                  cCaractere              =  '\0';
  long int              lOffsetInicio           =  0;

  DEBUG("--- Inicio");
  
  // Abre-se o arquivo.
  {
    if ((pfArquivo = fopen(pszNomeArquivoUsuarios, "r")) == NULL)
    {
      DEBUG("Erro: falha ao abrir o arquivo %s", szNomeArquivoUsuarios);
      DEBUG("--- Fim");
      return FAILURE;
    }
  }
  
  // Lê-se o arquivo, ajustando-o para a posição inicial dos dados.
  {
    DEBUG("Lendo o arquivo %s..", szNomeArquivoUsuarios);
    
    // Busca-se a posição inicial.
    {
      lOffsetInicio = ftell(pfArquivo);
      while ((cCaractere = fgetc(pfArquivo)) != EOF)
      {
        if (strchr(szDelimitadores, (int) cCaractere) == NULL)
        {
          // Não é um delimitador. Logo saímos deste laço.
          break;
        }
        DEBUG("Caractere ignorado (ASCII=[%d])", cCaractere);
        lOffsetInicio = ftell(pfArquivo);
      }
    }
    
    // Ajusta-se a nova posição.
    {
      if (feof(pfArquivo))
      {
        // Se o fim do arquivo foi alcançado...
        fclose(pfArquivo);
        pfArquivo = NULL;
        DEBUG("Erro: o arquivo nao possui dados validos");
        DEBUG("--- Fim");
        return FAILURE;
      }
      else
      {
        // Caso haja dados válidos no arquivo, a nova posição inicial é definida.
        fseek(pfArquivo, lOffsetInicio, SEEK_SET);
      }
    }
  }
  
  // Aloca-se memória para o ponteiro para o último item da lista.
  {
    if ((ppstUsuarioListaUltimo = calloc(1, sizeof (TIPO_LISTA_USUARIOS*))) == NULL)
    {
      fclose(pfArquivo);
      pfArquivo = NULL;
      DEBUG("Erro:falha ao alocar memoria");
      DEBUG("--- Fim");
      return FAILURE;
    }
    DEBUG("Ponteiro para o ponteiro do ultimo item = [%p]", ppstUsuarioListaUltimo);
  }
  
  // Lê-se o arquivo e inicializa-o em memória.
  {
    while (!feof(pfArquivo))
    {
      pstUsuarioListaAnterior = *ppstUsuarioLista;
      
      // Aloca-se memória para um item da lista.
      {
        if ((*ppstUsuarioLista = malloc(sizeof (TIPO_LISTA_USUARIOS))) == NULL)
        {
          if (pstUsuarioListaAnterior != NULL)
          {
            vLiberarUsuarios(pstUsuarioListaAnterior->pstPrimeiro);
          }
          fclose(pfArquivo);
          pfArquivo = NULL;
          DEBUG("Erro: falha ao alocar memoria");
          DEBUG("--- Fim");
          return FAILURE;
        }
        
        DEBUG("Novo usuario encontrado (ponteiro-lista=[%p])", *ppstUsuarioLista);
        
        if (pstUsuarioListaAnterior != NULL)
        {
          pstUsuarioListaAnterior->pstProximo = *ppstUsuarioLista;
        }

        (*ppstUsuarioLista)->pstAnterior = pstUsuarioListaAnterior;
        (*ppstUsuarioLista)->pstProximo  = NULL;

        if (pstUsuarioListaAnterior == NULL)
        {
          pstUsuarioListaPrimeiro = *ppstUsuarioLista;
        }

        *ppstUsuarioListaUltimo          = *ppstUsuarioLista;
        (*ppstUsuarioLista)->pstPrimeiro = pstUsuarioListaPrimeiro;
        (*ppstUsuarioLista)->ppstUltimo  = ppstUsuarioListaUltimo;

        if (((*ppstUsuarioLista)->pstUsuario = calloc(1, sizeof (TIPO_USUARIO))) == NULL)
        {
          fclose(pfArquivo);
          pfArquivo = NULL;
          vLiberarUsuarios((*ppstUsuarioLista)->pstPrimeiro);
          DEBUG("Erro:falha ao alocar memoria");
          DEBUG("--- Fim");
          return FAILURE;
        }
        
        DEBUG("Novo usuario encontrado (ponteiro-item=[%p])", (*ppstUsuarioLista)->pstUsuario);
      }

      // Inicializa-se o item da lista.
      {
        // Tipo de usuário.
        {
          iExtrairTokenArquivo(pfArquivo, szDelimitadores, sizeof ((*ppstUsuarioLista)->pstUsuario->szTipo) - 1, 
                              (*ppstUsuarioLista)->pstUsuario->szTipo);
          DEBUG("Usuario tipo extraido = [%s]", (*ppstUsuarioLista)->pstUsuario->szTipo);
        }
        
        // Login do usuário.
        {
          iExtrairTokenArquivo(pfArquivo, szDelimitadores, sizeof ((*ppstUsuarioLista)->pstUsuario->szLogin) - 1, 
                              (*ppstUsuarioLista)->pstUsuario->szLogin);
          DEBUG("Usuario login extraido = [%s]", (*ppstUsuarioLista)->pstUsuario->szLogin);
        }
        
        // Senha do usuário.
        {
          iExtrairTokenArquivo(pfArquivo, szDelimitadores, sizeof ((*ppstUsuarioLista)->pstUsuario->szSenha) - 1, 
                              (*ppstUsuarioLista)->pstUsuario->szSenha);
          DEBUG("Usuario senha extraido = [%s]", (*ppstUsuarioLista)->pstUsuario->szSenha);
        }
        
        // Nome do usuário.
        {
          iExtrairTokenArquivo(pfArquivo, szDelimitadores, sizeof ((*ppstUsuarioLista)->pstUsuario->szNome) - 1, 
                              (*ppstUsuarioLista)->pstUsuario->szNome);
          DEBUG("Usuario nome extraido = [%s]", (*ppstUsuarioLista)->pstUsuario->szNome);
        }
      }

      // Ajusta-se a posição para o início do próximo item.
      {
        // Busca-se a posição inicial.
        {
          lOffsetInicio = ftell(pfArquivo);
          while ((cCaractere = fgetc(pfArquivo)) != EOF)
          {
            if (strchr(szDelimitadores, (int) cCaractere) == NULL)
            {
              // Não é um delimitador. Logo saímos deste laço.
              break;
            }
            DEBUG("Caractere ignorado (ASCII=[%d])", cCaractere);
            lOffsetInicio = ftell(pfArquivo);
          }
        }
        
        // Ajusta-se a nova posição.
        {
          if (!feof(pfArquivo))
          {            
            fseek(pfArquivo, lOffsetInicio, SEEK_SET);
          }
        }
      }
    }
    
    *ppstUsuarioLista = (*ppstUsuarioLista)->pstPrimeiro;
    DEBUG("Ponteiro para o primeiro item = [%p]", *ppstUsuarioLista);
  }
  
  // Fecha-se o arquivo.
  {
    fclose(pfArquivo);
    pfArquivo = NULL;
  }
  
  DEBUG("--- Fim");
  return SUCCESS;
}

// Abre o arquivo de SKUs e carrega os dados em memória.
int iCarregarSKUs(const char *pszNomeArquivoSKUs, TIPO_LISTA_SKUS **ppstSKULista)
{
  FILE             *pfArquivo           =  NULL;
  TIPO_LISTA_SKUS  *pstSKUListaAnterior = *ppstSKULista = NULL;
  TIPO_LISTA_SKUS  *pstSKUListaPrimeiro =  NULL;
  TIPO_LISTA_SKUS **ppstSKUListaUltimo  =  NULL;
  char              szDelimitadores []  =  "|\n";
  char              cCaractere          =  '\0';
  long int          lOffsetInicio       =  0;

  DEBUG("--- Inicio");
  
  // Abre-se o arquivo.
  {
    if ((pfArquivo = fopen(pszNomeArquivoSKUs, "r")) == NULL)
    {
      DEBUG("Erro: falha ao abrir o arquivo %s", szNomeArquivoSKUs);
      DEBUG("--- Fim");
      return FAILURE;
    }
  }
  
  // Lê-se o arquivo, ajustando-o para a posição inicial dos dados.
  {
    DEBUG("Lendo o arquivo %s..", szNomeArquivoSKUs);
    
    // Busca-se a posição inicial.
    {
      lOffsetInicio = ftell(pfArquivo);
      while ((cCaractere = fgetc(pfArquivo)) != EOF)
      {
        if (strchr(szDelimitadores, (int) cCaractere) == NULL)
        {
          // Não é um delimitador. Logo saímos deste laço.
          break;
        }
        DEBUG("Caractere ignorado (ASCII=[%d])", cCaractere);
        lOffsetInicio = ftell(pfArquivo);
      }
    }
    
    // Ajusta-se a nova posição.
    {
      if (feof(pfArquivo))
      {
        // Se o fim do arquivo foi alcançado...
        fclose(pfArquivo);
        pfArquivo = NULL;
        DEBUG("Erro: o arquivo nao possui dados validos");
        DEBUG("--- Fim");
        return FAILURE;
      }
      else
      {
        // Caso haja dados válidos no arquivo, a nova posição inicial é definida.
        fseek(pfArquivo, lOffsetInicio, SEEK_SET);
      }
    }
  }
  
  // Aloca-se memória para o ponteiro para o último item da lista.
  {
    if ((ppstSKUListaUltimo = calloc(1, sizeof (TIPO_LISTA_SKUS*))) == NULL)
    {
      fclose(pfArquivo);
      pfArquivo = NULL;
      DEBUG("Erro:falha ao alocar memoria");
      DEBUG("--- Fim");
      return FAILURE;
    }
    DEBUG("Ponteiro para o ponteiro do ultimo item = [%p]", ppstSKUListaUltimo);
  }
  
  // Lê-se o arquivo e inicializa-o em memória.
  {
    while (!feof(pfArquivo))
    {
      pstSKUListaAnterior = *ppstSKULista;
      
      // Aloca-se memória para um item da lista.
      {
        if ((*ppstSKULista = malloc(sizeof (TIPO_LISTA_SKUS))) == NULL)
        {
          if (pstSKUListaAnterior != NULL)
          {
            vLiberarSKUs(pstSKUListaAnterior->pstPrimeiro);
          }
          fclose(pfArquivo);
          pfArquivo = NULL;
          DEBUG("Erro: falha ao alocar memoria");
          DEBUG("--- Fim");
          return FAILURE;
        }
        
        DEBUG("Novo SKU encontrado (ponteiro-lista=[%p])", *ppstSKULista);
        
        if (pstSKUListaAnterior != NULL)
        {
          pstSKUListaAnterior->pstProximo = *ppstSKULista;
        }

        (*ppstSKULista)->pstAnterior = pstSKUListaAnterior;
        (*ppstSKULista)->pstProximo  = NULL;

        if (pstSKUListaAnterior == NULL)
        {
          pstSKUListaPrimeiro = *ppstSKULista;
        }

        *ppstSKUListaUltimo          = *ppstSKULista;
        (*ppstSKULista)->pstPrimeiro = pstSKUListaPrimeiro;
        (*ppstSKULista)->ppstUltimo  = ppstSKUListaUltimo;

        if (((*ppstSKULista)->pstSKU = calloc(1, sizeof (TIPO_SKU))) == NULL)
        {
          fclose(pfArquivo);
          pfArquivo = NULL;
          vLiberarSKUs((*ppstSKULista)->pstPrimeiro);
          DEBUG("Erro: falha ao alocar memoria");
          DEBUG("--- Fim");
          return FAILURE;
        }
        
        DEBUG("Novo SKU encontrado (ponteiro-item=[%p])", (*ppstSKULista)->pstSKU);
      }

      // Inicializa-se o item da lista.
      {
        char szTmpToken [64];
        
        // Código de identificação.
        {
          memset(szTmpToken, '\0', sizeof (szTmpToken));
          iExtrairTokenArquivo(pfArquivo, szDelimitadores, sizeof (szTmpToken) - 1, 
                              szTmpToken);
          (*ppstSKULista)->pstSKU->ulCodigo = (unsigned long int) atol(szTmpToken);
          DEBUG("SKU codigo extraido = [%lu]", (*ppstSKULista)->pstSKU->ulCodigo);
        }
        
        // Descrição da mercadoria.
        {
          iExtrairTokenArquivo(pfArquivo, szDelimitadores, sizeof ((*ppstSKULista)->pstSKU->szDescricao) - 1, 
                              (*ppstSKULista)->pstSKU->szDescricao);
          DEBUG("SKU descricao extraido = [%s]", (*ppstSKULista)->pstSKU->szDescricao);
        }
        
        // Valor unitário.
        {
          memset(szTmpToken, '\0', sizeof (szTmpToken));
          iExtrairTokenArquivo(pfArquivo, szDelimitadores, sizeof (szTmpToken) - 1, 
                              szTmpToken);
          (*ppstSKULista)->pstSKU->fValorUnitario = (float) atof(szTmpToken);
          DEBUG("SKU valor unitario extraido = [%.3f]", (*ppstSKULista)->pstSKU->fValorUnitario);
        }
      }

      // Ajusta-se a posição para o início do próximo item.
      {
        // Busca-se a posição inicial.
        {
          lOffsetInicio = ftell(pfArquivo);
          while ((cCaractere = fgetc(pfArquivo)) != EOF)
          {
            if (strchr(szDelimitadores, (int) cCaractere) == NULL)
            {
              // Não é um delimitador. Logo saímos deste laço.
              break;
            }
            DEBUG("Caractere ignorado (ASCII=[%d])", cCaractere);
            lOffsetInicio = ftell(pfArquivo);
          }
        }
        
        // Ajusta-se a nova posição.
        {
          if (!feof(pfArquivo))
          {            
            fseek(pfArquivo, lOffsetInicio, SEEK_SET);
          }
        }
      }
    }
    
    *ppstSKULista = (*ppstSKULista)->pstPrimeiro;
    DEBUG("Ponteiro para o primeiro item = [%p]", *ppstSKULista);
  }
  
  // Fecha-se o arquivo.
  {
    fclose(pfArquivo);
    pfArquivo = NULL;
  }
  
  DEBUG("--- Fim");
  return SUCCESS;
}


// Abre o arquivo de cupons e carrega os dados em memória.
int iCarregarCupons(const char *pszNomeArquivoCupons, TIPO_LISTA_CUPONS **ppstCupomLista)
{
  FILE                 *pfArquivo             =  NULL;
  TIPO_LISTA_CUPONS    *pstCupomListaAnterior = *ppstCupomLista = NULL;
  TIPO_LISTA_CUPONS    *pstCupomListaPrimeiro =  NULL;
  TIPO_LISTA_CUPONS   **ppstCupomListaUltimo  =  NULL;
  TIPO_LISTA_USUARIOS  *pstUsuarioLista       =  NULL;
  TIPO_LISTA_SKUS      *pstSKULista           =  NULL;
  char                  szDelimitadores []    =  "|\n";
  char                  cCaractere            =  '\0';
  long int              lOffsetInicio         =  0;

  DEBUG("--- Inicio");
  
  // Abre-se o arquivo.
  {
    if ((pfArquivo = fopen(pszNomeArquivoCupons, "r")) == NULL)
    {
      DEBUG("Erro: falha ao abrir o arquivo %s", pszNomeArquivoCupons);
      DEBUG("--- Fim");
      return FAILURE;
    }
  }
  
  // Carrega-se as listas de usuários e SKUs.
  {
    if (iCarregarUsuarios(szNomeArquivoUsuarios, &pstUsuarioLista) == FAILURE)
    {
      fclose(pfArquivo);
      pfArquivo = NULL;
      DEBUG("Erro: falha ao inicializar a lista de usuarios");
      DEBUG("--- Fim");
      return FAILURE;
    }
    
    if (iCarregarSKUs(szNomeArquivoSKUs, &pstSKULista) == FAILURE)
    {
      fclose(pfArquivo);
      pfArquivo = NULL;
      vLiberarUsuarios(pstUsuarioLista);
      DEBUG("Erro: falha ao inicializar a lista de SKUs");
      DEBUG("--- Fim");
      return FAILURE;
    }
  }

  // Lê-se o arquivo, ajustando-o para a posição inicial dos dados.
  {
    DEBUG("Lendo o arquivo %s..", pszNomeArquivoCupons);
    
    // Busca-se a posição inicial.
    {
      lOffsetInicio = ftell(pfArquivo);
      while ((cCaractere = fgetc(pfArquivo)) != EOF)
      {
        if (strchr(szDelimitadores, (int) cCaractere) == NULL)
        {
          // Não é um delimitador. Logo saímos deste laço.
          break;
        }
        
        DEBUG("Caractere ignorado (ASCII=[%d])", cCaractere);
        lOffsetInicio = ftell(pfArquivo);
      }
    }
    
    // Ajusta-se a nova posição.
    {
      if (feof(pfArquivo))
      {
        // Se o fim do arquivo foi alcançado...
        fclose(pfArquivo);
        pfArquivo = NULL;
        DEBUG("Erro: o arquivo nao possui dados validos");
        DEBUG("--- Fim");
        return FAILURE;
      }
      else
      {
        // Caso haja dados válidos no arquivo, a nova posição inicial é definida.
        fseek(pfArquivo, lOffsetInicio, SEEK_SET);
      }
    }
  }
  
  // Aloca-se memória para o ponteiro para o último item da lista.
  {
    if ((ppstCupomListaUltimo = calloc(1, sizeof (TIPO_LISTA_CUPONS*))) == NULL)
    {
      fclose(pfArquivo);
      pfArquivo = NULL;
      vLiberarUsuarios(pstUsuarioLista);
      vLiberarSKUs(pstSKULista);
      DEBUG("Erro: falha ao alocar memoria");
      DEBUG("--- Fim");
      return FAILURE;
    }

    DEBUG("Ponteiro para o ponteiro do ultimo item = [%p]", ppstCupomListaUltimo);
  }
  
  // Lê-se o arquivo e inicializa-o em memória.
  {
    while (!feof(pfArquivo))
    {
      pstCupomListaAnterior = *ppstCupomLista;
      
      // Aloca-se memória para um item da lista.
      {
        if ((*ppstCupomLista = malloc(sizeof (TIPO_LISTA_CUPONS))) == NULL)
        {
          if (pstCupomListaAnterior != NULL)
          {
            vLiberarCupons(pstCupomListaAnterior->pstPrimeiro);
          }
          
          fclose(pfArquivo);
          pfArquivo = NULL;
          vLiberarUsuarios(pstUsuarioLista);
          vLiberarSKUs(pstSKULista);
          DEBUG("Erro: falha ao alocar memoria");
          DEBUG("--- Fim");
          return FAILURE;
        }
        
        DEBUG("Novo cupom encontrado (ponteiro-lista=[%p])", *ppstCupomLista);
        
        if (pstCupomListaAnterior != NULL)
        {
          pstCupomListaAnterior->pstProximo = *ppstCupomLista;
        }

        (*ppstCupomLista)->pstAnterior = pstCupomListaAnterior;
        (*ppstCupomLista)->pstProximo  = NULL;

        if (pstCupomListaAnterior == NULL)
        {
          pstCupomListaPrimeiro = *ppstCupomLista;
        }

        *ppstCupomListaUltimo          = *ppstCupomLista;
        (*ppstCupomLista)->pstPrimeiro = pstCupomListaPrimeiro;
        (*ppstCupomLista)->ppstUltimo  = ppstCupomListaUltimo;

        if (((*ppstCupomLista)->pstCupom = calloc(1, sizeof (TIPO_CUPOM))) == NULL)
        {
          fclose(pfArquivo);
          pfArquivo = NULL;
          vLiberarUsuarios(pstUsuarioLista);
          vLiberarSKUs(pstSKULista);
          vLiberarCupons((*ppstCupomLista)->pstPrimeiro);
          DEBUG("Erro: falha ao alocar memoria");
          DEBUG("--- Fim");
          return FAILURE;
        }
        
        DEBUG("Novo cupom encontrado (ponteiro-item=[%p])", (*ppstCupomLista)->pstCupom);
      }

      // Inicializa-se o item da lista.
      {        
        char szTmpToken [64];
        
        // Data-hora.
        {
          iExtrairTokenArquivo(pfArquivo, szDelimitadores, sizeof ((*ppstCupomLista)->pstCupom->szDataHora) - 1, 
                              (*ppstCupomLista)->pstCupom->szDataHora);
          DEBUG("Cupom data-hora extraido = [%s]", (*ppstCupomLista)->pstCupom->szDataHora);
        }
        
        // Número do cupom.
        {
          memset(szTmpToken, '\0', sizeof (szTmpToken));
          iExtrairTokenArquivo(pfArquivo, szDelimitadores, sizeof (szTmpToken) - 1, 
                              szTmpToken);
          (*ppstCupomLista)->pstCupom->ulNumeroCupom = (unsigned long int) atol(szTmpToken);
          DEBUG("Cupom numero extraido = [%lu]", (*ppstCupomLista)->pstCupom->ulNumeroCupom);
        }
        
        // Operador.
        {
          memset(szTmpToken, '\0', sizeof (szTmpToken));
          iExtrairTokenArquivo(pfArquivo, szDelimitadores, sizeof ((*ppstCupomLista)->pstCupom->stOperador.szLogin) - 1, 
                              szTmpToken);
          {
            TIPO_USUARIO stUsuario;
            memset(&stUsuario, '\0', sizeof (stUsuario));
            iProcurarUsuario(pstUsuarioLista, szTmpToken, &stUsuario);
            memcpy(&(*ppstCupomLista)->pstCupom->stOperador, &stUsuario, sizeof (TIPO_USUARIO));
          }
          DEBUG("Cupom operador extraido (login=[%s])", (*ppstCupomLista)->pstCupom->stOperador.szLogin);
        }
        
        // Número de itens.
        {
          memset(szTmpToken, '\0', sizeof (szTmpToken));
          iExtrairTokenArquivo(pfArquivo, szDelimitadores, sizeof (szTmpToken) - 1, 
                              szTmpToken);
          (*ppstCupomLista)->pstCupom->uiTotalItens = (unsigned int) atoi(szTmpToken);
          DEBUG("Cupom numero de itens extraido = [%u]", (*ppstCupomLista)->pstCupom->uiTotalItens);
        }
        
        // Itens do cupom.
        {
          unsigned int uiTotalItens = (*ppstCupomLista)->pstCupom->uiTotalItens;
          unsigned int uiIndice     = 0;
          
          while (uiIndice < uiTotalItens)
          {
            // Número de sequência do item.
            {
              memset(szTmpToken, '\0', sizeof (szTmpToken));
              iExtrairTokenArquivo(pfArquivo, szDelimitadores, sizeof (szTmpToken) - 1, 
                              szTmpToken);
              (*ppstCupomLista)->pstCupom->astItem[uiIndice].uiSequenciaItem = (unsigned int) atoi(szTmpToken);
              DEBUG("Cupom sequencia do item extraido = [%u]", (*ppstCupomLista)->pstCupom->astItem[uiIndice].uiSequenciaItem);
            }
            
            // SKU do item.
            {
              memset(szTmpToken, '\0', sizeof (szTmpToken));
              iExtrairTokenArquivo(pfArquivo, szDelimitadores, sizeof (szTmpToken) - 1, 
                                  szTmpToken);
              {              
                TIPO_SKU stSKU;
                memset(&stSKU, '\0', sizeof (stSKU));
                iProcurarSKU(pstSKULista, (unsigned long int) atol(szTmpToken), &stSKU);
                memcpy(&(*ppstCupomLista)->pstCupom->astItem[uiIndice].stSKUItem, &stSKU, sizeof (TIPO_SKU));
              }
              DEBUG("Cupom SKU do item extraido (codigo=[%lu])", (*ppstCupomLista)->pstCupom->astItem[uiIndice].stSKUItem.ulCodigo);
            }
            
            // Quantidade.
            {
              memset(szTmpToken, '\0', sizeof (szTmpToken));
              iExtrairTokenArquivo(pfArquivo, szDelimitadores, sizeof (szTmpToken) - 1, 
                              szTmpToken);
              (*ppstCupomLista)->pstCupom->astItem[uiIndice].fQuantidade = (float) atof(szTmpToken);
              DEBUG("Cupom quantidade do item extraido = [%.3f]", (*ppstCupomLista)->pstCupom->astItem[uiIndice].fQuantidade);
            }
            
            // Valor do item.
            {
              memset(szTmpToken, '\0', sizeof (szTmpToken));
              iExtrairTokenArquivo(pfArquivo, szDelimitadores, sizeof (szTmpToken) - 1, 
                              szTmpToken);
              (*ppstCupomLista)->pstCupom->astItem[uiIndice].dValorItem = atof(szTmpToken);
              DEBUG("Cupom valor do item extraido = [%.3lf]", (*ppstCupomLista)->pstCupom->astItem[uiIndice].dValorItem);
            }
            
            uiIndice++;
          }
        }
        
        // Valor do cupom.
        {
          memset(szTmpToken, '\0', sizeof (szTmpToken));
          iExtrairTokenArquivo(pfArquivo, szDelimitadores, sizeof (szTmpToken) - 1, 
                              szTmpToken);
          (*ppstCupomLista)->pstCupom->dValorCupom = atof(szTmpToken);
          DEBUG("Cupom valor extraido = [%.3lf]", (*ppstCupomLista)->pstCupom->dValorCupom);
        }
      }

      // Ajusta-se a posição para o início do próximo item.
      {
        // Busca-se a posição inicial.
        {
          lOffsetInicio = ftell(pfArquivo);
          while ((cCaractere = fgetc(pfArquivo)) != EOF)
          {
            if (strchr(szDelimitadores, (int) cCaractere) == NULL)
            {
              // Não é um delimitador. Logo saímos deste laço.
              break;
            }
            DEBUG("Caractere ignorado (ASCII=[%d])", cCaractere);
            lOffsetInicio = ftell(pfArquivo);
          }
        }
        
        // Ajusta-se a nova posição.
        {
          if (!feof(pfArquivo))
          {            
            fseek(pfArquivo, lOffsetInicio, SEEK_SET);
          }
        }
      }
    }
    
    *ppstCupomLista = (*ppstCupomLista)->pstPrimeiro;
    DEBUG("Ponteiro para o primeiro item = [%p]", *ppstCupomLista);
  }
  
  // Fecha-se o arquivo e libera-se as listas de usuários e SKUs inicializadas.
  {
    fclose(pfArquivo);
    pfArquivo = NULL;
    vLiberarUsuarios(pstUsuarioLista);
    vLiberarSKUs(pstSKULista);
  }
  
  DEBUG("--- Fim");
  return SUCCESS;
}

// Libera a memória de dados de usuários carregados.
void vLiberarUsuarios(TIPO_LISTA_USUARIOS *pstUsuarioLista)
{
  static int iChamada = 0;
  
  DEBUG("--- Inicio [%d]", ++iChamada);
  DEBUG("Valor do argumento = [%p]", pstUsuarioLista);

  if (pstUsuarioLista != NULL)
  {
    vLiberarUsuarios(pstUsuarioLista->pstProximo);
    if (pstUsuarioLista->pstProximo == NULL)
    {
      // Chegamos ao final da lista. Logo:
      DEBUG("Liberados %3d bytes de memoria em %p", sizeof (*pstUsuarioLista->ppstUltimo), pstUsuarioLista->ppstUltimo);
      free(pstUsuarioLista->ppstUltimo);
      pstUsuarioLista->ppstUltimo = NULL;
    }
    
    DEBUG("Liberados %3d bytes de memoria em %p", sizeof (*pstUsuarioLista->pstUsuario), pstUsuarioLista->pstUsuario);
    free(pstUsuarioLista->pstUsuario);
    pstUsuarioLista->pstUsuario = NULL;
    
    DEBUG("Liberados %3d bytes de memoria em %p", sizeof (*pstUsuarioLista), pstUsuarioLista);
    free(pstUsuarioLista);
    pstUsuarioLista = NULL;
  }
  
  DEBUG("--- Fim [%d]", iChamada--);
  return;
}

// Libera a memória de dados de SKUs carregados.
void vLiberarSKUs(TIPO_LISTA_SKUS *pstSKULista)
{
  static int iChamada = 0;
  
  DEBUG("--- Inicio [%d]", ++iChamada);
  DEBUG("Valor do argumento = [%p]", pstSKULista);

  if (pstSKULista != NULL)
  {
    vLiberarSKUs(pstSKULista->pstProximo);
    if (pstSKULista->pstProximo == NULL)
    {
      // Chegamos ao final da lista. Logo:
      DEBUG("Liberados %3d bytes de memoria em %p", sizeof (*pstSKULista->ppstUltimo), pstSKULista->ppstUltimo);
      free(pstSKULista->ppstUltimo);
      pstSKULista->ppstUltimo = NULL;
    }
    
    DEBUG("Liberados %3d bytes de memoria em %p", sizeof (*pstSKULista->pstSKU), pstSKULista->pstSKU);
    free(pstSKULista->pstSKU);
    pstSKULista->pstSKU = NULL;
    
    DEBUG("Liberados %3d bytes de memoria em %p", sizeof (*pstSKULista), pstSKULista);
    free(pstSKULista);
    pstSKULista = NULL;
  }
  
  DEBUG("--- Fim [%d]", iChamada--);
  return;
}

// Libera a memória de dados de cupons carregados.
void vLiberarCupons(TIPO_LISTA_CUPONS *pstCupomLista)
{
  static int iChamada = 0;
  
  DEBUG("--- Inicio [%d]", ++iChamada);
  DEBUG("Valor do argumento = [%p]", pstCupomLista);

  if (pstCupomLista != NULL)
  {
    vLiberarCupons(pstCupomLista->pstProximo);
    if (pstCupomLista->pstProximo == NULL)
    {
      // Chegamos ao final da lista. Logo:
      DEBUG("Liberados %3d bytes de memoria em %p", sizeof (*pstCupomLista->ppstUltimo), pstCupomLista->ppstUltimo);
      free(pstCupomLista->ppstUltimo);
      pstCupomLista->ppstUltimo = NULL;
    }
    
    DEBUG("Liberados %3d bytes de memoria em %p", sizeof (*pstCupomLista->pstCupom), pstCupomLista->pstCupom);
    free(pstCupomLista->pstCupom);
    pstCupomLista->pstCupom = NULL;
    
    DEBUG("Liberados %3d bytes de memoria em %p", sizeof (*pstCupomLista), pstCupomLista);
    free(pstCupomLista);
    pstCupomLista = NULL;
  }
  
  DEBUG("--- Fim [%d]", iChamada--);
  return;
}

// Procura um usuário numa lista, copiando-o para a variável passada via parâmetro.
int iProcurarUsuario(TIPO_LISTA_USUARIOS *pstUsuarioLista, const char *pszLogin, TIPO_USUARIO *pstUsuario)
{ 
  DEBUG("--- Inicio");
  
  if (pstUsuarioLista == NULL || pszLogin == NULL || pstUsuario == NULL)
  {
    DEBUG("Erro: argumentos invalidos");
    DEBUG("--- Fim");
    return FAILURE;
  }
  
  while (pstUsuarioLista != NULL)
  {
    DEBUG("Analisando item da lista... (ponteiro=[%p])", pstUsuarioLista);
    
    if (pstUsuarioLista->pstUsuario == NULL)
    {
      DEBUG("Erro: lista de usuarios inconsistente");
      DEBUG("--- Fim");
      return FAILURE;
    }

    if (strcmp(pstUsuarioLista->pstUsuario->szLogin, pszLogin) == 0)
    {
      strcpy(pstUsuario->szTipo,  pstUsuarioLista->pstUsuario->szTipo);
      strcpy(pstUsuario->szLogin, pstUsuarioLista->pstUsuario->szLogin);
      strcpy(pstUsuario->szSenha, pstUsuarioLista->pstUsuario->szSenha);
      strcpy(pstUsuario->szNome,  pstUsuarioLista->pstUsuario->szNome);

      DEBUG("Usuario de login %s encontrado", pszLogin);
      DEBUG("--- Fim");
      return SUCCESS;
    }

    pstUsuarioLista = pstUsuarioLista->pstProximo;
  }
  
  DEBUG("Usuario de login %s nao encontrado", pszLogin);
  DEBUG("--- Fim");
  return FAILURE;
}

// Escreve no arquivo os dados de usuário recebidos via parâmetro.
int iEscreverUsuario(const TIPO_USUARIO stUsuario)
{
  FILE         *pfArquivoAtual     = NULL;
  FILE         *pfArquivoNovo      = NULL;
  long int      lOffsetFim         = 0;
  char          cCaractere         = '\0';
  char          szDelimitadores [] = "|\n";
  TIPO_USUARIO  stUsuarioAux;
  
  memset(&stUsuarioAux, '\0', sizeof (stUsuarioAux));
  
  DEBUG("--- Inicio");

  // Abre-se e lê o arquivo existente, buscando a posição final dos dados.
  if ((pfArquivoAtual = fopen(szNomeArquivoUsuarios, "r")) != NULL)
  {
    DEBUG("Lendo o arquivo %s..", szNomeArquivoUsuarios);
    
    do
    {
      cCaractere = fgetc(pfArquivoAtual);

      if (cCaractere != EOF)
      {
        if (strchr(szDelimitadores, cCaractere) == NULL)
        {
          // Não é um delimitador. Logo temos a posição final de um dado válido.
          lOffsetFim = ftell(pfArquivoAtual);
        }
      }
    } while (!feof(pfArquivoAtual));
    
    fseek(pfArquivoAtual, 0L, SEEK_SET);
  }

  // Copia-se para um novo arquivo o conteúdo do arquivo existente até a posição indicada.
  if (pfArquivoAtual != NULL)
  {    
    if ((pfArquivoNovo = fopen("tmp.dat", "w")) == NULL)
    {
      fclose(pfArquivoAtual);
      pfArquivoAtual = NULL;
      DEBUG("Erro: falha ao criar o arquivo temporario");
      DEBUG("--- Fim");
      return FAILURE;
    }
    
    DEBUG("Criado o arquivo temporario tmp.dat");
    
    while (ftell(pfArquivoNovo) < lOffsetFim)
    {
      fprintf(pfArquivoNovo, "%c", fgetc(pfArquivoAtual));
    }
    fprintf(pfArquivoNovo, "|\n");
    
    fclose(pfArquivoAtual);
    pfArquivoAtual = NULL;
    fclose(pfArquivoNovo);
    pfArquivoNovo = NULL;
    
    // Apagamos o arquivo atual e renomeamos a cópia do arquivo.
    unlink(szNomeArquivoUsuarios);
    DEBUG("Apagado o arquivo atual %s", szNomeArquivoUsuarios);
    rename("tmp.dat", szNomeArquivoUsuarios);
    DEBUG("Renomeado o arquivo temporario para %s", szNomeArquivoUsuarios);
  }
  
  // Abre-se ou cria-se, caso não exista, o novo arquivo.
  if ((pfArquivoNovo = fopen(szNomeArquivoUsuarios, "a")) == NULL)
  {
    DEBUG("Erro: falha ao abrir/criar o arquivo %s", szNomeArquivoUsuarios);
    DEBUG("--- Fim");
    return FAILURE;
  }

  // Escreve-se no arquivo os dados recebidos.
  {
    // Tipo de usuário.
    strncpy(stUsuarioAux.szTipo, stUsuario.szTipo, sizeof (stUsuario.szTipo) - 1);
    fprintf(pfArquivoNovo, "%s|", stUsuarioAux.szTipo);
    DEBUG("Usuario tipo escrito = [%s]", stUsuarioAux.szTipo);

    // Login do usuário.
    strncpy(stUsuarioAux.szLogin, stUsuario.szLogin, sizeof (stUsuario.szLogin) - 1);
    fprintf(pfArquivoNovo, "%s|", stUsuarioAux.szLogin);
    DEBUG("Usuario login escrito = [%s]", stUsuarioAux.szLogin);
    
    // Senha do usuário.
    strncpy(stUsuarioAux.szSenha, stUsuario.szSenha, sizeof (stUsuario.szSenha) - 1);
    fprintf(pfArquivoNovo, "%s|", stUsuarioAux.szSenha);
    DEBUG("Usuario senha escrito = [%s]", stUsuarioAux.szSenha);
    
    // Nome do usuário.
    strncpy(stUsuarioAux.szNome, stUsuario.szNome, sizeof (stUsuario.szNome) - 1);
    fprintf(pfArquivoNovo, "%s|", stUsuarioAux.szNome);
    DEBUG("Usuario nome escrito = [%s]", stUsuarioAux.szNome);
  }

  fclose(pfArquivoNovo);
  pfArquivoNovo = NULL;
  DEBUG("--- Fim");
  return SUCCESS;
}

// Procura um SKU numa lista, copiando-o para a variável passada via parâmetro.
int iProcurarSKU(TIPO_LISTA_SKUS *pstSKULista, const unsigned long int ulCodigo, TIPO_SKU *pstSKU)
{
  DEBUG("--- Inicio");
  
  if (pstSKULista == NULL || ulCodigo == 0 || pstSKU == NULL)
  {
    DEBUG("Erro: argumentos invalidos");
    DEBUG("--- Fim");
    return FAILURE;
  }
  
  while (pstSKULista != NULL)
  {
    DEBUG("Analisando item da lista... (ponteiro=[%p])", pstSKULista);
    
    if (pstSKULista->pstSKU == NULL)
    {
      DEBUG("Erro: lista de SKUs inconsistente");
      DEBUG("--- Fim");
      return FAILURE;
    }

    if (pstSKULista->pstSKU->ulCodigo == ulCodigo)
    {
      pstSKU->ulCodigo       = pstSKULista->pstSKU->ulCodigo;
      strcpy(pstSKU->szDescricao, pstSKULista->pstSKU->szDescricao);
      pstSKU->fValorUnitario = pstSKULista->pstSKU->fValorUnitario;

      DEBUG("SKU de codigo %lu encontrado", ulCodigo);
      DEBUG("--- Fim");
      return SUCCESS;
    }
    
    pstSKULista = pstSKULista->pstProximo;
  }
  
  DEBUG("SKU de codigo %lu nao encontrado", ulCodigo);
  DEBUG("--- Fim");
  return FAILURE;
}

// Escreve no arquivo os dados de SKU recebidos via parâmetro.
int iEscreverSKU(const TIPO_SKU stSKU)
{
  FILE     *pfArquivoAtual     = NULL;
  FILE     *pfArquivoNovo      = NULL;
  long int  lOffsetFim         = 0;
  char      cCaractere         = '\0';
  char      szDelimitadores [] = "|\n";
  TIPO_SKU  stSKUAux;
  
  memset(&stSKUAux, '\0', sizeof (stSKUAux));
  
  DEBUG("--- Inicio");

  // Abre-se e lê o arquivo existente, buscando a posição final dos dados.
  if ((pfArquivoAtual = fopen(szNomeArquivoSKUs, "r")) != NULL)
  {
    DEBUG("Lendo o arquivo %s..", szNomeArquivoSKUs);
    
    do
    {
      cCaractere = fgetc(pfArquivoAtual);

      if (cCaractere != EOF)
      {
        if (strchr(szDelimitadores, cCaractere) == NULL)
        {
          // Não é um delimitador. Logo temos a posição final de um dado válido.
          lOffsetFim = ftell(pfArquivoAtual);
        }
      }
    } while (!feof(pfArquivoAtual));
    
    fseek(pfArquivoAtual, 0L, SEEK_SET);
  }

  // Copia-se para um novo arquivo o conteúdo do arquivo existente até a posição indicada.
  if (pfArquivoAtual != NULL)
  {    
    if ((pfArquivoNovo = fopen("tmp.dat", "w")) == NULL)
    {
      fclose(pfArquivoAtual);
      pfArquivoAtual = NULL;
      DEBUG("Erro: falha ao criar o arquivo temporario");
      DEBUG("--- Fim");
      return FAILURE;
    }
    
    DEBUG("Criado o arquivo temporario tmp.dat");
    
    while (ftell(pfArquivoNovo) < lOffsetFim)
    {
      fprintf(pfArquivoNovo, "%c", fgetc(pfArquivoAtual));
    }
    fprintf(pfArquivoNovo, "|\n");
    
    fclose(pfArquivoAtual);
    pfArquivoAtual = NULL;
    fclose(pfArquivoNovo);
    pfArquivoNovo = NULL;
    
    // Apagamos o arquivo atual e renomeamos a cópia do arquivo.
    unlink(szNomeArquivoSKUs);
    DEBUG("Apagado o arquivo atual %s", szNomeArquivoSKUs);
    rename("tmp.dat", szNomeArquivoSKUs);
    DEBUG("Renomeado o arquivo temporario para %s", szNomeArquivoSKUs);
  }
  
  // Abre-se ou cria-se, caso não exista, o novo arquivo.
  if ((pfArquivoNovo = fopen(szNomeArquivoSKUs, "a")) == NULL)
  {
    DEBUG("Erro: falha ao abrir/criar o arquivo %s", szNomeArquivoSKUs);
    DEBUG("--- Fim");
    return FAILURE;
  }

  // Escreve-se no arquivo os dados recebidos.
  {
    // Código de identificação.
    fprintf(pfArquivoNovo, "%013lu|", stSKU.ulCodigo);
    DEBUG("SKU codigo escrito = [%013lu]", stSKU.ulCodigo);

    // Descrição da mercadoria.
    strncpy(stSKUAux.szDescricao, stSKU.szDescricao, sizeof (stSKU.szDescricao) - 1);
    fprintf(pfArquivoNovo, "%s|", stSKUAux.szDescricao);
    DEBUG("SKU descricao escrito = [%s]", stSKUAux.szDescricao);
     
    // Valor unitário.
    fprintf(pfArquivoNovo, "%8.3f|\n", stSKU.fValorUnitario);
    DEBUG("SKU valor unitario escrito = [%8.3f]", stSKU.fValorUnitario);
  }

  fclose(pfArquivoNovo);
  pfArquivoNovo = NULL;
  DEBUG("--- Fim");
  return SUCCESS;
}

// Procura um cupom numa lista, copiando-o para a variável passada via parâmetro.
int iProcurarCupom(TIPO_LISTA_CUPONS *pstCupomLista, const unsigned long int ulNumeroCupom, TIPO_CUPOM *pstCupom)
{
  DEBUG("--- Inicio");
  
  if (pstCupomLista == NULL || ulNumeroCupom == 0 || pstCupom == NULL)
  {
    DEBUG("Erro: argumentos invalidos");
    DEBUG("--- Fim");
    return FAILURE;
  }
  
  while (pstCupomLista != NULL)
  {
    DEBUG("Analisando item da lista... (ponteiro=[%p])", pstCupomLista);
    
    if (pstCupomLista->pstCupom == NULL)
    {
      DEBUG("Erro: lista de cupons inconsistente");
      DEBUG("--- Fim");
      return FAILURE;
    }

    if (pstCupomLista->pstCupom->ulNumeroCupom == ulNumeroCupom)
    {
      strcpy(pstCupom->szDataHora, pstCupomLista->pstCupom->szDataHora);
      pstCupom->ulNumeroCupom = pstCupomLista->pstCupom->ulNumeroCupom;
      memcpy(&pstCupom->stOperador, &pstCupomLista->pstCupom->stOperador, sizeof (TIPO_USUARIO));
      pstCupom->uiTotalItens  = pstCupomLista->pstCupom->uiTotalItens;
      memcpy(pstCupom->astItem, pstCupomLista->pstCupom->astItem, sizeof (TIPO_ITEM) * MAX_ITENS);
      pstCupom->dValorCupom   = pstCupomLista->pstCupom->dValorCupom;
      
      DEBUG("Cupom de numero %lu encontrado", ulNumeroCupom);
      DEBUG("--- Fim");
      return SUCCESS;
    }
    
    pstCupomLista = pstCupomLista->pstProximo;
  }
  
  DEBUG("Cupom de numero %lu nao encontrado", ulNumeroCupom);
  DEBUG("--- Fim");
  return FAILURE;
}

// Escreve no arquivo os dados de cupons recebidos via parâmetro.
int iEscreverCupom(const TIPO_CUPOM stCupom)
{
  FILE       *pfArquivoAtual     = NULL;
  FILE       *pfArquivoNovo      = NULL;
  long int    lOffsetFim         = 0;
  char        cCaractere         = '\0';
  char        szDelimitadores [] = "|\n";
  TIPO_CUPOM  stCupomAux;
  
  memset(&stCupomAux, '\0', sizeof (stCupomAux));
  
  DEBUG("--- Inicio");

  // Abre-se e lê o arquivo existente, buscando a posição final dos dados.
  if ((pfArquivoAtual = fopen(szNomeArquivoCupons, "r")) != NULL)
  {
    DEBUG("Lendo o arquivo %s..", szNomeArquivoCupons);
    
    do
    {
      cCaractere = fgetc(pfArquivoAtual);

      if (cCaractere != EOF)
      {
        if (strchr(szDelimitadores, cCaractere) == NULL)
        {
          // Não é um delimitador. Logo temos a posição final de um dado válido.
          lOffsetFim = ftell(pfArquivoAtual);
        }
      }
    } while (!feof(pfArquivoAtual));
    
    fseek(pfArquivoAtual, 0L, SEEK_SET);
  }

  // Copia-se para um novo arquivo o conteúdo do arquivo existente até a posição indicada.
  if (pfArquivoAtual != NULL)
  {    
    if ((pfArquivoNovo = fopen("tmp.dat", "w")) == NULL)
    {
      fclose(pfArquivoAtual);
      pfArquivoAtual = NULL;
      DEBUG("Erro: falha ao criar o arquivo temporario");
      DEBUG("--- Fim");
      return FAILURE;
    }
    
    DEBUG("Criado o arquivo temporario tmp.dat");
    
    while (ftell(pfArquivoNovo) < lOffsetFim)
    {
      fprintf(pfArquivoNovo, "%c", fgetc(pfArquivoAtual));
    }
    fprintf(pfArquivoNovo, "|\n");
    
    fclose(pfArquivoAtual);
    pfArquivoAtual = NULL;
    fclose(pfArquivoNovo);
    pfArquivoNovo = NULL;
    
    // Apagamos o arquivo atual e renomeamos a cópia do arquivo.
    unlink(szNomeArquivoCupons);
    DEBUG("Apagado o arquivo atual %s", szNomeArquivoCupons);
    rename("tmp.dat", szNomeArquivoCupons);
    DEBUG("Renomeado o arquivo temporario para %s", szNomeArquivoCupons);
  }
  
  // Abre-se ou cria-se, caso não exista, o novo arquivo.
  if ((pfArquivoNovo = fopen(szNomeArquivoCupons, "a")) == NULL)
  {
    DEBUG("Erro: falha ao abrir/criar o arquivo %s", szNomeArquivoCupons);
    DEBUG("--- Fim");
    return FAILURE;
  }

  // Escreve-se no arquivo os dados recebidos.
  {
    // Data-hora.
    strncpy(stCupomAux.szDataHora, stCupom.szDataHora, sizeof (stCupom.szDataHora) - 1);
    fprintf(pfArquivoNovo, "\n%s|", stCupomAux.szDataHora);
    DEBUG("Cupom data-hora escrito = [%s]", stCupomAux.szDataHora);
    
    // Número do cupom.
    fprintf(pfArquivoNovo, "%lu|", stCupom.ulNumeroCupom);
    DEBUG("Cupom numero escrito = [%lu]", stCupom.ulNumeroCupom);
    
    // Operador (login).
    strncpy(stCupomAux.stOperador.szLogin, stCupom.stOperador.szLogin, sizeof (stCupom.stOperador.szLogin) - 1);
    fprintf(pfArquivoNovo, "%s|", stCupomAux.stOperador.szLogin);
    DEBUG("Cupom operador escrito (login=[%s])", stCupomAux.stOperador.szLogin);
    
    // Número de itens.
    fprintf(pfArquivoNovo, "%u|\n", stCupom.uiTotalItens);
    DEBUG("Cupom numero de itens escrito = [%u]", stCupom.uiTotalItens);
    
    // Itens do cupom.
    {
      unsigned int uiTotalItens = stCupom.uiTotalItens;
      unsigned int uiIndice     = 0;
      
      while (uiIndice < uiTotalItens)
      {        
        // Número de sequência do item.
        fprintf(pfArquivoNovo, "%03u|", stCupom.astItem[uiIndice].uiSequenciaItem);
        DEBUG("Cupom sequencia do item escrito = [%03u]", stCupom.astItem[uiIndice].uiSequenciaItem);
        
        // SKU do item (código).
        fprintf(pfArquivoNovo, "%013lu|", stCupom.astItem[uiIndice].stSKUItem.ulCodigo);
        DEBUG("Cupom SKU do item escrito (codigo=[%013lu])", stCupom.astItem[uiIndice].stSKUItem.ulCodigo);
        
        // Quantidade.
        fprintf(pfArquivoNovo, "%7.3f|", stCupom.astItem[uiIndice].fQuantidade);
        DEBUG("Cupom quantidade do item escrito = [%7.3f]", stCupom.astItem[uiIndice].fQuantidade);
        
        // Valor do item.
        fprintf(pfArquivoNovo, "%7.3lf|\n", stCupom.astItem[uiIndice].dValorItem);
        DEBUG("Cupom valor do item escrito = [%7.3lf]", stCupom.astItem[uiIndice].dValorItem);
        
        uiIndice++;
      }
    }
        
    // Valor total do cupom.
    fprintf(pfArquivoNovo, "%18s%15.3lf|\n", "", stCupom.dValorCupom);
    DEBUG("Cupom valor total escrito = [%7.3lf]", stCupom.dValorCupom);
  }

  fclose(pfArquivoNovo);
  pfArquivoNovo = NULL;
  DEBUG("--- Fim");
  return SUCCESS;
}

// Gera uma arquivo de log do programa.
void vDebugFormatado(const char *pszFonte, const int iLinha, const char *pszFuncao, const char *pszFormato, ...)
{
  FILE      *pfArquivo       = NULL;
  struct tm *pstTempo        = NULL;
  time_t     lTempo;
  char       szDataHora[32];
  char       szBuffer[32767];
  int        iContadorBytes  = 0;
 
  memset(szBuffer,   '\0', sizeof (szBuffer));
  memset(&lTempo,    '\0', sizeof (lTempo));
  memset(szDataHora, '\0', sizeof (szDataHora));

  time(&lTempo);
  pstTempo = localtime(&lTempo);
  sprintf(szDataHora, "%02d-%02d-%04d %02d:%02d:%02d", 
          pstTempo->tm_mday,
          pstTempo->tm_mon + 1,
          pstTempo->tm_year + 1900,
          pstTempo->tm_hour,
          pstTempo->tm_min,
          pstTempo->tm_sec % 60);
  
  if ((pfArquivo = fopen(szNomeArquivoDebug, "r")) == NULL)
  {
    // O arquivo ainda não existe.
    if ((pfArquivo = fopen(szNomeArquivoDebug, "a")) == NULL)
    {
      return;
    }
    
    fprintf(pfArquivo, "[%s] %4s: %10s: %5s: %24s: %s\n",
            "dd-mm-aaaa hh:mm:ss",
            "PID",
            "ARQUIVO",
            "LINHA",
            "FUNCAO",
            "MENSAGEM");
    fflush(pfArquivo);
  }
  else
  {
    // O arquivo já existe.
    fclose(pfArquivo);
    pfArquivo = NULL;
    
    if ((pfArquivo = fopen(szNomeArquivoDebug, "a")) == NULL)
    {
      return;
    }
  }
  
  if (pszFonte == NULL || pszFuncao == NULL || pszFormato == NULL)
  {
    fprintf(pfArquivo, "[%s] %4d: %10s: %5d: %24s: Erro: parametros nulos.\n", 
            szDataHora, 
            getpid(),
            __FILE__,
            __LINE__,
            __func__);
    fflush(pfArquivo);
    fclose(pfArquivo);
    pfArquivo = NULL;
    return;
  }
  
  iContadorBytes = sprintf(szBuffer, "[%s] %4d: %10s: %5d: %24s: ",
                           szDataHora,
                           getpid(),
                           pszFonte,
                           iLinha,
                           pszFuncao);
                           
  {
    va_list vlArgs;
    va_start(vlArgs, pszFormato);
    vsprintf(szBuffer + iContadorBytes, pszFormato, vlArgs);
    va_end(vlArgs);
  }
  
  fprintf(pfArquivo, "%s.\n", szBuffer);
  fflush(pfArquivo);
  fclose(pfArquivo);
  pfArquivo = NULL;
  return;
}

// Extrai tokens de um arquivo texto aberto.
int iExtrairTokenArquivo(FILE* pfArquivo, const char* szDelimitadores, size_t uTamMax, char* pszToken)
{
  char     cCaractere = '\0';
  int      iIndice    = 0;
  
  DEBUG("--- Inicio");

  // Valida-se os parâmetros.
  if (pfArquivo == NULL || szDelimitadores == NULL || uTamMax < 0 || pszToken == NULL)
  {
    DEBUG("Erro: parametros invalidos");
    DEBUG("--- Fim");
    return FAILURE;
  }
  
  // Ignora-se os delimitadores iniciais.
  while ((cCaractere = fgetc(pfArquivo)) != EOF)
  {
    if (strchr(szDelimitadores, (int) cCaractere) == NULL)
    {
      // Não é um delimitador. Então saímos do laço.
      break;
    }
  }
  
  // Lê-se o token.
  while (!feof(pfArquivo))
  {
    if (strchr(szDelimitadores, (int) cCaractere) != NULL)
    {
      DEBUG("Delimitador encontrado");
      break;
    }
    
    if (uTamMax != 0)
    {
      if(iIndice >= uTamMax)
      {
        DEBUG("Tamanho maximo alcancado");
        break;
      }
    }
    
    pszToken[iIndice++] = cCaractere;
    DEBUG("Caractere=[%c]", cCaractere);
    cCaractere = fgetc(pfArquivo);
  }
  
  if (feof(pfArquivo))
  {
    DEBUG("Final do arquivo alcancado");
  }
  
  DEBUG("--- Fim");
  return SUCCESS;
}

// Exibe interfaces padronizadas.
int bExibirTela(int iOpcao)
{
  int iRetorno = TRUE;
  
  DEBUG("--- Inicio");
  switch (iOpcao)
  {
    case LIMPA_TELA :
    {
      DEBUG("Tela (opcao=[%d])", iOpcao);
      system("clear");
      break;
    }
    case TELA_MATRIZ :
    {
      DEBUG("Tela (opcao=[%d])", iOpcao);
      bExibirTela(LIMPA_TELA);
      //               10        20        30        40        50        60        70        80
      //      123456789012345678901234567890123456789012345678901234567890123456789012345678901234
      printf("////////////////////////////////////////////////////////////////////////////////////\n"); // 01
      printf("//                                                                                //\n"); // 02
      printf("//  SEÇÃO I                                                                       //\n"); // 03
      printf("//                                                                                //\n"); // 04
      printf("//                                                                                //\n"); // 05
      printf("//                                                                                //\n"); // 06
      printf("//                                                                                //\n"); // 07
      printf("////////////////////////////////////////////////////////////////////////////////////\n"); // 08
      //      123456789012345678901234567890123456789012345678901234567890123456789012345678901234
      //               10        20        30        40        50        60        70        80
      printf("\n");                                                                                     // 09
      //               10        20        30        40        50        60        70        80
      //      123456789012345678901234567890123456789012345678901234567890123456789012345678901234
      printf("////////////////////////////////////////////////////////////////////////////////////\n"); // 10
      printf("//                                                                                //\n"); // 11
      printf("//  SEÇÃO II                                                                      //\n"); // 12
      printf("//                                                                                //\n"); // 13
      printf("//                                                                                //\n"); // 14
      printf("//                                                                                //\n"); // 15
      printf("//                                                                                //\n"); // 16
      printf("//                                                                                //\n"); // 17
      printf("//                                                                                //\n"); // 18
      printf("//                                                                                //\n"); // 19
      printf("//                                                                                //\n"); // 20
      printf("//                                                                                //\n"); // 21
      printf("//                                                                                //\n"); // 22
      printf("//                                                                                //\n"); // 23
      printf("//                                                                                //\n"); // 24
      printf("//                                                                                //\n"); // 25
      printf("//                                                                                //\n"); // 26
      printf("////////////////////////////////////////////////////////////////////////////////////\n"); // 27
      //      123456789012345678901234567890123456789012345678901234567890123456789012345678901234
      //               10        20        30        40        50        60        70        80
      printf("\n");                                                                                     // 28
      //               10        20        30        40        50        60        70        80
      //      123456789012345678901234567890123456789012345678901234567890123456789012345678901234
      printf("////////////////////////////////////////////////////////////////////////////////////\n"); // 29
      printf("//                                                                                //\n"); // 30
      printf("//  SEÇÃO III                                                                     //\n"); // 31
      printf("//                                                                                //\n"); // 32
      printf("//================================================================================//\n"); // 33
      printf("//                                                                                //\n"); // 34
      printf("//  SEÇÃO IV                                                                      //\n"); // 35
      printf("//                                                                                //\n"); // 36
      printf("////////////////////////////////////////////////////////////////////////////////////\n"); // 37
      //      123456789012345678901234567890123456789012345678901234567890123456789012345678901234
      //               10        20        30        40        50        60        70        80
      vLimparBufferPadrao(stdout);
      break;
    }
    case LIMPA_SECAO_I :
    {
      unsigned int  uiLinha = 0;
      DEBUG("Tela (opcao=[%d])", iOpcao);
      for (uiLinha = 2; uiLinha <= 7; uiLinha++)
      {
        vLimparLinha(uiLinha);
      }
      break;
    }
    case LIMPA_SECAO_II :
    {
      unsigned int  uiLinha = 0;
      DEBUG("Tela (opcao=[%d])", iOpcao);
      for (uiLinha = 11; uiLinha <= 26; uiLinha++)
      {
        vLimparLinha(uiLinha);
      }
      break;
    }
    case LIMPA_SECAO_III :
    {
      unsigned int  uiLinha = 0;
      DEBUG("Tela (opcao=[%d])", iOpcao);
      for (uiLinha = 30; uiLinha <= 32; uiLinha++)
      {
        vLimparLinha(uiLinha);
      }
      break;
    }
    case LIMPA_SECAO_IV :
    {
      unsigned int  uiLinha = 0;
      DEBUG("Tela (opcao=[%d])", iOpcao);
      for (uiLinha = 34; uiLinha <= 36; uiLinha++)
      {
        vLimparLinha(uiLinha);
      }
      break;
    }
    case LIMPA_SECOES :
    {
      DEBUG("Tela (opcao=[%d])", iOpcao);
      bExibirTela(LIMPA_SECAO_I);
      bExibirTela(LIMPA_SECAO_II);
      bExibirTela(LIMPA_SECAO_III);
      bExibirTela(LIMPA_SECAO_IV);
      break;
    }
    case TELA_LOGO :
    {
      DEBUG("Tela (opcao=[%d])", iOpcao);
      bExibirTela(LIMPA_SECAO_I);
      iExibirMensagem(5, 3, "SysPDV");
      iExibirMensagem(5, 5, "Sistema gerenciador de vendas em pontos de venda");
      break;
    }
    case TELA_APRESENTACAO :
    {
      DEBUG("Tela (opcao=[%d])", iOpcao);
      bExibirTela(LIMPA_SECAO_II);
      iExibirMensagem(5, 12, "Seja bem-vindo(a)!");
      iExibirMensagem(5, 14, "  Este  é  o  trabalho  prático da disciplina de Linguagem de");
      iExibirMensagem(5, 15, "Programação (C) pelos seguintes alunos do 2º ciclo de A.D.S.:");
      iExibirMensagem(5, 17, "  Carlos Eduardo Carvalho da Silva");
      iExibirMensagem(5, 18, "  Evelyn Caroline de Oliveira Santiago");
      iExibirMensagem(5, 19, "  Jesiele do Santos");
      iExibirMensagem(5, 20, "  Tayná Colombarolli Lima");
      iExibirMensagem(5, 21, "  Wajihah Ibrahim Kourani");
      iExibirMensagem(5, 25, "Profª. Me. Sandra Geroldo");
      break;
    }
    case TELA_ESPERA_USUARIO :
    {
      char szEntrada[32];
      memset(szEntrada, '\0', sizeof (szEntrada));
      
      DEBUG("Tela (opcao=[%d])", iOpcao);
      bExibirTela(LIMPA_SECAO_III);
      iExibirMensagem(5, 31, "Digite ENTER para continuar...");
      bExibirTela(LIMPA_SECAO_IV);
      vTerminal_GoTo(5, 35);
      vLimparBufferPadrao(stdin);
      fgets(szEntrada, sizeof (szEntrada), stdin);
      
      if ((int) strlen(szEntrada) != 1)
      {
        vLimparLinha(25);
        iExibirMensagem(5, 25, "\"Entrada inválida! Valores permitidos: ENTER ou ESC (+ ENTER).\"");
        bExibirTela(TELA_ESPERA_USUARIO);
        vLimparLinha(25);
      }
      
      break;
    }
    case TELA_RESPOSTA_USUARIO :
    {
      char szEntrada[32];
      memset(szEntrada, '\0', sizeof (szEntrada));
      
      DEBUG("Tela (opcao=[%d])", iOpcao);
      bExibirTela(LIMPA_SECAO_III);
      iExibirMensagem(5, 31, "Digite ENTER para continuar ou ESC para voltar...");
      bExibirTela(LIMPA_SECAO_IV);
      vTerminal_GoTo(5, 35);
      vLimparBufferPadrao(stdin);
      fgets(szEntrada, sizeof (szEntrada), stdin);
      
      switch (szEntrada[0])
      {
        case 10 : // ASCII 10 = ENTER
        {
          DEBUG("Resposta: ENTER (ASCII=[10])");
          bExibirTela(LIMPA_SECAO_III);
          bExibirTela(LIMPA_SECAO_IV);
          iRetorno = TRUE;
          break;
        }
        case 27 : // ASCII 27 = ESC
        {
          if ((int) strlen(szEntrada) == 2)
          {
            DEBUG("Resposta: ESC (ASCII=[27])");
            bExibirTela(LIMPA_SECAO_III);
            bExibirTela(LIMPA_SECAO_IV);
            iRetorno = FALSE;
            break;
          }
        }
        default :
        {
          vLimparLinha(25);
          iExibirMensagem(5, 25, "\"Entrada inválida! Valores permitidos: ENTER ou ESC (+ ENTER).\"");
          bExibirTela(TELA_ESPERA_USUARIO);
          vLimparLinha(25);
          iRetorno = bExibirTela(TELA_RESPOSTA_USUARIO);
        }
      }
      
      break;
    }
    case TELA_FALHA_SISTEMA :
    {
      DEBUG("Tela (opcao=[%d])", iOpcao);
      bExibirTela(LIMPA_SECAO_II);
      iExibirMensagem(5, 12, "FALHA DO SISTEMA");
      iExibirMensagem(5, 17, "Consulte o suporte ao usuario.");
      break;
    }
    case TELA_MANUTENCAO :
    {
      DEBUG("Tela (opcao=[%d])", iOpcao);
      bExibirTela(TELA_LOGO);
      bExibirTela(LIMPA_SECAO_II);
      iExibirMensagem(30, 17, "CAIXA EM MANUTENCAO");
      bExibirTela(LIMPA_SECAO_III);
      bExibirTela(LIMPA_SECAO_IV);
      break;
    }
    case TELA_CAIXA_FECHADO :
    {
      DEBUG("Tela (opcao=[%d])", iOpcao);
      bExibirTela(TELA_LOGO);
      bExibirTela(LIMPA_SECAO_II);
      iExibirMensagem(5, 12, "CAIXA FECHADO");
      iExibirMensagem(5, 17, "Favor dirigir-se ao caixa ao lado.");
      bExibirTela(LIMPA_SECAO_III);
      bExibirTela(LIMPA_SECAO_IV);
      break;
    }
    default :
    {
      bExibirTela(TELA_FALHA_SISTEMA);
      bExibirTela(TELA_ESPERA_USUARIO);
      bExibirTela(TELA_MANUTENCAO);
      vTerminal_GoTo(1, 38);
      
      DEBUG("Tela (opcao=[%d])", iOpcao);
      DEBUG("Erro: opcao invalida = [%d]", iOpcao);
      DEBUG("--- Fim");
      exit(EXIT_FAILURE);
    }
  }
  DEBUG("--- Fim");
  return iRetorno;
}

// Exibe mensagens a partir de uma posicao especificada. Retorna a quantidade de caracteres da mensagem.
int iExibirMensagem(unsigned int uiPosX, unsigned int uiPosY, char* pszFormato, ...)
{
  char    szBuffer[2048];
  va_list vaArgs;
  int     iTamanhoMsg    = 0;
  
  memset(szBuffer, '\0', sizeof (szBuffer));

  DEBUG("--- Inicio");
  va_start(vaArgs, pszFormato);
  if (pszFormato != NULL)
  {
    vsprintf(szBuffer, pszFormato, vaArgs);
    iTamanhoMsg = strlen(szBuffer);
    vTerminal_GoTo(uiPosX, uiPosY);
    printf("%s", szBuffer);
    vLimparBufferPadrao(stdout);
    DEBUG("PosX=[%u], PosY=[%u], Mensagem=[%s]", uiPosX, uiPosY, szBuffer);
  }
  va_end(vaArgs);
  DEBUG("--- Fim");  
  return iTamanhoMsg;
}

// Limpa um trecho de uma linha do console e posiciona o cursor em sua posição final.
void vLimparLinha(unsigned int uiLinha)
{
  const unsigned int uiInicio = 3;
  const unsigned int uiFim    = 82;
  
  DEBUG("--- Inicio");
  if (uiLinha == 0)
  {
    bExibirTela(TELA_FALHA_SISTEMA);
    bExibirTela(TELA_ESPERA_USUARIO);
    bExibirTela(TELA_MANUTENCAO);
    vTerminal_GoTo(1, 38);
    
    DEBUG("Linha invalida = [%u]", uiLinha);
    DEBUG("--- Fim");
    exit(EXIT_FAILURE);
  }
  vTerminal_GoTo(uiInicio, uiLinha);
  printf("%*s", uiFim - uiInicio + 1, "");
  vLimparBufferPadrao(stdout);
  DEBUG("Linha=[%u], Inicio=[%u], Fim=[%u]", uiLinha, uiInicio, uiFim);
  vTerminal_GoTo(uiFim, uiLinha);
  DEBUG("--- Fim");
}

// Limpa o buffer padrão de entrada, saída ou erros.
void vLimparBufferPadrao(struct _IO_FILE *pstIOBuffer)
{
  DEBUG("--- Inicio");
  if (pstIOBuffer->_flags == stdin->_flags)
  {
    DEBUG("Limpando o buffer padrao de entrada");
    __fpurge(pstIOBuffer);
  }
  else if (pstIOBuffer->_flags == stdout->_flags)
  {
    DEBUG("Limpando o buffer padrao de saida");
    fflush(pstIOBuffer);
  }
  else if (pstIOBuffer->_flags == stderr->_flags)
  {
    DEBUG("Limpando o buffer padrao de erros");
    __fpurge(pstIOBuffer);
  }
  else
  {
    bExibirTela(TELA_FALHA_SISTEMA);
    bExibirTela(TELA_ESPERA_USUARIO);
    bExibirTela(TELA_MANUTENCAO);
    vTerminal_GoTo(1, 38);
    
    DEBUG("Erro: buffer nao padrao (_flags=[%ld])", pstIOBuffer->_flags);
    DEBUG("--- Fim");
    exit(EXIT_FAILURE);
  } 
  DEBUG("--- Fim");
}

// Move o cursor para uma posição especificada.
void vTerminal_GoTo(unsigned int uiPosX, unsigned int uiPosY)
{
  DEBUG("--- Inicio");
  DEBUG("PosX=[%u], PosY=[%u]", uiPosX, uiPosY);
  printf("\033[%d;%dH", uiPosY, uiPosX);
  vLimparBufferPadrao(stdout);
  DEBUG("--- Fim");
}

// Obtém data e hora atuais.
void vObterDataHora(TIPO_DATA_HORA *pstDataHora)
{
  time_t     lTempo;
  struct tm *pstTempo = NULL;
  
  memset(pstDataHora, '\0', sizeof (*pstDataHora));
  memset(&lTempo,     '\0', sizeof (lTempo));
  
  DEBUG("--- Inicio");
  time(&lTempo);
  pstTempo = localtime(&lTempo);
  sprintf(pstDataHora->szDia,  "%02d", pstTempo->tm_mday);
  sprintf(pstDataHora->szMes,  "%02d", pstTempo->tm_mon + 1);
  sprintf(pstDataHora->szAno,  "%04d", pstTempo->tm_year + 1900);
  sprintf(pstDataHora->szHora, "%02d", pstTempo->tm_hour);
  sprintf(pstDataHora->szMin,  "%02d", pstTempo->tm_min);
  sprintf(pstDataHora->szSeg,  "%02d", pstTempo->tm_sec % 60); 
  DEBUG("--- Fim");
}

