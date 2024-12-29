/*
 * ProjectMicro.c
 *
 * Created: 04/06/2021 11:24:34
 * Author : Bernardo Milheiro e Mateus Araujo
 */ 

//==============================================//
//												//
//					SUM�RIO						//
//												//
//			Defini��es e Bibliotecas			//	
//			Porta Serie (USART)					//
//			LEDs								//
//			Timers								//
//			Sensor HC-SR04						//
//			Buzzer								//
//			Sensor de Estacionamento			//
//										        //
//==============================================//


#define F_CPU 16000000UL							// 16 MHz clock speed
#include <avr/io.h>									// Carrega a biblioteca que permite utilizar as fun��es de input e output
#include <avr/interrupt.h>							// Carrega a biblioteca que permite utilizar as fun��es de interrup��o
#include <stdint.h>									// Carrega a biblioteca que permite utilizar inteiros com larguras especificadas
#include <stdlib.h>									// Carrega a biblioteca que permite utilizar inteiros com larguras especificadas
	
// Opera��o de "Entre"	
#define BETWEEN(value, min, max) (value < max && value > min)
	
// Fun��o necess�ria para transformar um array de char numa variavel inteira
int char2int (char *array, size_t n){				
	int number = 0;									// Inteiro que vai retornar
	int mult = 1;									// Multiplicador
	while (n--){									// por cada caracter no vetor
		// Numero igual � conversao para inteiro multiplicado por mult, que vai definir a posi��o do numero
		number += (array[n] - '0') * mult;			
		mult *= 10;
	}
	return number;
}



//====================================================================================//
//									PORTA SERIE							              //
//====================================================================================//
// Define os parametros necess�rios para a utiliza��o da Porta Serie
#define BAUD 9600									// Baud Rate
#define MYUBRR F_CPU/16/BAUD-1						// Taxa de Transmiss�o
char data;											// Inicializa variavel data
unsigned char tecla;								// Inicializa a variavel tecla
	
// Fun��es necess�rias � utiliza��o do USART	
void USART_Init (unsigned int ubrr){				// Fun��o para Inicializa��o da USART
	UBRR0 = ubrr;									// Ajusta a Taxa de Transmiss�o
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);					// Ativa o transmissor e o recetor
}
	
void USART_Transmit (char data){					// Fun��o de Transmiss�o de Dados
	while(!(UCSR0A & (1<<UDRE0)));					// Opera��o while para garantir que o registo de transmiss�o est� vazio
	UDR0 = (data++);								// Serve para colocar os dados e enviar para o USART
}
	
char USART_Receive (void){							// Fun��o de Rece��o de Dados
	while(!(UCSR0A & (1<<RXC0)));					// Enquanto n�o detetar o stop bit
	return UDR0;									// L� o registo e retorna
}
	
void enviaString(unsigned char string[]){			// Fun��o para enviar uma string		
	unsigned int i=0;								// Variavel de posi��o no string
	while(string[i] != 0){							// Enquanto houver caracteres
		USART_Transmit(string[i]);					// Envia os caracteres
		i++;										// Incrementa i
	}
}

ISR(USART_RX_vect){									// Interrup��o da USART
	data = UDR0;									// D� a variavel data o valor do registo UDR0
}

char *menu = "Iniciar Sensor de Estacionamento?\n\nSim - Pressionar a tecla 's'.\nN�o - Pressionar a tecla 'n'.\n\nOp��o: ";
char *nao = "\nBoa Viagem!\n";
	
	
	
//====================================================================================//
//										LED's							              //
//====================================================================================//
// Define as m�scaras que permitem aceder aos pinos associados aos LEDs
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
	TIMSK0 = (1<<TOIE0);							// Ativa��o da Interrup��o do timer0
	sei();											// Ativa��o Interrup��es Globais
}

volatile unsigned char cont_ovf0 = 0;				// Inicializa a variavel de contagem de Overflows

ISR(TIMER0_OVF_vect){								// Interrup��o Timer0
	TCNT0 = 6;										// Valor inicial de contagem
	cont_ovf0++;									// Incrementa o contador � passagem de 1ms
}

void timer1(void){		// Fun��o Timer1 para uso das interrup��es
	TCCR1A = 0x00;
	// WGM13:0 = 0000 para modo normal;
	// COM1A1:0 = 0 e COM1B1:0 = 0 no modo normal
	
	TCCR1B = 0x03;
	// WGM13:0 = 0000 para modo normal;
	// CS12:0 = 011 para rel�gio interno com divisor por 64;
	// ICNC1 = 0 e ICES1 = 0 no modo normal
		
	TCCR1C = 0x00;
	// FOC1A = 0 e FOC1B = 0 no modo normal
		
	TIMSK1 = (1 << TOIE1);
	// Ativa as interrup��es do Timer1
	
	TCNT1 = 65286;
	// Valor inicial de contagem = 2^16-250 = 65286	
	
	sei();
	// Ativa��o Interrup��es Globais	
}

volatile unsigned char cont_ovf1 = 0;				// Inicializa a variavel de contagem de Overflows

ISR(TIMER1_OVF_vect){								// Interrup��o Timer1
	TCNT1 = 65286;									// Valor inicial de contagem
	cont_ovf1++;									// Incrementa o contador � passagem de 1ms
}



//====================================================================================//
//									SENSOR HC-SR04						              //
//====================================================================================//
volatile int16_t distance;							// Inicializa a variavel de distancia
char vetor[8];										// Inicializa o vetor de char que conter� a distancia a ser enviada (USART)

// Define as m�scaras que permitem aceder aos pinos associados ao Sensor
#define SR_Trigger			PINB1
#define SR_Echo				PINB0
#define ECHO				(1<<SR_Echo)
  
void trigger(){										// Fun��o que ativa o trigger
	PORTB |= 1<<SR_Trigger;							// Ativa o Trigger
	while (cont_ovf0 != 60);						// Aguarda at� que o n� de Overflows seja 60 (60ms)
	PORTB &= ~(1<<SR_Trigger);						// Desativa o Trigger
}
  
void HCSR04_Init(){									// Fun��o que inicializa o Sensor HC-SR04	
	TCNT1 = 0;										// Coloca os registos de contagem a 0
	trigger();										// Aciona o Trigger do Sensor
	while((PINB & ECHO) == 0);						// Enquanto o Echo n�o estiver a receber ondas
	TCCR1B = (1<<CS10);								// Inicia o Timer1
	while((PINB & ECHO) != 0);						// Enquanto o Echo estiver a receber ondas
	TCCR1B = 0;										// Interrompe o Timer1
	distance = TCNT1*0.68;							// Variavel que guarda a distancia
	dtostrf(distance, 2, 0, vetor);					// Transforma a variavel distance num vetor
}
  
void enviaDistancia(char numChar){					// Fun��o que envia a distancia para o USART
	uint8_t i;										// Variavel para posi��o no vetor
	char mm[4] = " mm.";							// Unidade da distancia
	char distancia[13] = "\n\nDistancia:  ";		// Distancia:
	for(i = 0; i<sizeof(distancia); i++){			// Percorre as posi��es dos chars do vetor
		USART_Transmit(distancia[i]);				// Transmite a string char a char
	}
	for(i = 0; i<numChar; i++){						// Percorre as posi��es dos chars do vetor
		USART_Transmit(vetor[i]);					// Transmite a distancia contida no vetor
	}
	for(i = 0; i<sizeof(mm); i++){					// Percorre as posi��es dos chars do vetor
		USART_Transmit(mm[i]);						// Transmite a string char a char
	}
}
  
  
  
//====================================================================================//
//										BUZZER							              //
//====================================================================================//
volatile unsigned char nmr_ovf;						// Inicializa a variavel de n� de Overflows desejado

void buzzer(){										// Fun��o que vai produzir o som do Buzzer	
	TCNT0 = 0;										// Reset no Timer0
	cont_ovf0 = 0;									// Reset no Contador de Overflows
	PORTC = 0xff;									// Liga o buzzer conectado no PORTC							
	while(cont_ovf0 != 100);						// Aguarda at� que o n� de Overflows seja 100
	PORTC = 0x00;									// Desliga o buzzer conectado no PORTC	
}

volatile unsigned char buzzerIntervalo(nmr_ovf){	// Fun��o que recebe o intervalo de tempo pretendido entre �apitos� 
	TCNT0 = 0;										// Reset no Timer0
	cont_ovf0 = 0;									// Reset no Contador de Overflows
	while(cont_ovf0 != nmr_ovf);					// Aguarda at� que o n� de Overflows seja igual ao n� de Overflows pretendido
	buzzer();										// Fun��o que vai produzir o som do Buzzer
}

void buzzerVermelho(){								// Fun��o do Comportamento do Buzzer relativamente ao LED Vermelho
	// Buzzer apita 3 vezes em intervalos de 10ms
	buzzerIntervalo(75);
	buzzerIntervalo(75);
	buzzerIntervalo(75);
}

void buzzerAmarelo(){								// Fun��o do Comportamento do Buzzer relativamente ao LED Amarelo
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
		}else {										// Caso a distancia n�o esteja no intervalo pretendido
		PORTD &= ~(LED_RED|LED_YELLOW|LED_GREEN);	// Desliga todos os LEDs
	}
}
	
//====================================================================================//
//										MAIN							              //
//====================================================================================//
int main(){
	USART_Init(MYUBRR);									// Inicializa��o da rotina de configura��o da USART0
	
	DDRD = 0x70;										// Coloca os LEDs como Output	
	
	timer0();											// Inicia o Timer0
	timer1();											// Inicia o Timer1								
	
	DDRB = (1<<SR_Trigger);								// Coloca o pino Trigger como Input 
	
	DDRC = 0xff;										// Configura a PORTC como Output  
	
	while(1){
		enviaString(menu);								// Envia para o USART o menu
		tecla = USART_Receive();						// A USART recebe a tecla pressionada e d� esse valor a tecla
		switch(tecla){		
			case 's':									// Caso seja pressionada a tecla s
				for (int i=0; i<15; i++){				// Faz 15 medi��es
					HCSR04_Init();						// Fun��o que inicializa o Sensor HC-SR04
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

	
