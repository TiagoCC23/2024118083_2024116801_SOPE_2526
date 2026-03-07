# Guia de Classificação de Eventos de Log

## Índice
1. [Visão Geral](#visão-geral)
2. [Categorias de Eventos](#categorias-de-eventos)
3. [Níveis de Severidade](#níveis-de-severidade)
4. [Apache Combined Logs](#apache-combined-logs)
5. [JSON Structured Logs](#json-structured-logs)
6. [Syslog (RFC 3164)](#syslog-rfc-3164)
7. [Nginx Error Logs](#nginx-error-logs)
8. [Exemplos Práticos](#exemplos-práticos)
9. [Tabela de Referência Rápida](#tabela-de-referência-rápida)

---

## Visão Geral

Este documento descreve como os eventos de log são classificados nas seguintes categorias:

- **SECURITY**: Eventos relacionados com segurança, autenticação, autorização
- **PERFORMANCE**: Eventos que afetam a performance do sistema
- **TRAFFIC**: Eventos relacionados com padrões de tráfego e utilização
- **FULL**: Todos os eventos (combinação de todas as categorias)

---

## Categorias de Eventos

### 🔐 SECURITY (Segurança)

**Objetivo**: Identificar tentativas de intrusão, ataques, acessos não autorizados

**Indicadores**:
- Tentativas de autenticação falhadas
- Padrões de ataque (SQL Injection, XSS, Path Traversal)
- Acessos negados (401, 403)
- Scanners e bots maliciosos
- Violações de política de segurança

---

### ⚡ PERFORMANCE (Desempenho)

**Objetivo**: Identificar problemas que afetam a disponibilidade e velocidade do sistema

**Indicadores**:
- Timeouts e erros de conexão
- Respostas lentas ou muito grandes
- Erros de servidor (5xx)
- Esgotamento de recursos (memória, CPU, conexões)
- Crashes e falhas de serviço

---

### 📊 TRAFFIC (Tráfego)

**Objetivo**: Analisar padrões de utilização e comportamento de utilizadores

**Indicadores**:
- Todos os pedidos HTTP
- Volumes de dados transferidos
- Padrões de acesso temporal
- Rate limiting
- Distribuição de métodos HTTP

---

## Níveis de Severidade

| Nível | Valor | Descrição | Ação Requerida |
|-------|-------|-----------|----------------|
| **INFO** | 0 | Informativo, operação normal | Nenhuma |
| **LOW** | 1 | Anomalia menor, monitorizar | Investigação opcional |
| **MEDIUM** | 2 | Problema que requer atenção | Investigar em breve |
| **HIGH** | 3 | Problema sério, ação necessária | Investigar imediatamente |
| **CRITICAL** | 4 | Falha crítica, sistema comprometido | Ação urgente |

---

## Apache Combined Logs

### Formato Geral
```
<IP> - - [<timestamp>] "<método> <URL> HTTP/<versão>" <status> <bytes> "<referer>" "<user-agent>"
```

### Classificação por Status Code

#### Status 2xx (Sucesso)

| Código | Nome | Categorias | Severidade | Descrição |
|--------|------|------------|------------|-----------|
| 200 | OK | TRAFFIC | INFO | Pedido bem-sucedido |
| 201 | Created | TRAFFIC | INFO | Recurso criado com sucesso |
| 204 | No Content | TRAFFIC | INFO | Sucesso sem conteúdo de resposta |

**Exemplo**:
```
192.168.1.100 - - [15/Feb/2024:10:23:45 +0000] "GET /api/users HTTP/1.1" 200 1234 "-" "Mozilla/5.0"
```
- **Classificação**: TRAFFIC
- **Severidade**: INFO
- **Explicação**: Pedido GET normal, resposta bem-sucedida

---

#### Status 3xx (Redirecionamento)

| Código | Nome | Categorias | Severidade | Descrição |
|--------|------|------------|-----------|-----------|
| 301 | Moved Permanently | TRAFFIC | INFO | Redirecionamento permanente |
| 302 | Found | TRAFFIC | INFO | Redirecionamento temporário |
| 304 | Not Modified | TRAFFIC | INFO | Conteúdo não modificado (cache) |

**Exemplo**:
```
10.0.0.50 - - [15/Feb/2024:10:25:12 +0000] "GET /old-page HTTP/1.1" 301 0 "-" "curl/7.68.0"
```
- **Classificação**: TRAFFIC
- **Severidade**: INFO
- **Explicação**: Redirecionamento configurado, comportamento normal

---

#### Status 4xx (Erro do Cliente)

| Código | Nome | Categorias | Severidade | Descrição |
|--------|------|------------|-----------|-----------|
| 400 | Bad Request | TRAFFIC | LOW | Pedido malformado |
| 401 | Unauthorized | **SECURITY** + TRAFFIC | **MEDIUM** | Autenticação necessária |
| 403 | Forbidden | **SECURITY** + TRAFFIC | **MEDIUM** | Acesso negado |
| 404 | Not Found | TRAFFIC | LOW | Recurso não encontrado |
| 429 | Too Many Requests | **SECURITY** + TRAFFIC | MEDIUM | Rate limit excedido |

**Exemplos**:
```
192.168.1.100 - - [15/Feb/2024:10:30:00 +0000] "GET /admin HTTP/1.1" 401 1234 "-" "Mozilla/5.0"
```
- **Classificação**: SECURITY + TRAFFIC
- **Severidade**: MEDIUM
- **Explicação**: Tentativa de acesso a área restrita sem autenticação
```
192.168.1.101 - - [15/Feb/2024:10:31:00 +0000] "GET /secret.txt HTTP/1.1" 403 560 "-" "Mozilla/5.0"
```
- **Classificação**: SECURITY + TRAFFIC
- **Severidade**: MEDIUM
- **Explicação**: Acesso negado, possível tentativa de acesso não autorizado
```
10.0.0.25 - - [15/Feb/2024:10:32:00 +0000] "GET /api/search?q=test HTTP/1.1" 429 234 "-" "Python-requests/2.28"
```
- **Classificação**: SECURITY + TRAFFIC
- **Severidade**: MEDIUM
- **Explicação**: Rate limit atingido, possível scraping ou ataque automatizado

---

#### Status 5xx (Erro do Servidor)

| Código | Nome | Categorias | Severidade | Descrição |
|--------|------|------------|-----------|-----------|
| 500 | Internal Server Error | **PERFORMANCE** + ERROR + TRAFFIC | **HIGH** | Erro interno do servidor |
| 502 | Bad Gateway | **PERFORMANCE** + ERROR + TRAFFIC | **HIGH** | Erro no upstream/backend |
| 503 | Service Unavailable | **PERFORMANCE** + ERROR + TRAFFIC | **CRITICAL** | Serviço indisponível (overload) |
| 504 | Gateway Timeout | **PERFORMANCE** + ERROR + TRAFFIC | **HIGH** | Timeout no upstream |

**Exemplos**:
```
192.168.1.100 - - [15/Feb/2024:10:40:00 +0000] "POST /api/orders HTTP/1.1" 500 1234 "-" "Mozilla/5.0"
```
- **Classificação**: PERFORMANCE + ERROR + TRAFFIC
- **Severidade**: HIGH
- **Explicação**: Erro interno, possível bug ou problema de código
```
10.0.1.50 - - [15/Feb/2024:10:41:00 +0000] "GET /api/products HTTP/1.1" 503 234 "-" "curl/7.68.0"
```
- **Classificação**: PERFORMANCE + ERROR + TRAFFIC
- **Severidade**: CRITICAL
- **Explicação**: Serviço indisponível, possível sobrecarga ou manutenção

---

### Classificação por Padrões de URL

#### SQL Injection

**Padrões detectados**:
- `' OR '1'='1`
- `UNION SELECT`
- `DROP TABLE`
- `admin'--`
- `1=1`

**Exemplo**:
```
203.0.113.45 - - [15/Feb/2024:11:00:00 +0000] "GET /api/search?q=' OR '1'='1 HTTP/1.1" 200 5678 "-" "Mozilla/5.0"
```
- **Classificação**: **SECURITY** + TRAFFIC
- **Severidade**: **CRITICAL**
- **Explicação**: Tentativa de SQL Injection, ataque ativo detectado

---

#### Cross-Site Scripting (XSS)

**Padrões detectados**:
- `<script>`
- `javascript:`
- `onerror=`
- `onload=`
- `<img src=x onerror=`

**Exemplo**:
```
198.51.100.23 - - [15/Feb/2024:11:05:00 +0000] "GET /search?q=<script>alert(1)</script> HTTP/1.1" 200 1234 "-" "Mozilla/5.0"
```
- **Classificação**: **SECURITY** + TRAFFIC
- **Severidade**: **HIGH**
- **Explicação**: Tentativa de XSS, injeção de código JavaScript malicioso

---

#### Path Traversal

**Padrões detectados**:
- `../`
- `..\`
- `....//`
- `..%2F`

**Exemplo**:
```
93.184.216.34 - - [15/Feb/2024:11:10:00 +0000] "GET /../../etc/passwd HTTP/1.1" 404 234 "-" "curl/7.68.0"
```
- **Classificação**: **SECURITY** + TRAFFIC
- **Severidade**: **HIGH**
- **Explicação**: Tentativa de path traversal, acesso a ficheiros do sistema

---

### Classificação por User-Agent

#### Scanners e Bots Maliciosos

**User-Agents suspeitos**:
- `nikto`
- `sqlmap`
- `nmap`
- `masscan`
- `dirbuster`
- `w3af`
- `acunetix`

**Exemplo**:
```
185.220.100.240 - - [15/Feb/2024:11:15:00 +0000] "GET / HTTP/1.1" 200 1234 "-" "nikto/2.1.6"
```
- **Classificação**: **SECURITY** + TRAFFIC
- **Severidade**: **HIGH**
- **Explicação**: Scanner de vulnerabilidades detectado, reconhecimento ativo
```
45.33.32.156 - - [15/Feb/2024:11:16:00 +0000] "GET /admin HTTP/1.1" 401 234 "-" "sqlmap/1.4.7"
```
- **Classificação**: **SECURITY** + TRAFFIC
- **Severidade**: **CRITICAL**
- **Explicação**: Ferramenta de exploração SQL, ataque automatizado

---

### Classificação por Tamanho de Resposta

#### Respostas Muito Grandes

**Threshold**: > 10 MB

**Exemplo**:
```
192.168.1.100 - - [15/Feb/2024:11:20:00 +0000] "GET /api/export/all HTTP/1.1" 200 15728640 "-" "Mozilla/5.0"
```
- **Classificação**: **PERFORMANCE** + TRAFFIC
- **Severidade**: MEDIUM
- **Explicação**: Resposta muito grande (15 MB), possível impacto em performance ou exfiltração de dados

---

### Classificação por Método HTTP

#### Métodos Modificadores (POST, PUT, DELETE)

**Exemplo**:
```
10.0.0.50 - - [15/Feb/2024:11:25:00 +0000] "POST /api/users HTTP/1.1" 201 567 "-" "Mozilla/5.0"
```
- **Classificação**: TRAFFIC
- **Severidade**: INFO
- **Explicação**: Operação de criação, tráfego normal mas importante para auditoria
```
10.0.0.51 - - [15/Feb/2024:11:26:00 +0000] "DELETE /api/users/123 HTTP/1.1" 204 0 "-" "curl/7.68.0"
```
- **Classificação**: TRAFFIC
- **Severidade**: INFO (ou MEDIUM se não autorizado)
- **Explicação**: Operação destrutiva, importante para auditoria

---

## JSON Structured Logs

### Formato Geral
```json
{
  "timestamp": "2024-02-15T10:23:45Z",
  "level": "ERROR|WARN|INFO|DEBUG|CRITICAL",
  "service": "nome-do-servico",
  "message": "mensagem descritiva",
  "metadata": {
    "ip": "10.0.1.50",
    "user_id": 12345
  }
}
```

### Classificação por Log Level

| Level | Categorias | Severidade | Descrição |
|-------|------------|------------|-----------|
| DEBUG | (variável) | INFO (0) | Informação de debug |
| INFO | (variável) | LOW (1) | Informação geral |
| WARN/WARNING | (variável) | MEDIUM (2) | Aviso, requer atenção |
| ERROR | ERROR + (variável) | HIGH (3) | Erro que requer correção |
| CRITICAL/FATAL | ERROR + (variável) | CRITICAL (4) | Erro crítico, sistema comprometido |

---

### Classificação por Mensagem - SECURITY

#### Autenticação Falhada

**Padrões detectados**:
- `authentication failed`
- `invalid credentials`
- `unauthorized`
- `access denied`
- `permission denied`

**Exemplos**:
```json
{
  "timestamp": "2024-02-15T10:30:00Z",
  "level": "ERROR",
  "service": "auth-api",
  "message": "authentication failed for user: admin",
  "metadata": {"ip": "203.0.113.45", "user_id": null}
}
```
- **Classificação**: **SECURITY** + ERROR
- **Severidade**: **HIGH**
- **Explicação**: Tentativa de login falhada, possível ataque de força bruta

---
```json
{
  "timestamp": "2024-02-15T10:31:00Z",
  "level": "CRITICAL",
  "service": "auth-api",
  "message": "Multiple failed authentication attempts detected from same IP",
  "metadata": {"ip": "203.0.113.45", "attempts": 15}
}
```
- **Classificação**: **SECURITY** + ERROR
- **Severidade**: **CRITICAL**
- **Explicação**: Múltiplas tentativas falhadas, ataque de força bruta confirmado

---

#### Rate Limiting
```json
{
  "timestamp": "2024-02-15T10:35:00Z",
  "level": "WARN",
  "service": "api-gateway",
  "message": "rate limit exceeded for user 12345",
  "metadata": {"ip": "10.0.1.50", "user_id": 12345, "limit": 1000}
}
```
- **Classificação**: **SECURITY** + **TRAFFIC**
- **Severidade**: MEDIUM
- **Explicação**: Rate limit excedido, possível abuso ou scraping

---

### Classificação por Mensagem - PERFORMANCE

#### Timeouts

**Padrões detectados**:
- `timeout`
- `timed out`
- `connection timeout`

**Exemplo**:
```json
{
  "timestamp": "2024-02-15T10:40:00Z",
  "level": "ERROR",
  "service": "payment-gateway",
  "message": "Database connection timeout after 30s",
  "metadata": {"ip": "10.0.1.100", "query": "SELECT * FROM orders"}
}
```
- **Classificação**: **PERFORMANCE** + ERROR
- **Severidade**: HIGH
- **Explicação**: Timeout de base de dados, impacto em performance

---

#### Slow Queries

**Padrões detectados**:
- `slow query`
- `high latency`
- `took XXXms`

**Exemplo**:
```json
{
  "timestamp": "2024-02-15T10:45:00Z",
  "level": "WARN",
  "service": "database-proxy",
  "message": "Slow query detected: took 5234ms",
  "metadata": {"query": "SELECT * FROM users WHERE...", "duration_ms": 5234}
}
```
- **Classificação**: **PERFORMANCE**
- **Severidade**: MEDIUM
- **Explicação**: Query lenta, possível falta de índices ou problema de otimização

---

#### Out of Memory

**Padrões detectados**:
- `out of memory`
- `OOM`
- `memory exhausted`

**Exemplo**:
```json
{
  "timestamp": "2024-02-15T10:50:00Z",
  "level": "CRITICAL",
  "service": "analytics-engine",
  "message": "Out of memory allocating 2GB buffer",
  "metadata": {"requested_bytes": 2147483648, "available_bytes": 524288000}
}
```
- **Classificação**: **PERFORMANCE** + ERROR
- **Severidade**: **CRITICAL**
- **Explicação**: Memória esgotada, serviço pode crashar

---

#### Connection Pool Exhausted
```json
{
  "timestamp": "2024-02-15T10:55:00Z",
  "level": "ERROR",
  "service": "database-proxy",
  "message": "Connection pool exhausted: all 100 connections in use",
  "metadata": {"pool_size": 100, "waiting_requests": 45}
}
```
- **Classificação**: **PERFORMANCE** + ERROR
- **Severidade**: **CRITICAL**
- **Explicação**: Pool de conexões esgotado, possível sobrecarga ou connection leak

---

### Classificação por Service Name

#### Serviços de Segurança

**Services**:
- `auth-api`
- `auth-service`
- `security-*`
- `firewall-*`

**Exemplo**:
```json
{
  "timestamp": "2024-02-15T11:00:00Z",
  "level": "INFO",
  "service": "auth-service",
  "message": "User login successful",
  "metadata": {"ip": "10.0.0.50", "user_id": 12345}
}
```
- **Classificação**: **SECURITY** + TRAFFIC
- **Severidade**: INFO
- **Explicação**: Login bem-sucedido em serviço de autenticação

---

#### Serviços de Database/Cache

**Services**:
- `database-*`
- `cache-*`
- `redis-*`
- `postgres-*`
- `mysql-*`

**Exemplo**:
```json
{
  "timestamp": "2024-02-15T11:05:00Z",
  "level": "ERROR",
  "service": "redis-cache",
  "message": "Cache miss rate exceeded threshold: 85%",
  "metadata": {"miss_rate": 0.85, "threshold": 0.70}
}
```
- **Classificação**: **PERFORMANCE**
- **Severidade**: MEDIUM
- **Explicação**: Taxa de cache miss alta, impacto em performance

---

#### Serviços de API/Gateway

**Services**:
- `*-api`
- `api-gateway`
- `proxy-*`

**Exemplo**:
```json
{
  "timestamp": "2024-02-15T11:10:00Z",
  "level": "INFO",
  "service": "api-gateway",
  "message": "Request processed successfully",
  "metadata": {"ip": "10.0.0.50", "path": "/api/users", "duration_ms": 45}
}
```
- **Classificação**: **TRAFFIC**
- **Severidade**: INFO
- **Explicação**: Pedido processado, análise de tráfego

---

## Syslog (RFC 3164)

### Formato Geral
```
<priority>timestamp hostname service[pid]: message
```

**Priority**: `facility * 8 + severity`
- Facility: tipo de sistema (0=kernel, 4=auth, 16=local0, etc.)
- Severity: 0=emerg, 1=alert, 2=crit, 3=err, 4=warning, 5=notice, 6=info, 7=debug

---

### Mapeamento de Severidade Syslog

| Syslog Severity | Nome | Nossa Severidade | Descrição |
|-----------------|------|------------------|-----------|
| 0 | Emergency | CRITICAL (4) | Sistema inutilizável |
| 1 | Alert | CRITICAL (4) | Ação imediata necessária |
| 2 | Critical | HIGH (3) | Condição crítica |
| 3 | Error | HIGH (3) | Condição de erro |
| 4 | Warning | MEDIUM (2) | Condição de aviso |
| 5 | Notice | LOW (1) | Condição normal mas significativa |
| 6 | Informational | LOW (1) | Mensagem informativa |
| 7 | Debug | INFO (0) | Mensagem de debug |

---

### Classificação por Service - SECURITY

#### SSH (sshd)

**Exemplos**:
```
<38>Feb 15 10:30:00 web-server sshd[12345]: Accepted publickey for admin from 192.168.1.100 port 55234 ssh2
```
- **Classificação**: **SECURITY** + TRAFFIC
- **Severidade**: INFO
- **Explicação**: Login SSH bem-sucedido com chave pública

---
```
<38>Feb 15 10:31:00 web-server sshd[12346]: Failed password for root from 203.0.113.45 port 22334 ssh2
```
- **Classificação**: **SECURITY**
- **Severidade**: **CRITICAL**
- **Explicação**: Tentativa de login como root falhada, possível ataque de força bruta

---
```
<38>Feb 15 10:32:00 web-server sshd[12347]: Failed password for invalid user admin from 198.51.100.88 port 44556 ssh2
```
- **Classificação**: **SECURITY**
- **Severidade**: **CRITICAL**
- **Explicação**: Tentativa de login com utilizador inválido, reconhecimento ativo

---
```
<38>Feb 15 10:33:00 web-server sshd[12348]: Connection closed by 185.220.100.240 port 33445 [preauth]
```
- **Classificação**: **SECURITY**
- **Severidade**: MEDIUM
- **Explicação**: Conexão fechada antes de autenticação, possível scanning

---

#### Sudo

**Exemplos**:
```
<44>Feb 15 10:35:00 web-server sudo[23456]: admin : TTY=pts/0 ; PWD=/home/admin ; USER=root ; COMMAND=/bin/ls
```
- **Classificação**: **SECURITY**
- **Severidade**: INFO
- **Explicação**: Comando sudo executado com sucesso, auditoria normal

---
```
<44>Feb 15 10:36:00 web-server sudo[23457]: pam_unix(sudo:auth): authentication failure; user=hacker
```
- **Classificação**: **SECURITY**
- **Severidade**: **CRITICAL**
- **Explicação**: Tentativa sudo falhada, possível tentativa de escalação de privilégios

---

#### Firewall (kernel/iptables)

**Exemplos**:
```
<36>Feb 15 10:40:00 firewall kernel[45678]: iptables: IN=eth0 OUT= SRC=203.0.113.45 DST=10.0.0.1 PROTO=TCP DPT=22 REJECT
```
- **Classificação**: **SECURITY** + TRAFFIC
- **Severidade**: MEDIUM
- **Explicação**: Tentativa de conexão SSH bloqueada por firewall

---
```
<36>Feb 15 10:41:00 firewall kernel[45679]: iptables: IN=eth0 OUT= SRC=198.51.100.88 DST=10.0.0.1 PROTO=TCP DPT=3389 DROP
```
- **Classificação**: **SECURITY** + TRAFFIC
- **Severidade**: MEDIUM
- **Explicação**: Tentativa de RDP bloqueada, scanning de portas

---

### Classificação por Service - PERFORMANCE

#### Kernel Panics
```
<0>Feb 15 11:00:00 web-server kernel[1234]: Kernel panic - not syncing: Fatal exception
```
- **Classificação**: **PERFORMANCE** + ERROR
- **Severidade**: **CRITICAL**
- **Explicação**: Kernel panic, sistema crashou

---

#### Out of Memory (OOM Killer)
```
<2>Feb 15 11:05:00 db-server kernel[2345]: Out of memory: Kill process 12345 (java) score 850 or sacrifice child
```
- **Classificação**: **PERFORMANCE** + ERROR
- **Severidade**: **CRITICAL**
- **Explicação**: OOM killer ativado, memória esgotada

---

#### Service Crashes
```
<3>Feb 15 11:10:00 app-server systemd[1]: apache2.service: Main process exited, code=killed, status=11/SEGV
```
- **Classificação**: **PERFORMANCE** + ERROR
- **Severidade**: **CRITICAL**
- **Explicação**: Apache crashou com segmentation fault

---
```
<3>Feb 15 11:11:00 app-server kernel[5678]: nginx[9876]: segfault at 7f8a4c0 ip 00007f8a4c0 sp 00007fff error 4 in nginx[400000+6000]
```
- **Classificação**: **PERFORMANCE** + ERROR
- **Severidade**: **CRITICAL**
- **Explicação**: Nginx sofreu segmentation fault

---

### Classificação por Service - TRAFFIC

#### Web Servers (nginx, apache)
```
<38>Feb 15 11:15:00 web-server nginx[34567]: limiting requests, excess: 5.123 by zone "req_limit_per_ip", client: 10.0.0.50
```
- **Classificação**: **PERFORMANCE** + **TRAFFIC**
- **Severidade**: MEDIUM
- **Explicação**: Rate limiting ativo, possível scraping

---
```
<38>Feb 15 11:16:00 web-server apache2[45678]: [client 192.168.1.100] client denied by server configuration: /var/www/html/admin
```
- **Classificação**: **SECURITY** + TRAFFIC
- **Severidade**: MEDIUM
- **Explicação**: Acesso negado por configuração, tentativa de acesso a área restrita

---

#### System Services
```
<46>Feb 15 11:20:00 web-server systemd[1]: Started User Manager for UID 1000
```
- **Classificação**: TRAFFIC
- **Severidade**: INFO
- **Explicação**: Serviço iniciado, operação normal

---
```
<46>Feb 15 11:21:00 web-server cron[56789]: pam_unix(cron:session): session opened for user root by (uid=0)
```
- **Classificação**: TRAFFIC
- **Severidade**: INFO
- **Explicação**: Tarefa cron executada, operação normal

---

## Nginx Error Logs

### Formato Geral
```
YYYY/MM/DD HH:MM:SS [level] pid#tid: *connection_id message, client: IP, server: hostname, request: "METHOD /path HTTP/version"
```

### Classificação por Level

| Nginx Level | Nossa Severidade | Descrição |
|-------------|------------------|-----------|
| emerg | CRITICAL (4) | Emergência, sistema inutilizável |
| alert | CRITICAL (4) | Alerta, ação imediata necessária |
| crit | HIGH (3) | Condição crítica |
| error | HIGH (3) | Erro |
| warn | MEDIUM (2) | Aviso |
| notice | LOW (1) | Aviso normal |
| info | LOW (1) | Informação |
| debug | INFO (0) | Debug |

---

### Classificação por Tipo de Erro - PERFORMANCE

#### Connection Errors
```
2024/02/15 11:00:00 [error] 12345#0: *1234 connect() failed (111: Connection refused) while connecting to upstream, client: 192.168.1.100, server: example.com, request: "GET /api HTTP/1.1"
```
- **Classificação**: **PERFORMANCE** + TRAFFIC
- **Severidade**: **HIGH**
- **Explicação**: Erro ao conectar ao backend, serviço indisponível

---
```
2024/02/15 11:01:00 [error] 12345#0: *1235 upstream timed out (110: Connection timed out) while reading response header from upstream, client: 10.0.0.50, server: example.com
```
- **Classificação**: **PERFORMANCE** + TRAFFIC
- **Severidade**: **HIGH**
- **Explicação**: Timeout no upstream, backend lento ou sobrecarregado

---
```
2024/02/15 11:02:00 [error] 12345#0: *1236 recv() failed (104: Connection reset by peer) while reading response header from upstream, client: 192.168.1.101, server: example.com
```
- **Classificação**: **PERFORMANCE** + TRAFFIC
- **Severidade**: **HIGH**
- **Explicação**: Conexão resetada pelo backend, possível crash

---
```
2024/02/15 11:03:00 [crit] 12345#0: *1237 no live upstreams while connecting to upstream, client: 10.0.1.50, server: example.com
```
- **Classificação**: **PERFORMANCE** + TRAFFIC
- **Severidade**: **CRITICAL**
- **Explicação**: Nenhum backend disponível, serviço totalmente indisponível

---

#### SSL/TLS Errors
```
2024/02/15 11:05:00 [error] 12345#0: *1238 SSL_do_handshake() failed (SSL: error:14094410:SSL routines:ssl3_read_bytes:sslv3 alert handshake failure) while SSL handshaking, client: 192.168.1.100, server: 0.0.0.0:443
```
- **Classificação**: **SECURITY** + TRAFFIC
- **Severidade**: MEDIUM
- **Explicação**: Falha no handshake SSL, possível cliente antigo ou ataque

---

#### Rate Limiting
```
2024/02/15 11:10:00 [warn] 12345#0: *1239 limiting requests, excess: 5.123 by zone "req_limit_per_ip", client: 203.0.113.45, server: example.com
```
- **Classificação**: **PERFORMANCE** + **TRAFFIC** + SECURITY
- **Severidade**: MEDIUM
- **Explicação**: Rate limit excedido, possível abuso ou ataque DoS

---

#### Request Too Large
```
2024/02/15 11:15:00 [error] 12345#0: *1240 client intended to send too large body: 11534336 bytes, client: 10.0.0.50, server: example.com, request: "POST /api/upload HTTP/1.1"
```
- **Classificação**: **PERFORMANCE** + TRAFFIC
- **Severidade**: MEDIUM
- **Explicação**: Upload muito grande, possível ataque ou configuração incorreta

---
```
2024/02/15 11:16:00 [error] 12345#0: *1241 upstream sent too big header while reading response header from upstream, client: 192.168.1.100, server: example.com
```
- **Classificação**: **PERFORMANCE** + TRAFFIC
- **Severidade**: MEDIUM
- **Explicação**: Header do backend muito grande, possível problema de configuração

---

### Classificação por Tipo de Erro - SECURITY

#### Access Forbidden
```
2024/02/15 11:20:00 [error] 12345#0: *1242 access forbidden by rule, client: 203.0.113.45, server: example.com, request: "GET /admin HTTP/1.1"
```
- **Classificação**: **SECURITY** + TRAFFIC
- **Severidade**: MEDIUM
- **Explicação**: Acesso bloqueado por regra, tentativa de acesso não autorizado

---
```
2024/02/15 11:21:00 [error] 12345#0: *1243 directory index of "/var/www/html/" is forbidden, client: 198.51.100.88, server: example.com
```
- **Classificação**: **SECURITY** + TRAFFIC
- **Severidade**: MEDIUM
- **Explicação**: Tentativa de listagem de diretórios, reconhecimento

---

#### File Not Found (pode indicar scanning)
```
2024/02/15 11:25:00 [error] 12345#0: *1244 open() "/var/www/html/phpMyAdmin/index.php" failed (2: No such file or directory), client: 185.220.100.240, server: example.com
```
- **Classificação**: SECURITY + TRAFFIC
- **Severidade**: MEDIUM
- **Explicação**: Procura por phpMyAdmin, scanning de aplicações vulneráveis conhecidas

---

## Exemplos Práticos

### Cenário 1: Ataque de Força Bruta SSH

**Sequência de eventos**:
```
<38>Feb 15 14:00:00 web-server sshd[10001]: Failed password for root from 203.0.113.45 port 55001 ssh2
<38>Feb 15 14:00:05 web-server sshd[10002]: Failed password for root from 203.0.113.45 port 55002 ssh2
<38>Feb 15 14:00:10 web-server sshd[10003]: Failed password for root from 203.0.113.45 port 55003 ssh2
<38>Feb 15 14:00:15 web-server sshd[10004]: Failed password for admin from 203.0.113.45 port 55004 ssh2
<38>Feb 15 14:00:20 web-server sshd[10005]: Failed password for admin from 203.0.113.45 port 55005 ssh2
```

**Análise**:
- **Classificação**: SECURITY
- **Severidade**: CRITICAL
- **Padrão detectado**: 5+ tentativas falhadas em 20 segundos do mesmo IP
- **Ação recomendada**: Bloquear IP 203.0.113.45 no firewall

---

### Cenário 2: SQL Injection Seguida de Exfiltração de Dados

**Sequência de eventos**:
```
192.168.1.100 - - [15/Feb/2024:14:10:00 +0000] "GET /api/search?q=' OR '1'='1 HTTP/1.1" 200 45678 "-" "sqlmap/1.4.7"
192.168.1.100 - - [15/Feb/2024:14:10:05 +0000] "GET /api/users?id=1 UNION SELECT * FROM passwords HTTP/1.1" 200 234567 "-" "sqlmap/1.4.7"
192.168.1.100 - - [15/Feb/2024:14:10:10 +0000] "GET /api/export?table=users HTTP/1.1" 200 15728640 "-" "curl/7.68.0"
```

**Análise**:
- **Evento 1**: SQL Injection detectado (CRITICAL)
- **Evento 2**: Tentativa de união de tabelas (CRITICAL)
- **Evento 3**: Exportação de 15MB de dados (HIGH - possível exfiltração)
- **Ação recomendada**: Bloquear IP imediatamente, auditar acesso à base de dados

---

### Cenário 3: Problema de Performance Escalando

**Sequência de eventos**:
```json
{"timestamp":"2024-02-15T14:20:00Z","level":"WARN","service":"database-proxy","message":"Slow query: took 1234ms"}
{"timestamp":"2024-02-15T14:20:30Z","level":"WARN","service":"database-proxy","message":"Slow query: took 2345ms"}
{"timestamp":"2024-02-15T14:21:00Z","level":"ERROR","service":"database-proxy","message":"Connection pool exhausted: 100/100 in use"}
{"timestamp":"2024-02-15T14:21:15Z","level":"CRITICAL","service":"database-proxy","message":"Database connection timeout after 30s"}
```
```
2024/02/15 14:21:30 [error] 12345#0: *5000 upstream timed out while reading response header from upstream
2024/02/15 14:21:35 [crit] 12345#0: *5001 no live upstreams while connecting to upstream
```

**Análise**:
- **Classificação**: PERFORMANCE
- **Progressão**: WARN → ERROR → CRITICAL
- **Problema**: Queries lentas → Pool esgotado → Timeouts → Serviço indisponível
- **Ação recomendada**: Escalar base de dados, investigar queries lentas, aumentar pool

---

### Cenário 4: Scan de Vulnerabilidades

**Sequência de eventos**:
```
198.51.100.88 - - [15/Feb/2024:14:30:00 +0000] "GET / HTTP/1.1" 200 1234 "-" "nikto/2.1.6"
198.51.100.88 - - [15/Feb/2024:14:30:01 +0000] "GET /admin HTTP/1.1" 404 234 "-" "nikto/2.1.6"
198.51.100.88 - - [15/Feb/2024:14:30:02 +0000] "GET /phpMyAdmin HTTP/1.1" 404 234 "-" "nikto/2.1.6"
198.51.100.88 - - [15/Feb/2024:14:30:03 +0000] "GET /wp-admin HTTP/1.1" 404 234 "-" "nikto/2.1.6"
198.51.100.88 - - [15/Feb/2024:14:30:04 +0000] "GET /.git/config HTTP/1.1" 404 234 "-" "nikto/2.1.6"
198.51.100.88 - - [15/Feb/2024:14:30:05 +0000] "GET /../../../etc/passwd HTTP/1.1" 404 234 "-" "nikto/2.1.6"
```

**Análise**:
- **Classificação**: SECURITY
- **Severidade**: HIGH
- **Padrão**: Scanner Nikto procurando caminhos conhecidos vulneráveis
- **Ação recomendada**: Bloquear IP, ativar rate limiting

---

## Tabela de Referência Rápida

### Apache - Status Codes

| Status | Categoria | Severidade | Quando ocorre |
|--------|-----------|------------|---------------|
| 200-299 | TRAFFIC | INFO | Sucesso |
| 301-302 | TRAFFIC | INFO | Redirecionamento |
| 400 | TRAFFIC | LOW | Bad request |
| 401 | **SECURITY** + TRAFFIC | **MEDIUM** | Não autenticado |
| 403 | **SECURITY** + TRAFFIC | **MEDIUM** | Acesso negado |
| 404 | TRAFFIC | LOW | Não encontrado |
| 429 | **SECURITY** + TRAFFIC | MEDIUM | Rate limit |
| 500 | **PERFORMANCE** + ERROR | **HIGH** | Erro servidor |
| 502 | **PERFORMANCE** + ERROR | **HIGH** | Bad gateway |
| 503 | **PERFORMANCE** + ERROR | **CRITICAL** | Indisponível |
| 504 | **PERFORMANCE** + ERROR | **HIGH** | Gateway timeout |

---

### Padrões de Ataque

| Padrão | Tipo | Severidade | Descrição |
|--------|------|------------|-----------|
| `' OR '1'='1` | SQL Injection | CRITICAL | Tentativa de bypass de autenticação |
| `UNION SELECT` | SQL Injection | CRITICAL | Extração de dados |
| `<script>` | XSS | HIGH | Injeção de JavaScript |
| `../` | Path Traversal | HIGH | Acesso a ficheiros do sistema |
| `sqlmap` | Scanner | CRITICAL | Ferramenta de exploração automática |
| `nikto` | Scanner | HIGH | Scanner de vulnerabilidades |

---

### Syslog - Services Críticos

| Service | Categoria | Quando é CRITICAL |
|---------|-----------|-------------------|
| `sshd` | SECURITY | 3+ failed logins em 1 min |
| `sudo` | SECURITY | Authentication failure |
| `kernel` | PERFORMANCE | panic, OOM |
| `iptables` | SECURITY | Múltiplos bloqueios do mesmo IP |

---

### JSON - Services por Tipo

| Service Pattern | Categoria Principal |
|-----------------|---------------------|
| `*-api`, `api-*` | TRAFFIC |
| `auth-*`, `*-auth` | SECURITY |
| `database-*`, `*-db`, `redis`, `postgres` | PERFORMANCE |
| `cache-*` | PERFORMANCE |
| `gateway`, `proxy` | TRAFFIC |

---

### Nginx - Erros Críticos

| Mensagem | Severidade | Categoria |
|----------|------------|-----------|
| `no live upstreams` | CRITICAL | PERFORMANCE |
| `connection refused` | HIGH | PERFORMANCE |
| `upstream timed out` | HIGH | PERFORMANCE |
| `limiting requests` | MEDIUM | PERFORMANCE + TRAFFIC |
| `SSL_do_handshake() failed` | MEDIUM | SECURITY |
| `access forbidden` | MEDIUM | SECURITY |

---

## Notas Finais

### Combinação de Categorias

Um único evento pode pertencer a **múltiplas categorias**. Por exemplo:
```
<38>Feb 15 10:00:00 server sshd[1234]: Failed password for root from 203.0.113.45
```

**Classificação**: SECURITY (falha de autenticação) + potencialmente TRAFFIC (se estiver a contar tentativas)

---

### Modos de Análise

- **MODE_SECURITY**: Mostra apenas eventos com flag SECURITY
- **MODE_PERFORMANCE**: Mostra apenas eventos com flag PERFORMANCE
- **MODE_TRAFFIC**: Mostra apenas eventos com flag TRAFFIC
- **MODE_FULL**: Mostra todos os eventos (OR de todas as flags)

---

### Ajuste de Thresholds

Os thresholds podem ser ajustados conforme o ambiente:
```c
// Exemplo: Ajustar threshold de resposta grande
#define LARGE_RESPONSE_THRESHOLD (10 * 1024 * 1024)  // 10 MB

// Exemplo: Ajustar threshold de brute force
#define BRUTE_FORCE_THRESHOLD 5  // tentativas
#define BRUTE_FORCE_WINDOW 60    // em 60 segundos
```

