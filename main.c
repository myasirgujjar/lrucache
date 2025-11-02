#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "lru.h"

#define PORT 8080
#define BUF_SIZE 8192

LRUCache* cache;

const char* html_ui =
"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
"<!DOCTYPE html><html><head><title>LRU Cache</title>"
"<style>body{font-family:sans-serif;background:#111;color:#eee;text-align:center;padding:2em}"
"input,button{padding:0.5em;margin:0.3em;border:none;border-radius:6px;font-size:1em}"
"button{background:#28a745;color:#fff;cursor:pointer}table{margin:1em auto;border-collapse:collapse}"
"td,th{border:1px solid #444;padding:6px 12px}</style></head><body>"
"<h2>🔄 LRU Cache Dashboard</h2>"
"<div><input id='key' placeholder='Key'><input id='val' placeholder='Value'>"
"<button onclick='setItem()'>Set</button><button onclick='getItem()'>Get</button></div>"
"<pre id='output'></pre>"
"<table id='cache'><thead><tr><th>Key</th><th>Value</th></tr></thead><tbody></tbody></table>"
"<script>"
"async function refresh(){let r=await fetch('/cache');let j=await r.json();"
"let tb=document.querySelector('#cache tbody');tb.innerHTML='';"
"j.forEach(e=>tb.innerHTML+=`<tr><td>${e.key}</td><td>${e.value}</td></tr>`)};"
"async function setItem(){let k=key.value,v=val.value;if(!k||!v)return;"
"await fetch(`/set?key=${encodeURIComponent(k)}&val=${encodeURIComponent(v)}`);"
"output.textContent=`Set ${k}=${v}`;refresh();}"
"async function getItem(){let k=key.value;if(!k)return;"
"let r=await fetch(`/get?key=${encodeURIComponent(k)}`);let t=await r.text();"
"output.textContent=t;refresh();}"
"refresh();setInterval(refresh,3000);</script></body></html>";

static void send_http(int sock, const char* type, const char* body) {
    char header[256];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %zu\r\n\r\n",
             type, strlen(body));
    write(sock, header, strlen(header));
    write(sock, body, strlen(body));
}

void* handle_client(void* arg) {
    int sock = *(int*)arg;
    free(arg);
    char buffer[BUF_SIZE];
    int n = read(sock, buffer, BUF_SIZE - 1);
    if (n <= 0) { close(sock); return NULL; }
    buffer[n] = '\0';

    if (strncmp(buffer, "GET / ", 6) == 0) {
        write(sock, html_ui, strlen(html_ui));
    } else if (strstr(buffer, "GET /cache")) {
        char* json = lru_to_json(cache);
        send_http(sock, "application/json", json);
        free(json);
    } else if (strstr(buffer, "GET /set?")) {
        char key[128] = "", val[128] = "";
        char* start = strstr(buffer, "/set?key=");
        if (start)
            sscanf(start, "/set?key=%127[^&]&val=%127[^ \r\n]", key, val);
        if (strlen(key) > 0 && strlen(val) > 0) {
            lru_put(cache, key, val);
            char msg[256];
            snprintf(msg, sizeof(msg), "Set %s=%s\n", key, val);
            send_http(sock, "text/plain", msg);
        } else {
            send_http(sock, "text/plain", "Invalid /set request\n");
        }
    } else if (strstr(buffer, "GET /get?")) {
        char key[128] = "";
        char* start = strstr(buffer, "/get?key=");
        if (start)
            sscanf(start, "/get?key=%127[^ \r\n]", key);
        const char* val = lru_get(cache, key);
        char msg[256];
        snprintf(msg, sizeof(msg), "%s=%s\n", key, val ? val : "(null)");
        send_http(sock, "text/plain", msg);
    } else {
        send_http(sock, "text/plain", "Invalid Request\n");
    }

    close(sock);
    return NULL;
}

int main() {
    cache = lru_create(5);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);

    printf("🚀 Safe LRU Cache Web UI running at http://localhost:%d\n", PORT);

    while (1) {
        int* client = malloc(sizeof(int));
        *client = accept(server_fd, NULL, NULL);
        if (*client < 0) { free(client); continue; }
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, client);
        pthread_detach(tid);
    }

    lru_free(cache);
    close(server_fd);
    return 0;
}

