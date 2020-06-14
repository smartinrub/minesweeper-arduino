/*********************************************
Buscaminas - NOKIA 5110 LCD Display
 ---> hugrama.dynu.com

The circuit:
 - Powered by Arduino UNO board
 - Graphic LCD 84x48 - NOKIA 5110
 - Joystick Module
 - 3 x Pushbuttons
 - Wires
 - Resistors (use 330 Ohm resistor on backlight LED)
 - Buzzer (optional)

Version: 1.0
Created 21/04/2016
By Sergio Martín Rubio
Modified 23/04/2016
By Sergio Martín Rubio
Contact: econsergio@gmail.com
All text above must be included in any redistribution
**********************************************/

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

#define DIMENSION 6
#define MINES 3

//Declaración de pines LCD NOKIA 5110
int pin_DIN = 4;   //Serial data input - 10k Ohm resistor
int pin_DC = 5;    //Data/Commands choice - 10k Ohm resistor
int pin_SCLK = 3;  //Serial clock - 10k Ohm resistor
int pin_CE = 6;    //Selección de chip - 1k Ohm resistor
int pin_RST = 7;   //RESET - 10k Ohm resistor
Adafruit_PCD8544 display = Adafruit_PCD8544 (pin_SCLK, pin_DIN, pin_DC, pin_CE, pin_RST);

// Joystick pins
int SW_pin = 2; // pin digital pulsador joystick
int X_pin = 0; // pin analogico eje X
int Y_pin = 1; // pin analogico eje Y

//pin botones para marcar minas
int mark_pin = 12;
int unmark_pin = 13;

int buzzer_pin = 8;

char board[7][7];
char board_mines[7][7];
int row_mine[7];
int column_mine[7];

//Funciones
void generate_board(char b[][7]);
void display_board(char b[][7], int cursor_x, int cursor_y, int fl);
void position_mines(int mine_x[], int mine_y[]);
void set_mines(char bm[][7], int mine_x[], int mine_y[]);
void position_cursor(int *cursor_x, int *cursor_y);
int check_position(int mine_x[], int mine_y[], int cursor_x, int cursor_y);
int count_mines(char bm[][7], int cursor_x, int cursor_y);
void expand(char b[][7], char bm[][7], int cursor_x, int cursor_y);
int check_unconvers(char b[][7], char bm[][7]);
void clear_mines(char bm[][7]);

void setup() {
  pinMode(SW_pin, INPUT);
  pinMode(mark_pin, INPUT);
  pinMode(unmark_pin, INPUT);
  pinMode (buzzer_pin, OUTPUT);
  Serial.begin(9600);

  //iniciamos pantalla LCD
  display.begin();
  display.setContrast(50); //Es el valor de contraste recomendado
  display.clearDisplay();
  display.setTextSize(2);
  display.println("Busca");
  display.println("Minas");
  display.setTextSize(1);
  display.println("By: Sergio");
  display.println("Martin Rubio");
  display.display();
  delay(2000);
}

void loop() {
  char option = ' ';
  int state = 0; //Cuando esta variable es 1 el juador pierde
  int adjacent_mines;
  int cursor_row = 3, cursor_column = 3;
  int	mines_uncovered = 0;
  int	flags = 0;

  /*Generamos tablero*/
  generate_board(board);
  /*Generamos las posiciones aleatorias de las minas*/
  position_mines(row_mine, column_mine);
  /*Colocamos minas en tablero_minas*/
  set_mines(board_mines, row_mine, column_mine);

  do{
    //Nos permite movernos entre las posiciones del tablero
    position_cursor(&cursor_row, &cursor_column);
    /*Mostramos el tablero*/
    display_board(board, cursor_row, cursor_column, flags);

    //Seleccionamos opcion
    if(digitalRead(SW_pin) == 1){
      option = 'a';
      delay(100);
    }
    else{
      if(digitalRead(mark_pin) == 1){
        option = 'b';
        delay(100);
      }
      else
        if(digitalRead(unmark_pin) == 1){
          option = 'c';
          delay(100);
        }
        else
          option = ' ';
    }

    //Nucleo programa en donde se realiza la opcion elegida
    switch(option){
      //Descubrimos posicion
      case 'a':
        if (board[cursor_row][cursor_column] != '+'){ //comprobams que no sea un posicion marcada
          //Devuelve 1 si encontro mina o 0 en caso contrario
          state = check_position(row_mine, column_mine, cursor_row, cursor_column);
          if(state == 0){
            adjacent_mines = count_mines(board_mines, cursor_row, cursor_column);
            board[cursor_row][cursor_column] = adjacent_mines + '0'; //Convertimos int a char con '0'
            if(adjacent_mines == 0){ /*Si no hay ninguna mina alrededor
                                comprobamos posiciones adyacentes*/
              board[cursor_row][cursor_column] = '-';
              //Descubre automaticamente posiciones sin minas alrededor
              expand(board, board_mines, cursor_row, cursor_column);
            }
          }
        }
        break;
      //marcamos posicion donde se encuentra la mina
      case 'b':
        if(board[cursor_row][cursor_column] == '#' && board_mines[cursor_row][cursor_column] == '*' && flags < MINES){
          board[cursor_row][cursor_column] = '+';
          mines_uncovered++;
          flags++;
        }
        if(board[cursor_row][cursor_column] == '#' && board_mines[cursor_row][cursor_column] != '*' && flags < MINES){
          board[cursor_row][cursor_column] = '+';
          flags++;
        }
        break;
      //desmarcamos posicion de mina
      case 'c':
        if(board[cursor_row][cursor_column] == '+' && board_mines[cursor_row][cursor_column] == '*' && flags > 0){
          board[cursor_row][cursor_column] = '#';
          mines_uncovered--;
          flags--;
        }
        if(board[cursor_row][cursor_column] == '+' && board_mines[cursor_row][cursor_column] != '*' && flags > 0){
          board[cursor_row][cursor_column] = '#';
          flags--;
        }
        break;
    }

    if (check_unconvers(board, board_mines) == 1)
      state = 2;

  }while(state == 0);

    delay(200);
    display.display();
    display.clearDisplay();
    display.setTextSize(2);
  if(state == 1){
    display.println("  You");
    display.println("  Lose");
  }
  else{
    display.println(" Bien");
    display.println("Hecho!");
  }
  display.display();
  delay(2000);
  clear_mines(board_mines); //limpiamos el tablero de minas para empezar de nuevo
}

void generate_board(char b[][7]){
	int x, y;
	for(x = 1;  x <= DIMENSION; x++){
		for(y = 1; y <= DIMENSION; y++){
			b[x][y] = '#';
		}
	}
}
void display_board(char b[][7], int cursor_x, int cursor_y, int fl){
	int x, y;
  char text_flags[] = "FLAGs";
  int r; //salto de linea

  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);

	for(x = 1; x <= DIMENSION; x++){
		for(y = 1; y <= DIMENSION; y++){
      if(x == cursor_x && y == cursor_y){
        display.print(b[x][y]);
        display.print("<");
      }
      else{
  			display.print(b[x][y]);
        display.print(" ");
      }
		}
    display.println();
	}
  for(int i = 0, r = 0; i < 6; i++, r += 8){
    if(i < 5){
      if(i == 4)
        r -= 2;
      display.setTextSize(1);
      display.setCursor(75, r);
      display.println(text_flags[i]);
    }
    else{
      display.setTextSize(1);
      display.setCursor(75, r + 2);
      display.print(MINES - fl);
    }
  }
  display.display();
  display.clearDisplay();
}
void position_mines(int mine_x[], int mine_y[]){
	int i, j;
	randomSeed(analogRead(5));

	for(i = 1; i <= MINES; i++){
		mine_x[i] = random(1, DIMENSION);
		mine_y[i] = random(1, DIMENSION);
		for(j = 1 ; j<i ; j++){
			if(mine_x[i] == mine_x[j] && mine_y[i] == mine_y[j])
				i--;
		}
	}
}

void set_mines(char bm[][7], int mine_x[], int mine_y[]){
	int i;
	for(i=1; i <= MINES; i++){
		bm[mine_x[i]][mine_y[i]] = '*';
	}
}
void position_cursor(int *cursor_x, int *cursor_y){
  int joy_x = analogRead(Y_pin);
  int joy_y = analogRead(X_pin);

  if(joy_x > 700 && *cursor_x < 6){
    (*cursor_x)++;
    digitalWrite (buzzer_pin, HIGH);
    delay (200);
    digitalWrite (buzzer_pin, LOW);
  }
  else if(joy_x < 300 && *cursor_x > 1){
    (*cursor_x)--;
    digitalWrite (buzzer_pin, HIGH);
    delay (200);
    digitalWrite (buzzer_pin, LOW);
  }

  if(joy_y > 700 && *cursor_y < 6){
    (*cursor_y)++;
    digitalWrite (buzzer_pin, HIGH);
    delay (200);
    digitalWrite (buzzer_pin, LOW);
  }
  else if(joy_y < 300 && *cursor_y > 1){
    (*cursor_y)--;
    digitalWrite (buzzer_pin, HIGH);
    delay (200);
    digitalWrite (buzzer_pin, LOW);
  }
}
int check_position(int mine_x[], int mine_y[], int cursor_x, int cursor_y){
	int i;
	for(i = 1; i <= MINES; i++){
		if(mine_x[i] == cursor_x && mine_y[i] == cursor_y)
			return 1;
	}
	return 0;
}
int count_mines(char bm[][7], int cursor_x, int cursor_y){
  int i, j;
  int total = 0;

  for(i = -1; i <= 1; i++){
    for(j = -1; j <= 1; j++){
      if(bm[cursor_x - i][cursor_y - j] == '*')
        total++;
    }
  }
	return total;
}

void expand(char b[][7], char bm[][7], int cursor_x, int cursor_y){
  int x, y;
  int i, j;

  for(i = -1; i <= 1; i++){
    for(j = -1; j <= 1; j++){
      x = cursor_x;
      y = cursor_y;
      while(count_mines(bm, x, y) == 0 && b[x+i][y+j] != '-'){
        x += i;
        y += j;
        if(x > 0 && y > 0 && x < 7 && y < 7){
          if(count_mines(bm, x, y) == 0){
      			b[x][y] = '-';
            expand(b, bm, x, y);
          }
      		else
      			b[x][y] = count_mines(bm, x, y) + '0';
        }
    	}
    }
  }
}
int check_unconvers(char b[][7], char bm[][7]){
	int x, y;
	for(x = 1 ; x <= DIMENSION; x++){
		for(y = 1 ; y <= DIMENSION; y++){
			if(bm[x][y] != '*'){
				if(b[x][y] == '#' || b[x][y] == '+')
					return 0;
			}
		}
	}
	return 1;
}
void clear_mines(char bm[][7]){
  int x, y;
  for(x = 1;  x <= DIMENSION; x++){
    for(y = 1; y <= DIMENSION; y++){
      bm[x][y] = ' ';
    }
  }
}
