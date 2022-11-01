
#define _CRT_SECURE_NO_WARNINGS
#include "wincon.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>

#include <time.h>


#define	 LETTER_a_RUS			224
#define  INV					"inv.txt"
#define  DICT					"dict.txt"
#define  START_WORDS			"start_words.txt"
#define  SAVE_FILE				"save.txt"
#define	 ALPHABET_POW			32
#define  MAX_WORD_LEN			30
#define  MAX_WORDS_COUNT		21
#define	 clr_bg					CON_CLR_BLACK
#define  clr_bg_active			CON_CLR_RED
#define  clr_font				CON_CLR_WHITE_LIGHT
#define  clr_bg_chosen			CON_CLR_GREEN
#define  clr_bg_warning         CON_CLR_YELLOW
#define  five_count             2236
#define  x_coord_field          80
#define  y_coord_field          5
#define  x_coord_menu           x_coord_field + 14
#define  debug_buff				21
#define	 debug_file				"course_work.txt"
#define  test 					"dsadsa"

/*Структура узла словарного и инвертированного префиксных деревьев*/
typedef struct node {
	unsigned char letters[ALPHABET_POW];
	struct node* next[ALPHABET_POW];
	char* word;
}NODE;



/*Графика*/
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

/*Считывание файлов*/
void read_dict_to_tree(NODE*);
void read_inv_to_tree(NODE*);
/*Алгоритм поиска*/
int bot_move();
void check_all_letters(int, int);

/*Служебные*/
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
char field_for_search[5][5] = { {1} }; // Нужно для отметок уже отработанных клеток для хода ИИ
unsigned char* longest_word[MAX_WORD_LEN] = { '\0' };
unsigned char words_bank[MAX_WORDS_COUNT][MAX_WORD_LEN];
unsigned char letter_chosen = 0;
unsigned char field_letters[5][5] = { {'\0'} };
int btn_bg;
int difficult = EASY; //сложность изначально "лёгкий"
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

/*Функции бота*/
/*Поиск индекса буквы внутри узла*/
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

/*Поиск индекса буквы внутри узла*/
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

/*Вставка слова в дерево*/
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
				/*Нет доступной памяти*/
				return -1;
			}
			memset(new_node, 0, sizeof(NODE));
			new_node->word = NULL;
			node->next[result] = new_node;
		}
		node = node->next[result];
	}
	/*Вставка всего слова в поле word?*/
	//if (root != roots.inv) {
	node->word = (char*)malloc(strlen(word));
	strcpy(node->word, word);
	//}
}

/*Считывание словаря в словарное дерево*/
void read_dict_to_tree(NODE* root) {
	FILE* file;
	char line[MAX_WORD_LEN + 2];
	int len = 0;
	if (fopen_s(&file, DICT, "r+")) {
		printf("Словарь не найден...");
		exit(EXIT_FAILURE);
	}
	while (fgets(line, sizeof(line), file) != NULL) {
		len = strlen(line) - 1;
		(line[len] == '\n') ? line[len] = '\0' : line[len];
		insert_word_tree(line, root);
	}
	fclose(file);
}

void read_inv_to_tree(NODE* root) {
	FILE* file;
	char line[MAX_WORD_LEN + 2];
	int len = 0;
	if (fopen_s(&file, INV, "r+")) {
		printf("Словарь не найден...");
		exit(EXIT_FAILURE);
	}
	while (fgets(line, sizeof(line), file) != NULL) {
		len = strlen(line) - 1;
		(line[len] == '\n') ? line[len] = '\0' : line[len];
		insert_word_tree(line, root);
	}
	fclose(file);
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

/*Основная функция поиска в словарном дереве*/
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
	// Проверяем все смежные клетки
	//Верхняя клетка
	res = get_letter_index(field_letters[x - 1][y], node);
	if (field_letters[x - 1][y] != '\0' && field_for_search[x - 1][y] != 1 && res != -1 && x > 0) {
		curr_word[strlen(curr_word)] = field_letters[x - 1][y];
		curr_word[strlen(curr_word)] = '\0';
		search_dict_tree(x - 1, y, node->next[res], curr_word);
		//if (found) return;
		curr_word[strlen(curr_word) - 1] = '\0';// Подумать над необходимостью
	}
	// Правая клетка
	res = get_letter_index(field_letters[x][y + 1], node);
	if (field_letters[x][y + 1] != '\0' && field_for_search[x][y + 1] != 1 && res != -1 && y < 4) {
		curr_word[strlen(curr_word)] = field_letters[x][y + 1];
		curr_word[strlen(curr_word)] = '\0';
		search_dict_tree(x, y + 1, node->next[res], curr_word);
		//if (found) return;
		curr_word[strlen(curr_word) - 1] = '\0';// Подумать над необходимостью
	}
	// Нижняя клетка
	res = get_letter_index(field_letters[x + 1][y], node);
	if (field_letters[x + 1][y] != '\0' && field_for_search[x + 1][y] != 1 && res != -1 && x < 4) {
		curr_word[strlen(curr_word)] = field_letters[x + 1][y];
		curr_word[strlen(curr_word)] = '\0';
		search_dict_tree(x + 1, y, node->next[res], curr_word);
		//if (found) return;
		curr_word[strlen(curr_word) - 1] = '\0';// Подумать над необходимостью
	}


	// Левая клетка
	res = get_letter_index(field_letters[x][y - 1], node);
	if (field_letters[x][y - 1] != '\0' && field_for_search[x][y - 1] != 1 && res != -1 && y > 0) {
		curr_word[strlen(curr_word)] = field_letters[x][y - 1];
		curr_word[strlen(curr_word)] = '\0';
		search_dict_tree(x, y - 1, node->next[res], curr_word);
		//if (found) return;
		curr_word[strlen(curr_word) - 1] = '\0';// Подумать над необходимостью
	}

	(checked == 1) ? field_for_search[x][y] = 0 : field_for_search[x][y];
	return;
}


/*Функция разворота слова(создание инвар.)*/
void reverse_word(unsigned char* word) {
	for (int i = 0, j = strlen(word) - 1; i < strlen(word) / 2; i++, j--) {
		unsigned char temp = word[i];
		word[i] = word[j];
		word[j] = temp;
	}
}


// Фукнция нужна для нахождения аналогичного узла из инвертированного дерева в словарном дереве 
// Который в дальнейшем используется в поиске по словарному дереву
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

/*Основная функция поиска в инв дереве*/
void search_inv_tree(int x, int y, unsigned char* curr_word, int end_ind) {
	if (field_letters[x][y] == '\0' || x < 0 || y < 0 || x >= 5 || y >= 5) {
		return;
	}
	curr_word[end_ind] = field_letters[x][y];
	field_for_search[x][y] = 1;
	curr_word[end_ind + 1] = '\0';
	// Находим окончание слова
	unsigned char new_word[MAX_WORD_LEN] = { '\0' };
	strcpy(new_word, curr_word);
	if (find_word_tree(curr_word, roots.inv)) {
		NODE* node = inv_to_dict_node(new_word);
		search_dict_tree(start_cords.x, start_cords.y, node, new_word);
		//if (found) return;
	}
	//Можно добавить оптимизацию
	else {
		
	}
	// Проверяем все смежные клетки
	//Верхняя клетка
	if (field_letters[x - 1][y] != '\0' && field_for_search[x - 1][y] != 1) {
		search_inv_tree(x - 1, y, curr_word, end_ind + 1);
	}
	curr_word[end_ind + 1] = '\0';

	// Левая клетка
	if (field_letters[x][y + 1] != '\0' && field_for_search[x][y + 1] != 1) {
		search_inv_tree(x, y + 1, curr_word, end_ind + 1);
	}
	curr_word[end_ind + 1] = '\0';

	// Нижняя клетка
	if (field_letters[x + 1][y] != '\0' && field_for_search[x + 1][y] != 1) {
		search_inv_tree(x + 1, y, curr_word, end_ind + 1);
	}
	curr_word[end_ind + 1] = '\0';

	// Правая клетка
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
		if (difficult == EASY && max_len >= 3) {
			break;
		}
		if (difficult == MEDIUM && max_len >= 4) {
			break;
		}
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
	// Стартовый индекс 1 или 0?
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
		//printf("Слит ботяра");
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
	read_dict_to_tree(roots.dict);
	read_inv_to_tree(roots.inv);
	//read_dict_to_tree()
	/*file = fopen(DICT, "r");
	if (file == NULL) {
		printf("I just don't have the words to describe the pain i feel!");
		return 1;
	}
	fclose(file);*/
	file = fopen(START_WORDS, "r");
	if (file == NULL) {
		printf("I just don't have the words with five letters to describe the pain i feel!");
		return 1;
	}
	fclose(file);
	// Инициализируется консоль, скрывается курсор, добавляется русский язык в консоль
	con_init(100, 50);
	// system("mode con cols=100 lines=25");
	show_cursor(0);
	score.first_player = 0, score.second_player = 0;
	debug_info.debug_on = 1;
	// Запуск главного меню
	main_menu();

	return 0;
}


// Поддержка главного меню
void main_menu()
{
	const char* menu_items[] = { "Балда" ,"Игра", "Настройки", "О программе", "Выход" };
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
		// Заблокировать отрисовку
		con_draw_lock();

		// Очистка экрана
		con_set_color(clr_font, clr_bg);
		clrscr();
		// Цикл отрисовывает кнопку
		for (b = 0; b < menu_items_count; b++)
		{
			short btn_bg = clr_bg; // По умолчанию фон кнопки - как фон экрана
			if (b == menu_active_idx)
				btn_bg = clr_bg_active; // Если кнопка активна - то рисуется другим цветом
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

		// Данные подготовлены, вывести на экран
		con_draw_release();


		while (!key_is_pressed()) // Если пользователь нажимает кнопку
		{
			int code = key_pressed_code();
			if (code == 'w' || code == 'W' || code == (unsigned char)'ц' || code == (unsigned char)'Ц') // Если это стрелка вверх
			{
				// То переход к верхнему пункту (если это возможно)
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
			else if (code == 's' || code == 'S' || code == (unsigned char)'ы' || code == (unsigned char)'Ы') // Если стрелка вниз
			{
				// То переход к нижнему пункту (если это возможно)
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
				code == (unsigned char)'й' || code == (unsigned char)'Й') // ESC или 'q' - выход
			{
				if (ask_for_save()) {
					save_progress();
				}
				return;
			}
			else if (code == KEY_ENTER) // Нажата кнопка Enter
			{
				if (menu_active_idx == 4) { // Выбран последний пункт - это выход
					if (ask_for_save()) {
						save_progress();
						return;
					}
				}
				//if (menu_active_idx == 0)

				if (menu_active_idx == 2) // Выбран пункт "настройки"
					settings_menu();
				if (menu_active_idx == 1) {
					set_letter();
				}
				//if (menu_active_idx == 3)
				if (menu_active_idx == 3)
					about();

				break;
			}


			pause(40); // Небольная пауза (чтобы не загружать процессор)
		} // while (!key_is_pressed())


		// "Съедается" оставшийся ввод
		while (key_is_pressed())
			key_pressed_code();

	} // while(1)
}

void mode_selection() {
	char* menu_items[] = { "Режим" ,"Против игрока", "Против компьютера", "Дзен" ,"Назад или ESC" };
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

		// Заблокировать отрисовку
		con_draw_lock();

		// Очистка экрана
		con_set_color(clr_font, clr_bg);
		clrscr();
		// Цикл отрисовывает кнопку
		for (b = 0; b < menu_items_count; b++)
		{
			short btn_bg = clr_bg; // По умолчанию фон кнопки - как фон экрана

			//if (b == difficult)
			//	btn_bg = clr_bg_chosen;//Кнопка не активна и это текущая сложность
			if (b == menu_active_idx)
				btn_bg = clr_bg_active; // Если кнопка активна - то рисуется другим цветом
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

		// Данные подготовлены, вывести на экран
		con_draw_release();


		while (!key_is_pressed()) // Если пользователь нажимает кнопку
		{
			int code = key_pressed_code();
			if (code == 'w' || code == 'W' || code == (unsigned char)'ц' || code == (unsigned char)'Ц') // Если это стрелка вверх
			{
				// То переход к верхнему пункту (если это возможно)
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
			else if (code == 's' || code == 'S' || code == (unsigned char)'ы' || code == (unsigned char)'Ы') // Если стрелка вниз
			{
				// То переход к нижнему пункту (если это возможно)
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
				code == (unsigned char)'й' || code == (unsigned char)'Й') // ESC или 'q' - выход
			{
				return;
			}
			else if (code == KEY_ENTER) // Нажата кнопка Enter
			{
				if (menu_active_idx == 4) // Выбран последний пункт - это выход
					return;

				if (menu_active_idx == 1)//Лёгкая сложность
					game_mode = VS_PLAYER;


				if (menu_active_idx == 2)//Средняя сложность
					game_mode = VS_ROBOT;

				if (menu_active_idx == 3)//Сложная
					game_mode = DZEN;
				break;
			}


			pause(40); // Небольная пауза (чтобы не загружать процессор)
		} // while (!key_is_pressed())


		// "Съедается" оставшийся ввод
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
		//Случайнок слово в начале
		FILE* five_file;
		char five_word[7];
		five_file = fopen(START_WORDS, "r");
		srand(time(NULL));
		int r = rand() % five_count;
		system("cls");
		for (i = 1; i != r; i++) fgets(five_word, 7, five_file);
		field_letters[2][0] = five_word[0];
		field_letters[2][1] = five_word[1];
		field_letters[2][2] = five_word[2];
		field_letters[2][3] = five_word[3];
		field_letters[2][4] = five_word[4];
		words_bank[0][0] = five_word[0];
		words_bank[0][1] = five_word[1];
		words_bank[0][2] = five_word[2];
		words_bank[0][3] = five_word[3];
		words_bank[0][4] = five_word[4];
		words_bank[0][5] = '\0';
		words_bank_len = 1;
		/*field_letters[2][0] = 'ж';
		field_letters[2][1] = 'у';
		field_letters[2][2] = 'ч';
		field_letters[2][3] = 'о';
		field_letters[2][4] = 'к';
		words_bank[0][0] = 'ж';
		words_bank[0][1] = 'у';
		words_bank[0][2] = 'ч';
		words_bank[0][3] = 'о';
		words_bank[0][4] = 'к';
		words_bank[0][5] = '\0';
		words_bank_len = 1;*/
		fclose(five_file);
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
			debug_info.time_pass[turn_test] = (double)(clock() - start_time) / CLOCKS_PER_SEC;
			debug_info.cells_left[turn_test] = 0;
			for (i = 0; i < 5; i++) {
				for (int j = 0; j < 5; j++) {
					if (field_letters[i][j] == '\0') debug_info.cells_left[turn_test]++;
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
		// Заблокировать отрисовку
		con_draw_lock();

		// Очистка экрана
		con_set_color(clr_font, clr_bg);
		clrscr();
		// Цикл отрисовывает кнопку

		for (i = 0; i < field_letters_column_count; i++)
		{
			for (j = 0; j < field_letters_line_count; j++) {
				corners.left = x_coord_field + j * 9;
				corners.top = y_coord_field + i * 5;
				btn_bg = clr_bg; // По умолчанию фон кнопки - как фон экрана
				if (i == column_active_idx && j == line_active_idx)
					btn_bg = clr_bg_active; // Если кнопка активна - то рисуется другим цветом

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
				//printf("А", field_letters[i][j]);

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

		// Данные подготовлены, вывести на экран
		con_draw_release();
		debug();
		while (!key_is_pressed()) // Если пользователь нажимает кнопку
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
			if (code == 'w' || code == 'W' || code == (unsigned char)'ц' || code == (unsigned char)'Ц') // Если это стрелка вверх
			{
				// То переход к верхнему пункту (если это возможно)
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
			else if (code == 's' || code == 'S' || code == (unsigned char)'ы' || code == (unsigned char)'Ы') // Если стрелка вниз
			{
				// То переход к нижнему пункту (если это возможно)
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
			else if (code == 'd' || code == 'D' || code == (unsigned char)'в' || code == (unsigned char)'В') // Если стрелка вправо
			{
				// То переход к правому пункту (если это возможно)
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
			else if (code == 'a' || code == 'A' || code == (unsigned char)'ф' || code == (unsigned char)'Ф') // Если это стрелка влево
			{
				// То переход к левому пункту (если это возможно)
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
				//printf("А", field_letters[i][j]);

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
					if (code >= (unsigned char)'А' && code <= (unsigned char)'Я') {
						field_letters[column_active_idx][line_active_idx] = code;
					}
					else if (code >= (unsigned char)'а' && code <= (unsigned char)'я') {
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
			else if (code == KEY_ESC) // ESC - выход
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

			pause(40); // Небольная пауза (чтобы не загружать процессор)
		} // while (!key_is_pressed())


		// "Съедается" оставшийся ввод
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
		debug();
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
		// Заблокировать отрисовку
		con_draw_lock();

		// Очистка экрана
		con_set_color(clr_font, clr_bg);
		clrscr();
		// Цикл отрисовывает кнопку
		for (i = 0; i < 5; i++)
		{
			for (j = 0; j < 5; j++) {
				corners.left = x_coord_field + j * 9;
				corners.top = y_coord_field + i * 5;
				btn_bg = clr_bg; // По умолчанию фон кнопки - как фон экрана
				if (i == column_active_idx && j == line_active_idx)
					btn_bg = clr_bg_chosen; // Если кнопка активна - то рисуется другим цветом
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
				//printf("А", field_letters[i][j]);

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
		// Данные подготовлены, вывести на экран
		con_draw_release();
		while (!key_is_pressed()) // Если пользователь нажимает кнопку
		{
			int code = key_pressed_code();
			if (code == 'w' || code == 'W' || code == (unsigned char)'ц' || code == (unsigned char)'Ц') // Если это стрелка вверх
			{
				if (word_length > 0) {
					if (column_active_idx > 0)
					{
						for (n = 0; n < word_length; n++) {
							if (column_active_idx - 1 == field_word[n][0] && line_active_idx == field_word[n][1]) flag = 1; //прошли уже эту клетку?
						}
						if (field_letters[column_active_idx - 1][line_active_idx] != '\0') {
							//Добавляем букву
							if (flag != 1) {
								column_active_idx--;
								field_word[word_length][0] = column_active_idx;
								field_word[word_length][1] = line_active_idx;
								word_length++;
							}
							//Оформляем возврат
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
			else if (code == 's' || code == 'S' || code == (unsigned char)'ы' || code == (unsigned char)'Ы') // Если стрелка вниз
			{

				if (word_length > 0) {
					if (column_active_idx < 4)
					{
						for (n = 0; n < word_length; n++) {
							if (column_active_idx + 1 == field_word[n][0] && line_active_idx == field_word[n][1]) flag = 1;//прошли уже эту клетку?
						}
						if (field_letters[column_active_idx + 1][line_active_idx] != '\0') {
							//Добавляем букву
							if (flag != 1) {
								column_active_idx++;
								field_word[word_length][0] = column_active_idx;
								field_word[word_length][1] = line_active_idx;
								word_length++;
							}
							//Оформляем возврат
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
			else if (code == 'd' || code == 'D' || code == (unsigned char)'в' || code == (unsigned char)'В') // Если стрелка вправо
			{
				if (word_length > 0) {
					if (line_active_idx < 4)
					{
						for (n = 0; n < word_length; n++) {
							if (line_active_idx + 1 == field_word[n][1] && column_active_idx == field_word[n][0]) flag = 1;//прошли уже эту клетку?
						}
						if (field_letters[column_active_idx][line_active_idx + 1] != '\0') {
							//Добавляем букву
							if (flag != 1) {
								line_active_idx++;
								field_word[word_length][0] = column_active_idx;
								field_word[word_length][1] = line_active_idx;
								word_length++;
							}
							//Оформляем возврат
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
			else if (code == 'a' || code == 'A' || code == (unsigned char)'ф' || code == (unsigned char)'Ф') // Если это стрелка влево
			{
				if (word_length > 0) {
					if (line_active_idx > 0)
					{
						for (n = 0; n < word_length; n++) {
							if (line_active_idx - 1 == field_word[n][1] && column_active_idx == field_word[n][0]) flag = 1;//прошли уже эту клетку?
						}
						if (field_letters[column_active_idx][line_active_idx - 1] != '\0') {
							//Добавляем букву
							if (flag != 1) {
								line_active_idx--;
								field_word[word_length][0] = column_active_idx;
								field_word[word_length][1] = line_active_idx;
								word_length++;
							}
							//Оформляем возврат
							else {
								if (word_length > 1 && field_word[word_length - 2][1] == line_active_idx - 1) {
									line_active_idx--;
									word_length--;
									memset(field_word[word_length], 6, 2 * sizeof(char));
								}
							}
						}
					}
					//добавляем букву в слово
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
					//Проверка: входит ли поставленная буква в выделенное слово
					flag = 0;
					for (n = 0; n < word_length; n++) {
						if (column_letter_idx == field_word[n][0] && line_letter_idx == field_word[n][1]) flag = 1;
					}
					if (flag == 0)
					{
						corners.left = x_coord_field;
						corners.top = y_coord_field;
						// Заблокировать отрисовку
						con_draw_lock();

						// Очистка экрана
						con_set_color(clr_font, clr_bg);
						clrscr();
						// Цикл отрисовывает кнопку
						for (i = 0; i < 5; i++)
						{
							for (j = 0; j < 5; j++) {
								corners.left = x_coord_field + j * 9;
								corners.top = y_coord_field + i * 5;
								btn_bg = clr_bg; // По умолчанию фон кнопки - как фон экрана
								if (i == column_letter_idx && j == line_letter_idx)
									btn_bg = clr_bg_warning; // Если это ячейка с не попавшей в слово буквой, то подсвечиваем жёлтым

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
								//printf("А", field_letters[i][j]);

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
						// Данные подготовлены, вывести на экран
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
						//Проверка на наличие слова в словаре
						FILE* file = fopen(DICT, "r+");
						unsigned char str[MAX_WORD_LEN];
						int flag_compare = 0;
						while (!feof(file))
						{
							fgets(str, 30, file);
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
			else if (code == KEY_ESC) // ESC - выход
			{
				return 1;
			}

			pause(40); // Небольная пауза (чтобы не загружать процессор)
		} // while (!key_is_pressed())


		// "Съедается" оставшийся ввод
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
	printf("%s", "Игрок 1   Игрок 2");
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
			printf("%s", "Игрок 1 победил!");
		}
		else {
			gotoxy(corners.left + 4, corners.top);
			printf("%s", "Игрок 2 победил!");
		}
	}
	if (is_sur == 0) {
		if (score.first_player > score.second_player) {
			gotoxy(corners.left + 1, corners.top);
			printf("%s", "Игрок 1 победил!");
		}
		else if (score.first_player < score.second_player) {
			gotoxy(corners.left + 4, corners.top);
			printf("%s", "Игрок 2 победил!");
		}
		else {
			gotoxy(corners.left + 9, corners.top);
			printf("%s", "Ничья");
		}
	}
	corners.top++;
	gotoxy(corners.left, corners.top);
	printf("%s", "Игрок 1         Игрок 2");
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
		printf("        Игрок 1                 Игрок 2");
	else
		printf("        Игрок 2                 Игрок 1");
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
	const char* menu_items[] = { "Настройки" ,"Сложность", "Режим", "Первый ход", "Назад или ESC" };
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

		// Заблокировать отрисовку
		con_draw_lock();

		// Очистка экрана
		con_set_color(clr_font, clr_bg);
		clrscr();
		// Цикл отрисовывает кнопку
		for (b = 0; b < menu_items_count; b++)
		{
			short btn_bg = clr_bg; // По умолчанию фон кнопки - как фон экрана
			if (b == menu_active_idx)
				btn_bg = clr_bg_active; // Если кнопка активна - то рисуется другим цветом

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

		// Данные подготовлены, вывести на экран
		con_draw_release();


		while (!key_is_pressed()) // Если пользователь нажимает кнопку
		{
			int code = key_pressed_code();

			if (code == 'w' || code == 'W' || code == (unsigned char)'ц' || code == (unsigned char)'Ц') // Если это стрелка вверх
			{
				// То переход к верхнему пункту (если это возможно)
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
			else if (code == 's' || code == 'S' || code == (unsigned char)'ы' || code == (unsigned char)'Ы') // Если стрелка вниз
			{
				// То переход к нижнему пункту (если это возможно)
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
				code == (unsigned char)'й' || code == (unsigned char)'Й') // ESC или 'q' - выход
			{
				return;
			}
			else if (code == KEY_ENTER) // Нажата кнопка Enter
			{
				if (menu_active_idx == 4) // Выбран последний пункт - это выход
					return;
				if (menu_active_idx == 2) {
					mode_selection();
				}

				if (menu_active_idx == 1)//Выбран пункт сложность
					difficulty_selection();

				if (menu_active_idx == 3)//Выбран пункт Первый ход
					first_turn_selection();

				break;
			}


			pause(40); // Небольная пауза (чтобы не загружать процессор)
		} // while (!key_is_pressed())


		// "Съедается" оставшийся ввод
		while (key_is_pressed())
			key_pressed_code();

	} // while(1)
}

int surrender_window() {
	const char* menu_items[] = { "Да", "Нет" };
	int menu_active_idx = 1;
	int menu_items_count = sizeof(menu_items) / sizeof(menu_items[0]);
	while (1) {
		corners.left = x_coord_menu;
		corners.top = y_coord_field;
		int b;
		int code;

		// Заблокировать отрисовку
		con_draw_lock();

		// Очистка экрана
		con_set_color(clr_font, clr_bg);
		clrscr();
		// Цикл отрисовывает кнопку
		short btn_bg = clr_bg;
		gotoxy(corners.left, corners.top);
		printf("Вы уверены, что хотите сдаться?");
		corners.top += 2;
		for (b = 0; b < 2; b++)
		{
			btn_bg = clr_bg; // По умолчанию фон кнопки - как фон экрана
			if (b == menu_active_idx)
				btn_bg = clr_bg_active; // Если кнопка активна - то рисуется другим цветом
			gotoxy(corners.left, corners.top);
			con_set_color(clr_font, btn_bg);
			printf("%s", menu_items[b]);
			corners.left += 3;
		}
		con_draw_release();

		while (!key_is_pressed()) // Если пользователь нажимает кнопку
		{
			code = key_pressed_code();
			if (code == 'a' || code == 'A' || code == (unsigned char)'ф' || code == (unsigned char)'Ф')
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
			else if (code == 'd' || code == 'D' || code == (unsigned char)'в' || code == (unsigned char)'В')
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
				code == (unsigned char)'й' || code == (unsigned char)'Й') // ESC или 'q' - выход
			{
				return 0;
			}
			else if (code == KEY_ENTER) // Нажата кнопка Enter
			{
				if (menu_active_idx == 1) // Выбран первый пункт - это выход
					return 0;

				if (menu_active_idx == 0)// Выбран второй пункт - это выход
					return 1;
			}


			pause(40); // Небольная пауза (чтобы не загружать процессор)
		} // while (!key_is_pressed())


		// "Съедается" оставшийся ввод
		while (key_is_pressed())
			key_pressed_code();

	}
}

int pass_turn_window() {
	const char* menu_items[] = { "Да", "Нет" };
	int menu_active_idx = 1;
	int menu_items_count = sizeof(menu_items) / sizeof(menu_items[0]);
	while (1) {
		corners.left = x_coord_menu;
		corners.top = y_coord_field;
		int b;
		int code;

		// Заблокировать отрисовку
		con_draw_lock();

		// Очистка экрана
		con_set_color(clr_font, clr_bg);
		clrscr();
		// Цикл отрисовывает кнопку
		short btn_bg = clr_bg;
		gotoxy(corners.left, corners.top);
		printf("Вы уверены, что хотите пропустить ход?");
		corners.top += 2;
		for (b = 0; b < 2; b++)
		{
			btn_bg = clr_bg; // По умолчанию фон кнопки - как фон экрана
			if (b == menu_active_idx)
				btn_bg = clr_bg_active; // Если кнопка активна - то рисуется другим цветом
			gotoxy(corners.left, corners.top);
			con_set_color(clr_font, btn_bg);
			printf("%s", menu_items[b]);
			corners.left += 3;
		}
		con_draw_release();

		while (!key_is_pressed()) // Если пользователь нажимает кнопку
		{
			code = key_pressed_code();
			if (code == 'a' || code == 'A' || code == (unsigned char)'ф' || code == (unsigned char)'Ф')
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
			else if (code == 'd' || code == 'D' || code == (unsigned char)'в' || code == (unsigned char)'В')
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
				code == (unsigned char)'й' || code == (unsigned char)'Й') // ESC или 'q' - выход
			{
				return 0;
			}
			else if (code == KEY_ENTER) // Нажата кнопка Enter
			{
				if (menu_active_idx == 1) // Выбран первый пункт - это пропуск хода
					return 0;

				if (menu_active_idx == 0)
					return 1;
			}


			pause(40); // Небольная пауза (чтобы не загружать процессор)
		} // while (!key_is_pressed())


		// "Съедается" оставшийся ввод
		while (key_is_pressed())
			key_pressed_code();

	}
}

void difficulty_selection()
{
	char* menu_items[] = { "Сложность", "Лёгкая", "Средняя", "Сложная" ,"Назад или ESC" };
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

		// Заблокировать отрисовку
		con_draw_lock();

		// Очистка экрана
		con_set_color(clr_font, clr_bg);
		clrscr();
		// Цикл отрисовывает кнопку
		for (b = 0; b < menu_items_count; b++)
		{
			short btn_bg = clr_bg; // По умолчанию фон кнопки - как фон экрана

			//if (b == difficult)
			//	btn_bg = clr_bg_chosen;//Кнопка не активна и это текущая сложность
			if (b == menu_active_idx)
				btn_bg = clr_bg_active; // Если кнопка активна - то рисуется другим цветом
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

		// Данные подготовлены, вывести на экран
		con_draw_release();


		while (!key_is_pressed()) // Если пользователь нажимает кнопку
		{
			int code = key_pressed_code();
			if (code == 'w' || code == 'W' || code == (unsigned char)'ц' || code == (unsigned char)'Ц') // Если это стрелка вверх
			{
				// То переход к верхнему пункту (если это возможно)
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
			else if (code == 's' || code == 'S' || code == (unsigned char)'ы' || code == (unsigned char)'Ы') // Если стрелка вниз
			{
				// То переход к нижнему пункту (если это возможно)
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
				code == (unsigned char)'й' || code == (unsigned char)'Й') // ESC или 'q' - выход
			{
				return;
			}
			else if (code == KEY_ENTER) // Нажата кнопка Enter
			{
				if (menu_active_idx == 4) // Выбран последний пункт - это выход
					return;

				if (menu_active_idx == EASY)
					difficult = EASY;

				if (menu_active_idx == MEDIUM)
					difficult = MEDIUM;

				if (menu_active_idx == HARD)
					difficult = HARD;
				break;
			}


			pause(40); // Небольная пауза (чтобы не загружать процессор)
		} // while (!key_is_pressed())


		// "Съедается" оставшийся ввод
		while (key_is_pressed())
			key_pressed_code();

	} // while(1)
}

void first_turn_selection() {
	const char* menu_items[] = { "Право первого хода" ,"Игрок 1", "Игрок 2" ,"Назад или ESC" };
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

		// Заблокировать отрисовку
		con_draw_lock();

		// Очистка экрана
		con_set_color(clr_font, clr_bg);
		clrscr();
		// Цикл отрисовывает кнопку
		for (b = 0; b < menu_items_count; b++)
		{
			short btn_bg = clr_bg; // По умолчанию фон кнопки - как фон экрана
			if (b == menu_active_idx)
				btn_bg = clr_bg_active; // Если кнопка активна - то рисуется другим цветом

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

		// Данные подготовлены, вывести на экран
		con_draw_release();


		while (!key_is_pressed()) // Если пользователь нажимает кнопку
		{
			int code = key_pressed_code();
			if (code == 'w' || code == 'W' || code == (unsigned char)'ц' || code == (unsigned char)'Ц') // Если это стрелка вверх
			{
				// То переход к верхнему пункту (если это возможно)
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
			else if (code == 's' || code == 'S' || code == (unsigned char)'ы' || code == (unsigned char)'Ы') // Если стрелка вниз
			{
				// То переход к нижнему пункту (если это возможно)
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
				code == (unsigned char)'й' || code == (unsigned char)'Й') // ESC или 'q' - выход
			{
				return;
			}
			else if (code == KEY_ENTER) // Нажата кнопка Enter
			{
				if (menu_active_idx == 3) // Выбран последний пункт - это выход
					return;

				if (menu_active_idx == 1)//Человек ходит первым
					start_turn = 1;

				if (menu_active_idx == 2)//Компьютер ходит первым
					start_turn = 2;

				break;
			}


			pause(40); // Небольная пауза (чтобы не загружать процессор)
		} // while (!key_is_pressed())


		// "Съедается" оставшийся ввод
		while (key_is_pressed())
			key_pressed_code();

	} // while(1)
}

void about() {
	con_set_color(CON_CLR_WHITE_LIGHT, CON_CLR_BLACK);
	clrscr();

	gotoxy(8, 2);
	printf("О программе:");

	con_set_color(CON_CLR_GRAY, CON_CLR_BLACK);
	gotoxy(8, 3);
	printf("Очередная реализованная на языке Си игра балда)\n\n");

	gotoxy(8, 4);
	printf("Для продолжения нажмите любую клавишу.");

	key_pressed_code();
	return;
}

int ask_for_save() {
	const char* menu_items[] = { "Да", "Нет" };
	int menu_active_idx = 1;
	int menu_items_count = sizeof(menu_items) / sizeof(menu_items[0]);
	while (1) {
		corners.left = x_coord_menu;
		corners.top = y_coord_field;
		int b;
		int code;

		// Заблокировать отрисовку
		con_draw_lock();

		// Очистка экрана
		con_set_color(clr_font, clr_bg);
		clrscr();
		// Цикл отрисовывает кнопку
		short btn_bg = clr_bg;
		gotoxy(corners.left, corners.top);
		printf("Хотите сохранить прогресс и выйти?");
		corners.top += 2;
		for (b = 0; b < 2; b++)
		{
			btn_bg = clr_bg; // По умолчанию фон кнопки - как фон экрана
			if (b == menu_active_idx)
				btn_bg = clr_bg_active; // Если кнопка активна - то рисуется другим цветом
			gotoxy(corners.left, corners.top);
			con_set_color(clr_font, btn_bg);
			printf("%s", menu_items[b]);
			corners.left += 3;
		}
		con_draw_release();

		while (!key_is_pressed()) // Если пользователь нажимает кнопку
		{
			code = key_pressed_code();
			if (code == 'a' || code == 'A' || code == (unsigned char)'ф' || code == (unsigned char)'Ф')
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
			else if (code == 'd' || code == 'D' || code == (unsigned char)'в' || code == (unsigned char)'В')
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
				code == (unsigned char)'й' || code == (unsigned char)'Й') // ESC или 'q' - выход
			{
				return 0;
			}
			else if (code == KEY_ENTER) // Нажата кнопка Enter
			{
				if (menu_active_idx == 1) // Выбран первый пункт - это выход
					return 0;

				if (menu_active_idx == 0)// Выбран второй пункт - это выход
					return 1;
			}


			pause(40); // Небольная пауза (чтобы не загружать процессор)
		} // while (!key_is_pressed())


		// "Съедается" оставшийся ввод
		while (key_is_pressed())
			key_pressed_code();

	}
}

int ask_for_load() {
	const char* menu_items[] = { "Да", "Нет" };
	int menu_active_idx = 1;
	int menu_items_count = sizeof(menu_items) / sizeof(menu_items[0]);
	while (1) {
		corners.left = x_coord_menu;
		corners.top = y_coord_field;
		int b;
		int code;

		// Заблокировать отрисовку
		con_draw_lock();

		// Очистка экрана
		con_set_color(clr_font, clr_bg);
		clrscr();
		// Цикл отрисовывает кнопку
		short btn_bg = clr_bg;
		gotoxy(corners.left, corners.top);
		printf("Хотите загрузить прошлую игру?");
		corners.top += 2;
		for (b = 0; b < 2; b++)
		{
			btn_bg = clr_bg; // По умолчанию фон кнопки - как фон экрана
			if (b == menu_active_idx)
				btn_bg = clr_bg_active; // Если кнопка активна - то рисуется другим цветом
			gotoxy(corners.left, corners.top);
			con_set_color(clr_font, btn_bg);
			printf("%s", menu_items[b]);
			corners.left += 3;
		}
		con_draw_release();

		while (!key_is_pressed()) // Если пользователь нажимает кнопку
		{
			code = key_pressed_code();
			if (code == 'a' || code == 'A' || code == (unsigned char)'ф' || code == (unsigned char)'Ф')
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
			else if (code == 'd' || code == 'D' || code == (unsigned char)'в' || code == (unsigned char)'В')
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
				code == (unsigned char)'й' || code == (unsigned char)'Й') // ESC или 'q' - выход
			{
				return 0;
			}
			else if (code == KEY_ENTER) // Нажата кнопка Enter
			{
				if (menu_active_idx == 1) // Выбран первый пункт - это выход
					return 0;

				if (menu_active_idx == 0)// Выбран второй пункт - это выход
					return 1;
			}


			pause(40); // Небольная пауза (чтобы не загружать процессор)
		} // while (!key_is_pressed())


		// "Съедается" оставшийся ввод
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
