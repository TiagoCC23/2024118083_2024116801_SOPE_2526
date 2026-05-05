/**
 * @file workers_pipes.h
 * @brief Implementação do processamento de logs utilizando Pipes para IPC.
 * * Este módulo contém as estruturas e funções necessárias para a comunicação
 * entre o processo pai (diretor) e os processos filhos (workers) através de 
 * pipes anónimos, conforme o Requisito C do projeto.
 */

#ifndef WORKERS_PIPES_H
#define WORKERS_PIPES_H

#include "log_parser.h"
#include "logAnalyzer.h"
#include "workers.h"
#include <errno.h>

/** @name Tipos de Mensagem
 * Constantes que definem o tipo de payload enviado pelo pipe.
 * @{ */
#define MSG_TYPE_NORMAL  1  /**< Resumo estatístico final. */
#define MSG_TYPE_VERBOSE 2  /**< Evento crítico em tempo real. */
#define MSG_TYPE_DONE    3  /**< Sinalização de fim de processamento do worker. */
/** @} */

/**
 * @struct NormalMsg
 * @brief Estrutura de dados para o relatório final de um worker (Modo Normal).
 */
typedef struct {
    pid_t  pid;             /**< ID do processo filho. */
    long   total_lines;     /**< Total de linhas processadas. */
    long   errors;          /**< Total de erros detetados. */
    long   warnings;        /**< Total de avisos detetados. */
    char   top_ip[64];      /**< IP com maior número de ocorrências. */
} NormalMsg;

/**
 * @struct VerboseMsg
 * @brief Estrutura de dados para envio de alertas em tempo real (Modo Verbose).
 */
typedef struct {
    pid_t  pid;             /**< ID do processo filho. */
    char   timestamp[32];   /**< Data e hora do evento. */
    char   type[16];        /**< Severidade: INFO, WARN, ERROR, CRITICAL. */
    char   msg[256];        /**< Mensagem descritiva do erro. */
    char   ip[64];          /**< Endereço IP associado ao evento. */
} VerboseMsg;

/**
 * @struct Message
 * @brief Cabeçalho (Header) enviado antes de cada payload pelo pipe.
 */
typedef struct {
    int  type;              /**< Tipo da mensagem (MSG_TYPE_*). */
    int  size;              /**< Tamanho em bytes do payload que vem a seguir. */
} Message;

// ------------------ FUNCAO PRINCIPAL ------------------

/**
 * @brief Função principal que gere a criação de workers e a recolha de dados via pipes.
 * * Orquestra a divisão de ficheiros, cria os pipes necessários, faz o fork dos
 * processos e agrega os resultados enviados pelos filhos no final.
 * * @param config Ponteiro para a estrutura de configuração do sistema.
 */
void logWorker_pipes(CONFIG *config);

// ------------------ FUNCOES ADICIONAIS ------------------

/**
 * @brief Lê exatamente 'n' bytes de um descritor de ficheiro.
 * * Garante que a leitura é concluída mesmo se for interrompida por sinais.
 * * @param fd Descritor do ficheiro (pipe).
 * @param ptr Buffer para armazenar os dados lidos.
 * @param n Número de bytes a ler.
 * @return ssize_t Número de bytes lidos ou -1 em caso de erro.
 */
ssize_t readn(int fd, void *ptr, size_t n);

/**
 * @brief Escreve exatamente 'n' bytes num descritor de ficheiro.
 * * @param fd Descritor do ficheiro (pipe).
 * @param ptr Ponteiro para os dados a enviar.
 * @param n Número de bytes a escrever.
 * @return ssize_t Número de bytes escritos ou -1 em caso de erro.
 */
ssize_t writen(int fd, const void *ptr, size_t n);

/**
 * @brief Prepara e envia uma mensagem formatada (header + payload) pelo pipe.
 * * @param fd_write Pipe de escrita.
 * @param type Tipo da mensagem (definido nas constantes MSG_TYPE_*).
 * @param payload Ponteiro para a estrutura de dados (NormalMsg ou VerboseMsg).
 * @param size Tamanho do payload em bytes.
 */
void send_msg(int fd_write, int type, const void *payload, int size);

/**
 * @brief Contém a lógica de processamento de logs executada pelo processo filho.
 * * Percorre os ficheiros atribuídos, faz o parsing das linhas e comunica
 * os resultados ao pai.
 * * @param fd_write Pipe para comunicação com o pai.
 * @param id Identificador numérico do worker.
 * @param config Configurações globais.
 * @param ficheiros Matriz com os nomes de todos os ficheiros.
 * @param start Índice inicial do lote de ficheiros deste worker.
 * @param end Índice final (exclusive) do lote.
 */
void filho_logic(int fd_write, int id, CONFIG *config, char ficheiros[][512], int start, int end);

#endif