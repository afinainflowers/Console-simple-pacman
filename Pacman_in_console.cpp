// Pacman_in_console.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
using namespace std;
const unsigned short FIELD_WIDTH = 40;
const unsigned short FIELD_HEIGHT = 23; // DO NOT MAKE BIGGER! 
const unsigned short INTRO_WIDTH = 40;
const unsigned short INTRO_HEIGHT = 7;
const unsigned short OVER_WIDTH = 40;
const unsigned short OVER_HEIGHT = 13;
const unsigned short ADD_WIDTH = 8; // for future additions of lives and such; also, for score
const unsigned short POINTS_PER_COOKIE = 10;
const char* LEV_PATH = "pac_level.txt";
const char* INTRO_PATH = "pac_intro_screen.txt";
const char* OVER_PATH = "game_over_screen.txt";
const float PAC_MOVE_TIME = 0.1;
const float BOO_MOVE_TIME = 0.1;

SMALL_RECT srctReadRect = {0, 0, FIELD_WIDTH+ADD_WIDTH-1, FIELD_HEIGHT-1}; 
SMALL_RECT srctWriteRect = {0, 0, FIELD_WIDTH+ADD_WIDTH-1, FIELD_HEIGHT-1}; 
CHAR_INFO chiBuffer[(FIELD_WIDTH+ADD_WIDTH)*FIELD_HEIGHT]; // [40][40]; 
COORD coordBufSize = {FIELD_WIDTH+ADD_WIDTH, FIELD_HEIGHT}; 
COORD coordBufCoord = {0, 0}; 
//-FCOORD f_coords[3]; // float coords of pacman and two boos
BOOL fSuccess; 
//I know global variables are bad, but it's just logical here
COORD coords_pac = {0}; // pacman and two boos, x and y
COORD coords_boo1 = {0};
COORD coords_boo2 = {0};
//COORD start_coords[3] = {0}; // pacman and two boos, x and y
dirs pac_dir = RT; // from the enum; pacman direction
dirs last_key = RT;  // from the enum; last arrow key pressed
unsigned int cookies_left = 0;
field_obj level[FIELD_HEIGHT][FIELD_WIDTH] = {PASS};
char intro_matr[INTRO_HEIGHT][INTRO_WIDTH];
char gameover_matr[OVER_HEIGHT][OVER_WIDTH];
//-bool pac_invincible = false; // not used for now
unsigned int points = 0;
unsigned int turns = 0; // for debugging
//-bool should_exit = false;
bool intro_loaded = false;
bool gameover_loaded = false;

HANDLE hStdout, hNewScreenBuffer, hStdin; 

void clear_screen();
void chngConsole1Symbol(char sy, COORD place, WORD attrs){
	CHAR_INFO chi;
	chi.Char.AsciiChar = sy;
	chi.Attributes = attrs;
	COORD cbs; cbs.X = 1; cbs.Y = 1;
	COORD cbc; cbc.X = 0; cbc.Y = 0;
	SMALL_RECT dest; dest.Bottom = place.Y; dest.Top = place.Y; dest.Left = place.X; dest.Right = place.X;
	bool fSuccess = WriteConsoleOutputA( 
        hNewScreenBuffer, // screen buffer to write to 
        &chi,        // buffer to copy from 
        cbs,     // col-row size of chiBuffer 
        cbc,    // top left src cell in chiBuffer 
        &dest);  // dest. screen buffer rectangle 
    if (! fSuccess) 
    {
        printf("WriteConsoleOutput failed - (%d)\n", GetLastError()); 
		string errmsg = "Something went wrong with the console in chngConsole1Symbol(), cannot write ";
		errmsg += "\"";
		errmsg.push_back(sy);
		errmsg += "\"\n";
        throw runtime_error(errmsg); 
    }
}
void game_over_screen(){
	clear_screen();
	int cellnum = 0;
	for (int i = 0; i < OVER_HEIGHT; i++){
		for (int j = 0; j < FIELD_WIDTH + ADD_WIDTH; j++){
			if (j < OVER_WIDTH){
				chiBuffer[cellnum].Char.AsciiChar = gameover_matr[i][j];
			}
			cellnum++;
		}
	}
	fSuccess = WriteConsoleOutput( 
        hNewScreenBuffer, // screen buffer to write to 
        chiBuffer,        // buffer to copy from 
        coordBufSize,     // col-row size of chiBuffer 
        coordBufCoord,    // top left src cell in chiBuffer 
        &srctWriteRect);  // dest. screen buffer rectangle 
    if (! fSuccess) 
    {
        throw runtime_error("Something went wrong with the console");
    }
}
void end_game(bool game_over){
	if (game_over && gameover_loaded){
		game_over_screen();
		Sleep(500);
	}
	exit(EXIT_SUCCESS);
}
void pause(){ //TODO

}
void intro(){ // TO REDO
	clear_screen();
	int cellnum = 0;
	for (int i = 0; i < INTRO_HEIGHT; i++){
		for (int j = 0; j < FIELD_WIDTH + ADD_WIDTH; j++){
			if (j < INTRO_WIDTH){
				chiBuffer[cellnum].Char.AsciiChar = intro_matr[i][j];
			}
			cellnum++;
		}
	}
	fSuccess = WriteConsoleOutput( 
        hNewScreenBuffer, // screen buffer to write to 
        chiBuffer,        // buffer to copy from 
        coordBufSize,     // col-row size of chiBuffer 
        coordBufCoord,    // top left src cell in chiBuffer 
        &srctWriteRect);  // dest. screen buffer rectangle 
    if (! fSuccess) 
    {
        throw runtime_error("Something went wrong with the console");
    }
	Sleep(500);
}
void init_level(){
	int cellnum = 0;
	for (int i = 0; i < FIELD_HEIGHT; i++){
		for (int j = 0; j < FIELD_WIDTH + ADD_WIDTH; j++){
			if (j < FIELD_WIDTH){
				chiBuffer[cellnum].Attributes = BACKGROUND_BLUE;
				chiBuffer[cellnum].Char.AsciiChar = (char)level[i][j];
			}
			cellnum++;
		}
	}
	chiBuffer[coords_pac.X+(coords_pac.Y)*(FIELD_WIDTH+ADD_WIDTH)].Char.AsciiChar = 'C';
	chiBuffer[coords_boo1.X+(coords_boo1.Y)*(FIELD_WIDTH+ADD_WIDTH)].Char.AsciiChar = 'B';
	chiBuffer[coords_boo2.X+(coords_boo2.Y)*(FIELD_WIDTH+ADD_WIDTH)].Char.AsciiChar = 'b';

	fSuccess = WriteConsoleOutput( 
        hNewScreenBuffer, // screen buffer to write to 
        chiBuffer,        // buffer to copy from 
        coordBufSize,     // col-row size of chiBuffer 
        coordBufCoord,    // top left src cell in chiBuffer 
        &srctWriteRect);  // dest. screen buffer rectangle 
    if (! fSuccess) 
    {
        throw runtime_error("Something went wrong with the console");
    }
}
bool check_caught(){
	return (((coords_pac.X == coords_boo1.X)&&(coords_pac.Y == coords_boo1.Y))||((coords_pac.X == coords_boo2.X)&&(coords_pac.Y == coords_boo2.Y)));
}
void event_resp_upd_last_key(){ // call this for every event
	if (GetAsyncKeyState(0x57) & 0x7FFF){ // if W key is pressed
		last_key = UP;
	}
	if (GetAsyncKeyState(0x41) & 0x7FFF){ // if A key is pressed
		last_key = LF;
	}
	if (GetAsyncKeyState(0x53) & 0x7FFF){ // if S key is pressed
		last_key = DN;
	}
	if (GetAsyncKeyState(0x44) & 0x7FFF){ // if D key is pressed
		last_key = RT;
	}
	if (GetAsyncKeyState(VK_UP) & 0x7FFF){ // if up arrow key is pressed
		last_key = UP;
	}
	if (GetAsyncKeyState(VK_LEFT) & 0x7FFF){ // if left arrow key is pressed
		last_key = LF;
	}
	if (GetAsyncKeyState(VK_DOWN) & 0x7FFF){ // if up down key is pressed
		last_key = DN;
	}
	if (GetAsyncKeyState(VK_RIGHT) & 0x7FFF){ // if up right key is pressed
		last_key = RT;
	}
	if (GetAsyncKeyState(0x50) & 0x7FFF){ // if P key is pressed
		pause();
	}
	if (GetAsyncKeyState(0x51) & 0x7FFF){ // if Q key is pressed
		end_game(false);
	}
}
void clear_screen(){
	int cellnum = 0;
	for (int i = 0; i < FIELD_HEIGHT; i++){
		for (int j = 0; j < FIELD_WIDTH + ADD_WIDTH; j++){
			chiBuffer[cellnum].Attributes = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN; 
			chiBuffer[cellnum].Char.AsciiChar = ' ';
			cellnum++;
		}
	}
	fSuccess = WriteConsoleOutput( 
        hNewScreenBuffer, // screen buffer to write to 
        chiBuffer,        // buffer to copy from 
        coordBufSize,     // col-row size of chiBuffer 
        coordBufCoord,    // top left src cell in chiBuffer 
        &srctWriteRect);  // dest. screen buffer rectangle 
    if (! fSuccess) 
    {
        throw runtime_error("Something went wrong with the console");
    }
}
void load_intro(){
	fstream load;
	load.open(INTRO_PATH);
	if (!load.is_open()){
		cerr<<"Cannot open the intro file"<<endl;
		return;
	}
	for (short i = 0; i < INTRO_HEIGHT; i++){
		char line[INTRO_WIDTH+3];
		load.getline(line, INTRO_WIDTH+3, '\n');
		if(load.fail()){
			cerr<<"Problem occured while reading the level file"<<endl;
		}
		for (int j = 0; j < INTRO_WIDTH; j++){
			intro_matr[i][j] = line[j];
		}
	}
	load.close();
	intro_loaded = true;
}
void load_gameover(){
	fstream load;
	load.open(OVER_PATH);
	if (!load.is_open()){
		cerr<<"Cannot open the OVER file"<<endl;
		return;
	}
	for (short i = 0; i < OVER_HEIGHT; i++){
		char line[OVER_WIDTH+3];
		load.getline(line, OVER_WIDTH+3, '\n');
		if(load.fail()){
			cerr<<"Problem occured while reading the level file"<<endl;
		}
		for (int j = 0; j < OVER_WIDTH; j++){
			gameover_matr[i][j] = line[j];
		}
	}
	load.close();
	gameover_loaded = true;
}
void load_level(){
	fstream load;
	load.open(LEV_PATH);
	if (!load.is_open()){
		throw runtime_error("Cannot open the level file");
	}
	for (int i = 0; i < FIELD_HEIGHT; i++){
		char line[FIELD_WIDTH+3];
		load.getline(line, FIELD_WIDTH+3, '\n');
		if(load.fail()){
			throw runtime_error("Problem occured while reading the level file");
		}
		for (int j = 0; j < FIELD_WIDTH; j++){
			switch(line[j]){
				//case ' ': break; // no need, already initiated
				case '#': level[i][j] = WALL; break;
				case '.': level[i][j] = COOKIE; cookies_left++; break;
				case '*': level[i][j] = POWERUP; break;
				case 'B': coords_boo1.X = j; coords_boo1.Y = i; break;
				case 'b': coords_boo2.X = j;coords_boo2.Y = i; break;	
				case 'C': coords_pac.X = j; coords_pac.Y = i; break;
			}
		}
	}
	if(load.fail()){
		throw runtime_error("Problem occured while reading the level file");
	}
	load.close();
	//-cout<<"Pacman: "<<coords[0].X<<", "<<coords[0].Y<<endl;
	//-cout<<"Boo1: "<<coords[1].X<<", "<<coords[1].Y<<endl;
	//-cout<<"Boo2: "<<coords[2].X<<", "<<coords[2].Y<<endl;
}
unsigned short where_can_go(COORD coord){
	unsigned short rez = 0;
	if (0 == coord.Y || level[coord.Y - 1][coord.X] != WALL){ // if we can go up from coord
		rez = rez|UP; // add an UP bit to the returned value
	}
	if (FIELD_WIDTH-1 == coord.X || level[coord.Y][coord.X + 1] != WALL){ // if we can go right from coord
		rez = rez|RT; // add a RT (right) bit to the returned value
	}
	if (FIELD_HEIGHT-1 == coord.Y || level[coord.Y + 1][coord.X] != WALL){ // if we can go down from coord
		rez = rez|DN; // add a DN bit to the returned value
	}
	if (0 == coord.X || level[coord.Y][coord.X - 1] != WALL){ // if we can go left from coord
		rez = rez|LF; // add a LF (left) bit to the returned value
	}
	return rez;
}
void move_pac(){
	unsigned short can_move = where_can_go(coords_pac); // all the directions pacman can take now
	dirs dir_rez = pac_dir;
	if (can_move & last_key){ // if the pacman can go where the last key press points
		dir_rez = last_key;
	}
	if (!(can_move & dir_rez)){ // if the user just sends pacman into the walljn this turn
		return; // just stand still
	}
	pac_dir = dir_rez; 
	chngConsole1Symbol(char(level[coords_pac.Y][coords_pac.X]), coords_pac, BACKGROUND_BLUE);
	switch (pac_dir){
		case UP: coords_pac.Y -= 1; if (coords_pac.Y < 0) {coords_pac.Y = FIELD_HEIGHT-1;}; break; //the field is wrapped around
		case RT: coords_pac.X += 1; if (coords_pac.X > FIELD_WIDTH-1) {coords_pac.X = 0;}; break;
		case DN: coords_pac.Y += 1; if (coords_pac.Y > FIELD_HEIGHT-1) {coords_pac.Y = 0;}; break;
		case LF: coords_pac.X -= 1; if (coords_pac.X < 0) {coords_pac.X = FIELD_WIDTH-1;}; break;
	}
	chngConsole1Symbol('C', coords_pac, BACKGROUND_BLUE|FOREGROUND_RED|FOREGROUND_GREEN);
	if (COOKIE == level[coords_pac.Y][coords_pac.X]){
		level[coords_pac.Y][coords_pac.X] = PASS;
		points += POINTS_PER_COOKIE;
	}
}
void move_ghost1(){
	unsigned short can_move = where_can_go(coords_boo1); // all the directions first boo can take now
	unsigned int dist_to_pac[4] = {10000, 10000, 10000, 10000}; // start with an unrealistically big number
	if (can_move & UP){ // manhattan distance between the next boo step and the pacman
		dist_to_pac[0] = abs((int)((int)coords_pac.X - (int)coords_boo1.X)) + abs((int)((int)coords_pac.Y - ((int)coords_boo1.Y-1))); 
	}
	if (can_move & RT){
		dist_to_pac[1] = abs((int)((int)coords_pac.X - (int)(coords_boo1.X+1))) + abs((int)((int)coords_pac.Y - (int)coords_boo1.Y));
	}
	if (can_move & DN){
		dist_to_pac[2] = abs((int)((int)coords_pac.X - (int)coords_boo1.X)) + abs((int)((int)coords_pac.Y - (int)(coords_boo1.Y+1)));
	}
	if (can_move & LF){
		dist_to_pac[3] = abs((int)((int)coords_pac.X - (int)(coords_boo1.X-1))) + abs((int)((int)coords_pac.Y - (int)coords_boo1.Y));
	}
	dirs rez = UP;
	unsigned int dist_rez = dist_to_pac[0];
	if (dist_rez > dist_to_pac[1]){
		rez = RT;
		dist_rez = dist_to_pac[1];
	}
	if (dist_rez > dist_to_pac[2]){
		rez = DN;
		dist_rez = dist_to_pac[2];
	}
	if (dist_rez > dist_to_pac[3]){
		rez = LF;
		dist_rez = dist_to_pac[3];
	}
	chngConsole1Symbol(char(level[coords_boo1.Y][coords_boo1.X]), coords_boo1, BACKGROUND_BLUE);
	switch (rez){
		case UP: coords_boo1.Y -= 1; break;
		case RT: coords_boo1.X += 1;break;
		case DN: coords_boo1.Y += 1; break;
		case LF: coords_boo1.X -= 1;break;
	}
	chngConsole1Symbol('B', coords_boo1, BACKGROUND_BLUE|FOREGROUND_RED);
	if (WALL == level[coords_boo1.Y][coords_boo1.X]){ // debug block
		string msg =  "Oops, boo 1 (B) is in the wall!";
		OutputDebugStringA(msg.c_str()); 
	}
	return;
}
void move_ghost2(){
	chngConsole1Symbol(char(level[coords_boo2.Y][coords_boo2.X]), coords_boo2, BACKGROUND_BLUE);
	unsigned short can_move = where_can_go(coords_boo2); // all the directions first boo can take now
	unsigned int dist_to_pac[4] = {10000, 10000, 10000, 10000};  // start with an unrealistically big number
	COORD coords_pac_4_steps_straight = coords_pac;
	switch(pac_dir){
		case UP: coords_pac_4_steps_straight.Y -= 4; 
			if (coords_pac_4_steps_straight.Y < 0) coords_pac_4_steps_straight.Y = FIELD_HEIGHT + coords_pac_4_steps_straight.Y; break;
		case RT: coords_pac_4_steps_straight.X += 4;break;
		case DN: coords_pac_4_steps_straight.Y += 4;break;
		case LF: coords_pac_4_steps_straight.X -= 4;
			if (coords_pac_4_steps_straight.X < 0) coords_pac_4_steps_straight.X = FIELD_WIDTH + coords_pac_4_steps_straight.X; break;
	}
	if (can_move & UP){
		dist_to_pac[0] = abs((int)(coords_pac_4_steps_straight.X - coords_boo2.X)) + abs((int)(coords_pac_4_steps_straight.Y - (coords_boo2.Y-1)));
	}
	if (can_move & RT){
		dist_to_pac[1] = abs((int)(coords_pac_4_steps_straight.X - (coords_boo2.X+1))) + abs((int)(coords_pac_4_steps_straight.Y - coords_boo2.Y));
	}
	if (can_move & DN){
		dist_to_pac[2] = abs((int)(coords_pac_4_steps_straight.X - coords_boo2.X)) + abs((int)(coords_pac_4_steps_straight.Y - (coords_boo2.Y+1)));
	}
	if (can_move & LF){
		dist_to_pac[3] = abs((int)(coords_pac_4_steps_straight.X - (coords_boo2.X-1))) + abs((int)(coords_pac_4_steps_straight.Y - coords_boo2.Y));
	}
	dirs rez = UP;
	unsigned int dist_rez = dist_to_pac[0];
	if (dist_rez > dist_to_pac[1]){
		rez = RT;
		dist_rez = dist_to_pac[1];
	}
	if (dist_rez > dist_to_pac[2]){
		rez = DN;
		dist_rez = dist_to_pac[2];
	}
	if (dist_rez > dist_to_pac[3]){
		rez = LF;
		dist_rez = dist_to_pac[3];
	}
	switch (rez){
		case UP: coords_boo2.Y -= 1;break;
		case RT: coords_boo2.X += 1;break;
		case DN: coords_boo2.Y += 1; break;
		case LF: coords_boo2.X -= 1;break;
	}
	chngConsole1Symbol('b', coords_boo2, BACKGROUND_BLUE|FOREGROUND_GREEN);
	if (WALL == level[coords_boo2.Y][coords_boo2.X]){ // debug block
		string msg =  "Oops, boo 2 (b) is in the wall!";
		OutputDebugStringA(msg.c_str()); 
	}
	return;
}
void main_loop(){
	clock_t start_time = clock();
	clock_t now_time;
	clock_t clocks_passed;
	double seconds_passed;
	double last_pac_move_time = 0;
	double last_boos_move_time = 0;
	while (true){
		event_resp_upd_last_key();
		now_time = clock();
		clocks_passed = now_time - start_time;
		seconds_passed = clocks_passed/(double)CLOCKS_PER_SEC;
		if (last_pac_move_time + PAC_MOVE_TIME <= seconds_passed){
			move_pac();
			last_pac_move_time = seconds_passed;
		}
		if (last_boos_move_time + BOO_MOVE_TIME <= seconds_passed){
			move_ghost1();
			move_ghost2();
			last_boos_move_time = seconds_passed;
		}
		if (check_caught()){
			break;
		}
		Sleep(100);
		turns++;
	}
	end_game(true);
}
int main(void) 
{ 
	//-intro();
    // Get a handle to the STDOUT screen buffer to copy from and 
    // create a new screen buffer to copy to. 
	DWORD old_mode;
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 
    hNewScreenBuffer = CreateConsoleScreenBuffer( 
       GENERIC_READ |           // read/write access 
       GENERIC_WRITE, 
       FILE_SHARE_READ | 
       FILE_SHARE_WRITE,        // shared 
       NULL,                    // default security attributes 
       CONSOLE_TEXTMODE_BUFFER, // must be TEXTMODE 
       NULL);                   // reserved; must be NULL 
    if (hStdout == INVALID_HANDLE_VALUE || 
            hNewScreenBuffer == INVALID_HANDLE_VALUE) 
    {
        printf("CreateConsoleScreenBuffer failed - (%d)\n", GetLastError()); 
        return 1;
    }
    // Make the new screen buffer the active screen buffer. 
 
    if (! SetConsoleActiveScreenBuffer(hNewScreenBuffer) ) 
    {
        printf("SetConsoleActiveScreenBuffer failed - (%d)\n", GetLastError()); 
        return 1;
    }
	GetConsoleMode(hStdout, &old_mode);
	SetConsoleMode(hStdout, ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);
    // Set the source rectangle. 
 
    srctReadRect.Top = 0;    // top left: row 0, col 0 
    srctReadRect.Left = 0; 
    srctReadRect.Bottom = FIELD_HEIGHT - 1;
    srctReadRect.Right = FIELD_WIDTH + ADD_WIDTH - 1; 
    coordBufSize.Y = FIELD_HEIGHT; 
    coordBufSize.X = FIELD_WIDTH + ADD_WIDTH; 
    coordBufCoord.X = 0; 
    coordBufCoord.Y = 0; 
    // Copy the block from the screen buffer to the temp. buffer. 
    fSuccess = ReadConsoleOutput( 
       hStdout,        // screen buffer to read from 
       chiBuffer,      // buffer to copy into 
       coordBufSize,   // col-row size of chiBuffer 
       coordBufCoord,  // top left dest. cell in chiBuffer 
       &srctReadRect); // screen buffer source rectangle 
    if (! fSuccess) 
    {
        printf("ReadConsoleOutput failed - (%d)\n", GetLastError()); 
        return 1;
    }
    // Set the destination rectangle. 
    srctWriteRect.Top = 0;    // top lt
    srctWriteRect.Left = 0; 
    srctWriteRect.Bottom = FIELD_HEIGHT; // bot. rt
    srctWriteRect.Right = FIELD_WIDTH + ADD_WIDTH; 
	try{
		load_intro();
		load_gameover();
		intro();
		load_level();
		init_level();
		main_loop();
	}
	catch(runtime_error &e){
		cerr<<e.what()<<endl;
	}
    // Copy from the temporary buffer to the new screen buffer. 
	for (int i = 0; i < (FIELD_WIDTH + ADD_WIDTH)*FIELD_HEIGHT; i++){
		chiBuffer[i].Attributes = FOREGROUND_GREEN|FOREGROUND_RED|BACKGROUND_BLUE; 
		if (i<FIELD_WIDTH)
			chiBuffer[i].Char.AsciiChar = (char)'#';
	}
    fSuccess = WriteConsoleOutput( 
        hNewScreenBuffer, // screen buffer to write to 
        chiBuffer,        // buffer to copy from 
        coordBufSize,     // col-row size of chiBuffer 
        coordBufCoord,    // top left src cell in chiBuffer 
        &srctWriteRect);  // dest. screen buffer rectangle 
    if (! fSuccess) 
    {
        printf("WriteConsoleOutput failed - (%d)\n", GetLastError()); 
        return 1;
    }

    Sleep(500); 
 
    // Restore the original active screen buffer. 
 
    if (! SetConsoleActiveScreenBuffer(hStdout)) 
    {
        printf("SetConsoleActiveScreenBuffer failed - (%d)\n", GetLastError()); 
        return 1;
    }

    return 0;
}