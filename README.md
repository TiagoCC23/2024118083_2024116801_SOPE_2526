# 📊 Log Analyzer

![Language](https://img.shields.io/badge/Language-C-blue.svg)
![Course](https://img.shields.io/badge/Course-SOPE-yellow.svg)
![University](https://img.shields.io/badge/University-UFP-green.svg)

> **Projeto Prático de Desenvolvimento de Software em C**
> 
> *Sistemas Operativos*
> 
> ***FEITO POR: Rayssa Santos e Tiago Chousal***

---

## Sobre o Projeto

O **Log Analyzer** é um sistema concorrente e de alto desempenho projetado para o processamento e análise em larga escala de ficheiros de registo (*logs*) em múltiplos formatos padrão (**Apache Combined**, **JSON Structured**, **Syslog RFC 3164** e **Nginx Error**). 

Desenvolvido para ambientes Linux/Unix no âmbito da unidade curricular de Sistemas Operativos da UFP, o sistema explora de forma estrita as primitivas do sistema operativo (chamadas ao sistema nativas POSIX como `open`, `read`, `write`, `pipe`, `fork`, `socket`, `sem_init` e `pthread`). O projeto implementa tanto a arquitetura multiprocesso de isolamento de memória (com IPC por Pipes e Unix Sockets) como a arquitetura multithread de memória partilhada de alto rendimento.

---

## Funcionalidades Principais

* **Descodificação e Classificação de Eventos:** Motores de *parsing* robustos capazes de ler dinamicamente 4 formatos de log, mapeando-os para categorias de análise (`security`, `performance`, `traffic` e `full`) e níveis de severidade (de `INFO` a `CRITICAL`).
* **Multiprocessamento Paralelo Estático:** Divisão balanceada de ficheiros físicos entre múltiplos processos-filho que operam de forma autónoma e geram relatórios isolados (`results_<pid>.txt`).
* **Comunicação Avançada Interprocesso (IPC):** Protocolo de transmissão de dados estruturado em formato binário encapsulado que opera através de **Pipes Anónimos** ou via **Sockets de Domínio Unix** (`SOCK_STREAM` no caminho `/tmp/loganalyzer.sock`).
* **Dashboard Visual em Tempo Real:** Interface gráfica desenhada diretamente no terminal (através de sequências de escape ANSI) que monitoriza o progresso de cada trabalhador, vazão global (eventos/segundo), erros e estimativas de tempo (Elapsed/ETA).
* **Paralelismo por Fatiamento de Memória (Multithreading):** Mapeamento do ficheiro completo para a memória RAM e divisão física de blocos lógicos (*chunks*) por threads operacionais atómicas, minimizando as chamadas do sistema de leitura no disco.
* **Modelo Produtor-Consumidor com Bounded Buffer:** Implementação clássica com $P$ threads produtoras (leitura e parsing) e $C$ threads consumidoras (classificação e análise) que comunicam de forma assíncrona através de um buffer circular limitado protegido por semáforos POSIX e mutexes.
* **Deteção de Padrões e Ataques em Tempo de Execução:** Algoritmos em tempo de execução capazes de identificar tentativas de força bruta (mais de 5 falhas de autenticação no mesmo IP), erros de barramento consecutivos (HTTP 5xx) e picos de tráfego anómalos.

---

## Estruturas de Dados

O projeto baseia-se nas seguintes estruturas principais de dados:

* `CONFIG` (em `R3_1_logAnalyzer.h`): Centraliza as diretivas recolhidas pela linha de comando necessárias para parametrizar toda a execução (diretório, processos, threads, modo, verbosidade, ficheiro de saída e protocolo IPC).
* `SHAREDSTATS` (em `R3_1_logAnalyzer.h`): Estrutura que unifica as contagens atómicas partilhadas de linhas totais, erros e avisos detetados concorrentemente por threads operacionais.
* `LogEntry` e `LogFormat` (em `R3_1_logAnalyzer.h`): Modelo de encapsulamento universal que uniformiza os diferentes tipos de logs lidos pelo sistema numa estrutura de análise comum para os consumidores da fila.
* `Message`, `NormalMsg` e `VerboseMsg` (em `R3_3_workers_pipes.h`): Define o empacotamento de dados para canais de IPC, enviando estatísticas agregadas (Modo Normal) ou disparando eventos críticos imediatos (Modo Verboso/Verbose).
* `WorkerStatus` (em `R3_4_R4_2_dashboard.h`): Estrutura que rastreia dinamicamente a percentagem de progresso, linhas processadas e o estado operacional (`IDLE`, `WORKING`, `DONE`) de cada thread ou processo.

---

## Compilação

Para compilar o projeto de forma automática, utilize o `Makefile` disponibilizado:

```bash
# Compilar o executável principal (logAnalyzer)
make

# Limpar ficheiros objeto compilados (.o), sockets e resultados anteriores
make clean
```

## Execução

O executável ./logAnalyzer recebe os argumentos e opções flexíveis pela linha de
comandos:

./logAnalyzer <diretorio_logs> <num_processos> <modo> [opcoes]

Exemplos Práticos de Execução:

  - 1. Multi-Processo Básico (Req. B):

    ./logAnalyzer ./datasets/apache/ 4 full --ipc=basic

  - 2. Multi-Processo com Pipes Anónimos (Req. C):

    ./logAnalyzer ./datasets/apache/ 4 full --ipc=pipes

  - 3. Multi-Processo com Pipes (Modo Verboso / Logs em Tempo Real):

    ./logAnalyzer ./datasets/apache/ 4 full --ipc=pipes --verbose

  - 4. Multi-Processo com Dashboard em Tempo Real (Req. D):

    ./logAnalyzer ./datasets/apache/ 4 full --ipc=dashboard

  - 5. Multi-Processo com Sockets de Domínio Unix (Req. E):

    ./logAnalyzer ./datasets/apache/ 4 full --ipc=sockets

  - 6. Multithreading Básico (Chunk Partition em RAM - Req. R4.1 e R4.2):

    ./logAnalyzer ./datasets/apache/ 0 full --threads=4

  - 7. Produtor-Consumidor Avançado (Bounded Buffer - Req. R4.3):

    ./logAnalyzer ./datasets/apache/ 0 full --produtores=2 --consumidores=2

##Testes

Os testes unitários e funcionais dos parsers e do classificador de eventos estão
localizados na raiz do projeto e podem ser compilados de forma independente para
validar as rotinas de descodificação:

# Compilar os testes unitários
gcc -Wall -Wextra -Isrc -o test_parsers test_parsers.c src/log_parser_simple.c
gcc -Wall -Wextra -Isrc -o test_classifier test_classifier.c src/log_parser_simple.c src/event_classifier.c

# Executar a verificação de formatos de logs
./test_parsers

# Executar a verificação do classificador de eventos
./test_classifier ./datasets/apache/access_100k.log full

##Estado de Implementação dos Requisitos

Abaixo encontra-se a tabela detalhada do estado de implementação dos requisitos
exigidos para as duas fases de avaliação do projeto.

**Fase 1 (Processos e IPC)**

| ID       | Descrição Resumida                                          | Peso | Estado         |
| :------- | :---------------------------------------------------------- | :--: | :------------: |
| **R3.1** | Interface de Linha de Comandos (CLI)                        | 10%  | ✅ Implementado |
| **R3.2** | Arquitetura Multi-Processo Básica (Isolamento de Ficheiros) | 15%  | ✅ Implementado |
| **R3.3** | Comunicação via Pipes Anónimos (Normal e Verboso)           | 30%  | ✅ Implementado |
| **R3.4** | Dashboard de Progresso em Tempo Real (Monotónico)           | 15%  | ✅ Implementado |
| **R3.5** | Comunicação via Unix Domain Sockets (SOCK\_STREAM)          | 30%  | ✅ Implementado |

**Fase 2 (Threads e Sincronização)**

| ID       | Descrição Resumida                                            | Peso | Estado         |
| :------- | :------------------------------------------------------------ | :--: | :------------: |
| **R4.1** | Modelo Worker Threads Básico (Divisão de Fatias)              | 20%  | ✅ Implementado |
| **R4.2** | Dashboard de Progresso Multi-Thread (Thread de Monitorização) | 20%  | ✅ Implementado |
| **R4.3** | Modelo Produtor-Consumidor com Bounded Buffer (Semáforos)     | 60%  | ✅ Implementado |

