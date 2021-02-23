/*
 * Space_invaders.c
 *
 *  Created on: 28 mag 2020
 *      Author: rafus
 */
#include "../Space_invaders_bsp/drivers/inc/altera_up_avalon_video_dma_controller.h"
#include "../Space_invaders_bsp/drivers/inc/altera_up_avalon_video_rgb_resampler.h"
#include "../Space_invaders_bsp/drivers/inc/altera_up_avalon_accelerometer_spi.h"
#include "../Space_invaders_bsp/HAL/inc/sys/alt_stdio.h"
#include "../Space_invaders_bsp/HAL/inc/alt_types.h"
#include "../Space_invaders_bsp/HAL/inc/sys/alt_irq.h"
#include "../Space_invaders_bsp/system.h"
#include <stdint.h>
#include <time.h>
#include <stdlib.h>

//colori su 24 bit
#define RGB_24BIT_INTEL_BLUE	0x0071C5
#define YELLOW_24	0xFFFF00
#define RED_24	0xFF0000
#define WHITE_24 0xFFFFFF
#define GREEN_24  0x00CC00
#define ORANGE_24 0xF75E25

//posizioni iniziali navicella
#define Y1_NAV 100
#define X1_NAV 75

//dimensioni navicella
#define NAV_LENGTH 9
#define NAV_WIDTH 3
#define HALF_LENGTH 4

//dimensioni proiettile
#define SHOT_WIDTH 3
#define SHOT_LENGTH 1

//posizione iniziale invasore [0][0]
#define Y1_INV 15
#define X1_INV 10

//lato invasore
#define SIDE_INV 5

//numero di invasori sulle righe e sulle colonne
#define ROW_INV 3
#define COLUMN_INV 8

//vita iniziale invasore e navicella
#define LIFE_INV 1
#define LIFE_SHIP 3

//numero totale di invasori a inizio gioco
#define NUM_INV 24

/*inizio del bordo*/
#define START_BOARD 5
#define END_BOARD 110

//dimensione massima del bordo lungo y
#define Y_MAX 110

/*spessore bordo*/
#define EDGE_THICKNESS 1

//Indirizzo per i pulsanti
#define KEY (*(volatile uint32_t*)(PUSHBUTTONS_BASE))

//Maschera usata per prendere il bit 0 di Edgecapture
#define MASK0 0x1;

//Maschera usata per prendere il bit 1 di Edgecapture
#define MASK1 0x2;

/* declare volatile pointer for interval timer */
volatile int * interval_timer_ptr =
			(int *)INTERVAL_TIMER_BASE; // interal timer base address

volatile int * interval_timer_ptr_2 =
			(int *)INTERVAL_TIMER_2_BASE; // interal timer base address

/*Variabili utilizzate nelle routine degli interrupt*/
volatile uint32_t timeout;
volatile uint32_t timeout_shot;

//Genera un numero casuale compreso tra upper e lower
uint8_t randoms(uint8_t lower, uint8_t upper)
{
	uint8_t num = (rand() % (upper - lower + 1)) + lower;
	return num;
}

/*Definizioni delle strutture navicella, invasore e proiettile*/
struct nav
{
	uint8_t x1; //la navicella è costituita principalmente da un quadrato, x1 e y1 sono le coordinate dell'angolo in alto a sinistra del quadrato
	uint8_t y1;
	int lives;	//vite della navicella
}navicella;

struct inv
{
	uint8_t x1;	//gli invasori sono quadrati, x1 e y1 sono le coordinate dell'angolo in alto a sinistra dei quadrati
	uint8_t y1;
	int life;	//vita degli invasori
}invasore[COLUMN_INV][ROW_INV];	//notazione utilizzata per la matrice degli invasori: invasore[colonna][riga]

struct proiettile
{
	uint8_t x1;	//I proiettili sono quadrati, x1 ed y1 sono le coordinate dell'angolo in alto a sinistra dei quadrati
	uint8_t y1;
	int life;	//"vita" del proiettile
}proiettile[2]; //proiettile[0] -> navicella; proiettile[1] -> invasori

/*Tiene conto del numero di invasori vivi*/
uint8_t num_inv;

/*Utilizzate nei cicli for*/
uint8_t i,j;

/*Direzione invasori, se 0 si muovono verso destra, se 1 verso sinistra*/
	uint8_t dir_inv;

/*Routine da eseguire per interrupt movimento invasori*/
void interval_timer_ISR(void* context, alt_u32 id) {

    *(interval_timer_ptr) = 0; // clear the interrupt
    timeout               = 1; // set global variable

    return;
}

/*Routine da eseguire per interrupt movimento proiettile*/
void interval_timer_ISR2(void* context, alt_u32 id) {

    *(interval_timer_ptr_2) = 0; // clear the interrupt
    timeout_shot            = 1; // set global variable

    return;
}

/*Inizializzazione interval timer: contatori, modalità  di utilizzo. Registrazione degli interrupt*/
void interval_timer_init()
{

	/* set the interval timer period */
	int counter = 10000000; // 1/(100 MHz) x (10000000) = 0.1 sec, 100Mhz = clock del sistema
	*(interval_timer_ptr + 0x2) = (counter & 0xFFFF);		//registro periodl
	*(interval_timer_ptr + 0x3) = (counter >> 16) & 0xFFFF;	//registro periodh

	/* set the interval timer period */
	int counter_shot = 15000; // 1/(100 MHz) x (15000) = 0.15  ms, 100Mhz = clock del sistema
	*(interval_timer_ptr_2 + 0x2) = (counter_shot & 0xFFFF);		//registro periodl
	*(interval_timer_ptr_2 + 0x3) = (counter_shot >> 16) & 0xFFFF;	//registro periodh

	/* start interval timer, enable its interrupts */
	*(interval_timer_ptr + 1) = 0x7; // STOP = 0, START = 1, CONT = 1, ITO = 1

	/* start interval timer, enable its interrupts */
	*(interval_timer_ptr_2 + 1) = 0x7; // STOP = 0, START = 1, CONT = 1, ITO = 1

	/* use the HAL facility for registering interrupt service routines. */
	alt_irq_register(INTERVAL_TIMER_IRQ, (void*) &timeout, (void *)interval_timer_ISR);

	/* use the HAL facility for registering interrupt service routines. */
	alt_irq_register(INTERVAL_TIMER_2_IRQ, (void*) &timeout_shot, (void *)interval_timer_ISR2);

}


/*Vengono definite le condizioni iniziali della navicella, del proiettile e degli invasori. Viene inizializzato il numero di invasori e la loro direzione*/
void inizializzazione_parametri()
{
	/*Condizione iniziale navicella*/
	navicella.y1 = Y1_NAV;
	navicella.x1 = X1_NAV;
	navicella.lives = LIFE_SHIP;

	/*Condizione iniziale proiettile della navicella*/
	proiettile[0].x1=navicella.x1+HALF_LENGTH;
	proiettile[0].y1=navicella.y1-2*SHOT_WIDTH;

	/*Condizione iniziale invasori*/
	for (j=0; j<ROW_INV; j++)
	{
		for(i=0; i<COLUMN_INV; i++)
		{
			invasore[i][j].y1 = Y1_INV+10*j;
			invasore[i][j].x1 = X1_INV+15*i;
			invasore[i][j].life = LIFE_INV;
		}
	}

	/*Condizione iniziale proiettile degli invasori*/
	proiettile[1].x1=randoms(invasore[0][ROW_INV-1].x1,invasore[COLUMN_INV-1][ROW_INV - 1].x1+SIDE_INV);
	proiettile[1].y1=invasore[0][ROW_INV - 1].y1+SIDE_INV;
	proiettile[1].life=1;

	//numero iniziale degli invasori
	num_inv = NUM_INV;

	//direzione iniziale degli invasori
	dir_inv=0;

}

//funzione utilizzata per disegnare il bordo bianco che delimita lo spazio di gioco
void board(int color_board, alt_up_video_dma_dev * pixel_dma_dev)
{
		/*Bordo in alto*/
		alt_up_video_dma_draw_box
			(
				pixel_dma_dev, color_board,
				0, START_BOARD,
				(pixel_dma_dev->x_resolution)-1, START_BOARD+EDGE_THICKNESS,
				1, 1
			);

		/*bordo in basso*/
		alt_up_video_dma_draw_box
			(
				pixel_dma_dev, color_board,
				0, END_BOARD,
				(pixel_dma_dev->x_resolution)-1, END_BOARD+EDGE_THICKNESS,
				1, 1
			);

		/* bordo sinistro*/
		alt_up_video_dma_draw_box
			(
				pixel_dma_dev, color_board,
				0, START_BOARD,
				EDGE_THICKNESS, Y_MAX,
				1, 1
			);

		/* bordo destro*/
		alt_up_video_dma_draw_box
			(
				pixel_dma_dev, color_board,
				(pixel_dma_dev->x_resolution)-1-EDGE_THICKNESS, START_BOARD,
				(pixel_dma_dev->x_resolution)-1, Y_MAX,
				1, 1
			);
}

int main()
{
	/*Puntatore che punta ad edgecapture*/
	volatile uint32_t *Edge=&KEY + 0x3 ;

	/*dichiarazione dell'enumeratore*/
	enum state_t {RESET,GAME,LOSE,WIN,PAUSE};

	/*state è una variabile di tipo state_t*/
	enum state_t state=RESET;

	/*variabili che conterranno il contenuto di edgecapture*/
	uint32_t contenitore_key_0;
	uint32_t contenitore_key_1;

	/*Definizioni dei dispositivi: dma controller per il pixel buffer, accelerometro e dma controller per il char buffer*/
	alt_up_video_dma_dev * pixel_dma_dev;
	alt_up_accelerometer_spi_dev * acc_dev;
	alt_up_video_dma_dev * char_dma_dev;

	/*Inizializzazione interval timer*/
	interval_timer_init();

	/*Colori*/
	int color_space_ship;
	int color_invaders 		= YELLOW_24;
	int color_shot			= RED_24;
	int color_board 		= WHITE_24;

	/*Variabile utilizzata dall'accelerometro*/
	int32_t x_axis;

	/*Stringhe di caratteri da visualizzare a schermo*/
	char Lives3[]	= "Lives: 3\0";
	char Lives2[]	= "Lives: 2\0";
	char Lives1[]	= "Lives: 1\0";
	char Lose[]		= " Partita persa\0";
	char Win[]		= " Partita vinta\0";
	char Press[]	= "  Premere KEY1 per giocare una partita\0";
	char Init[]		= " Premere KEY0 per iniziare una nuova partita\0";
	char Welcome[]	= " Benvenuto\0";
	char Pause[] 	= "  Pausa\0";
	char Game[]		= " Premere KEY1 per riprendere il gioco\0";

	/* initialize the pixel buffer HAL */
	pixel_dma_dev = alt_up_video_dma_open_dev("/dev/VGA_Subsystem_VGA_Pixel_DMA");

	/* initialize the accelerometer HAL */
	acc_dev= alt_up_accelerometer_spi_open_dev("/dev/Accelerometer");

	/* initialize the char buffer HAL */
	char_dma_dev = alt_up_video_dma_open_dev(
	        "/dev/VGA_Subsystem_Char_Buf_Subsystem_Char_Buf_DMA");

	/* I colori vengono convertiti da 24 bit a 16 bit */
	color_invaders = ALT_UP_VIDEO_RESAMPLE_RGB_24BIT_TO_16BIT(color_invaders);
	color_shot = ALT_UP_VIDEO_RESAMPLE_RGB_24BIT_TO_16BIT(color_shot);
	color_board = ALT_UP_VIDEO_RESAMPLE_RGB_24BIT_TO_16BIT(color_board);

	/*Utilizzate per scrivere nel centro del monitor*/
	uint8_t char_mid_x = char_dma_dev->x_resolution / 2;
	uint8_t char_mid_y = char_dma_dev->y_resolution / 2;

	/*Clear the char screen*/
	alt_up_video_dma_screen_clear(char_dma_dev, 0);

	/* clear the graphics screen */
	alt_up_video_dma_screen_clear(pixel_dma_dev, 0);
	
	/* clear the backbuffer */
	alt_up_video_dma_screen_clear(pixel_dma_dev, 1);

	/*Generazione seme per la funzione random*/
	srand(time(0));


	while(1)
	{
		/*Vengono prelevati i bit di edgecapture*/
		contenitore_key_0=*Edge & MASK0;
		contenitore_key_1=*Edge & MASK1;

		/*Pulisco Edgecapture*/
		*Edge=0xFF;


		switch(state)
		{
			//Stato iniziale
			case(RESET):

				//Se è stato premuto KEY1, cioè si vuole iniziare a giocare
				if(contenitore_key_1)
				{
					//Si passa allo stato game
					state=GAME;

					//inizializzazione dei parametri
					inizializzazione_parametri();

					/*Clear the char screen*/
					alt_up_video_dma_screen_clear(char_dma_dev, 0);

					/* clear the graphics screen */
					alt_up_video_dma_screen_clear(pixel_dma_dev, 0);
					
					/* clear the backbuffer */
					alt_up_video_dma_screen_clear(pixel_dma_dev, 1);

					/*Viene eseguito uno swap*/
					alt_up_video_dma_ctrl_swap_buffers(pixel_dma_dev);
				}
				else
				{

					/*Disegno le stringhe*/
					alt_up_video_dma_draw_string(char_dma_dev, Welcome, char_mid_x - 5, char_mid_y - 1, 0);
					alt_up_video_dma_draw_string(char_dma_dev, Press, char_mid_x / 2, char_mid_y, 0);

				}

				break;

			case(GAME):

				/*Se la partita è stata persa*/
				if(navicella.lives == 0)
				{
					//cambio di stato
					state = LOSE;

					/*Clear the char screen*/
					alt_up_video_dma_screen_clear(char_dma_dev, 0);

					/* clear the graphics screen */
					alt_up_video_dma_screen_clear(pixel_dma_dev, 0);
					
					/* clear the backbuffer */
					alt_up_video_dma_screen_clear(pixel_dma_dev, 1);

				}/*altrimenti se la partita è stata vinta*/
				else if(num_inv == 0)
				{
					//cambio di stato
					state = WIN;

					/*Clear the char screen*/
					alt_up_video_dma_screen_clear(char_dma_dev, 0);

					/* clear the graphics screen */
					alt_up_video_dma_screen_clear(pixel_dma_dev, 0);
					
					/* clear the backbuffer */
					alt_up_video_dma_screen_clear(pixel_dma_dev, 1);

				}/*altrimenti se viene premuto il tasto di reset*/
				else if(contenitore_key_0)
				{
					//cambio di stato
					state = RESET;

					/*Clear the char screen*/
					alt_up_video_dma_screen_clear(char_dma_dev, 0);

					/* clear the graphics screen */
					alt_up_video_dma_screen_clear(pixel_dma_dev, 0);
					
					/* clear the backbuffer */
					alt_up_video_dma_screen_clear(pixel_dma_dev, 1);
				}
				else if (contenitore_key_1)
				{
					state = PAUSE;
				}
				else /*altrimenti fase di gioco*/
				{	
					if(alt_up_video_dma_ctrl_check_swap_status(pixel_dma_dev) == 0)
					{
						/* Se lo schermo è stato disegnato completamente è possibile disegnare una nuova immagine */

						//In base al numero di vite della navicella cambia il suo colore e il numero di vite rimanenti viene mostrato a schermo
						
						switch(navicella.lives)
						{
							case(3):
									/*Clear the char screen*/
									alt_up_video_dma_draw_string(char_dma_dev, Lives3, 1, 1, 0);
									color_space_ship = GREEN_24;
									color_space_ship = ALT_UP_VIDEO_RESAMPLE_RGB_24BIT_TO_16BIT(color_space_ship);
									break;
							case(2):
									/*Clear the char screen*/

									alt_up_video_dma_draw_string(char_dma_dev, Lives2, 1, 1, 0);
									color_space_ship=YELLOW_24;
									color_space_ship = ALT_UP_VIDEO_RESAMPLE_RGB_24BIT_TO_16BIT(color_space_ship);
									break;

							case(1):
									/*Clear the char screen*/
									alt_up_video_dma_draw_string(char_dma_dev, Lives1, 1, 1, 0);
									color_space_ship=ORANGE_24;
									color_space_ship = ALT_UP_VIDEO_RESAMPLE_RGB_24BIT_TO_16BIT(color_space_ship);
									break;

							default:

									alt_up_video_dma_draw_string(char_dma_dev, Lives3, 1, 1, 0);
									break;
							}
						
						/* clear the backbuffer */
						alt_up_video_dma_screen_clear(pixel_dma_dev, 1);

						//Se c'è stato un interrupt relativo al movimento degli invasori
						if(timeout)
						{
							/*Se gli invasori toccano la navicella, la navicella muore (partita persa)*/
							for(j=0; j<ROW_INV; j++)
								for(i=0; i<COLUMN_INV; i++)
									if(invasore[i][j].life &&
										invasore[i][j].y1 + SIDE_INV >= navicella.y1-2*SHOT_WIDTH
										)
									{
										navicella.lives = 0;
									}

							//utilizzato per trovare un invasore vivo nelle colonne più a destra o più a sinistra (dipende dalla direzione degli invasori) della matrice degli invasori
							uint8_t vivo = 0;

							/*Aggiorno le coordinate*/
							if(!dir_inv)/*Se gli invasori si spostano verso destra*/
							{

								/*viene utilizzato un algoritmo per far si che una volta che è stata eliminata la colonna più a destra/sinistra
									gli invasori rimanenti raggiungano la fine del bordo. Si determinano gli invasori più a destra/sinistra della matrice
									degli invasori che sono vivi*/

								//coordinata colonna degli invasori più a destra inizialmente della matrice degli invasori
								i = COLUMN_INV - 1;

								//conterrà la coordinata riga dell'invasore vivo che si trova più a destra
								uint8_t riga_dx;

								//Finchè vivo == 0
								while(!vivo)
								{	//si cerca se nella colonna i c'è un invasore vivo
									for(j=0; j<ROW_INV; j++)
										if(invasore[i][j].life)	//se nella colonna i c'è un invasore vivo
										{
											riga_dx = j;		//viene salvata la coordinata riga dell'invasore vivo
											vivo=1;				//viene posto vivo=1, così da poter uscire dal ciclo while
										}

									if (vivo == 0)				//se vivo == 0 significa che nella colonna i tutti gli invasori sono stati eliminati
										i--;					//significa che il ciclo while verrà ripetuto ed dovrà essere analizzata la colonna i-1
								}

								/*Se l'invasore vivo che si trova più a destra ha raggiunto il margine destro del bordo*/
								if(invasore[i][riga_dx].x1+SIDE_INV>=((pixel_dma_dev->x_resolution)-1-EDGE_THICKNESS-1))
								{
									for (j=0; j<ROW_INV; j++)
									{
										for(i=0; i<COLUMN_INV; i++)
										{
											invasore[i][j].y1 +=5;	//tutti gli invasori vengono spostati di cinque pixel verso il basso
										}
									}
									dir_inv=1;	//una volta arrivati al margine dx gli invasori dovranno spostarsi verso sx
								}
								else/*altrimenti sposta tutti gli invasori a dx*/
								{
									for (j=0; j<ROW_INV; j++)
									{
										for(i=0; i<COLUMN_INV; i++)
										{
											invasore[i][j].x1 ++;
										}
									}
								}

							}
							else if(dir_inv)/*Se gli invasori si spostano verso sinistra*/
							{
								//ad i viene assegnata la coordinata colonna dell'invasore che nello stato iniziale del gioco (tutti gli invasori presenti) si trova più a sx
								i = 0;
								//conterrà la coordinata riga dell'invasore vivo che si trova più a sinistra
								uint8_t riga_sx;

								//finche vivo == 0 /*comportamento analogo a quanto visto sopra, solo che riferito al bordo sinistro*/
								while(!vivo)
								{
									for(j=0; j<ROW_INV; j++)
										if(invasore[i][j].life)
										{
											riga_sx = j;
											vivo=1;
										}

									if (vivo == 0)
										i++;
								}

								if(invasore[i][riga_sx].x1<=(EDGE_THICKNESS+1))/*Se gli invasori hanno raggiunto il margine sx dello schermo*/
								{
									for (j=0; j<ROW_INV; j++)
									{
										for(i=0; i<COLUMN_INV; i++)
										{
											invasore[i][j].y1 +=5;
										}
									}
									dir_inv=0;
								}/*altrimenti sposta gli invasori a sx*/
								else
								{
									for (j=0; j<ROW_INV; j++)
									{
										for(i=0; i<COLUMN_INV; i++)
										{
											invasore[i][j].x1 --;
										}
									}
								}
							}
								for(j=0; j<ROW_INV; j++)
						{
							for(i=0; i<COLUMN_INV; i++)
							{
								if(invasore[i][j].life)
									alt_up_video_dma_draw_box
									(
										pixel_dma_dev, color_invaders,
										invasore[i][j].x1, invasore[i][j].y1,
										invasore[i][j].x1 + SIDE_INV, invasore[i][j].y1 + SIDE_INV,
										1, 1
									);
							}
						}

							timeout = 0; //la variabile relativa all'interrupt viene posta a zero
						}

						//Nel caso in cui c'è stato un interrupt relativo al movimento dei proiettili
						if(timeout_shot)
						{
							/*Aggiorno le coordinate dei proiettili*/

							//nel caso in cui ci sia collisione dei proiettili con un invasore/ la navicella queste variabili assumono il valore "1"
							uint8_t collisione_nav = 0;
							uint8_t collisione_inv = 0;

							//variabili utilizzate nell'riposizionamento del proiettile dell'invasore dopo una collisione
							uint8_t controllo_i, controllo_j;

							for(j=0; j<ROW_INV; j++)
								for(i=0; i<COLUMN_INV; i++)	/*Se il proiettile della navicella colpisce l'invasore*/
									if(invasore[i][j].life &&
										
										((( proiettile[0].x1 >= invasore[i][j].x1 -1) &&
											( proiettile[0].x1  <= invasore[i][j].x1 + SIDE_INV+1) &&
											( proiettile[0].y1 >= invasore[i][j].y1 ) &&
											( proiettile[0].y1 <= invasore[i][j].y1 + SIDE_INV+1)) ||

											 

											((proiettile[0].x1+SHOT_LENGTH >= invasore[i][j].x1 -1) &&
											( proiettile[0].x1+SHOT_LENGTH  <= invasore[i][j].x1 + SIDE_INV+1) &&
											( proiettile[0].y1 >= invasore[i][j].y1 ) &&
											( proiettile[0].y1 <= invasore[i][j].y1 + SIDE_INV+1)) ||

											 

											((proiettile[0].x1 >= invasore[i][j].x1 -1) &&
											( proiettile[0].x1  <= invasore[i][j].x1 + SIDE_INV+1) &&
											( proiettile[0].y1+SHOT_WIDTH >= invasore[i][j].y1 ) &&
											( proiettile[0].y1+SHOT_WIDTH <= invasore[i][j].y1 + SIDE_INV+1)) ||

											 

											((proiettile[0].x1+SHOT_LENGTH >= invasore[i][j].x1-1 ) &&
											( proiettile[0].x1+SHOT_LENGTH  <= invasore[i][j].x1 + SIDE_INV+1) &&
											( proiettile[0].y1+SHOT_WIDTH  >= invasore[i][j].y1 ) &&
											( proiettile[0].y1+SHOT_WIDTH  <= invasore[i][j].y1 + SIDE_INV+1)) )
											
										)
									{
										invasore[i][j].life = 0;		//l'invasore muore
										collisione_nav = 1;				//c'è stata una collisione
										num_inv--;						//è stato eliminato un invasore, quindi il numero totale di invasori vivi è diminuito
									}

							/*Se il proiettile dell'invasore colpisce la navicella*/
							if(proiettile[1].life && proiettile[1].y1+SHOT_WIDTH >= navicella.y1 - SHOT_WIDTH &&
								proiettile[1].x1 <= (navicella.x1+NAV_LENGTH) &&
								proiettile[1].x1 + SHOT_LENGTH >= navicella.x1
								)
							{
								navicella.lives--;			//vengono ridotte le vite dell'invasore
								collisione_inv = 1;			//c'è stata una collisione
							}

							//Se il proiettile della navicella ha raggiunto il bordo superiore o ha colpito un invasore
							if(proiettile[0].y1<=(START_BOARD+EDGE_THICKNESS+1) || collisione_nav)
							{
								proiettile[0].x1=navicella.x1+HALF_LENGTH;						//posizione iniziale proiettile navicella
								proiettile[0].y1=navicella.y1-2*SHOT_WIDTH;
							}
							else/*altrimenti sposta il proiettile della navicella in alto*/
							{
								proiettile[0].y1-=2;
							}

							//Se il proiettile dell'invasore ha raggiunto il bordo inferiore o ha colpito la navicella
							if((proiettile[1].y1+SHOT_WIDTH >= (END_BOARD-1) || collisione_inv))
							{
									/*il proiettile dell'invasore deve essere sparato nuovamente*/
									/*viene utilizzato un algoritmo per decidere in maniera casuale quale invasore, fra gli invasori vivi, spara il proiettile*/

									//vengono generati due numeri casuali, uno compreso fra 0 e COLUMN_INV-1 ed uno compreso fra 0 e ROW_INV-1
									//così si ottiene una coordinata (colonna,riga) per un invasore all'interno della matrice
									controllo_i = randoms(0,COLUMN_INV-1);
									controllo_j = randoms(0,ROW_INV-1);

								if(invasore[controllo_i][controllo_j].life)	//se l'invasore determinato casualmente è vivo
								{	//controllo_i verrà utilizzata come coordinata colonna dell'invasore che spara il proiettile
									//vogliamo determinare l'invasore vivo nella colonna controllo_i che si trova più in basso nella matrice, così da far
									//partire il proiettile da questo invasore

									proiettile[1].life=1;	//viene posta la vita del proiettile ad "1"

									j=ROW_INV-1;

									while(!(invasore[controllo_i][j].life))//viene determinato quale invasore nella colonna controllo_i si trova più in basso ed è vivo
									{
										j--;
									}

									// genero il nuovo proiettile
									proiettile[1].x1=invasore[controllo_i][j].x1+2;
									proiettile[1].y1=invasore[controllo_i][j].y1 + SIDE_INV;
								}
								else //se l'invasore determinato in maniera casuale non è vivo allora la vita del proiettile viene posta a "0"
									proiettile[1].life=0;

							}
							else /*altrimenti sposta il proiettile degli invasori verso il basso*/
							{
								proiettile[1].y1+=2;
							}
							timeout_shot=0; //la variabile relativa all'interrupt viene posta a zero
						}

						//Leggo il valore dall'accelerometro
						alt_up_accelerometer_spi_read_x_axis(acc_dev, &x_axis);

						//Movimento navicella
						if(x_axis>10)
						{
							if((navicella.x1+NAV_LENGTH )>= ((pixel_dma_dev->x_resolution)-1-EDGE_THICKNESS-1))
								navicella.x1 = ((pixel_dma_dev->x_resolution)-EDGE_THICKNESS-1)-NAV_LENGTH;
							else
							{
								navicella.x1+=2;
							}
						}
						else if(x_axis<-10)
						{
							if(navicella.x1<=(EDGE_THICKNESS+1))
								navicella.x1=(EDGE_THICKNESS+1);
							else
							{
							navicella.x1-=2;
							}
						}

							/*Disegno il proiettile della navicella*/
							alt_up_video_dma_draw_box
							(
								pixel_dma_dev, color_shot,
								proiettile[0].x1, proiettile[0].y1,
								proiettile[0].x1+SHOT_LENGTH, proiettile[0].y1+SHOT_WIDTH,
								1, 1
							);

							/*Disegno il proiettile dell' invasore*/
							if(proiettile[1].life)
							alt_up_video_dma_draw_box
							(
								pixel_dma_dev, color_shot,
								proiettile[1].x1, proiettile[1].y1,
								proiettile[1].x1 + SHOT_LENGTH, proiettile[1].y1+SHOT_WIDTH,
								1, 1
							);
							
						//Disegno gli invasori
						for(j=0; j<ROW_INV; j++)
						{
							for(i=0; i<COLUMN_INV; i++)
							{
								if(invasore[i][j].life)
									alt_up_video_dma_draw_box
									(
										pixel_dma_dev, color_invaders,
										invasore[i][j].x1, invasore[i][j].y1,
										invasore[i][j].x1 + SIDE_INV, invasore[i][j].y1 + SIDE_INV,
										1, 1
									);
							}
						}

						//Disegno la navicella
						alt_up_video_dma_draw_box
						(
							pixel_dma_dev, color_space_ship,
							navicella.x1, navicella.y1,
							navicella.x1+NAV_LENGTH, navicella.y1+NAV_WIDTH,
							1, 1
						);

						alt_up_video_dma_draw_box
						(
							pixel_dma_dev, color_space_ship,
							navicella.x1+HALF_LENGTH, navicella.y1-SHOT_WIDTH,
							navicella.x1+NAV_LENGTH-HALF_LENGTH, navicella.y1,
							1, 1
						);

						/*disegno il bordo*/
						board(color_board,pixel_dma_dev);
							
						/* Execute a swap buffer command. This will allow us to check if the
						 * screen has
						 * been redrawn before generating a new animation frame. */
						alt_up_video_dma_ctrl_swap_buffers(pixel_dma_dev);
						}
				}

				break;

			case(LOSE):

				if(contenitore_key_0)
				{
					state=RESET;

					/*Clear the char screen*/
					alt_up_video_dma_screen_clear(char_dma_dev, 0);

					/* clear the graphics screen */
					alt_up_video_dma_screen_clear(pixel_dma_dev, 0);
					
					/* clear the backbuffer */
					alt_up_video_dma_screen_clear(pixel_dma_dev, 1);
				}
				else
				{

					/*Disegno le stringa*/
					alt_up_video_dma_draw_string(char_dma_dev, Lose, char_mid_x - 5, char_mid_y - 1, 0);
					alt_up_video_dma_draw_string(char_dma_dev, Init, char_mid_x/2, char_mid_y, 0);


				}
				break;

			case(WIN):
				if(contenitore_key_0)
				{
					state=RESET;

					/*Clear the char screen*/
					alt_up_video_dma_screen_clear(char_dma_dev, 0);

					/* clear the graphics screen */
					alt_up_video_dma_screen_clear(pixel_dma_dev, 0);
					
					/* clear the backbuffer */
					alt_up_video_dma_screen_clear(pixel_dma_dev, 1);
				}
				else
				{

					/*Disegno le stringa*/
					alt_up_video_dma_draw_string(char_dma_dev, Win, char_mid_x - 5, char_mid_y - 1, 0);
					alt_up_video_dma_draw_string(char_dma_dev, Init, char_mid_x /2, char_mid_y, 0);

				}
				break;

			case(PAUSE):
				if(contenitore_key_0)
				{
					state=RESET;

					/*Clear the char screen*/
					alt_up_video_dma_screen_clear(char_dma_dev, 0);

					/* clear the graphics screen */
					alt_up_video_dma_screen_clear(pixel_dma_dev, 0);
					
					/* clear the backbuffer */
					alt_up_video_dma_screen_clear(pixel_dma_dev, 1);
				}
				else if(contenitore_key_1)
				{
					state = GAME;

					/* clear the char screen */
					alt_up_video_dma_screen_clear(char_dma_dev, 0);
				}
				else
				{
					/*Disegno le stringa*/
					alt_up_video_dma_draw_string(char_dma_dev, Pause, char_mid_x - 5, char_mid_y - 1, 0);
					alt_up_video_dma_draw_string(char_dma_dev, Game, char_mid_x /2, char_mid_y, 0);
					alt_up_video_dma_draw_string(char_dma_dev, Init, char_mid_x /2, char_mid_y+1, 0);
				}
			break;
				default:
					state=RESET;
					break;
		}
	}
}
