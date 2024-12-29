/*
 * ProjectMicro.c
 *
 * Created: 04/06/2021 11:24:34
 * Author : Bernardo Milheiro e Mateus Araujo
 */ 

//==============================================//
//												//
//					SUMÁRIO						//
//												//
//			Definições e Bibliotecas			//	
//			Porta Serie (USART)					//
//			LEDs								//
//			Timers								//
//			Sensor HC-SR04						//
//			Buzzer								//
//			Sensor de Estacionamento			//
//										        //
//==============================================//


#define F_CPU 16000000UL							// 16 MHz clock speed
#include <avr/io.h>									// Carrega a biblioteca que permite utilizar as funções de input e output
#include <avr/interrupt.h>							// Carrega a biblioteca que permite utilizar as funções de interrupção
#include <stdint.h>									// Carrega a biblioteca que permite utilizar inteiros com larguras especificadas
#include <stdlib.h>									// Carrega a biblioteca que permite utilizar inteiros com larguras especificadas
	
// Operação de "Entre"	
#define BETWEEN(value, min, max) (value < max && value > min)
	
// Função necessária para transformar um array de char numa variavel inteira
int char2int (char *array, size_t n){				
	int number = 0;									// Inteiro que vai retornar
	int mult = 1;									// Multiplicador
	while (n--){									// por cada caracter no vetor
		// Numero igual à conversao para inteiro multiplicado por mult, que vai definir a posição do numero
		number += (array[n] - '0') * mult;			
		mult *= 10;
	}
	return number;
}



//====================================================================================//
//									PORTA SERIE							              //
//====================================================================================//
// Define os parametros necessários para a utilização da Porta Serie
#define BAUD 9600									// Baud Rate
#define MYUBRR F_CPU/16/BAUD-1						// Taxa de Transmissão
char data;											// Inicializa variavel data
unsigned char tecla;								// Inicializa a variavel tecla
	
// Funções necessárias à utilização do USART	
void USART_Init (unsigned int ubrr){				// Função para Inicialização da USART
	UBRR0 = ubrr;									// Ajusta a Taxa de Transmissão
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);					// Ativa o transmissor e o recetor
}
	
void USART_Transmit (char data){					// Função de Transmissão de Dados
	while(!(UCSR0A & (1<<UDRE0)));					// Operação while para garantir que o registo de transmissão está vazio
	UDR0 = (data++);								// Serve para colocar os dados e enviar para o USART
}
	
char USART_Receive (void){							// Função de Receção de Dados
	while(!(UCSR0A & (1<<RXC0)));					// Enquanto não detetar o stop bit
	return UDR0;									// Lê o registo e retorna
}
	
void enviaString(unsigned char string[]){			// Função para enviar uma string		
	unsigned int i=0;								// Variavel de posição no string
	while(string[i] != 0){							// Enquanto houver caracteres
		USART_Transmit(string[i]);					// Envia os caracteres
		i++;										// Incrementa i
	}
}

ISR(USART_RX_vect){									// Interrupção da USART
	data = UDR0;									// Dá a variavel data o valor do registo UDR0
}

char *menu = "Iniciar Sensor de Estacionamento?\n\nSim - Pressionar a tecla 's'.\nNão - Pressionar a tecla 'n'.\n\nOpção: ";
char *nao = "\nBoa Viagem!\n";
	
	
	
//====================================================================================//
//										LED's							              //
//====================================================================================//
// Define as máscaras que permitem aceder aos pinos associados aos LEDs
#define LED_RED		0x10
#define LED_GREEN	0x20
#define LED_YELLOW	0x40



//====================================================================================//
//										TIMERS							              //
//====================================================================================//
void timer0(void) {
	TCCR0A = (0<<WGM00);							// Modo Normal
	TCCR0B = (0b011 << CS00);						// Prescaler de 64
	TCNT0 = 256-250;								// Valor inicial de contagem
	TIMSK0 = (1<<TOIE0);							// Ativação da Interrupção do timer0
	sei();											// Ativação Interrupções Globais
}

volatile unsigned char cont_ovf0 = 0;				// Inicializa a variavel de contagem de Overflows

ISR(TIMER0_OVF_vect){								// Interrupção Timer0
	TCNT0 = 6;										// Valor inicial de contagem
	cont_ovf0++;									// Incrementa o contador à passagem de 1ms
}

void timer1(void){		// Função Timer1 para uso das interrupções
	TCCR1A = 0x00;
	// WGM13:0 = 0000 para modo normal;
	// COM1A1:0 = 0 e COM1B1:0 = 0 no modo normal
	
	TCCR1B = 0x03;
	// WGM13:0 = 0000 para modo normal;
	// CS12:0 = 011 para relógio interno com divisor por 64;
	// ICNC1 = 0 e ICES1 = 0 no modo normal
		
	TCCR1C = 0x00;
	// FOC1A = 0 e FOC1B = 0 no modo normal
		
	TIMSK1 = (1 << TOIE1);
	// Ativa as interrupções do Timer1
	
	TCNT1 = 65286;
	// Valor inicial de contagem = 2^16-250 = 65286	
	
	sei();
	// Ativação Interrupções Globais	
}

volatile unsigned char cont_ovf1 = 0;				// Inicializa a variavel de contagem de Overflows

ISR(TIMER1_OVF_vect){								// Interrupção Timer1
	TCNT1 = 65286;									// Valor inicial de contagem
	cont_ovf1++;									// Incrementa o contador à passagem de 1ms
}



//====================================================================================//
//									SENSOR HC-SR04						              //
//====================================================================================//
volatile int16_t distance;							// Inicializa a variavel de distancia
char vetor[8];										// Inicializa o vetor de char que conterá a distancia a ser enviada (USART)

// Define as máscaras que permitem aceder aos pinos associados ao Sensor
#define SR_Trigger			PINB1
#define SR_Echo				PINB0
#define ECHO				(1<<SR_Echo)
  
void trigger(){										// Função que ativa o trigger
	PORTB |= 1<<SR_Trigger;							// Ativa o Trigger
	while (cont_ovf0 != 60);						// Aguarda até que o nº de Overflows seja 60 (60ms)
	PORTB &= ~(1<<SR_Trigger);						// Desativa o Trigger
}
  
void HCSR04_Init(){									// Função que inicializa o Sensor HC-SR04	
	TCNT1 = 0;										// Coloca os registos de contagem a 0
	trigger();										// Aciona o Trigger do Sensor
	while((PINB & ECHO) == 0);						// Enquanto o Echo não estiver a receber ondas
	TCCR1B = (1<<CS10);								// Inicia o Timer1
	while((PINB & ECHO) != 0);						// Enquanto o Echo estiver a receber ondas
	TCCR1B = 0;										// Interrompe o Timer1
	distance = TCNT1*0.68;							// Variavel que guarda a distancia
	dtostrf(distance, 2, 0, vetor);					// Transforma a variavel distance num vetor
}
  
void enviaDistancia(char numChar){					// Função que envia a distancia para o USART
	uint8_t i;										// Variavel para posição no vetor
	char mm[4] = " mm.";							// Unidade da distancia
	char distancia[13] = "\n\nDistancia:  ";		// Distancia:
	for(i = 0; i<sizeof(distancia); i++){			// Percorre as posições dos chars do vetor
		USART_Transmit(distancia[i]);				// Transmite a string char a char
	}
	for(i = 0; i<numChar; i++){						// Percorre as posições dos chars do vetor
		USART_Transmit(vetor[i]);					// Transmite a distancia contida no vetor
	}
	for(i = 0; i<sizeof(mm); i++){					// Percorre as posições dos chars do vetor
		USART_Transmit(mm[i]);						// Transmite a string char a char
	}
}
  
  
  
//====================================================================================//
//										BUZZER							              //
//====================================================================================//
volatile unsigned char nmr_ovf;						// Inicializa a variavel de nº de Overflows desejado

void buzzer(){										// Função que vai produzir o som do Buzzer	
	TCNT0 = 0;										// Reset no Timer0
	cont_ovf0 = 0;									// Reset no Contador de Overflows
	PORTC = 0xff;									// Liga o buzzer conectado no PORTC							
	while(cont_ovf0 != 100);						// Aguarda até que o nº de Overflows seja 100
	PORTC = 0x00;									// Desliga o buzzer conectado no PORTC	
}

volatile unsigned char buzzerIntervalo(nmr_ovf){	// Função que recebe o intervalo de tempo pretendido entre “apitos” 
	TCNT0 = 0;										// Reset no Timer0
	cont_ovf0 = 0;									// Reset no Contador de Overflows
	while(cont_ovf0 != nmr_ovf);					// Aguarda até que o nº de Overflows seja igual ao nº de Overflows pretendido
	buzzer();										// Função que vai produzir o som do Buzzer
}

void buzzerVermelho(){								// Função do Comportamento do Buzzer relativamente ao LED Vermelho
	// Buzzer apita 3 vezes em intervalos de 10ms
	buzzerIntervalo(75);
	buzzerIntervalo(75);
	buzzerIntervalo(75);
}

void buzzerAmarelo(){								// Função do Comportamento do Buzzer relativamente ao LED Amarelo
	// Buzzer apita 2 vezes em intervalos de 50ms
	buzzerIntervalo(150);
	buzzerIntervalo(150);
}



//====================================================================================//
//								SENSOR DE ESTACIONAMENTO				              //
//====================================================================================//
int distancia;										// Variavel inteira de distancia

void sensorEstacionamento(){
	if (distancia > 0)								// Se a distancia for maior que 0
	{
		enviaDistancia(3);
		if (BETWEEN(distancia,191,260))				// Se a distancia for entre 191 e 260mm
		{
			PORTD &= ~(LED_RED|LED_YELLOW);			// Desliga os LEDs Vermelho e Amarelo
			PORTD |= LED_GREEN;						// Liga o LED Verde
		}
		if (BETWEEN(distancia,100,190))				// Se a distancia for entre 100 e 190mm
		{
			PORTD &= ~(LED_RED|LED_GREEN);			// Desliga os LEDs Vermelho e Verde
			PORTD |= LED_YELLOW;					// Liga o LED Amarelo
			buzzerAmarelo();						// Comportamento do Buzzer relativo ao LED Amarelo
		}
		if (distancia < 100 || distancia > 330)		// Se a distancia for menor que 100mm ou maior que 300mm
		{
			PORTD &= ~(LED_YELLOW|LED_GREEN);		// Desliga os LEDs Amarelo e Verde
			PORTD |= LED_RED;						// Liga o LED Vermelho
			buzzerVermelho();						// Comportamento do Buzzer relativo ao LED Vermelho
		}
		}else {										// Caso a distancia não esteja no intervalo pretendido
		PORTD &= ~(LED_RED|LED_YELLOW|LED_GREEN);	// Desliga todos os LEDs
	}
}
	
//====================================================================================//
//										MAIN							              //
//====================================================================================//
int main(){
	USART_Init(MYUBRR);									// Inicialização da rotina de configuração da USART0
	
	DDRD = 0x70;										// Coloca os LEDs como Output	
	
	timer0();											// Inicia o Timer0
	timer1();											// Inicia o Timer1								
	
	DDRB = (1<<SR_Trigger);								// Coloca o pino Trigger como Input 
	
	DDRC = 0xff;										// Configura a PORTC como Output  
	
	while(1){
		enviaString(menu);								// Envia para o USART o menu
		tecla = USART_Receive();						// A USART recebe a tecla pressionada e dá esse valor a tecla
		switch(tecla){		
			case 's':									// Caso seja pressionada a tecla s
				for (int i=0; i<15; i++){				// Faz 15 medições
					HCSR04_Init();						// Função que inicializa o Sensor HC-SR04
					distancia = char2int(vetor,3);		// Inicializa a variavel distancia com o valor inteiro de vetor
					sensorEstacionamento();				// Inicia o comportamento do Sensor de Estacionamento
				}
				break;
			case 'n':									// Caso seja pressionada a tecla n
			enviaString(nao);							// Envia para a USART a mensagem nao
			break;	
		}
	}
}

	
