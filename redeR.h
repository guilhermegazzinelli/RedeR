#ifndef REDER_H
#define REDER_H

#include "RF24.h"
#include "config.h"

// Configuração do Radio ----------------------------------------------------
#define RF_PAYLOAD 32 // payload usado no NRF24L01
#define RADIO_CE 9
#define RADIO_CSN 10
#define NETWORK_CH 120
#define ADDR_SIZE 5
#define NUM_CHANNELS 126

// Configuração da Rede ----------------------------------------------------
#define PATH_WIDTH 6
#define LEVELS (PATH_WIDTH-1) // 5 niveis mais iterator
#define ITERATOR LEVELS
#define BASE_ID 0
#define ADDR_NULL 0
#define N_PINGS 100
#define PING_TIMEOUT 10 // tempo em ms
#define N_MAX_MODULES 10 // numero maximo de modulos na rede
#define N_CONNECTIONS 2 // numero maximo de conecção para um modulo, permite multi-caminho


// Lista de variaveis do protocolo
#define ACK_FAIL 0
#define ACK_SUCCESS 1
#define LUMENS 2

#ifdef NETWORK_ROOT
  // Construção da tabela roteamento (Função usada apenas na base)
  void build_network(char *id_list, int n_modules);
  // Verifica se os dados recebidos são ack ou validos
  bool check();
  //pinga os modulos para construir a rede
#else
  // Verifica se os dados recebidos são para o modulo, caso contrario retransmite
  // dados para proximo nivel
  bool check_and_foward();
    void rede_ack(char *path, bool success);
#endif

// Obtem caminho para modulo de endereço "id"
void get_path(char *path, char id );

// Obtem caminho do modulo para base
void get_path(char *path );

void network_init(char *network_add, char id = 0);

// Envia dados para modulo X e aguarda Echo retorna verdadeiro ou falso
bool send_data(char *data,char id);

// Retorna o id do moulo e os dados do modulo que o enviou
char receive_data(char *data);


// Empacotador de dados constroi o buffer para enviou cuida das
void data_packing(char *path,char *data_in, char *data_out);

// Desempacotador de dados
void data_unpackig(char *data_in, char *data_out);

// Envia dados pelo radio
bool radio_send(char *data);

void spectrum_map(char *spectrum);

void print_buffer(char *data, int length = RF_PAYLOAD);

// Teste de comunicação de todos os modulos para construção da tabela
int ping_modules(char *id_path);

#endif
