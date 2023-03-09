#include <stdio.h>
#include <ncurses.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#define COLOR_WORDLE_GREY 17
#define COLOR_WORDLE_GREEN 16
#define COLOR_WORDLE_YELLOW 18

typedef enum {
    CONTINUE_GAME,
    NEW_GAME,
    QUIT
} options; //optiuni pentru pause menu

typedef enum {
    PLAY,
    PAUSE
} menus; //meniul activ: play menu sau pause menu

typedef enum {
    NOT_ENDED, 
    WON,
    LOST
} endstate; //starea jocului

typedef enum {
    BLACK = 0,
    GREY = 1,
    YELLOW = 2,
    GREEN = 3,
    WHITE = 4,
    RED = 5
} color; //toate culorile folosite 

typedef enum {
    NO_ERRORS,
    NOT_ENOUGH_LETTERS,
    NOT_IN_WORDLIST
} errortype; //tipul de eroare

typedef struct {
    char letter_grid[6][5]; //aici se retin literele scrise
    color color_grid[6][5]; //culoarea fiecarei litere
    int y, x; //coordonatele actuale
    endstate ended; //starea jocului
    errortype error; //tipul de eroare
    menus menu; //meniul activ
    int pausemenu; //optiunea din pausemenu la care ne aflam
    bool quit; //daca am dat quit sau nu
} gamestate;

void init_window() { 
    initscr();
    cbreak(); //permite oprirea programului cu CTRL+C
    noecho(); //nu afiseaza inputul de la tastatura
    keypad(stdscr, true); // imi permite sa folosesc backspace
    
    start_color();
    init_color(COLOR_WORDLE_GREY, 300, 300, 300); 
    init_color(COLOR_WORDLE_GREEN, 250, 475, 200);
    init_color(COLOR_WORDLE_YELLOW, 600, 525, 100); //definirea unor noi culori si perechi de culori
    init_pair(1, COLOR_WHITE, COLOR_WORDLE_GREY);
    init_pair(2, COLOR_WHITE, COLOR_WORDLE_YELLOW);
    init_pair(3, COLOR_WHITE, COLOR_WORDLE_GREEN);
    init_pair(4, COLOR_BLACK, COLOR_WHITE);
    init_pair(5, COLOR_RED, COLOR_WHITE);
}

void draw_logo (int y, int x) { //desenarea logo-ului
    move(y, x);
    attron(COLOR_PAIR(GREEN));
    printw("W");
    attroff(COLOR_PAIR(GREEN));
    printw(" ");
    attron(COLOR_PAIR(GREY));
    printw("O");
    attroff(COLOR_PAIR(GREY));
    printw(" ");
    attron(COLOR_PAIR(YELLOW));
    printw("R");
    attroff(COLOR_PAIR(YELLOW));
    printw(" ");
    attron(COLOR_PAIR(GREEN));
    printw("D");
    attroff(COLOR_PAIR(GREEN));
    printw(" ");
    attron(COLOR_PAIR(GREY));
    printw("L");
    attroff(COLOR_PAIR(GREY));
    printw(" ");
    attron(COLOR_PAIR(YELLOW));
    printw("E");
    attroff(COLOR_PAIR(YELLOW));
}

void draw_border(int y1, int x1, int y2, int x2) { //desenarea chenarului de joc
    mvaddch(y1, x1, ACS_ULCORNER);
    mvaddch(y1, x2, ACS_URCORNER);
    mvaddch(y2, x1, ACS_LLCORNER);
    mvaddch(y2, x2, ACS_LRCORNER);
    
    for(int i = y1 + 1; i < y2; i++) {
        mvaddch(i, x1, ACS_VLINE);
        mvaddch(i, x2, ACS_VLINE);
    }

    for(int i = x1 + 1; i < x2; i++ ) {
        mvaddch(y1, i, ACS_HLINE);
        mvaddch(y2, i, ACS_HLINE);
    }
}

void pick_word(char *guessword, char **word, int n) { //alegerea randomizata a cuvantului de ghicit din lista de cuvinte
    srand(time(0)); //schimb seed ul in functie de timpul actual
    strcpy(guessword, word[rand() % n]);
}

void generate_word(char ***word, int *n, char *guessword, char *limba) { //citirea listei de cuvinte
    char temp[7];
    FILE *inputfile;
    if(!strcmp(limba, "engleza")) { //in functie de argumentele din linia de comanda, aleg limba cuvantului de ghicit
        inputfile = fopen("word_list.txt", "r");
    }
    else if(!strcmp(limba, "romana")) {
        inputfile = fopen("lista_cuvinte.txt", "r");
    }
        
    while(fgets(temp, 7, inputfile)) {
        *word = realloc(*word, (*n + 1) * sizeof(char *));
        (*word)[*n] = malloc(7 * sizeof(char));
        strcpy((*word)[*n], temp);
        (*word)[*n][5] = 0; 
        (*n)++;
    }

    fclose(inputfile);

    pick_word(guessword, *word, *n);
}

void draw_gamestate(gamestate game, char *guessword) { //functia care imi deseneaza fiecare stare a jocului
    int x_offset = getmaxx(stdscr) / 2 - 2, y_offset = getmaxy(stdscr) / 2 - 3; // mijlocul terminalului
    clear(); //inainte de fiecare desenare, tot ecranul este sters

    draw_logo(y_offset - 3, x_offset - 3);
  
    if(game.menu == PLAY) { //trateaza meniul de play
        curs_set(1); //afiseaza cursorul
        draw_border(y_offset - 1, x_offset - 1, y_offset + 6, x_offset + 5);
        for(int i = 0; i < 6; i++) {
            move(y_offset + i, x_offset);

            for(int j = 0; j < 5; j++) {
                if(game.letter_grid[i][j]) {
                    attron(COLOR_PAIR(game.color_grid[i][j]));
                    printw("%c", toupper(game.letter_grid[i][j]));
                    attroff(COLOR_PAIR(game.color_grid[i][j]));
                } 
                else {
                    printw(" ");
                }
            }
        }

        if(game.error == NOT_IN_WORDLIST) {
            attron(COLOR_PAIR(WHITE));
            mvprintw(y_offset + 8, x_offset - 6, "Not in word list"); //daca cuvantul scris nu se afla in lista, afiseaza aceasta eroare
            attroff(COLOR_PAIR(WHITE));
        }
        if(game.error == NOT_ENOUGH_LETTERS) {
            attron(COLOR_PAIR(WHITE));
            mvprintw(y_offset + 8, x_offset - 7, "Not enough letters"); //daca am apasat enter fara sa fi scris 5 litere, afiseaza aceasta eroare
            attroff(COLOR_PAIR(WHITE));
        }

        if(game.ended == WON) {
            curs_set(0);
            attron(COLOR_PAIR(GREEN));
            mvprintw(y_offset + 8, x_offset - 3, "You won! :)"); //daca am ghicit cuvantul, afiseaza acest mesaj
            attroff(COLOR_PAIR(GREEN));
            mvprintw(getmaxy(stdscr) - 1, 1, "Press 'Enter' to play again");
            mvprintw(getmaxy(stdscr) - 1, getmaxx(stdscr) - 17, "Press 'q' to quit"); //cu optiunile de a juca din nou sau de a iesi din joc
        }
        
        if(game.ended == LOST) {
            curs_set(0);
            for(int i = 0; i < 5; i++) {
                guessword[i] = toupper(guessword[i]); //transforma toate literele in majuscule pt a putea fi afisate
            }
            attron(COLOR_PAIR(RED));
            mvprintw(y_offset + 8, x_offset - 3, "You lost :("); //daca nu am ghicit cuvantul in 6 incercari, afiseaza acest mesaj
            mvprintw(y_offset + 9, x_offset - 7, "The word was '%s'", guessword); //impreuna cu cuvantul care trebuia ghicit
            attroff(COLOR_PAIR(RED));
            mvprintw(getmaxy(stdscr) - 1, 1, "Press 'Enter' to play again");
            mvprintw(getmaxy(stdscr) - 1, getmaxx(stdscr) - 17, "Press 'q' to quit");
        }

        if(game.ended == NOT_ENDED) {
            mvprintw(getmaxy(stdscr) - 1, 1, "Press ':' for the options menu");
        }

        move(y_offset + game.y, x_offset + game.x);
    }
    else if(game.menu == PAUSE) {
        curs_set(0); //ascunde cursorul
        mvprintw(y_offset + 2, x_offset - 4, "Continue game");
        mvprintw(y_offset + 3, x_offset - 4 , "New game"); //optiunile din meniul de pauza
        mvprintw(y_offset + 4, x_offset - 4, "Quit");
        mvprintw(y_offset + 2 + game.pausemenu, x_offset - 6, ">"); //sageata imi indica deasupra carei optiuni ma aflu
    }
}

bool check_wordlist (char *word, char **wordlist, int n) { //functie care cauta cuvantul introdus de jucator in lista de cuvinte
    for(int i = 0; i < n; i++) {
        if(!strcmp(word, wordlist[i])) {
            return true;
        }
    }

    return false;
}

void wait_input(gamestate *game, char *word, char **wordlist, int n) {
    game->error = NO_ERRORS; //la fiecare intrare in functie, curat erorile
    int input = getch(); //input de la tastatura
    if(game->menu == PLAY) { //trateaza meniul de play
        if(input == '\n') { //cazul in care jucatorul apasa "Enter"
            if(game->x == 5) { //tratez daca am scris 5 litere
                char buffer[6];
                strcpy(buffer, word);

                if(!check_wordlist(game->letter_grid[game->y], wordlist, n)) { //verific daca se afla in wordlist cuvantul introdus de jucator
                    game->error = NOT_IN_WORDLIST;
                    return;
                }

                int guessed = 0;
                for(int i = 0; i < 5; i++) {
                    char *p = strchr(buffer, game->letter_grid[game->y][i]); 
                    if(p) {
                        if(game->letter_grid[game->y][i] == word[i]) { // ;a prima trecere prin cuvant, verific daca literele gasite se afla pe aceeasi pozitie ca
                            game->color_grid[game->y][i] = GREEN; //in guessword si le colorez cu verde
                            *p = ' '; //scot din buffer litera tratata
                            guessed++; //pentru fiecare litera colorata cu verde, adun 1
                        }
                    }
                    else {
                        game->color_grid[game->y][i] = GREY; //daca nu e gasita in guessword, o colorez cu gri
                    }
                }
                for(int i = 0; i < 5; i++) {
                    char *p = strchr(buffer, game->letter_grid[game->y][i]);
                    if(!game->color_grid[game->y][i]) {
                        if(p) {
                            *p = ' ';
                            game->color_grid[game->y][i] = YELLOW; //la a doua trecere, colorez cu galben literele gasite, dar nu pe aceeasi pozitie
                        }
                        else {
                            game->color_grid[game->y][i] = GREY;
                        }
                    }
                }

                if(guessed == 5) { //daca am colorat 5 litere cu verde, am ghicit cuvantul
                    game->ended = WON;
                }
                else if(game->y == 5) {
                    game->ended = LOST; //daca ma aflam la cuvantul 6 si tot nu am ghicit, am pierdut
                }

                game->y++; //trec la urmatorul rand
                game->x = 0;
            }
            else {
                game->error = NOT_ENOUGH_LETTERS; //daca nu scrisesem 5 litere, trec la aceasta eroare
            }
        }
        else if(isalpha(input)) { //jocul accepta ca input doar litere
            if(game->x < 5) {
                game->letter_grid[game->y][game->x++] = tolower(input); //trece tot inputul in lowercase
            }
        }
        else if(input == KEY_BACKSPACE) { //backspace ul sterge caracterele scrise pana randul este gol
            if(game->x > 0) {
                game->letter_grid[game->y][--game->x] = 0;
            }
        }
        else if(input == ':') { //acces meniu de optiuni
            game->menu = PAUSE;
        }
    }
    else {
        if(input == KEY_DOWN && game->pausemenu < QUIT) { //controale meniu de optiuni
            game->pausemenu++;
        }
        else if(input == KEY_UP && game->pausemenu > CONTINUE_GAME) { //continuare joc curent
            game->pausemenu--;
        }
        else if(input == '\n') {
            if(game->pausemenu == CONTINUE_GAME) { //incepere joc nou
                game->menu = PLAY;
            }
            else if(game->pausemenu == NEW_GAME) { //iesire din joc
                memset(game, 0, sizeof(gamestate));
                pick_word(word, wordlist, n); 
            }
            else {
                game->quit = 1;
            }
        }
    }
}

int main(int argc, char **argv)
{ 
    init_window();

    int n = 0;
    char guessword[6], **wordlist = malloc(1 * sizeof(char *));
    generate_word(&wordlist, &n, guessword, argv[1]);
    gamestate game = {}; //structura care retine toate informatiile despre un anumit gamestate

    while(!game.quit) { //structura repetitiva continua atat timp cat utilizatorul nu a apasat quit, in oricare dintre meniuri
        draw_gamestate(game, guessword); //la fiecare nou input, redesenez intreg ecranul, pe baza structurii game
        wait_input(&game, guessword, wordlist, n);
        if(game.ended != NOT_ENDED) { //cand se termina jocul, verific daca jucatorul vrea sa joace din nou sau sa inchida jocul
            draw_gamestate(game, guessword);
            char input = getch();
            while(input != 'q' && input != '\n') {
                input = getch();
            }
            if(input == 'q') {
                game.quit = 1;
            }
            else if(input == '\n') {
                memset(&game, 0, sizeof(gamestate)); // daca vrea sa joace din nou, redeclar toti bitii din game ca 0
                pick_word(guessword, wordlist, n); //si aleg un cuvant nou din lista
            }
        }
    }

    endwin();

    for(int i = 0; i < n; i++) { //eliberarea memoriei
        free(wordlist[i]);
    }
    free(wordlist);
}