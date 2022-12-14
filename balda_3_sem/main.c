
#define _CRT_SECURE_NO_WARNINGS
#include "wincon.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>

#include "sqlite3.h"
#include <time.h>


#define	 LETTER_a_RUS			224
#define  SAVE_FILE				"save.txt"
#define	 ALPHABET_POW			32
#define  MAX_WORD_LEN			30
#define  MAX_WORDS_COUNT		21
#define	 clr_bg					CON_CLR_BLACK
#define  clr_bg_active			CON_CLR_RED
#define  clr_font				CON_CLR_WHITE_LIGHT
#define  clr_bg_chosen			CON_CLR_GREEN
#define  clr_bg_warning         CON_CLR_YELLOW
#define  START_WORDS_COUNT             2236
#define  x_coord_field          50
#define  y_coord_field          5
#define  x_coord_menu           x_coord_field + 14
#define  debug_buff				21
#define	 debug_file				"course_work.txt"

/*????????? ???? ?????????? ? ???????????????? ?????????? ????????*/
typedef struct node {
	unsigned char letters[ALPHABET_POW];
	struct node* next[ALPHABET_POW];
	char* word;
}NODE;



/*???????*/
void main_menu();
void about();

void settings_menu();
void mode_selection();
void difficulty_selection();
void first_turn_selection();

void show_score();
void show_end_game(int);
void show_words_bank();

int pass_turn_window();
int surrender_window();

/*?????????? ??????*/
void open_db();
void read_dict_to_tree(sqlite3*);
void read_inv_to_tree(sqlite3*);
void read_start_word(sqlite3*);
/*???????? ??????*/
int bot_move();
void check_all_letters(int, int);

/*?????????*/
void set_letter();
void insert_word_tree(unsigned char*, NODE*);
int find_node(unsigned char, NODE*);
int find_word_tree(unsigned char*, NODE*);
void reverse_word(unsigned char*);
NODE* inv_to_dict_node(unsigned char*);
void search_inv_tree(int, int, unsigned char*, int);
int check_end_game();
void end_game();
int set_word(int, int);
void init_inv_tree();
void init_dict_tree();
void search_dict_tree(int, int, NODE*, unsigned char*);
int set_word(int, int);
int get_letter_index(unsigned char, NODE*);
void save_progress();
void load_progress();
int ask_for_save();
int ask_for_load();


enum lvl {
	EASY = 1,
	MEDIUM = 2,
	HARD = 3
} difficulties;

enum modes {
	VS_PLAYER = 1,
	VS_ROBOT = 2,
	DZEN = 3
} game_modes;

struct roots {
	NODE* dict;
	NODE* inv;
} roots;

struct scores {
	int first_player;
	int second_player;

} score;

struct start_letter_cords {
	int x;
	int y;
} start_cords;

struct chosen_letter_cords {
	int x;
	int y;
} chosen_cords;

struct grahphics_corners {
	int left;
	int top;
} corners;

char game_mode = VS_ROBOT;
char found = 0;
char field_for_search[5][5] = { {1} }; // ????? ??? ??????? ??? ???????????? ?????? ??? ???? ??
unsigned char* longest_word[MAX_WORD_LEN] = { '\0' };
unsigned char words_bank[MAX_WORDS_COUNT][MAX_WORD_LEN];
unsigned char letter_chosen = 0;
unsigned char field_letters[5][5] = { {'\0'} };
char start_word[7] = {0};
int btn_bg;
int difficult = EASY; //????????? ?????????? "??????"
int max_len = 0;
int words_bank_len = 0;
int start_turn = 1, turn = 0, turn_test = 0;

/* DEBUG PURPOSE */

struct debug_info {
	int debug_on;
	int cells_left[debug_buff];
	double time_pass[debug_buff];
} debug_info;


void debug() {
	//clrscr();
	int i;
	int x = corners.left, y = corners.top;
	for (i = 0; debug_info.time_pass[i] != 0.0; i++) {
		gotoxy(x, y);
		//printf("Cells left: %d | time passed: %lf\n", debug_info.cells_left[i], debug_info.time_pass[i]);
		printf("%d %lf", debug_info.cells_left[i], debug_info.time_pass[i]);
		y++;
	}
	if (turn_test == 20) {
		FILE* file = fopen(debug_file, "w+");
		for (i = 0; i < 20; i++) {
			//fwrite(&debug_info.cells_left[i], sizeof(int), 1, file);
			//fwrite(",", sizeof(char), 1, file);
			//fwrite(&debug_info.time_pass[i], sizeof(double), 1, file);
			fprintf(file, "%d, ", debug_info.cells_left[i]);
		}
		fputc('\n', file);
		for (i = 0; i < 20; i++) {
			//fwrite(&debug_info.cells_left[i], sizeof(int), 1, file);
			//fwrite(",", sizeof(char), 1, file);
			//fwrite(&debug_info.time_pass[i], sizeof(double), 1, file);
			fprintf(file, "%lf, ", debug_info.time_pass[i]);
		}
		fclose(file);
		exit(EXIT_SUCCESS); // 0.001 0.65, 
	}
}

/*??????? ????*/
/*????? ??????? ????? ?????? ????*/
int find_node(unsigned char letter, NODE* node) {
	for (int i = 0; i < ALPHABET_POW; i++) {
		if (node->letters[i] == NULL) {
			return i;
		}
		if (node->letters[i] == letter) {
			return i;
		}
	}
}

/*????? ??????? ????? ?????? ????*/
int get_letter_index(unsigned char letter, NODE* node) {
	for (int i = 0; i < ALPHABET_POW; i++) {
		if (node->letters[i] == NULL) {
			return -1;
		}
		if (node->letters[i] == letter) {
			return i;
		}
	}
	return -1;
}

/*??????? ????? ? ??????*/
void insert_word_tree(unsigned char* word, NODE* root) {
	NODE* node = root;
	for (int i = 0; i < strlen(word); i++) {
		int result = find_node(word[i], node);
		if (result != -1 && node->letters[result] == NULL) {
			node->letters[result] = word[i];
		}
		if (result != -1 && node->next[result] == NULL) {
			NODE* new_node = (NODE*)malloc(sizeof(NODE));
			if (new_node == NULL) {
				/*??? ????????? ??????*/
				return -1;
			}
			memset(new_node, 0, sizeof(NODE));
			new_node->word = NULL;
			node->next[result] = new_node;
		}
		node = node->next[result];
	}
	/*??????? ????? ????? ? ???? word?*/
	//if (root != roots.inv) {
	node->word = (char*)malloc(strlen(word));
	strcpy(node->word, word);
	//}
}

void open_db() {
	sqlite3* db;

	if (sqlite3_open("data_ansi.db", &db))
	{
		sqlite3_close(db);
		printf("Can't open database: %s\n", sqlite3_errmsg(db));
		exit(EXIT_FAILURE);
	}

	read_dict_to_tree(db);

	read_inv_to_tree(db);

	read_start_word(db);

	sqlite3_close(db);

}

/*?????????? ??????? ? ????????? ??????*/
void read_dict_to_tree(sqlite3* db) {
	sqlite3_stmt* res;
	const char* tail;
	char* line[MAX_WORD_LEN] = { '\0' };
	int len = 0;
	if (sqlite3_prepare_v2(db, "select * from dict;", 128, &res, &tail) != SQLITE_OK)
	{
		sqlite3_close(db);
		printf("Can't retrieve data: %s\n", sqlite3_errmsg(db));
		exit(EXIT_FAILURE);
	}

	while (sqlite3_step(res) == SQLITE_ROW)
	{
		strcpy(line, sqlite3_column_text(res, 0));
		len = strlen(line) - 1;
		(line[len] == '\n') ? line[len] = '\0' : line[len];
		insert_word_tree(line, roots.dict);
	}
	sqlite3_finalize(res);

}

void read_inv_to_tree(sqlite3* db) {

	sqlite3_stmt* res;
	const char* tail;
	char* line[MAX_WORD_LEN] = { '\0' };
	int len = 0;
	if (sqlite3_prepare_v2(db, "select * from inv;", 128, &res, &tail) != SQLITE_OK)
	{
		sqlite3_close(db);
		printf("Can't retrieve data: %s\n", sqlite3_errmsg(db));
		exit(EXIT_FAILURE);
	}

	while (sqlite3_step(res) == SQLITE_ROW)
	{
		strcpy(line, sqlite3_column_text(res, 0));
		len = strlen(line) - 1;
		(line[len] == '\n') ? line[len] = '\0' : line[len];
		insert_word_tree(line, roots.inv);
	}
	sqlite3_finalize(res);

}

void read_start_word(sqlite3* db) {
	sqlite3_stmt* res;
	const char* tail;
	char* line[MAX_WORD_LEN] = { '\0' };
	int i = 0;
	int len = 0;

	if (sqlite3_prepare_v2(db, "select * from start_words;", 128, &res, &tail) != SQLITE_OK)
	{
		sqlite3_close(db);
		printf("Can't retrieve data: %s\n", sqlite3_errmsg(db));
		exit(EXIT_FAILURE);
	}

	srand(time(NULL));
	int r = rand() % START_WORDS_COUNT;
	

	while (sqlite3_step(res) == SQLITE_ROW)
	{
		if (i == r) {
			strcpy(start_word, sqlite3_column_text(res, 0));
			break;
		}
			i++;
	}

	len = strlen(start_word) - 1;
	(start_word[len] == '\n') ? start_word[len] = '\0' : start_word[len];

	sqlite3_finalize(res);
}

int find_word_tree(unsigned char* word, NODE* root) {
	NODE* node = root;
	for (int i = 0; i < strlen(word); i++) {
		for (int j = 0; j < ALPHABET_POW; j++) {
			if (node->letters[j] == NULL) {
				return 0;
			}
			if (node->letters[j] == word[i]) {
				node = node->next[j];
				break;
			}
		}
	}
	if (node->word != NULL) {
		return 1;
	}
	return 0;
}

/*???????? ??????? ?????? ? ????????? ??????*/
void search_dict_tree(int x, int y, NODE* node, unsigned char* curr_word) {
	if (difficult == MEDIUM && strlen(curr_word) > 6) {
		return;
	}
	int res = 0, checked = 0;
	//if (difficu/*lt == 2 && max_len >= 4) {
		//found = 1;
		//return;
	//}*/
	if (node->word != NULL && strlen(curr_word) > max_len) {
		if (max_len >= 3 && difficult == EASY) return;
		int flag = 0;
		for (int i = 0; i < words_bank_len; i++) {
			if (!strncmp(words_bank[i], curr_word, MAX_WORD_LEN)) {
				flag = 1;
				break;
			}

		}
		if (!flag) {
			max_len = strlen(curr_word);
			for (int k = 0; curr_word[k] != '\0'; k++) {
				longest_word[k] = curr_word[k];
			}
		}
	}
	if (difficult == EASY && strlen(curr_word) >= 4) {
		return;
	}
	if (field_for_search[x][y] == 0) {
		field_for_search[x][y] = 1;
		checked++;
	}
	// ????????? ??? ??????? ??????
	//??????? ??????
	res = get_letter_index(field_letters[x - 1][y], node);
	if (field_letters[x - 1][y] != '\0' && field_for_search[x - 1][y] != 1 && res != -1 && x > 0) {
		curr_word[strlen(curr_word)] = field_letters[x - 1][y];
		curr_word[strlen(curr_word)] = '\0';
		search_dict_tree(x - 1, y, node->next[res], curr_word);
		//if (found) return;
		curr_word[strlen(curr_word) - 1] = '\0';// ???????? ??? ??????????????
	}
	// ?????? ??????
	res = get_letter_index(field_letters[x][y + 1], node);
	if (field_letters[x][y + 1] != '\0' && field_for_search[x][y + 1] != 1 && res != -1 && y < 4) {
		curr_word[strlen(curr_word)] = field_letters[x][y + 1];
		curr_word[strlen(curr_word)] = '\0';
		search_dict_tree(x, y + 1, node->next[res], curr_word);
		//if (found) return;
		curr_word[strlen(curr_word) - 1] = '\0';// ???????? ??? ??????????????
	}
	// ?????? ??????
	res = get_letter_index(field_letters[x + 1][y], node);
	if (field_letters[x + 1][y] != '\0' && field_for_search[x + 1][y] != 1 && res != -1 && x < 4) {
		curr_word[strlen(curr_word)] = field_letters[x + 1][y];
		curr_word[strlen(curr_word)] = '\0';
		search_dict_tree(x + 1, y, node->next[res], curr_word);
		//if (found) return;
		curr_word[strlen(curr_word) - 1] = '\0';// ???????? ??? ??????????????
	}


	// ????? ??????
	res = get_letter_index(field_letters[x][y - 1], node);
	if (field_letters[x][y - 1] != '\0' && field_for_search[x][y - 1] != 1 && res != -1 && y > 0) {
		curr_word[strlen(curr_word)] = field_letters[x][y - 1];
		curr_word[strlen(curr_word)] = '\0';
		search_dict_tree(x, y - 1, node->next[res], curr_word);
		//if (found) return;
		curr_word[strlen(curr_word) - 1] = '\0';// ???????? ??? ??????????????
	}

	(checked == 1) ? field_for_search[x][y] = 0 : field_for_search[x][y];
	return;
}


/*??????? ????????? ?????(???????? ?????.)*/
void reverse_word(unsigned char* word) {
	for (int i = 0, j = strlen(word) - 1; i < strlen(word) / 2; i++, j--) {
		unsigned char temp = word[i];
		word[i] = word[j];
		word[j] = temp;
	}
}


// ??????? ????? ??? ?????????? ???????????? ???? ?? ???????????????? ?????? ? ????????? ?????? 
// ??????? ? ?????????? ???????????? ? ?????? ?? ?????????? ??????
NODE* inv_to_dict_node(unsigned char* word) {
	reverse_word(word);
	NODE* node = roots.dict;
	for (int i = 0; i < strlen(word); i++) {
		for (int j = 0; j < ALPHABET_POW; j++) {
			if (node->letters[j] == word[i]) {
				node = node->next[j];
				break;
			}
		}
	}
	return node;
}

/*???????? ??????? ?????? ? ??? ??????*/
void search_inv_tree(int x, int y, unsigned char* curr_word, int end_ind) {
	if (field_letters[x][y] == '\0' || x < 0 || y < 0 || x >= 5 || y >= 5) {
		return;
	}
	curr_word[end_ind] = field_letters[x][y];
	field_for_search[x][y] = 1;
	curr_word[end_ind + 1] = '\0';
	// ??????? ????????? ?????
	unsigned char new_word[MAX_WORD_LEN] = { '\0' };
	strcpy(new_word, curr_word);
	if (find_word_tree(curr_word, roots.inv)) {
		NODE* node = inv_to_dict_node(new_word);
		search_dict_tree(start_cords.x, start_cords.y, node, new_word);
		//if (found) return;
	}
	//????? ???????? ???????????
	else {
		
	}
	// ????????? ??? ??????? ??????
	//??????? ??????
	if (field_letters[x - 1][y] != '\0' && field_for_search[x - 1][y] != 1) {
		search_inv_tree(x - 1, y, curr_word, end_ind + 1);
	}
	curr_word[end_ind + 1] = '\0';

	// ????? ??????
	if (field_letters[x][y + 1] != '\0' && field_for_search[x][y + 1] != 1) {
		search_inv_tree(x, y + 1, curr_word, end_ind + 1);
	}
	curr_word[end_ind + 1] = '\0';

	// ?????? ??????
	if (field_letters[x + 1][y] != '\0' && field_for_search[x + 1][y] != 1) {
		search_inv_tree(x + 1, y, curr_word, end_ind + 1);
	}
	curr_word[end_ind + 1] = '\0';

	// ?????? ??????
	if (field_letters[x][y - 1] != '\0' && field_for_search[x][y - 1] != 1) {
		search_inv_tree(x, y - 1, curr_word, end_ind + 1);
	}
	curr_word[end_ind + 1] = '\0';

	field_for_search[x][y] = 0;
}

void check_all_letters(int x, int y) {
	int curr_letter = LETTER_a_RUS;
	unsigned char curr_word[MAX_WORD_LEN] = { 0 };
	int end_ind = 0;
	for (int i = 0; i < ALPHABET_POW; i++) {
		// COURSEWORK
		// var
		//if (difficult == EASY && max_len >= 3) {
			//break;
		//}
		//if (difficult == MEDIUM && max_len >= 4) {
			//break;
		//}
		// COURSEWORK
		int len = max_len;
		field_letters[x][y] = curr_letter;
		search_inv_tree(x, y, curr_word, end_ind);
		if (len < max_len) {
			chosen_cords.x = x;
			chosen_cords.y = y;
			letter_chosen = curr_letter;
		}

		field_letters[x][y] = '\0';
		memset(curr_word, 0, MAX_WORD_LEN);
		curr_letter++;
		//if (found) return;

	}
	return;
}

int bot_move() {
	// ????????? ?????? 1 ??? 0?
	int cell_count = 0;
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 5; j++) {
			if (field_letters[i][j] == '\0' && ((i + 1 < 5 && field_letters[i + 1][j] != '\0') ||
				(i - 1 >= 0 && field_letters[i - 1][j] != '\0') || (j + 1 < 5 && field_letters[i][j + 1] != '\0') || (j - 1 >= 0 && field_letters[i][j - 1] != '\0'))) {
				cell_count++;
				start_cords.x = i;
				start_cords.y = j;
				check_all_letters(i, j);
				//if (found) break;

			}
		}
		//if (found) break;
		//found = 0;
	}

	if (max_len == 0 && cell_count != 0) {
		//printf("???? ??????");
		if (debug_info.debug_on != 1) {
			show_end_game(0);
			end_game();
		}
		return 1;
	}
	else {
		field_letters[chosen_cords.x][chosen_cords.y] = letter_chosen;
		for (int i = 0; i < max_len; i++) {
			words_bank[words_bank_len][i] = longest_word[i];
		}
		words_bank_len += 1;
		score.second_player += max_len;

		max_len = 0;
	}
	return 0;
}

void init_dict_tree() {
	roots.dict = (NODE*)malloc(sizeof(NODE));
	if (roots.dict == NULL) {
		exit(EXIT_FAILURE);
	}
	memset(roots.dict, 0, sizeof(NODE));
}

void init_inv_tree() {
	roots.inv = (NODE*)malloc(sizeof(NODE));
	if (roots.inv == NULL) {
		exit(EXIT_FAILURE);
	}
	memset(roots.inv, 0, sizeof(NODE));
}

int main()
{
	system("chcp 1251");
	HWND hWnd = GetForegroundWindow();
	ShowWindow(hWnd, SW_MAXIMIZE);
	system("cls");
	FILE* file;
	init_dict_tree();
	init_inv_tree();
	
	open_db();

	//file = fopen(START_WORDS, "r");
	//if (file == NULL) {
	//	printf("I just don't have the words with five letters to describe the pain i feel!");
	//	return 1;
	//}
	//fclose(file);
	// ???????????????? ???????, ?????????? ??????, ??????????? ??????? ???? ? ???????
	con_init(100, 50);
	// system("mode con cols=100 lines=25");
	show_cursor(0);
	score.first_player = 0, score.second_player = 0;
	debug_info.debug_on = 0;
	// ?????? ???????? ????
	main_menu();

	return 0;
}


// ????????? ???????? ????
void main_menu()
{
	const char* menu_items[] = { "?????" ,"????", "?????????", "? ?????????", "?????" };
	int menu_active_idx = 1;
	int menu_items_count = sizeof(menu_items) / sizeof(menu_items[0]);
	/*short clr_bg = CON_CLR_BLACK;
	short clr_bg_active = CON_CLR_RED;
	short clr_font = CON_CLR_WHITE_LIGHT;*/
	corners.left = x_coord_menu;
	corners.top = y_coord_field;
	memset(debug_info.time_pass, 0, debug_buff * sizeof(double));
	int b;
	while (1)
	{
		corners.left = x_coord_menu;
		corners.top = y_coord_field;
		// ????????????? ?????????
		con_draw_lock();

		// ??????? ??????
		con_set_color(clr_font, clr_bg);
		clrscr();
		// ???? ???????????? ??????
		for (b = 0; b < menu_items_count; b++)
		{
			short btn_bg = clr_bg; // ?? ????????? ??? ?????? - ??? ??? ??????
			if (b == menu_active_idx)
				btn_bg = clr_bg_active; // ???? ?????? ??????? - ?? ???????? ?????? ??????
			gotoxy(corners.left, corners.top);
			con_set_color(clr_font, btn_bg);
			if (b == 0)
				printf("~~~~~~~~~~~~~~~~~~~~");
			else
				printf("====================");
			corners.top++;
			gotoxy(corners.left, corners.top);
			printf("|                   ");

			gotoxy(corners.left + 10 - strlen(menu_items[b]) / 2, corners.top);
			printf("%s", menu_items[b]);

			con_set_color(clr_font, btn_bg);
			gotoxy(corners.left + 19, corners.top);
			printf("|");
			corners.top++;
			gotoxy(corners.left, corners.top);
			if (b == 0)
				printf("~~~~~~~~~~~~~~~~~~~~");
			else
				printf("====================");
			corners.top += 2;
		}

		// ?????? ????????????, ??????? ?? ?????
		con_draw_release();


		while (!key_is_pressed()) // ???? ???????????? ???????? ??????
		{
			int code = key_pressed_code();
			if (code == 'w' || code == 'W' || code == (unsigned char)'?' || code == (unsigned char)'?') // ???? ??? ??????? ?????
			{
				// ?? ??????? ? ???????? ?????? (???? ??? ????????)
				if (menu_active_idx > 1)
				{
					menu_active_idx--;
					break;
				}
				else
				{
					menu_active_idx = menu_items_count - 1;
					break;
				}
			}
			else if (code == 's' || code == 'S' || code == (unsigned char)'?' || code == (unsigned char)'?') // ???? ??????? ????
			{
				// ?? ??????? ? ??????? ?????? (???? ??? ????????)
				if (menu_active_idx + 1 < menu_items_count)
				{
					menu_active_idx++;
					break;
				}
				else
				{
					menu_active_idx = 1;
					break;
				}
			}
			else if (code == KEY_ESC || code == 'q' || code == 'Q' ||
				code == (unsigned char)'?' || code == (unsigned char)'?') // ESC ??? 'q' - ?????
			{
				if (ask_for_save()) {
					save_progress();
				}
				return;
			}
			else if (code == KEY_ENTER) // ?????? ?????? Enter
			{
				if (menu_active_idx == 4) { // ?????? ????????? ????? - ??? ?????
					if (ask_for_save()) {
						save_progress();
						return;
					}
				}
				//if (menu_active_idx == 0)

				if (menu_active_idx == 2) // ?????? ????? "?????????"
					settings_menu();
				if (menu_active_idx == 1) {
					set_letter();
				}
				//if (menu_active_idx == 3)
				if (menu_active_idx == 3)
					about();

				break;
			}


			pause(40); // ????????? ????? (????? ?? ????????? ?????????)
		} // while (!key_is_pressed())


		// "?????????" ?????????? ????
		while (key_is_pressed())
			key_pressed_code();

	} // while(1)
}

void mode_selection() {
	char* menu_items[] = { "?????" ,"?????? ??????", "?????? ??????????", "????" ,"????? ??? ESC" };
	int menu_active_idx = 1;
	int menu_items_count = sizeof(menu_items) / sizeof(menu_items[0]);
	//short clr_bg = CON_CLR_BLACK;
	//short clr_bg_active = CON_CLR_RED;
	//short clr_font = CON_CLR_WHITE_LIGHT;
	//short clr_bg_chosen = CON_CLR_RED_LIGHT;
	while (1)
	{
		corners.left = x_coord_menu;
		corners.top = y_coord_field;
		int b;

		// ????????????? ?????????
		con_draw_lock();

		// ??????? ??????
		con_set_color(clr_font, clr_bg);
		clrscr();
		// ???? ???????????? ??????
		for (b = 0; b < menu_items_count; b++)
		{
			short btn_bg = clr_bg; // ?? ????????? ??? ?????? - ??? ??? ??????

			//if (b == difficult)
			//	btn_bg = clr_bg_chosen;//?????? ?? ??????? ? ??? ??????? ?????????
			if (b == menu_active_idx)
				btn_bg = clr_bg_active; // ???? ?????? ??????? - ?? ???????? ?????? ??????
			gotoxy(corners.left, corners.top);
			con_set_color(clr_font, btn_bg);

			if (b == 0)
				printf("~~~~~~~~~~~~~~~~~~~~");
			else
				printf("====================");
			corners.top++;
			if (b == game_mode) {
				gotoxy(corners.left - 4, corners.top);
				printf("--->|                   ");
			}
			else {
				gotoxy(corners.left, corners.top);
				printf("|                   ");
			}

			gotoxy(corners.left + 10 - strlen(menu_items[b]) / 2, corners.top);
			printf("%s", menu_items[b]);
			con_set_color(clr_font, btn_bg);
			if (b == game_mode) {
				gotoxy(corners.left + 19, corners.top);
				printf("|<---");
			}
			else {
				gotoxy(corners.left + 19, corners.top);
				printf("|");
			}
			corners.top++;
			gotoxy(corners.left, corners.top);
			if (b == 0)
				printf("~~~~~~~~~~~~~~~~~~~~");
			else
				printf("====================");
			corners.top += 2;
		}

		// ?????? ????????????, ??????? ?? ?????
		con_draw_release();


		while (!key_is_pressed()) // ???? ???????????? ???????? ??????
		{
			int code = key_pressed_code();
			if (code == 'w' || code == 'W' || code == (unsigned char)'?' || code == (unsigned char)'?') // ???? ??? ??????? ?????
			{
				// ?? ??????? ? ???????? ?????? (???? ??? ????????)
				if (menu_active_idx > 1)
				{
					menu_active_idx--;
					break;
				}
				else
				{
					menu_active_idx = menu_items_count - 1;
					break;
				}
			}
			else if (code == 's' || code == 'S' || code == (unsigned char)'?' || code == (unsigned char)'?') // ???? ??????? ????
			{
				// ?? ??????? ? ??????? ?????? (???? ??? ????????)
				if (menu_active_idx + 1 < menu_items_count)
				{
					menu_active_idx++;
					break;
				}
				else
				{
					menu_active_idx = 1;
					break;
				}
			}
			else if (code == KEY_ESC || code == 'q' || code == 'Q' ||
				code == (unsigned char)'?' || code == (unsigned char)'?') // ESC ??? 'q' - ?????
			{
				return;
			}
			else if (code == KEY_ENTER) // ?????? ?????? Enter
			{
				if (menu_active_idx == 4) // ?????? ????????? ????? - ??? ?????
					return;

				if (menu_active_idx == 1)//˸???? ?????????
					game_mode = VS_PLAYER;


				if (menu_active_idx == 2)//??????? ?????????
					game_mode = VS_ROBOT;

				if (menu_active_idx == 3)//???????
					game_mode = DZEN;
				break;
			}


			pause(40); // ????????? ????? (????? ?? ????????? ?????????)
		} // while (!key_is_pressed())


		// "?????????" ?????????? ????
		while (key_is_pressed())
			key_pressed_code();

	} // while(1)
}

void set_letter() {
	int i, j;
	int is_word;
	if (ask_for_load()) {
		load_progress();
	}

	if (field_letters[2][0] == '\0') {
		for (i = 0; i < 5; i++) {
			for (j = 0; j < 5; j++) {
				field_letters[i][j] = '\0';
			}
		}
		if (start_turn == 1)turn = 1;
		else turn = 0;
		//????????e ????? ? ??????
		// ADD DB
		//strcpy()
		//system("cls");
		field_letters[2][0] = start_word[0];
		field_letters[2][1] = start_word[1];
		field_letters[2][2] = start_word[2];
		field_letters[2][3] = start_word[3];
		field_letters[2][4] = start_word[4];
		words_bank[0][0] = start_word[0];
		words_bank[0][1] = start_word[1];
		words_bank[0][2] = start_word[2];
		words_bank[0][3] = start_word[3];
		words_bank[0][4] = start_word[4];
		words_bank[0][5] = '\0';
		words_bank_len = 1;
		/*field_letters[2][0] = '?';
		field_letters[2][1] = '?';
		field_letters[2][2] = '?';
		field_letters[2][3] = '?';
		field_letters[2][4] = '?';
		words_bank[0][0] = '?';
		words_bank[0][1] = '?';
		words_bank[0][2] = '?';
		words_bank[0][3] = '?';
		words_bank[0][4] = '?';
		words_bank[0][5] = '\0';
		words_bank_len = 1;*/
	}
	int column_active_idx = 0;
	int line_active_idx = 0;
	int field_letters_column_count = 5;
	int field_letters_line_count = 5;
	while (1)
	{
		if (check_end_game() == 1) return;
		if ((start_turn == 1 && turn >= 4 || start_turn == 2 && turn >= 3) && words_bank[turn][0] == '\0' && words_bank[turn - 1][0] == '\0' && words_bank[turn - 2][0] == '\0') {
			show_end_game(0);
			end_game();
			return;
		}
		if (turn % 2 == 0 && game_mode == VS_ROBOT) {
			clock_t start_time = clock();
			
			bot_move();
			if (debug_info.debug_on == 1) {
				debug_info.time_pass[turn_test] = (double)(clock() - start_time) / CLOCKS_PER_SEC;
				debug_info.cells_left[turn_test] = 0;
				for (i = 0; i < 5; i++) {
					for (int j = 0; j < 5; j++) {
						if (field_letters[i][j] == '\0') debug_info.cells_left[turn_test]++;
					}
				}
			}
			memset(longest_word, 0, sizeof(longest_word));
			max_len = 0;
			chosen_cords.x = 0; chosen_cords.y = 0; start_cords.x = 0; start_cords.y = 0;
			found = 0;
			turn++;
			turn_test++;
		}
		corners.left = x_coord_field;
		corners.top = y_coord_field;
		int i, j;
		short btn_bg;
		// ????????????? ?????????
		con_draw_lock();

		// ??????? ??????
		con_set_color(clr_font, clr_bg);
		clrscr();
		// ???? ???????????? ??????

		for (i = 0; i < field_letters_column_count; i++)
		{
			for (j = 0; j < field_letters_line_count; j++) {
				corners.left = x_coord_field + j * 9;
				corners.top = y_coord_field + i * 5;
				btn_bg = clr_bg; // ?? ????????? ??? ?????? - ??? ??? ??????
				if (i == column_active_idx && j == line_active_idx)
					btn_bg = clr_bg_active; // ???? ?????? ??????? - ?? ???????? ?????? ??????

				gotoxy(corners.left, corners.top);
				con_set_color(clr_font, btn_bg);

				printf("---------");
				corners.top++;
				gotoxy(corners.left, corners.top);
				printf("|       |");
				corners.top++;
				gotoxy(corners.left, corners.top);
				printf("|       ");

				gotoxy(corners.left + 4, corners.top);
				field_letters[i][j] != '\0' ? printf("%c", field_letters[i][j] - ALPHABET_POW) : printf("%c", field_letters[i][j]);
				//printf("?", field_letters[i][j]);

				gotoxy(corners.left + 8, corners.top);
				printf("|");
				corners.top++;

				gotoxy(corners.left, corners.top);
				printf("|       |");
				corners.top++;
				gotoxy(corners.left, corners.top);
				printf("---------");
			}
		}

		show_words_bank();

		show_score();

		// ?????? ????????????, ??????? ?? ?????
		con_draw_release();
		//debug();
		while (!key_is_pressed()) // ???? ???????????? ???????? ??????
		{
			if (turn % 2 == 0 && game_mode == VS_ROBOT) {
				bot_move();
			}
			int code;
			if (debug_info.debug_on == 1) {
				code = KEY_BACK; 
			}
			else {
				code = key_pressed_code();
			}
			if (code == 'w' || code == 'W' || code == (unsigned char)'?' || code == (unsigned char)'?') // ???? ??? ??????? ?????
			{
				// ?? ??????? ? ???????? ?????? (???? ??? ????????)
				if (column_active_idx > 0)
				{
					column_active_idx--;
					break;
				}
				else
				{
					column_active_idx = 4;
					break;
				}
			}
			else if (code == 's' || code == 'S' || code == (unsigned char)'?' || code == (unsigned char)'?') // ???? ??????? ????
			{
				// ?? ??????? ? ??????? ?????? (???? ??? ????????)
				if (column_active_idx < 4)
				{
					column_active_idx++;
					break;
				}
				else
				{
					column_active_idx = 0;
					break;
				}
			}
			else if (code == 'd' || code == 'D' || code == (unsigned char)'?' || code == (unsigned char)'?') // ???? ??????? ??????
			{
				// ?? ??????? ? ??????? ?????? (???? ??? ????????)
				if (line_active_idx < 4)
				{
					line_active_idx++;
					break;
				}
				else
				{
					line_active_idx = 0;
					break;
				}
			}
			else if (code == 'a' || code == 'A' || code == (unsigned char)'?' || code == (unsigned char)'?') // ???? ??? ??????? ?????
			{
				// ?? ??????? ? ?????? ?????? (???? ??? ????????)
				if (line_active_idx > 0)
				{
					line_active_idx--;
					break;
				}
				else
				{
					line_active_idx = 4;
					break;
				}
			}
			else if (code == KEY_ENTER) {
				if (field_letters[column_active_idx][line_active_idx] != '\0') break;
				if ((column_active_idx == 4 && field_letters[0][line_active_idx] == '\0'
					|| column_active_idx != 4 && field_letters[column_active_idx + 1][line_active_idx] == '\0')
					&& (column_active_idx == 0 && field_letters[4][line_active_idx] == '\0'
						|| column_active_idx != 0 && field_letters[column_active_idx - 1][line_active_idx] == '\0')
					&& (line_active_idx == 4 && field_letters[column_active_idx][0] == '\0'
						|| line_active_idx != 4 && field_letters[column_active_idx][line_active_idx + 1] == '\0')
					&& (line_active_idx == 0 && field_letters[column_active_idx][4] == '\0'
						|| line_active_idx != 0 && field_letters[column_active_idx][line_active_idx - 1] == '\0')) break;
				corners.left = x_coord_field + line_active_idx * 9;
				corners.top = y_coord_field + column_active_idx * 5;
				btn_bg = clr_bg_chosen;

				gotoxy(corners.left, corners.top);
				con_set_color(clr_font, btn_bg);

				printf("---------");
				corners.top++;
				gotoxy(corners.left, corners.top);
				printf("|       |");
				corners.top++;
				gotoxy(corners.left, corners.top);
				printf("|       ");

				gotoxy(corners.left + 4, corners.top);
				field_letters[column_active_idx][line_active_idx] != '\0' ? printf("%c", field_letters[column_active_idx][line_active_idx] - ALPHABET_POW) : printf("%c", field_letters[column_active_idx][line_active_idx]);
				//printf("?", field_letters[i][j]);

				gotoxy(corners.left + 8, corners.top);
				printf("|");
				corners.top++;

				gotoxy(corners.left, corners.top);
				printf("|       |");
				corners.top++;
				gotoxy(corners.left, corners.top);
				printf("---------");

				while (key_is_pressed())
					key_pressed_code();
				while (!key_is_pressed()) {
					code = key_pressed_code();
					if (code >= (unsigned char)'?' && code <= (unsigned char)'?') {
						field_letters[column_active_idx][line_active_idx] = code;
					}
					else if (code >= (unsigned char)'?' && code <= (unsigned char)'?') {
						field_letters[column_active_idx][line_active_idx] = code; //- ALPHABET_POW;;
					}
					else break;
					is_word = set_word(column_active_idx, line_active_idx);
					if (is_word == 1) field_letters[column_active_idx][line_active_idx] = '\0';
					break;
				}
				break;
			}
			else if (code == KEY_BACK) {
				int pass_ac;
				if (debug_info.debug_on == 1) {
					pass_ac = 1;
				}
				else {
					pass_ac = pass_turn_window();
				}
				if (pass_ac == 1) {
					turn++;
					for (j = 0; j < MAX_WORD_LEN; j++) {
						words_bank[turn][j] = '\0';
					}
					words_bank_len++;
					break;
				}
			}
			else if (code == KEY_ESC) // ESC - ?????
			{
				int sur_ac = surrender_window();
				if (sur_ac == 1) {
					show_end_game(1);
					end_game();
					return;
				}
				if (ask_for_save()) {
					save_progress();
					return;
				}
				break;
			}

			pause(40); // ????????? ????? (????? ?? ????????? ?????????)
		} // while (!key_is_pressed())


		// "?????????" ?????????? ????
		while (key_is_pressed())
			key_pressed_code();

	} // while(1)
}

int check_end_game() {
	int count = 0;
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 5; j++) {
			if (field_letters[i][j] != '\0') count++;
		}
	}
	if (count == 25) {
		//debug();
		if (debug_info.debug_on != 1) {
			show_end_game(0);
			end_game();
		}
		return 1;
	}
}

int set_word(int column_active_idx, int line_active_idx) {
	int i, j;
	char field_word[25][2];
	int word_length = 0;
	int column_letter_idx = column_active_idx;
	int line_letter_idx = line_active_idx;
	while (1)
	{
		corners.left = x_coord_field;
		corners.top = y_coord_field;
		int i, j, k, n;
		short btn_bg;
		int flag = 0;
		// ????????????? ?????????
		con_draw_lock();

		// ??????? ??????
		con_set_color(clr_font, clr_bg);
		clrscr();
		// ???? ???????????? ??????
		for (i = 0; i < 5; i++)
		{
			for (j = 0; j < 5; j++) {
				corners.left = x_coord_field + j * 9;
				corners.top = y_coord_field + i * 5;
				btn_bg = clr_bg; // ?? ????????? ??? ?????? - ??? ??? ??????
				if (i == column_active_idx && j == line_active_idx)
					btn_bg = clr_bg_chosen; // ???? ?????? ??????? - ?? ???????? ?????? ??????
				for (k = 0; k < word_length; k++) {
					if (i == field_word[k][0] && j == field_word[k][1]) {
						btn_bg = clr_bg_chosen;
					}
				}
				gotoxy(corners.left, corners.top);
				con_set_color(clr_font, btn_bg);

				printf("---------");
				corners.top++;
				gotoxy(corners.left, corners.top);
				printf("|       |");
				corners.top++;
				gotoxy(corners.left, corners.top);
				printf("|       ");

				gotoxy(corners.left + 4, corners.top);
				field_letters[i][j] == '\0' ? printf("%c", field_letters[i][j]) : printf("%c", field_letters[i][j] - ALPHABET_POW);
				//printf("?", field_letters[i][j]);

				gotoxy(corners.left + 8, corners.top);
				printf("|");
				corners.top++;

				gotoxy(corners.left, corners.top);
				printf("|       |");
				corners.top++;
				gotoxy(corners.left, corners.top);
				printf("---------");
			}
		}

		show_words_bank();

		show_score();
		// ?????? ????????????, ??????? ?? ?????
		con_draw_release();
		while (!key_is_pressed()) // ???? ???????????? ???????? ??????
		{
			int code = key_pressed_code();
			if (code == 'w' || code == 'W' || code == (unsigned char)'?' || code == (unsigned char)'?') // ???? ??? ??????? ?????
			{
				if (word_length > 0) {
					if (column_active_idx > 0)
					{
						for (n = 0; n < word_length; n++) {
							if (column_active_idx - 1 == field_word[n][0] && line_active_idx == field_word[n][1]) flag = 1; //?????? ??? ??? ???????
						}
						if (field_letters[column_active_idx - 1][line_active_idx] != '\0') {
							//????????? ?????
							if (flag != 1) {
								column_active_idx--;
								field_word[word_length][0] = column_active_idx;
								field_word[word_length][1] = line_active_idx;
								word_length++;
							}
							//????????? ???????
							else {
								if (word_length > 1 && field_word[word_length - 2][0] == column_active_idx - 1) {
									column_active_idx--;
									word_length--;
									memset(field_word[word_length], 6, 2 * sizeof(char));
								}
							}
						}
					}
				}
				else
				{
					if (column_active_idx > 0)
					{
						column_active_idx--;
					}
					else
					{
						column_active_idx = 4;
					}
				}
				break;
			}
			else if (code == 's' || code == 'S' || code == (unsigned char)'?' || code == (unsigned char)'?') // ???? ??????? ????
			{

				if (word_length > 0) {
					if (column_active_idx < 4)
					{
						for (n = 0; n < word_length; n++) {
							if (column_active_idx + 1 == field_word[n][0] && line_active_idx == field_word[n][1]) flag = 1;//?????? ??? ??? ???????
						}
						if (field_letters[column_active_idx + 1][line_active_idx] != '\0') {
							//????????? ?????
							if (flag != 1) {
								column_active_idx++;
								field_word[word_length][0] = column_active_idx;
								field_word[word_length][1] = line_active_idx;
								word_length++;
							}
							//????????? ???????
							else {
								if (word_length > 1 && field_word[word_length - 2][0] == column_active_idx + 1) {
									column_active_idx++;
									word_length--;
									memset(field_word[word_length], 6, 2 * sizeof(char));
								}
							}
						}
					}
				}
				else
				{
					if (column_active_idx < 4)
					{
						column_active_idx++;
					}
					else
					{
						column_active_idx = 0;
					}
				}
				break;
			}
			else if (code == 'd' || code == 'D' || code == (unsigned char)'?' || code == (unsigned char)'?') // ???? ??????? ??????
			{
				if (word_length > 0) {
					if (line_active_idx < 4)
					{
						for (n = 0; n < word_length; n++) {
							if (line_active_idx + 1 == field_word[n][1] && column_active_idx == field_word[n][0]) flag = 1;//?????? ??? ??? ???????
						}
						if (field_letters[column_active_idx][line_active_idx + 1] != '\0') {
							//????????? ?????
							if (flag != 1) {
								line_active_idx++;
								field_word[word_length][0] = column_active_idx;
								field_word[word_length][1] = line_active_idx;
								word_length++;
							}
							//????????? ???????
							else {
								if (word_length > 1 && field_word[word_length - 2][1] == line_active_idx + 1) {
									line_active_idx++;
									word_length--;
									memset(field_word[word_length], 6, 2 * sizeof(char));
								}
							}
						}

					}
				}
				else
				{
					if (line_active_idx < 4)
					{
						line_active_idx++;
					}
					else
					{
						line_active_idx = 0;
					}
				}
				break;
			}
			else if (code == 'a' || code == 'A' || code == (unsigned char)'?' || code == (unsigned char)'?') // ???? ??? ??????? ?????
			{
				if (word_length > 0) {
					if (line_active_idx > 0)
					{
						for (n = 0; n < word_length; n++) {
							if (line_active_idx - 1 == field_word[n][1] && column_active_idx == field_word[n][0]) flag = 1;//?????? ??? ??? ???????
						}
						if (field_letters[column_active_idx][line_active_idx - 1] != '\0') {
							//????????? ?????
							if (flag != 1) {
								line_active_idx--;
								field_word[word_length][0] = column_active_idx;
								field_word[word_length][1] = line_active_idx;
								word_length++;
							}
							//????????? ???????
							else {
								if (word_length > 1 && field_word[word_length - 2][1] == line_active_idx - 1) {
									line_active_idx--;
									word_length--;
									memset(field_word[word_length], 6, 2 * sizeof(char));
								}
							}
						}
					}
					//????????? ????? ? ?????
				}
				else
				{
					if (line_active_idx > 0)
					{
						line_active_idx--;
					}
					else
					{
						line_active_idx = 4;
					}
				}
				break;
			}
			else if (code == KEY_ENTER && field_letters[column_active_idx][line_active_idx] != '\0')
			{
				if (word_length == 0 && field_letters[column_active_idx][line_active_idx] != '\0') {
					word_length = 1;
					field_word[0][0] = column_active_idx;
					field_word[0][1] = line_active_idx;
				}
				else
				{
					//????????: ?????? ?? ???????????? ????? ? ?????????? ?????
					flag = 0;
					for (n = 0; n < word_length; n++) {
						if (column_letter_idx == field_word[n][0] && line_letter_idx == field_word[n][1]) flag = 1;
					}
					if (flag == 0)
					{
						corners.left = x_coord_field;
						corners.top = y_coord_field;
						// ????????????? ?????????
						con_draw_lock();

						// ??????? ??????
						con_set_color(clr_font, clr_bg);
						clrscr();
						// ???? ???????????? ??????
						for (i = 0; i < 5; i++)
						{
							for (j = 0; j < 5; j++) {
								corners.left = x_coord_field + j * 9;
								corners.top = y_coord_field + i * 5;
								btn_bg = clr_bg; // ?? ????????? ??? ?????? - ??? ??? ??????
								if (i == column_letter_idx && j == line_letter_idx)
									btn_bg = clr_bg_warning; // ???? ??? ?????? ? ?? ???????? ? ????? ??????, ?? ???????????? ??????

								gotoxy(corners.left, corners.top);
								con_set_color(clr_font, btn_bg);

								printf("---------");
								corners.top++;
								gotoxy(corners.left, corners.top);
								printf("|       |");
								corners.top++;
								gotoxy(corners.left, corners.top);
								printf("|       ");

								gotoxy(corners.left + 4, corners.top);
								field_letters[i][j] == '\0' ? printf("%c", field_letters[i][j]) : printf("%c", field_letters[i][j] - ALPHABET_POW);
								//printf("?", field_letters[i][j]);

								gotoxy(corners.left + 8, corners.top);
								printf("|");
								corners.top++;

								gotoxy(corners.left, corners.top);
								printf("|       |");
								corners.top++;
								gotoxy(corners.left, corners.top);
								printf("---------");
							}
						}
						show_words_bank();

						show_score();
						// ?????? ????????????, ??????? ?? ?????
						con_draw_release();

						word_length = 0;
						for (n = 0; n < word_length; n++) {
							field_word[n][0] = '\0';
							field_word[n][1] = '\0';
						}
						pause(100);
					}
					else
					{
						//???????? ?? ??????? ????? ? ???????
						sqlite3* db;
						sqlite3_stmt* res;
						const char* tail;
						if (sqlite3_open("data_ansi.db", &db))
						{
							sqlite3_close(db);
							printf("Can't open database: %s\n", sqlite3_errmsg(db));
							exit(EXIT_FAILURE);
						}

						if (sqlite3_prepare_v2(db, "select * from dict;", 128, &res, &tail) != SQLITE_OK)
						{
							sqlite3_close(db);
							printf("Can't retrieve data: %s\n", sqlite3_errmsg(db));
							exit(EXIT_FAILURE);
						}
						unsigned char str[MAX_WORD_LEN];
						int flag_compare = 0;

						while (sqlite3_step(res) == SQLITE_ROW)
						{
							strcpy(str, sqlite3_column_text(res, 0));
							int len = strlen(str);
							str[len] = '\n';
							int compare_idx = 0;
							while (compare_idx < word_length) {
								if (field_letters[field_word[compare_idx][0]][field_word[compare_idx][1]] == str[compare_idx]) compare_idx++;
								else break;
							}
							if (compare_idx == word_length && str[compare_idx] == '\n') {
								compare_idx = 0;
								for (int i = 0; i < words_bank_len; i++) {
									while (compare_idx < word_length) {
										if (field_letters[field_word[compare_idx][0]][field_word[compare_idx][1]] == words_bank[i][compare_idx]) compare_idx++;
										else break;
									}
									if (compare_idx == word_length && words_bank[i][compare_idx] == '\0') {
										flag_compare = 1;
										break;
									}
								}
								if (flag_compare != 1) {
									for (int i = 0; i < word_length; i++) {
										words_bank[words_bank_len][i] = field_letters[field_word[i][0]][field_word[i][1]];
									}
									words_bank_len += 1;
									if (game_mode == VS_PLAYER)
										if (turn % 2 != 0)
											score.first_player += word_length;
										else
											score.second_player += word_length;
									else
										score.first_player += word_length;
									turn++;
									return 0;
								}
							}
						}
						sqlite3_finalize(res);
						sqlite3_close(db);
						word_length = 0;
						for (n = 0; n < word_length; n++) {
							field_word[n][0] = '\0';
							field_word[n][1] = '\0';
						}

					}
				}
				break;
			}
			else if (code == KEY_BACK) {
				int pass_ac = pass_turn_window();
				if (pass_ac == 1) {
					turn++;
					for (j = 0; j < MAX_WORD_LEN; j++) {
						words_bank[turn][j] = '\0';
					}
					words_bank_len++;
					return 1;
				}
				break;
			}
			else if (code == KEY_ESC) // ESC - ?????
			{
				return 1;
			}

			pause(40); // ????????? ????? (????? ?? ????????? ?????????)
		} // while (!key_is_pressed())


		// "?????????" ?????????? ????
		while (key_is_pressed())
			key_pressed_code();

	}
}

void show_score() {
	corners.left = x_coord_field - 30;
	corners.top = y_coord_field - 3;
	btn_bg = clr_bg;
	con_set_color(clr_font, btn_bg);
	gotoxy(corners.left, corners.top);
	printf("%s", "????? 1   ????? 2");
	corners.left = x_coord_field - 28;
	corners.top++;
	gotoxy(corners.left, corners.top);
	printf("%d", score.first_player);
	gotoxy(corners.left + 10, corners.top);
	printf("%d", score.second_player);
}

void show_end_game(int is_sur) {
	corners.left = x_coord_menu;
	corners.top = y_coord_field;
	btn_bg = clr_bg;
	con_draw_lock();
	clrscr();
	con_set_color(clr_font, btn_bg);
	if (is_sur == 1) {
		if (turn % 2 == 0) {
			gotoxy(corners.left + 1, corners.top);
			printf("%s", "????? 1 ???????!");
		}
		else {
			gotoxy(corners.left + 4, corners.top);
			printf("%s", "????? 2 ???????!");
		}
	}
	if (is_sur == 0) {
		if (score.first_player > score.second_player) {
			gotoxy(corners.left + 1, corners.top);
			printf("%s", "????? 1 ???????!");
		}
		else if (score.first_player < score.second_player) {
			gotoxy(corners.left + 4, corners.top);
			printf("%s", "????? 2 ???????!");
		}
		else {
			gotoxy(corners.left + 9, corners.top);
			printf("%s", "?????");
		}
	}
	corners.top++;
	gotoxy(corners.left, corners.top);
	printf("%s", "????? 1         ????? 2");
	corners.left = x_coord_menu + 2;
	corners.top++;
	gotoxy(corners.left, corners.top);
	printf("%d", score.first_player);
	gotoxy(corners.left + 16, corners.top);
	printf("%d", score.second_player);
	con_draw_release();
	while (!key_is_pressed());
}

void end_game() {
	for (int i = 0; i < words_bank_len; i++) {
		for (int j = 0; j < MAX_WORD_LEN; j++) {
			words_bank[i][j] = '\0';
		}
	}
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 5; j++) {
			field_letters[i][j] = '\0';
		}
	}
	score.first_player = 0, score.second_player = 0;
	turn = 0;
}

void show_words_bank() {
	int top_player = y_coord_field - 2, top_computer = y_coord_field - 2;
	corners.left = x_coord_field + 50;
	corners.top = y_coord_field;
	gotoxy(corners.left, corners.top);
	if (start_turn == 1)
		printf("        ????? 1                 ????? 2");
	else
		printf("        ????? 2                 ????? 1");
	for (int i = 0; i < words_bank_len; i++)
	{
		if (i == 0) {
			corners.left = x_coord_field + 62;
			corners.top = y_coord_field - 3;
		}
		else if (i % 2 == 1) {
			corners.left = x_coord_field + 50;
			top_player += 3;
			corners.top = top_player;
		}
		else {
			corners.left = x_coord_field + 50 + 24;
			top_computer += 3;
			corners.top = top_computer;
		}
		btn_bg = clr_bg;
		gotoxy(corners.left, corners.top);
		con_set_color(clr_font, btn_bg);
		printf("-----------------------");
		corners.top++;
		gotoxy(corners.left, corners.top);
		printf("|                   ");

		gotoxy(corners.left + 12 - strlen(words_bank[i]) / 2, corners.top);
		printf("%s", words_bank[i]);

		con_set_color(clr_font, btn_bg);
		gotoxy(corners.left + 22, corners.top);
		printf("|");
		corners.top++;
		gotoxy(corners.left, corners.top);
		printf("-----------------------");
		corners.top++;
	}
}

void settings_menu()
{
	const char* menu_items[] = { "?????????" ,"?????????", "?????", "?????? ???", "????? ??? ESC" };
	int menu_active_idx = 1;
	int menu_items_count = sizeof(menu_items) / sizeof(menu_items[0]);
	/*short clr_bg = CON_CLR_BLACK;
	short clr_bg_active = CON_CLR_RED;
	short clr_font = CON_CLR_WHITE_LIGHT;*/
	while (1)
	{
		corners.left = x_coord_menu;
		corners.top = y_coord_field;
		int b;

		// ????????????? ?????????
		con_draw_lock();

		// ??????? ??????
		con_set_color(clr_font, clr_bg);
		clrscr();
		// ???? ???????????? ??????
		for (b = 0; b < menu_items_count; b++)
		{
			short btn_bg = clr_bg; // ?? ????????? ??? ?????? - ??? ??? ??????
			if (b == menu_active_idx)
				btn_bg = clr_bg_active; // ???? ?????? ??????? - ?? ???????? ?????? ??????

			gotoxy(corners.left, corners.top);
			con_set_color(clr_font, btn_bg);

			if (b == 0)
				printf("~~~~~~~~~~~~~~~~~~~~");
			else
				printf("====================");
			corners.top++;
			gotoxy(corners.left, corners.top);
			printf("|                   ");

			gotoxy(corners.left + 10 - strlen(menu_items[b]) / 2, corners.top);
			printf("%s", menu_items[b]);

			con_set_color(clr_font, btn_bg);
			gotoxy(corners.left + 19, corners.top);
			printf("|");
			corners.top++;
			gotoxy(corners.left, corners.top);
			if (b == 0)
				printf("~~~~~~~~~~~~~~~~~~~~");
			else
				printf("====================");
			corners.top += 2;
		}

		// ?????? ????????????, ??????? ?? ?????
		con_draw_release();


		while (!key_is_pressed()) // ???? ???????????? ???????? ??????
		{
			int code = key_pressed_code();

			if (code == 'w' || code == 'W' || code == (unsigned char)'?' || code == (unsigned char)'?') // ???? ??? ??????? ?????
			{
				// ?? ??????? ? ???????? ?????? (???? ??? ????????)
				if (menu_active_idx > 1)
				{
					menu_active_idx--;
					break;
				}
				else
				{
					menu_active_idx = menu_items_count - 1;
					break;
				}
			}
			else if (code == 's' || code == 'S' || code == (unsigned char)'?' || code == (unsigned char)'?') // ???? ??????? ????
			{
				// ?? ??????? ? ??????? ?????? (???? ??? ????????)
				if (menu_active_idx + 1 < menu_items_count)
				{
					menu_active_idx++;
					break;
				}
				else
				{
					menu_active_idx = 1;
					break;
				}
			}
			else if (code == KEY_ESC || code == 'q' || code == 'Q' ||
				code == (unsigned char)'?' || code == (unsigned char)'?') // ESC ??? 'q' - ?????
			{
				return;
			}
			else if (code == KEY_ENTER) // ?????? ?????? Enter
			{
				if (menu_active_idx == 4) // ?????? ????????? ????? - ??? ?????
					return;
				if (menu_active_idx == 2) {
					mode_selection();
				}

				if (menu_active_idx == 1)//?????? ????? ?????????
					difficulty_selection();

				if (menu_active_idx == 3)//?????? ????? ?????? ???
					first_turn_selection();

				break;
			}


			pause(40); // ????????? ????? (????? ?? ????????? ?????????)
		} // while (!key_is_pressed())


		// "?????????" ?????????? ????
		while (key_is_pressed())
			key_pressed_code();

	} // while(1)
}

int surrender_window() {
	const char* menu_items[] = { "??", "???" };
	int menu_active_idx = 1;
	int menu_items_count = sizeof(menu_items) / sizeof(menu_items[0]);
	while (1) {
		corners.left = x_coord_menu;
		corners.top = y_coord_field;
		int b;
		int code;

		// ????????????? ?????????
		con_draw_lock();

		// ??????? ??????
		con_set_color(clr_font, clr_bg);
		clrscr();
		// ???? ???????????? ??????
		short btn_bg = clr_bg;
		gotoxy(corners.left, corners.top);
		printf("?? ???????, ??? ?????? ????????");
		corners.top += 2;
		for (b = 0; b < 2; b++)
		{
			btn_bg = clr_bg; // ?? ????????? ??? ?????? - ??? ??? ??????
			if (b == menu_active_idx)
				btn_bg = clr_bg_active; // ???? ?????? ??????? - ?? ???????? ?????? ??????
			gotoxy(corners.left, corners.top);
			con_set_color(clr_font, btn_bg);
			printf("%s", menu_items[b]);
			corners.left += 3;
		}
		con_draw_release();

		while (!key_is_pressed()) // ???? ???????????? ???????? ??????
		{
			code = key_pressed_code();
			if (code == 'a' || code == 'A' || code == (unsigned char)'?' || code == (unsigned char)'?')
			{
				if (menu_active_idx > 0)
				{
					menu_active_idx--;
					break;
				}
				else
				{
					menu_active_idx = 1;
					break;
				}
			}
			else if (code == 'd' || code == 'D' || code == (unsigned char)'?' || code == (unsigned char)'?')
			{
				if (menu_active_idx < 1)
				{
					menu_active_idx++;
					break;
				}
				else
				{
					menu_active_idx = 0;
					break;
				}
			}
			else if (code == KEY_ESC || code == 'q' || code == 'Q' ||
				code == (unsigned char)'?' || code == (unsigned char)'?') // ESC ??? 'q' - ?????
			{
				return 0;
			}
			else if (code == KEY_ENTER) // ?????? ?????? Enter
			{
				if (menu_active_idx == 1) // ?????? ?????? ????? - ??? ?????
					return 0;

				if (menu_active_idx == 0)// ?????? ?????? ????? - ??? ?????
					return 1;
			}


			pause(40); // ????????? ????? (????? ?? ????????? ?????????)
		} // while (!key_is_pressed())


		// "?????????" ?????????? ????
		while (key_is_pressed())
			key_pressed_code();

	}
}

int pass_turn_window() {
	const char* menu_items[] = { "??", "???" };
	int menu_active_idx = 1;
	int menu_items_count = sizeof(menu_items) / sizeof(menu_items[0]);
	while (1) {
		corners.left = x_coord_menu;
		corners.top = y_coord_field;
		int b;
		int code;

		// ????????????? ?????????
		con_draw_lock();

		// ??????? ??????
		con_set_color(clr_font, clr_bg);
		clrscr();
		// ???? ???????????? ??????
		short btn_bg = clr_bg;
		gotoxy(corners.left, corners.top);
		printf("?? ???????, ??? ?????? ?????????? ????");
		corners.top += 2;
		for (b = 0; b < 2; b++)
		{
			btn_bg = clr_bg; // ?? ????????? ??? ?????? - ??? ??? ??????
			if (b == menu_active_idx)
				btn_bg = clr_bg_active; // ???? ?????? ??????? - ?? ???????? ?????? ??????
			gotoxy(corners.left, corners.top);
			con_set_color(clr_font, btn_bg);
			printf("%s", menu_items[b]);
			corners.left += 3;
		}
		con_draw_release();

		while (!key_is_pressed()) // ???? ???????????? ???????? ??????
		{
			code = key_pressed_code();
			if (code == 'a' || code == 'A' || code == (unsigned char)'?' || code == (unsigned char)'?')
			{
				if (menu_active_idx > 0)
				{
					menu_active_idx--;
					break;
				}
				else
				{
					menu_active_idx = 1;
					break;
				}
			}
			else if (code == 'd' || code == 'D' || code == (unsigned char)'?' || code == (unsigned char)'?')
			{
				if (menu_active_idx < 1)
				{
					menu_active_idx++;
					break;
				}
				else
				{
					menu_active_idx = 0;
					break;
				}
			}
			else if (code == KEY_ESC || code == 'q' || code == 'Q' ||
				code == (unsigned char)'?' || code == (unsigned char)'?') // ESC ??? 'q' - ?????
			{
				return 0;
			}
			else if (code == KEY_ENTER) // ?????? ?????? Enter
			{
				if (menu_active_idx == 1) // ?????? ?????? ????? - ??? ??????? ????
					return 0;

				if (menu_active_idx == 0)
					return 1;
			}


			pause(40); // ????????? ????? (????? ?? ????????? ?????????)
		} // while (!key_is_pressed())


		// "?????????" ?????????? ????
		while (key_is_pressed())
			key_pressed_code();

	}
}

void difficulty_selection()
{
	char* menu_items[] = { "?????????", "˸????", "???????", "???????" ,"????? ??? ESC" };
	int menu_active_idx = 1;
	int menu_items_count = sizeof(menu_items) / sizeof(menu_items[0]);
	//short clr_bg = CON_CLR_BLACK;
	//short clr_bg_active = CON_CLR_RED;
	//short clr_font = CON_CLR_WHITE_LIGHT;
	//short clr_bg_chosen = CON_CLR_RED_LIGHT;
	while (1)
	{
		corners.left = x_coord_menu;
		corners.top = y_coord_field;
		int b;

		// ????????????? ?????????
		con_draw_lock();

		// ??????? ??????
		con_set_color(clr_font, clr_bg);
		clrscr();
		// ???? ???????????? ??????
		for (b = 0; b < menu_items_count; b++)
		{
			short btn_bg = clr_bg; // ?? ????????? ??? ?????? - ??? ??? ??????

			//if (b == difficult)
			//	btn_bg = clr_bg_chosen;//?????? ?? ??????? ? ??? ??????? ?????????
			if (b == menu_active_idx)
				btn_bg = clr_bg_active; // ???? ?????? ??????? - ?? ???????? ?????? ??????
			gotoxy(corners.left, corners.top);
			con_set_color(clr_font, btn_bg);

			if (b == 0)
				printf("~~~~~~~~~~~~~~~~~~~~");
			else
				printf("====================");
			corners.top++;
			if (b == difficult) {
				gotoxy(corners.left - 4, corners.top);
				printf("--->|                   ");
			}
			else {
				gotoxy(corners.left, corners.top);
				printf("|                   ");
			}

			gotoxy(corners.left + 10 - strlen(menu_items[b]) / 2, corners.top);
			printf("%s", menu_items[b]);
			con_set_color(clr_font, btn_bg);
			if (b == difficult) {
				gotoxy(corners.left + 19, corners.top);
				printf("|<---");
			}
			else {
				gotoxy(corners.left + 19, corners.top);
				printf("|");
			}
			corners.top++;
			gotoxy(corners.left, corners.top);
			if (b == 0)
				printf("~~~~~~~~~~~~~~~~~~~~");
			else
				printf("====================");
			corners.top += 2;
		}

		// ?????? ????????????, ??????? ?? ?????
		con_draw_release();


		while (!key_is_pressed()) // ???? ???????????? ???????? ??????
		{
			int code = key_pressed_code();
			if (code == 'w' || code == 'W' || code == (unsigned char)'?' || code == (unsigned char)'?') // ???? ??? ??????? ?????
			{
				// ?? ??????? ? ???????? ?????? (???? ??? ????????)
				if (menu_active_idx > 1)
				{
					menu_active_idx--;
					break;
				}
				else
				{
					menu_active_idx = menu_items_count - 1;
					break;
				}
			}
			else if (code == 's' || code == 'S' || code == (unsigned char)'?' || code == (unsigned char)'?') // ???? ??????? ????
			{
				// ?? ??????? ? ??????? ?????? (???? ??? ????????)
				if (menu_active_idx + 1 < menu_items_count)
				{
					menu_active_idx++;
					break;
				}
				else
				{
					menu_active_idx = 1;
					break;
				}
			}
			else if (code == KEY_ESC || code == 'q' || code == 'Q' ||
				code == (unsigned char)'?' || code == (unsigned char)'?') // ESC ??? 'q' - ?????
			{
				return;
			}
			else if (code == KEY_ENTER) // ?????? ?????? Enter
			{
				if (menu_active_idx == 4) // ?????? ????????? ????? - ??? ?????
					return;

				if (menu_active_idx == EASY)
					difficult = EASY;

				if (menu_active_idx == MEDIUM)
					difficult = MEDIUM;

				if (menu_active_idx == HARD)
					difficult = HARD;
				break;
			}


			pause(40); // ????????? ????? (????? ?? ????????? ?????????)
		} // while (!key_is_pressed())


		// "?????????" ?????????? ????
		while (key_is_pressed())
			key_pressed_code();

	} // while(1)
}

void first_turn_selection() {
	const char* menu_items[] = { "????? ??????? ????" ,"????? 1", "????? 2" ,"????? ??? ESC" };
	int menu_active_idx = 1;
	int menu_items_count = sizeof(menu_items) / sizeof(menu_items[0]);
	//short clr_bg = CON_CLR_BLACK;
	//short clr_bg_active = CON_CLR_RED;
	//short clr_font = CON_CLR_WHITE_LIGHT;
	while (1)
	{
		corners.left = x_coord_menu;
		corners.top = y_coord_field;
		int b;

		// ????????????? ?????????
		con_draw_lock();

		// ??????? ??????
		con_set_color(clr_font, clr_bg);
		clrscr();
		// ???? ???????????? ??????
		for (b = 0; b < menu_items_count; b++)
		{
			short btn_bg = clr_bg; // ?? ????????? ??? ?????? - ??? ??? ??????
			if (b == menu_active_idx)
				btn_bg = clr_bg_active; // ???? ?????? ??????? - ?? ???????? ?????? ??????

			gotoxy(corners.left, corners.top);
			con_set_color(clr_font, btn_bg);
			if (b == 0)
				printf("~~~~~~~~~~~~~~~~~~~~");
			else
				printf("====================");
			corners.top++;
			if (b == start_turn) {
				gotoxy(corners.left - 4, corners.top);
				printf("--->|                   ");
			}
			else {
				gotoxy(corners.left, corners.top);
				printf("|                   ");
			}

			gotoxy(corners.left + 10 - strlen(menu_items[b]) / 2, corners.top);
			printf("%s", menu_items[b]);
			con_set_color(clr_font, btn_bg);
			if (b == start_turn) {
				gotoxy(corners.left + 19, corners.top);
				printf("|<---");
			}
			else {
				gotoxy(corners.left + 19, corners.top);
				printf("|");
			}
			corners.top++;
			gotoxy(corners.left, corners.top);
			if (b == 0)
				printf("~~~~~~~~~~~~~~~~~~~~");
			else
				printf("====================");
			corners.top += 2;
		}

		// ?????? ????????????, ??????? ?? ?????
		con_draw_release();


		while (!key_is_pressed()) // ???? ???????????? ???????? ??????
		{
			int code = key_pressed_code();
			if (code == 'w' || code == 'W' || code == (unsigned char)'?' || code == (unsigned char)'?') // ???? ??? ??????? ?????
			{
				// ?? ??????? ? ???????? ?????? (???? ??? ????????)
				if (menu_active_idx > 1)
				{
					menu_active_idx--;
					break;
				}
				else
				{
					menu_active_idx = menu_items_count - 1;
					break;
				}
			}
			else if (code == 's' || code == 'S' || code == (unsigned char)'?' || code == (unsigned char)'?') // ???? ??????? ????
			{
				// ?? ??????? ? ??????? ?????? (???? ??? ????????)
				if (menu_active_idx + 1 < menu_items_count)
				{
					menu_active_idx++;
					break;
				}
				else
				{
					menu_active_idx = 1;
					break;
				}
			}
			else if (code == KEY_ESC || code == 'q' || code == 'Q' ||
				code == (unsigned char)'?' || code == (unsigned char)'?') // ESC ??? 'q' - ?????
			{
				return;
			}
			else if (code == KEY_ENTER) // ?????? ?????? Enter
			{
				if (menu_active_idx == 3) // ?????? ????????? ????? - ??? ?????
					return;

				if (menu_active_idx == 1)//??????? ????? ??????
					start_turn = 1;

				if (menu_active_idx == 2)//????????? ????? ??????
					start_turn = 2;

				break;
			}


			pause(40); // ????????? ????? (????? ?? ????????? ?????????)
		} // while (!key_is_pressed())


		// "?????????" ?????????? ????
		while (key_is_pressed())
			key_pressed_code();

	} // while(1)
}

void about() {
	con_set_color(CON_CLR_WHITE_LIGHT, CON_CLR_BLACK);
	clrscr();

	gotoxy(8, 2);
	printf("? ?????????:");

	con_set_color(CON_CLR_GRAY, CON_CLR_BLACK);
	gotoxy(8, 3);
	printf("????????? ????????????? ?? ????? ?? ???? ?????)\n\n");

	gotoxy(8, 4);
	printf("??? ??????????? ??????? ????? ???????.");

	key_pressed_code();
	return;
}

int ask_for_save() {
	const char* menu_items[] = { "??", "???" };
	int menu_active_idx = 1;
	int menu_items_count = sizeof(menu_items) / sizeof(menu_items[0]);
	while (1) {
		corners.left = x_coord_menu;
		corners.top = y_coord_field;
		int b;
		int code;

		// ????????????? ?????????
		con_draw_lock();

		// ??????? ??????
		con_set_color(clr_font, clr_bg);
		clrscr();
		// ???? ???????????? ??????
		short btn_bg = clr_bg;
		gotoxy(corners.left, corners.top);
		printf("?????? ????????? ???????? ? ??????");
		corners.top += 2;
		for (b = 0; b < 2; b++)
		{
			btn_bg = clr_bg; // ?? ????????? ??? ?????? - ??? ??? ??????
			if (b == menu_active_idx)
				btn_bg = clr_bg_active; // ???? ?????? ??????? - ?? ???????? ?????? ??????
			gotoxy(corners.left, corners.top);
			con_set_color(clr_font, btn_bg);
			printf("%s", menu_items[b]);
			corners.left += 3;
		}
		con_draw_release();

		while (!key_is_pressed()) // ???? ???????????? ???????? ??????
		{
			code = key_pressed_code();
			if (code == 'a' || code == 'A' || code == (unsigned char)'?' || code == (unsigned char)'?')
			{
				if (menu_active_idx > 0)
				{
					menu_active_idx--;
					break;
				}
				else
				{
					menu_active_idx = 1;
					break;
				}
			}
			else if (code == 'd' || code == 'D' || code == (unsigned char)'?' || code == (unsigned char)'?')
			{
				if (menu_active_idx < 1)
				{
					menu_active_idx++;
					break;
				}
				else
				{
					menu_active_idx = 0;
					break;
				}
			}
			else if (code == KEY_ESC || code == 'q' || code == 'Q' ||
				code == (unsigned char)'?' || code == (unsigned char)'?') // ESC ??? 'q' - ?????
			{
				return 0;
			}
			else if (code == KEY_ENTER) // ?????? ?????? Enter
			{
				if (menu_active_idx == 1) // ?????? ?????? ????? - ??? ?????
					return 0;

				if (menu_active_idx == 0)// ?????? ?????? ????? - ??? ?????
					return 1;
			}


			pause(40); // ????????? ????? (????? ?? ????????? ?????????)
		} // while (!key_is_pressed())


		// "?????????" ?????????? ????
		while (key_is_pressed())
			key_pressed_code();

	}
}

int ask_for_load() {
	const char* menu_items[] = { "??", "???" };
	int menu_active_idx = 1;
	int menu_items_count = sizeof(menu_items) / sizeof(menu_items[0]);
	while (1) {
		corners.left = x_coord_menu;
		corners.top = y_coord_field;
		int b;
		int code;

		// ????????????? ?????????
		con_draw_lock();

		// ??????? ??????
		con_set_color(clr_font, clr_bg);
		clrscr();
		// ???? ???????????? ??????
		short btn_bg = clr_bg;
		gotoxy(corners.left, corners.top);
		printf("?????? ????????? ??????? ?????");
		corners.top += 2;
		for (b = 0; b < 2; b++)
		{
			btn_bg = clr_bg; // ?? ????????? ??? ?????? - ??? ??? ??????
			if (b == menu_active_idx)
				btn_bg = clr_bg_active; // ???? ?????? ??????? - ?? ???????? ?????? ??????
			gotoxy(corners.left, corners.top);
			con_set_color(clr_font, btn_bg);
			printf("%s", menu_items[b]);
			corners.left += 3;
		}
		con_draw_release();

		while (!key_is_pressed()) // ???? ???????????? ???????? ??????
		{
			code = key_pressed_code();
			if (code == 'a' || code == 'A' || code == (unsigned char)'?' || code == (unsigned char)'?')
			{
				if (menu_active_idx > 0)
				{
					menu_active_idx--;
					break;
				}
				else
				{
					menu_active_idx = 1;
					break;
				}
			}
			else if (code == 'd' || code == 'D' || code == (unsigned char)'?' || code == (unsigned char)'?')
			{
				if (menu_active_idx < 1)
				{
					menu_active_idx++;
					break;
				}
				else
				{
					menu_active_idx = 0;
					break;
				}
			}
			else if (code == KEY_ESC || code == 'q' || code == 'Q' ||
				code == (unsigned char)'?' || code == (unsigned char)'?') // ESC ??? 'q' - ?????
			{
				return 0;
			}
			else if (code == KEY_ENTER) // ?????? ?????? Enter
			{
				if (menu_active_idx == 1) // ?????? ?????? ????? - ??? ?????
					return 0;

				if (menu_active_idx == 0)// ?????? ?????? ????? - ??? ?????
					return 1;
			}


			pause(40); // ????????? ????? (????? ?? ????????? ?????????)
		} // while (!key_is_pressed())


		// "?????????" ?????????? ????
		while (key_is_pressed())
			key_pressed_code();

	}
}

void save_progress() {
	FILE* file = NULL;
	fopen_s(&file, SAVE_FILE, "w");
	if (file != NULL) {
		for (int i = 0; i < 5; i++) {
			for (int j = 0; j < 5; j++) {
				fputc(field_letters[i][j], file);
				fputc('\n', file);
			}
		}
		fputc(turn, file);
		fputc('\n', file);
		fputc(score.first_player, file);
		fputc('\n', file);
		fputc(score.second_player, file);
		fputc('\n', file);
		fputc(difficult, file);
		fputc('\n', file);
		fputc(game_mode, file);
		fputc('\n', file);
		for (int i = 0; i < MAX_WORDS_COUNT; i++) {
			for (int j = 0; j < MAX_WORD_LEN; j++) {
				fputc(words_bank[i][j], file);
				fputc('\n', file);
			}
		}
		fputc(words_bank_len, file);
		fputc('\n', file);
	}
}

void load_progress() {
	FILE* file = NULL;
	fopen_s(&file, SAVE_FILE, "r");
	if (file != NULL) {
		for (int i = 0; i < 5; i++) {
			for (int j = 0; j < 5; j++) {
				field_letters[i][j] = fgetc(file);
				fgetc(file);
			}
		}
		turn = fgetc(file);
		fgetc(file);
		score.first_player = fgetc(file);
		fgetc(file);
		score.second_player = fgetc(file);
		fgetc(file);
		difficult = fgetc(file);
		fgetc(file);
		game_mode = fgetc(file);
		fgetc(file);
		for (int i = 0; i < MAX_WORDS_COUNT; i++) {
			for (int j = 0; j < MAX_WORD_LEN; j++) {
				words_bank[i][j] = fgetc(file);
				fgetc(file);
			}
		}
		words_bank_len = fgetc(file);
		fgetc(file);
	}
	else {
		return;
	}
}
