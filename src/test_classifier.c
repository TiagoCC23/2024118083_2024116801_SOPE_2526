// test_classifier.c - Teste do classificador
#include "log_parser.h"
#include "event_classifier.h"
#include <stdio.h>

#define COLOR_RED "\033[0;31m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_BLUE "\033[0;34m"
#define COLOR_RESET "\033[0m"

const char* get_severity_color(int severity) {
    switch (severity) {
        case 4: return COLOR_RED;      // CRITICAL
        case 3: return COLOR_RED;      // HIGH
        case 2: return COLOR_YELLOW;   // MEDIUM
        case 1: return COLOR_GREEN;    // LOW
        default: return COLOR_RESET;   // INFO
    }
}

void print_event(const ClassifiedEvent* event, AnalysisMode mode) {
    // Filtrar por modo
    if (!event_matches_mode(event, mode)) {
        return;
    }
    
    const char* color = get_severity_color(event->severity);
    
    printf("%s[%s] %s - %s%s\n",
           color,
           get_severity_name(event->severity),
           get_event_type_name(event->event_types),
           event->description,
           COLOR_RESET);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <ficheiro.log> <modo>\n", argv[0]);
        fprintf(stderr, "Modos: security | performance | traffic | full\n");
        return 1;
    }
    
    const char* filename = argv[1];
    const char* mode_str = argv[2];
    
    // Determinar modo
    AnalysisMode mode;
    if (strcmp(mode_str, "security") == 0) {
        mode = MODE_SECURITY;
    } else if (strcmp(mode_str, "performance") == 0) {
        mode = MODE_PERFORMANCE;
    } else if (strcmp(mode_str, "traffic") == 0) {
        mode = MODE_TRAFFIC;
    } else if (strcmp(mode_str, "full") == 0) {
        mode = MODE_FULL;
    } else {
        fprintf(stderr, "Modo inválido: %s\n", mode_str);
        return 1;
    }
    
    printf("Analisando %s em modo: %s\n\n", filename, mode_str);
    
    // Abrir ficheiro
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        return 1;
    }
    
    char line[4096];
    int total_lines = 0;
    int matched_events = 0;
    
    // Contadores por severidade
    int severity_counts[5] = {0};
    
    // Processar linha a linha
    while (fgets(line, sizeof(line), fp)) {
        total_lines++;
        
        ClassifiedEvent event;
        
        // Tentar parsear como Apache
        ApacheLogEntry apache_entry;
        if (parse_apache_log(line, &apache_entry) == 0) {
            classify_apache_event(&apache_entry, &event);
            
            if (event_matches_mode(&event, mode)) {
                print_event(&event, mode);
                matched_events++;
                severity_counts[event.severity]++;
            }
            continue;
        }
        
        // Tentar parsear como JSON
        JSONLogEntry json_entry;
        if (parse_json_log(line, &json_entry) == 0) {
            classify_json_event(&json_entry, &event);
            
            if (event_matches_mode(&event, mode)) {
                print_event(&event, mode);
                matched_events++;
                severity_counts[event.severity]++;
            }
            continue;
        }
        
        // Tentar parsear como Syslog
        SyslogEntry syslog_entry;
        if (parse_syslog(line, &syslog_entry) == 0) {
            classify_syslog_event(&syslog_entry, &event);
            
            if (event_matches_mode(&event, mode)) {
                print_event(&event, mode);
                matched_events++;
                severity_counts[event.severity]++;
            }
            continue;
        }
    }
    
    fclose(fp);
    
    // Sumário
    printf("\n");
    printf("========================================\n");
    printf("SUMÁRIO\n");
    printf("========================================\n");
    printf("Total de linhas:      %d\n", total_lines);
    printf("Eventos relevantes:   %d\n", matched_events);
    printf("\n");
    printf("Por severidade:\n");
    printf("  CRITICAL: %d\n", severity_counts[4]);
    printf("  HIGH:     %d\n", severity_counts[3]);
    printf("  MEDIUM:   %d\n", severity_counts[2]);
    printf("  LOW:      %d\n", severity_counts[1]);
    printf("  INFO:     %d\n", severity_counts[0]);
    
    return 0;
}
