#include <avr/io.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

// LCD library
#include <LiquidCrystal.h>

#define B1 9
#define B0 8
#define D4 4
#define D5 5
#define D6 6
#define D7 7

// Initialize library
uint8_t rs = B1, en = B0;

LiquidCrystal lcd(rs, en, D4, D5, D6, D7);

// Declare functions
void setup_lcd(void);
void loop(void);
void analog_init(void);
int analog_rand(void);
void display_cards(void);
void led_on(void);
void read_bet(void);

// Define operators
#define SET_BIT(reg, pin)			(reg) |= (1 << (pin))
#define CLEAR_BIT(reg, pin)			(reg) &= ~(1 << (pin))
#define WRITE_BIT(reg, pin, value)	(reg) = (((reg) & ~(1 << (pin))) | ((value) << (pin)))
#define BIT_VALUE(reg, pin)			(((reg) >> (pin)) & 1)
#define BIT_IS_SET(reg, pin)		(BIT_VALUE((reg),(pin))==1)

// Create deck of 52 cards
// Jack = 11
// Queen = 12
// King = 13
// Ace = 0
int deck[52] = {0, 0, 0, 0, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 11, 11, 11, 11, 12, 12, 12, 12, 13, 13, 13, 13};

int shuffled_deck[52];
int players_cards[4];
int dealers_cards[4];
int player_score = 0;
int dealer_score = 0;
int last_position = 0;
int player_stood = 0;
int dealer_stood = 0;
int game_start = 0;
int player_money = 100;
int player_bet = 10;

// Hit button
uint8_t prev_state = 0;
volatile uint8_t state_count = 0;
volatile uint8_t switch_state = 0;

// Stand button
uint8_t prev_state1 = 0;
volatile uint8_t state_count1 = 0;
volatile uint8_t switch_state1 = 0;

void init_deck(void){
	analog_init();
	// Create a temporary deck and set to -1
	// Set temp cards array to -1
	int temp_deck[52];
	for (int i = 0; i < 52; i++){
		temp_deck[i] = -1;
	}
	
	// Set dealt cards to -1 to know which spot isnt an actual card
	for (int i = 0; i < 4; i++){
		players_cards[i] = -1;
		dealers_cards[i] = -1;
	}
	
	srand(analog_rand());
	
	// Set a random number between 0 and 51 inclusive to be the position that deck[i] is placed
	// If the temporary deck contains that position, set a new random number 
	for (int i = 0; i < 52; i++)
	{
		int rand_val = 0 + rand() % 51;
		
		for (int j = 0; j < 52; j++)
		{
			if (temp_deck[j] == rand_val) rand_val = 0 + rand() % 51;
		}
		temp_deck[i] = rand_val;
		shuffled_deck[rand_val] = deck[i];
	}
	
	// Reset position to start of deck 
	last_position = 0;
}

// Determine character card or number card to display
void display_card(int card)
{
	if (card == 10) lcd.print("10");
	else if (card == 9) lcd.print("9");
	else if (card == 8) lcd.print("8");
	else if (card == 7) lcd.print("7");
	else if (card == 6) lcd.print("6");
	else if (card == 5) lcd.print("5");
	else if (card == 4) lcd.print("4");
	else if (card == 3) lcd.print("3");
	else if (card == 2) lcd.print("2");
	else if (card == 1) lcd.print("1");
	else if (card == 11) lcd.print("J");
	else if (card == 12) lcd.print("Q");
	else if (card == 13) lcd.print("K");
	else if (card == 0) lcd.print("A");


}

void deal_cards(void)
{
	// Deal cards to player
	players_cards[0] = shuffled_deck[last_position];
	last_position++;
	
	// Deal cards to dealer
	dealers_cards[0] = shuffled_deck[last_position];
	last_position++;
}

// Deal a single card to whoevers turn it is
// Set the cards position in the shuffled deck to -1 to remove it from play 
void player_hit(int hits)
{	
	int how_many_cards = 0;
		
	// If player hits 
	if (hits == 1)
	{
		for (int i = 0; i < 4; i++)
		{
			if (players_cards[i] != -1) how_many_cards++;
		}
		
		if (how_many_cards == 4) player_stood = 1;
		else if (player_score >= 21) player_stood = 1;
		else if (how_many_cards == 1)
		{
			players_cards[1] = shuffled_deck[last_position];
		}
		else if (how_many_cards == 2)
		{
			players_cards[2] = shuffled_deck[last_position];
		}
		else if (how_many_cards == 3)
		{
			players_cards[3] = shuffled_deck[last_position];
		}
		
		if (how_many_cards != 4) last_position++;
		_delay_ms(500);
	}
	
	// If dealer hits
	else if (hits == 2)
	{
		for (int i = 0; i < 4; i++)
		{
			if (dealers_cards[i] != -1) how_many_cards++;
		}
		
		if (how_many_cards == 4) dealer_stood = 1;
		else if (dealer_score > 14) dealer_stood = 1;
		else if (how_many_cards == 1) 
		{
			dealers_cards[1] = shuffled_deck[last_position++];
		}
		else if (how_many_cards == 2)
		{
			dealers_cards[2] = shuffled_deck[last_position++];
		}
		else if (how_many_cards == 3)
		{
			dealers_cards[3] = shuffled_deck[last_position++];
		}
		
		if (how_many_cards != 4) last_position++;
	}
}

int confirm_val(int card)
{	
	int temp_score = 0;
	
	// Convert jack, queen, king to be worth 10
	if (card == 11 || card == 12 || card == 13) card = 10;
	
	
	// Convert ace to be worth 1 or 11
	if (card == 0) 
	{
		for (int i = 0; i < 4; i++)
		{
			if (players_cards[i] != -1)
			{
				temp_score += players_cards[i];
			}
		}
		
		if (temp_score + 11 <= 21) card = 11;
		else card = 1;
	}
	
	if (card == -1) card = 0;
	
	return card;
}

void calculate_scores(void)
{
	// Calculate player score
	player_score = confirm_val(players_cards[0]);
	player_score += confirm_val(players_cards[1]);
	player_score += confirm_val(players_cards[2]);
	player_score += confirm_val(players_cards[3]);

	// Calculate dealer score
	dealer_score = confirm_val(dealers_cards[0]);
	dealer_score += confirm_val(dealers_cards[1]);
	dealer_score += confirm_val(dealers_cards[2]);
	dealer_score += confirm_val(dealers_cards[3]);
	
	if (player_score >= 21) player_stood = 1;
	if (dealer_score >= 15) dealer_stood = 1;
}

void player_turn(void)
{
	if (game_start)
	{
		calculate_scores();
		if (switch_state != prev_state)
		{
			player_hit(1);
			prev_state = switch_state;
			
		}
		
		if (switch_state1 != prev_state1)
		{
			player_stood = 1;
			prev_state1 = switch_state1;
		}
	}
}

void dealer_turn(void)
{
	calculate_scores();
	if (dealer_stood == 0) player_hit(2);
}

void display_bet(void)
{
	char temp_buf[256];
	if (player_bet >= 10)lcd.print(itoa(player_bet, temp_buf, 10));
}

// Go through turns and check who wins
void play_game(void)
{
	int winner = 0;
	
	// Loop turns until player or dealer wins
	while(winner == 0)
	{
		// Update cards on screen
		display_cards();
		
		// Turn leds on
		led_on();
				
		// Check if the player and dealer have stood 
		if (player_stood == 0) player_turn();
		else if (player_stood == 1 && dealer_stood == 0) dealer_turn();
		
		// Check if both players have stood then determine winner
		if (player_stood == 1 && dealer_stood == 1)
		{
			// Draw
			if (player_score == dealer_score) winner = 3;
			// Player wins
			else if (player_score == 21 && dealer_score != 21) winner = 1;
			else if (player_score < 21 && player_score > dealer_score) winner = 1;
			// Dealer wins all other situations
			else if (dealer_score == 21 && player_score != 21) winner = 2;
			else if (dealer_score < 21 && player_score < dealer_score) winner = 2;
			else winner = 2;
		}
	}
	
	lcd.clear();
	lcd.setCursor(2,0);
	lcd.print("Calculating");
	lcd.setCursor(4,1);
	lcd.print("Scores");
	for (int i = 0; i < 3; i++)
	{
		_delay_ms(100);
		lcd.print(".");
	}
	_delay_ms(1000);
	lcd.clear();

	// Display who won
	_delay_ms(1500);
	if (winner == 1)
	{
		lcd.setCursor(0,0);
		lcd.print("Player wins!");
		lcd.setCursor(0,1);
		player_money += player_bet;
		lcd.print("You win $");
		display_bet();
	}
	else if (winner == 2)
	{
		lcd.print("Dealer wins!");
		lcd.setCursor(0,1);
		player_money -= player_bet;
		lcd.print("You lose $");
		display_bet();
	}
	else if (winner == 3)
	{
		lcd.print("Its a draw!");
		lcd.setCursor(0,1);
		lcd.print("You keep $"); 
		display_bet();
	}
	_delay_ms(3000);
	
	// Reset variables
	last_position = 0;
	player_stood = 0;
	dealer_stood = 0;
	player_score = 0;
	dealer_score = 0;
}

void display_cards(void)
{
	lcd.setCursor(0,0);
	lcd.print("You:");
	lcd.setCursor(7,0);
	lcd.print("Dealer:");
	lcd.setCursor(0,1);
	for (int i = 0; i < 4; i++)
	{
		if (players_cards[i] != -1)
		{
			display_card(players_cards[i]);
		}
	}
	
	lcd.setCursor(8,1);
	if (player_stood == 0)
	{
		display_card(dealers_cards[0]);
	}
	else 
	{
		for (int i = 0; i < 4; i++)
		{
			if (dealers_cards[i] != -1)
			{
				display_card(dealers_cards[i]);
			}
		}
		_delay_ms(1000);
	}
}

void place_bet(void)
{
	int bet_not_placed = 1;
	int prev_bet = 0;
	char bet_buffer[256];	
	while(bet_not_placed)
	{	
		// Print current money
		lcd.setCursor(0,0);
		lcd.print("Cash: $");
		lcd.print(itoa(player_money, bet_buffer, 10));
		
		// Choose bet
		lcd.setCursor(0,1);
		lcd.print("Bet:  $");
		
		// Check bet value 
		read_bet();
				
		player_bet = ADC/10;
		if (player_bet > player_money) player_bet = player_money;
		else if (player_bet > 100) player_bet = 100;
		else if (player_bet <= 1) player_bet = 10;
		lcd.setCursor(7,1);
		display_bet();
		
		if (prev_bet != player_bet) lcd.clear();
		prev_bet = player_bet;
		
		// Check if button has been pressed
		if (switch_state != prev_state)
		{
			prev_state = switch_state;
			bet_not_placed = 0;
		}
	}
	
	lcd.clear();
	lcd.print("Bet placed!");
	_delay_ms(1500);
	lcd.clear();
}

// ARDUINO FUNCTIONS
// Setup ports for leds 
void init_leds(void){
	DDRB |= (1<<5);
	DDRB |= (1<<4);
	DDRB |= (1<<3);
}

// Turn leds on
void led_on(void){
	if (player_score < 21) PORTB = 0b00100000;
	if (player_score < 21 && player_score > 15) PORTB = 0b00110000;
	if (player_score > 21) PORTB = 0b00111000;
	if (player_score == 0) PORTB = 0b00111000;
}

// Turn leds off
void clear_leds(void){
	PORTB = 0b00100000;
}

// Setup timer 0 for interrupts
void setup_timer1(void)
{
	TCCR1A = 0;
	TCCR1B = 1;
	TIMSK1 = 1;
	
	// Enable ports 5 and 4 up for buttons with debouncing
	CLEAR_BIT(DDRC, 5); 
	CLEAR_BIT(DDRC, 4); 
	sei();
}


ISR(TIMER1_OVF_vect){
	uint8_t mask = 0b00011111;
	
	// Hit Button
	state_count = ((state_count << 1) & mask) | ((PINC>>5)&1);
	if (state_count == 0) switch_state = 0;
	else if (state_count == mask) switch_state = 1;
	
	// Stand Button
	state_count1 = ((state_count1 << 1) & mask) | ((PINC>>4)&1);
	if (state_count1 == 0) switch_state1 = 0;
	else if (state_count1 == mask) switch_state1 = 1;
}

// Setup ADC to read pin for random value
void analog_init(void)
{
	ADMUX = 0b00000000;
	ADCSRA = 0b00000000;
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
	
	// A0
	ADMUX = (1 << REFS0);
}

// Get random value from A0 (unused port) 
int analog_rand(void) {
	
    ADCSRA |= (1 << ADSC);

    while (ADCSRA & (1 << ADSC)) {}

    return (ADC);
}

void read_bet(void)
{	
	ADMUX = 0b00000000;
	ADCSRA = 0b00000000;
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
	
	// Set port to A3
	ADMUX = (1 << REFS0) | (1 << MUX0) | (1 << MUX1);
	
	ADCSRA |= (1 << ADSC);
	
	while (ADCSRA & (1 << ADSC)) {}
}

// Set up a new game 
void new_game(void)
{
	led_on();
	place_bet();
	game_start = 1;
	init_deck();
	deal_cards();
	play_game();
}

int main(void) {
	setup_timer1();
	init_leds();
	setup_lcd();
	while(1){
		while(player_money > 0)
		{
			new_game();
			clear_leds();
			_delay_ms(50);
			lcd.clear();
		}
		if (player_money <= 0)
		{
			lcd.print("No money left!");
			lcd.setCursor(0,1);
			lcd.print("Game reset");
			for (int i = 0; i < 3; i++) 
			{
				lcd.print(".");
				_delay_ms(100);
			}
			_delay_ms(2000);
			player_money = 100;
			lcd.clear();
			lcd.setCursor(0,0);
		}
	}
}

void setup_lcd(void) {

  // Setup the LCD in 4-pin mode
  lcd.begin(16,2);

  // Print title message
  lcd.setCursor(1,0);
  lcd.print("BLACKJACK");
  _delay_ms(1000);
  lcd.clear();
}
