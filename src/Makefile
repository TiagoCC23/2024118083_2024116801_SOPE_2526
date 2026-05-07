# Makefile para o projeto Log Analyzer (compatível Linux/macOS)

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I.

# Detectar sistema operativo
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LDFLAGS = -lrt -pthread
endif
ifeq ($(UNAME_S),Darwin)
    LDFLAGS = -pthread
endif

# Ficheiros
PARSER_SRC = log_parser.c
PARSER_OBJ = log_parser.o
EVENT_SRC = event_classifier.c
EVENT_OBJ = event_classifier.o

TEST_SRC = test_parsers.c
TEST_BIN = test_parsers
CLASSIFIER_SRC = test_classifier.c
CLASSIFIER_BIN = test_classifier

# Geradores
GEN_DIR = ../generators
GEN_APACHE = $(GEN_DIR)/gen_apache
GEN_JSON = $(GEN_DIR)/gen_json
GEN_SYSLOG = $(GEN_DIR)/gen_syslog
GEN_NGINX = $(GEN_DIR)/gen_nginx

# Datasets
DATASET_DIR = ../datasets
APACHE_DIR = $(DATASET_DIR)/apache
JSON_DIR = $(DATASET_DIR)/json_logs
SYSLOG_DIR = $(DATASET_DIR)/syslog
NGINX_DIR = $(DATASET_DIR)/nginx

.PHONY: all clean test generators datasets help

all: classifier test generators

# Compilar Classificador
classifier: $(CLASSIFIER_BIN)

$(CLASSIFIER_BIN): $(CLASSIFIER_SRC) $(PARSER_OBJ) $(EVENT_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "✓ Classificador compilado: ./$(CLASSIFIER_BIN)"

# Compilar testes
test: $(TEST_BIN)

$(TEST_BIN): $(TEST_SRC) $(PARSER_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "✓ Test suite compilado: ./$(TEST_BIN)"

$(PARSER_OBJ): $(PARSER_SRC)
	$(CC) $(CFLAGS) -c $< -o $@
	
$(EVENT_OBJ): $(EVENT_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

# Compilar geradores
generators: $(GEN_APACHE) $(GEN_JSON) $(GEN_SYSLOG) $(GEN_NGINX)

$(GEN_APACHE): $(GEN_DIR)/generate_apache_logs.c
	$(CC) -O2 $< -o $@
	@echo "✓ Gerador Apache compilado"

$(GEN_JSON): $(GEN_DIR)/generate_json_logs.c
	$(CC) -O2 $< -o $@
	@echo "✓ Gerador JSON compilado"

$(GEN_SYSLOG): $(GEN_DIR)/generate_syslog.c
	$(CC) -O2 $< -o $@
	@echo "✓ Gerador Syslog compilado"

$(GEN_NGINX): $(GEN_DIR)/generate_nginx_error.c
	$(CC) -O2 $< -o $@
	@echo "✓ Gerador Nginx compilado"

# Gerar datasets
datasets: generators
	@echo "Gerando datasets de teste..."
	@mkdir -p $(APACHE_DIR) $(JSON_DIR) $(SYSLOG_DIR) $(NGINX_DIR)
	@./$(GEN_APACHE) 1000 > $(APACHE_DIR)/sample_1k.log 2>&1
	@echo "  ✓ Apache: 1K linhas geradas"
	@./$(GEN_JSON) 500 50 $(JSON_DIR)/ 2>&1
	@echo "  ✓ JSON: 500 linhas em 50 ficheiros"
	@./$(GEN_SYSLOG) 500 > $(SYSLOG_DIR)/sample_500.log 2>&1
	@echo "  ✓ Syslog: 500 linhas geradas"
	@./$(GEN_NGINX) 300 > $(NGINX_DIR)/sample_300.log 2>&1
	@echo "  ✓ Nginx: 300 linhas geradas"
	@echo ""
	@echo "Datasets criados:"
	@du -sh $(DATASET_DIR)/*

# Gerar datasets grandes
datasets-large: generators
	@echo "Gerando datasets GRANDES (pode demorar)..."
	@mkdir -p $(APACHE_DIR) $(JSON_DIR) $(SYSLOG_DIR) $(NGINX_DIR)
	@./$(GEN_APACHE) 100000 > $(APACHE_DIR)/access_100k.log 2>&1
	@echo "  ✓ Apache: 100K linhas (~10 MB)"
	@./$(GEN_JSON) 50000 500 $(JSON_DIR)/ 2>&1
	@echo "  ✓ JSON: 50K linhas em 500 ficheiros"
	@./$(GEN_SYSLOG) 50000 > $(SYSLOG_DIR)/security_50k.log 2>&1
	@echo "  ✓ Syslog: 50K linhas (~2 MB)"
	@./$(GEN_NGINX) 20000 > $(NGINX_DIR)/error_20k.log 2>&1
	@echo "  ✓ Nginx: 20K linhas (~1 MB)"
	@echo ""
	@du -sh $(DATASET_DIR)/*

# Executar testes
run-tests: test datasets
	@echo ""
	@echo "Executando suite de testes..."
	@echo ""
	@./$(TEST_BIN)

# Limpar ficheiros compilados
clean:
	rm -f $(PARSER_OBJ)
	rm -f $(EVENT_OBJ)
	rm -f $(TEST_BIN)
	rm -f $(CLASSIFIER_BIN)
	rm -f $(GEN_APACHE) $(GEN_JSON) $(GEN_SYSLOG) $(GEN_NGINX)
	rm -f $(APACHE_DIR)/*
	rm -f $(JSON_DIR)/*
	rm -f $(SYSLOG_DIR)/*
	rm -f $(NGINX_DIR)/*
	
	@echo "✓ Ficheiros compilados removidos"

# Limpar tudo incluindo datasets
clean-all: clean
	rm -rf $(DATASET_DIR)
	@echo "✓ Datasets removidos"

# Ajuda
help:
	@echo "Makefile - Log Analyzer Project"
	@echo ""
	@echo "Sistema detectado: $(UNAME_S)"
	@echo ""
	@echo "Targets disponíveis:"
	@echo "  make              	- Compila testes, geradores de logs e classificador de eventos"
	@echo "  make test         	- Compila apenas os testes"
	@echo "  make generators   	- Compila geradores de logs"
	@echo "  make classifier   	- Compila o classificador de eventos"
	@echo "  make datasets     	- Gera datasets pequenos (~1MB)"
	@echo "  make datasets-large 	- Gera datasets grandes (~15MB)"
	@echo "  make run-tests    	- Compila e executa testes"
	@echo "  make clean        	- Remove ficheiros compilados"
	@echo "  make clean-all    	- Remove tudo (incluindo datasets)"
	@echo "  make help         	- Mostra esta ajuda"
	@echo ""
	@echo "Uso típico:"
	@echo "  1. make              # Compilar tudo"
	@echo "  2. make datasets     # Gerar dados"
	@echo "  3. make run-tests    # Testar"
	
# A nossa regra para compilar o projeto principal
logAnalyzer: main.c logAnalyzer.c workers.c workers_pipes.c threads.c log_parser.c event_classifier.c
	gcc -Wall -Wextra -O2 main.c logAnalyzer.c workers.c workers_pipes.c threads.c log_parser.c event_classifier.c -o logAnalyzer -pthread