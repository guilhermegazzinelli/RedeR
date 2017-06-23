#include "redeR.h"

byte NETWORK_ADD[ADDR_SIZE];
byte ID;

char RECEIVE_BUFFER[RF_PAYLOAD];

RF24 nrf24(RADIO_CE,RADIO_CSN);

/*
struct id_data{
	char id_up;
	int quality;
}*/

//id_data routing_table[N_MAX_MODULES][N_CONECTIONS]
//---------------------------------------------------------------------------
// Construção da tabela roteamento (Função usada apenas na base)
void build_network(char *id_list, int n_modules){
	// Pingar todos os modulos
	// Analisar qualidade, escolher modulos da primeira camada
	// Enviar para todos modulos da primeira camada o id_list dos restantes
	// Receber a qualidade dos modulos da primeira camada e escolher segunda 
	// camada analisar possibilidade de mais de um caminho
	
	//1º percorrer toda estrutura e prencher ids com 255
	
	//2º construir
	/*
	for(int i;i<n_modules;i++){
		if(routing_table[id_list[i]][0].id_up==255){// ainda não tem modulo superior
			get_path(path,id_list[i]);// taves essa tenha que ser uma função especifica
			q=ping_modules(path);
			if(q>80){
				routing_table[i][0].id_up=0;
				routing_table[i][0].quality=q;
			}
		}
	}*/
}
//---------------------------------------------------------------------------
void network_init(char *network_add, char id){

  // Copia endereço de MAC e ID do modulo para variavel local, possivel ponto 
  // para gravação a EEPROM
  memcpy(NETWORK_ADD, network_add, ADDR_SIZE);

  ID=id;

  // Inicializa radio
  nrf24.begin();
  
  nrf24.setPALevel(RF24_PA_MAX);
  nrf24.setDataRate(RF24_2MBPS);
  nrf24.setAutoAck(0);
  //nrf24.setRetries(2,0);
  
  nrf24.setChannel(NETWORK_CH);
  
  #ifdef DEBUG
	Serial.write("Canal: ");
	Serial.println(nrf24.getChannel());
  #endif

  NETWORK_ADD[4]=ID;
  
  nrf24.openReadingPipe(0,NETWORK_ADD);
  
  nrf24.startListening();
}
//---------------------------------------------------------------------------
// Envia dados para modulo X e aguarda Echo retorna verdadeiro ou falso
bool send_data(char *data,char id=0){
  #define TRIALS 2
  // Variavel auxiliar que conterá o caminho até o modulo id
  char path[PATH_WIDTH];

  // Varialvel auxiliar de buffer
  char buf[RF_PAYLOAD];
  
  // Loop para multiplas tentativas de entrega
  for(int i=1;i<=TRIALS;i++){
      
    // Buscar o caminho para envio
	if(id)
		get_path(path,id);//se tiver um id, significa que esta indo pro modulo.
	else
		get_path(path);//se nao tiver um id, significa que esta indo pra base.

    // Modifica endereço de envio para primeiro modulo da sequencia
    NETWORK_ADD[ADDR_SIZE-1] = path[0];
    nrf24.openWritingPipe(NETWORK_ADD);
  
    // Empacotar
    data_packing(path,data,buf);
 
    // Emviar e testar conexão com primeiro nivel
    if(radio_send(buf)){
      return true;
    }
  }
  #ifdef NETWORK_ROOT
    // gerar erro para tabela de roteamento
  #endif
  return false;
}
//---------------------------------------------------------------------------
// Verifica se os dados recebidos são para o modulo, caso contrario retransmite 
// dados para proximo nivel, deve ser chamada após teste de available
bool check_and_foward(){
  if(nrf24.available()){
	 char buf[RF_PAYLOAD];

	 nrf24.read(buf,RF_PAYLOAD);

	 if(buf[ITERATOR]<10){// Informação descendo
		if(buf[ITERATOR]>=(LEVELS-1) || buf[buf[ITERATOR]+1]==ADDR_NULL){
		// o dado é para o modulo em questão
					
		//Copia dados para buffer de recebimento global
		memcpy(RECEIVE_BUFFER,buf,RF_PAYLOAD);
		
		buf[ITERATOR]+=10;// modifica path para que seja feito ACK
		// Retorna que o dado foi recebido com sucesso
		rede_ack(buf,true);
		return true;
	  }
	  else {
		// Caso contrario envia para próximo modulo
  
		// Modifica iterator para sequencia descendo
		buf[ITERATOR]++;

		// Modifica endereço de envio para primeiro modulo da sequencia
		NETWORK_ADD[ADDR_SIZE-1] = buf[buf[ITERATOR]];
		nrf24.openWritingPipe(NETWORK_ADD);
  
		// Envia e Testa ACK do radio
		if(radio_send(buf)){
			return true;
		}
		else
		{
			buf[ITERATOR]+=9;// já reduz uma camada e coloca o rool_back
			rede_ack(buf,false);
			Serial.print("ACK false ");
		}
		#ifdef DEBUG
			Serial.write("Descendo buf[ITERATOR]=");
			Serial.print((unsigned char)buf[ITERATOR]);
			Serial.write("\n");
		#endif
	  }
	}
	else if(buf[ITERATOR]>=10){// Informação subindo
		if(buf[ITERATOR]==10){
			//Enviar para base
			NETWORK_ADD[ADDR_SIZE-1] =0;
		}
		else{
			//Enviar para modulo anterior
			buf[ITERATOR]--;
			NETWORK_ADD[ADDR_SIZE-1] = buf[buf[ITERATOR]-10];
		}
		nrf24.openWritingPipe(NETWORK_ADD);
		// Envia
		radio_send(buf);
		#ifdef DEBUG
			Serial.write("Subindo buf[ITERATOR]=");
			Serial.print((unsigned char)buf[ITERATOR]);
			Serial.write("\n");
		#endif
	}
	  
  }
  return false;
}
//---------------------------------------------------------------------------
void rede_ack(char *buf, bool success){
	if (success){
		buf[PATH_WIDTH]=ACK_SUCCESS;
		//delayMicroseconds(1780);// espera modulo anterior voltar para modo de recepção TESTE

	}
	else{
		buf[PATH_WIDTH]=ACK_FAIL;
		//delayMicroseconds(1980);// espera modulo anterior voltar para modo de recepção TESTE

	}
	
	buf[PATH_WIDTH+1]=buf[buf[ITERATOR]-10];//id do modulo que respondeu.
		
	if(buf[ITERATOR]==10){
		//Enviar para base
		NETWORK_ADD[ADDR_SIZE-1] =0;
	}
	else{
		//Enviar para modulo anterior
		buf[ITERATOR]--;
		NETWORK_ADD[ADDR_SIZE-1] = buf[buf[ITERATOR]-10];//TESTE: se colocar pra do jeito que estava, perde 20%
	}
	nrf24.openWritingPipe(NETWORK_ADD);
	
	// Envia
	radio_send(buf);  
}
//---------------------------------------------------------------------------
bool check(){
	if(nrf24.available()){
		char buf[RF_PAYLOAD];
	  
		nrf24.read(buf,RF_PAYLOAD);
		
		//Copia dados para buffer de recebimento global
		memcpy(RECEIVE_BUFFER,buf,RF_PAYLOAD);
		
		#ifdef DEBUG
			Serial.write("Receive_Buffer: ");
			print_buffer(RECEIVE_BUFFER);
		#endif
		return true;
	}
	return false;
}
//---------------------------------------------------------------------------
// Retorna o id do modulo e os dados do modulo que o enviou
char receive_data(char *data){
  memcpy(data,&RECEIVE_BUFFER[PATH_WIDTH],RF_PAYLOAD-PATH_WIDTH);

  
  #ifdef DEBUG
	Serial.write("Receive_Buffer: ");
	print_buffer(RECEIVE_BUFFER);
  #endif
}
//---------------------------------------------------------------------------
// Obtem caminho para modulo de endereço "id"
void get_path(char *path, char id ){
	
	path[ITERATOR]=0;//Iterador
	if (id==1){
		path[0]=1;
		path[1]=ADDR_NULL;
		
	}
	if (id==2){
		path[0]=1;
		path[1]=2;
		path[2]=ADDR_NULL;
	}
	if (id==3){
		path[0]=1;
		path[1]=2;
		path[2]=3;
		path[3]=ADDR_NULL;
	}
	#ifdef DEBUG
	  Serial.write("Path: ");
	  print_buffer(path,PATH_WIDTH);
	#endif
}
//---------------------------------------------------------------------------
void get_path(char *path){
	// pode estar errado ou gerar problema no check and foward
	memcpy(path,RECEIVE_BUFFER,PATH_WIDTH);
    path[ITERATOR]+=10;
	
	#ifdef DEBUG
	  Serial.write("Path: ");
	  print_buffer(path,PATH_WIDTH);
	#endif
}
//---------------------------------------------------------------------------
// Empacotador de dados constroi o buffer para enviou cuida das criptografias
void data_packing(char *path,char *data_in, char *data_out){
  // Função deve criptografar data_in e chave

  // Função simples para teste, deve ser alterada
  memcpy(data_out,path,PATH_WIDTH);
  
  memcpy(&data_out[PATH_WIDTH],data_in, RF_PAYLOAD-PATH_WIDTH);
  
  #ifdef DEBUG
	Serial.write("Data_out: ");
	print_buffer(data_out);
  #endif
}
//---------------------------------------------------------------------------
// Desempacotador de dados
void data_unpackig(char *data_in, char *data_out){
  
}
//---------------------------------------------------------------------------
bool radio_send(char *data){
  bool success;
  nrf24.stopListening();
  success=nrf24.write(data,RF_PAYLOAD);
  nrf24.startListening();
 // delay(3);
  #ifdef DEBUG
	Serial.write("___Send_Buffer: ");
	print_buffer(data);
  #endif
  
  return success;
}
//---------------------------------------------------------------------------
//Funçao para checar os canais de frequencia disponiveis
void spectrum_map(char *spectrum){
	#define RETRIES 100
	memset(spectrum,0,NUM_CHANNELS);
	
	for(int i=0;i<RETRIES;i++){
		for(int ch=0;ch<NUM_CHANNELS;ch++){
			//Seta o canal [ch], a ser testado
			 nrf24.setChannel(ch);
						
			// Modo de recepção
			nrf24.startListening();
			delayMicroseconds(225);

			// Teste do canal
			if ( nrf24.testCarrier() ){
				spectrum[ch]++;
			}
			nrf24.stopListening();
		}
	}
	nrf24.setChannel(NETWORK_CH);
	#ifdef DEBUG
		Serial.write("Spectrum: ");
		print_buffer(spectrum,NUM_CHANNELS);
	#endif
}
//---------------------------------------------------------------------------
//Print para o DEBUG
void print_buffer(char *data, int length){
	for(int i=0;i<length;i++){
		Serial.print((unsigned char)data[i]);
		Serial.write(";");
	}
	Serial.write(" Fim \n");
}

//---------------------------------------------------------------------------
// Teste de comunicação de todos os modulos para construção da tabela
int ping_modules(char *path){

	int quality=0,time_out=0;
	// Varialvel auxiliar de buffer
	char buf[RF_PAYLOAD];
		

	NETWORK_ADD[ADDR_SIZE-1] = path[0]; 
    nrf24.openWritingPipe(NETWORK_ADD);
	
    // Empacotar
    data_packing(path,RECEIVE_BUFFER,buf);
	
    for(int i=0;i<N_PINGS;i++)
    {
	//Enviar e testar ack do radio
		
		radio_send(buf);
		// Caso dado tenha sido recebido pelo primeiro nó esperar ack da rede
		time_out=0;
		while(!nrf24.available()&&time_out<PING_TIMEOUT){
			time_out++;
			delay(1);
		}

		if(nrf24.available()){
			// Recebe ack da rede
			nrf24.read(RECEIVE_BUFFER,RF_PAYLOAD);
			
			if(RECEIVE_BUFFER[PATH_WIDTH]){//Testa se o ack da rede foi de sucesso
				quality++;
			}
			#ifdef DEBUG
				Serial.write("\nTime_out: ");
				Serial.print(time_out);
				Serial.write("\n");
				Serial.write("Receive_Buffer: ");
				print_buffer(RECEIVE_BUFFER);
			#endif
		}
    }
	return quality;
}