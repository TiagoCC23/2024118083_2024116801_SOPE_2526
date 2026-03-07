/**
 * test_parsers.c - Teste dos parsers de logs
 * Compila: gcc -o test_parsers test_parsers.c src/log_parser.c
 * Executa: ./test_parsers
 */

#include "log_parser.h"
#include <stdio.h>

// Cores ANSI para output
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_BLUE "\033[0;34m"
#define COLOR_RESET "\033[0m"

void print_separator() {
    printf("========================================\n");
}

void test_apache_parser() {
    printf(COLOR_BLUE "TEST 1: Apache Log Parser\n" COLOR_RESET);
    print_separator();
    
    const char* line = "192.168.1.100 - - [13/Feb/2024:10:23:45 +0000] "
                       "\"GET /api/users HTTP/1.1\" 200 1234 \"-\" \"Mozilla/5.0\"";
    
    ApacheLogEntry entry;
    
    if (parse_apache_log(line, &entry) == 0) {
        printf(COLOR_GREEN "✓ Parse bem-sucedido!\n" COLOR_RESET);
        printf("  IP:          %s\n", entry.ip);
        printf("  Método:      %s\n", entry.method);
        printf("  URL:         %s\n", entry.url);
        printf("  Status:      %d\n", entry.status_code);
        printf("  Tamanho:     %ld bytes\n", entry.response_size);
        printf("  HTTP Ver:    %s\n", entry.http_version);
    } else {
        printf(COLOR_RED "✗ Erro ao fazer parse!\n" COLOR_RESET);
    }
    printf("\n");
}

void test_json_parser() {
    printf(COLOR_BLUE "TEST 2: JSON Log Parser\n" COLOR_RESET);
    print_separator();
    
    const char* line = "{\"timestamp\":\"2024-02-13T10:23:45Z\","
                       "\"level\":\"ERROR\","
                       "\"service\":\"auth-api\","
                       "\"message\":\"Database connection failed\","
                       "\"metadata\":{\"ip\":\"10.0.1.50\",\"user_id\":12345}}";
    
    JSONLogEntry entry;
    
    if (parse_json_log(line, &entry) == 0) {
        printf(COLOR_GREEN "✓ Parse bem-sucedido!\n" COLOR_RESET);
        
        const char* level_names[] = {"DEBUG", "INFO", "WARN", "ERROR", "CRITICAL"};
        printf("  Level:       %s\n", level_names[entry.level]);
        printf("  Service:     %s\n", entry.service);
        printf("  Message:     %s\n", entry.message);
        printf("  IP:          %s\n", entry.ip);
        printf("  User ID:     %d\n", entry.user_id);
    } else {
        printf(COLOR_RED "✗ Erro ao fazer parse!\n" COLOR_RESET);
    }
    printf("\n");
}

void test_syslog_parser() {
    printf(COLOR_BLUE "TEST 3: Syslog Parser\n" COLOR_RESET);
    print_separator();
    
    const char* line = "<38>Feb 13 10:23:45 web-server sshd[12345]: "
                       "Failed password for root from 192.168.1.100 port 22 ssh2";
    
    SyslogEntry entry;
    
    if (parse_syslog(line, &entry) == 0) {
        printf(COLOR_GREEN "✓ Parse bem-sucedido!\n" COLOR_RESET);
        printf("  Priority:    %d\n", entry.priority);
        printf("  Hostname:    %s\n", entry.hostname);
        printf("  Service:     %s\n", entry.service);
        printf("  PID:         %d\n", entry.pid);
        printf("  Message:     %s\n", entry.message);
        printf("  Auth Fail:   %s\n", entry.is_auth_failure ? "SIM" : "NÃO");
    } else {
        printf(COLOR_RED "✗ Erro ao fazer parse!\n" COLOR_RESET);
    }
    printf("\n");
}

void test_file_processing() {
    printf(COLOR_BLUE "TEST 4: Processamento de Ficheiro\n" COLOR_RESET);
    print_separator();
    
    // Tentar abrir ficheiro de teste
    FILE* fp = fopen("../datasets/apache/sample_1k.log", "r");
    
    if (!fp) {
        printf(COLOR_RED "✗ Ficheiro datasets/apache/sample_1k.log não encontrado\n" COLOR_RESET);
        printf("  Execute primeiro: ./generators/gen_apache 1000 > datasets/apache/sample_1k.log\n\n");
        return;
    }
    
    char line[4096];
    ApacheLogEntry entry;
    
    int total_lines = 0;
    int parsed_ok = 0;
    int status_200 = 0;
    int status_404 = 0;
    int status_500 = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        total_lines++;
        
        if (parse_apache_log(line, &entry) == 0) {
            parsed_ok++;
            
            if (entry.status_code == 200) status_200++;
            else if (entry.status_code == 404) status_404++;
            else if (entry.status_code >= 500) status_500++;
        }
    }
    
    fclose(fp);
    
    printf(COLOR_GREEN "✓ Processamento concluído!\n" COLOR_RESET);
    printf("  Total linhas:     %d\n", total_lines);
    printf("  Parsed OK:        %d (%.1f%%)\n", parsed_ok, 
           (parsed_ok * 100.0) / total_lines);
    printf("  Status 200 (OK):  %d\n", status_200);
    printf("  Status 404:       %d\n", status_404);
    printf("  Status 5xx:       %d\n", status_500);
    printf("\n");
}

void test_performance() {
    printf(COLOR_BLUE "TEST 5: Performance Benchmark\n" COLOR_RESET);
    print_separator();
    
    const char* line = "192.168.1.100 - - [13/Feb/2024:10:23:45 +0000] "
                       "\"GET /api HTTP/1.1\" 200 1234 \"-\" \"Mozilla\"";
    
    ApacheLogEntry entry;
    int iterations = 100000;
    
    printf("  Parsing %d linhas...\n", iterations);
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < iterations; i++) {
        parse_apache_log(line, &entry);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double elapsed = (end.tv_sec - start.tv_sec) + 
                     (end.tv_nsec - start.tv_nsec) / 1e9;
    
    printf(COLOR_GREEN "✓ Benchmark concluído!\n" COLOR_RESET);
    printf("  Tempo total:      %.3f segundos\n", elapsed);
    printf("  Throughput:       %.0f linhas/segundo\n", iterations / elapsed);
    printf("  Tempo médio:      %.2f µs/linha\n", (elapsed * 1e6) / iterations);
    printf("\n");
}

void test_edge_cases() {
    printf(COLOR_BLUE "TEST 6: Casos Extremos\n" COLOR_RESET);
    print_separator();
    
    ApacheLogEntry entry;
    int tests_passed = 0;
    int tests_total = 0;
    
    // Teste 1: Linha vazia
    tests_total++;
    if (parse_apache_log("", &entry) != 0) {
        printf(COLOR_GREEN "  ✓ Linha vazia rejeitada\n" COLOR_RESET);
        tests_passed++;
    } else {
        printf(COLOR_RED "  ✗ Linha vazia aceite incorretamente\n" COLOR_RESET);
    }
    
    // Teste 2: NULL pointer
    tests_total++;
    if (parse_apache_log(NULL, &entry) != 0) {
        printf(COLOR_GREEN "  ✓ NULL pointer rejeitado\n" COLOR_RESET);
        tests_passed++;
    } else {
        printf(COLOR_RED "  ✗ NULL pointer aceite incorretamente\n" COLOR_RESET);
    }
    
    // Teste 3: Linha malformada
    tests_total++;
    if (parse_apache_log("isto nao e um log valido", &entry) != 0) {
        printf(COLOR_GREEN "  ✓ Linha malformada rejeitada\n" COLOR_RESET);
        tests_passed++;
    } else {
        printf(COLOR_RED "  ✗ Linha malformada aceite incorretamente\n" COLOR_RESET);
    }
    
    // Teste 4: Status code grande
    tests_total++;
    const char* line_valid = "192.168.1.100 - - [13/Feb/2024:10:23:45 +0000] "
                             "\"GET / HTTP/1.1\" 503 999999 \"-\" \"Mozilla\"";
    if (parse_apache_log(line_valid, &entry) == 0 && entry.status_code == 503) {
        printf(COLOR_GREEN "  ✓ Status 503 processado corretamente\n" COLOR_RESET);
        tests_passed++;
    } else {
        printf(COLOR_RED "  ✗ Status 503 processado incorretamente\n" COLOR_RESET);
    }
    
    printf("\n  Resultado: %d/%d testes passaram\n\n", tests_passed, tests_total);
}

int main() {
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║     LOG PARSER - SUITE DE TESTES       ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("\n");
    
    test_apache_parser();
    test_json_parser();
    test_syslog_parser();
    test_file_processing();
    test_performance();
    test_edge_cases();
    
    printf("╔════════════════════════════════════════╗\n");
    printf("║         TESTES CONCLUÍDOS              ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("\n");
    
    return 0;
}
