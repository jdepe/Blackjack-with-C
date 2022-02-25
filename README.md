# Blackjack-on-TinkerCAD using C

https://www.tinkercad.com/things/7GapfDzjE3K-cab202-assignment/editel?sharecode=tnRGU63bKEYeDIwvsyNXQwWHXqGGJ7Bhwtcd_dHeBM4

C project on Arduino Uno 
Simplified version of Blackjack where players:
	- only have up to 4 cards
	- can only hit or stand
	
A money system is implemented for the player where they can choose how much to bet. If all the money is lost, the system resets itself. 

Included functionality for Arduino:
	- Two switches (Hit and Stand)
	- Debouncing (used for reading the buttons)
	- 3 LEDs (coloured to give a warning about how close a player is to 21)
	- Potentiometer (uses analog input channels for a player to select a bet amount)
	- Empty analog input (generates noise for random seed for deck shuffling)
	- LCD screen
