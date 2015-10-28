#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

typedef struct _bst_node {
	char* word_data;
	struct _bst_node* left;
	struct _bst_node* right;
}bst_node;

typedef struct emp_node{
	struct emp_node* next_record;
	char* name;
	char* salary;
	char* address;
	char* email;
	char* phone;
}Employee;

typedef struct _word_stack{
	char* word;
	struct _word_stack* next_record;
}word_stack;

int count_words(char* raw, int raw_len);
int dict_to_array(char* dict_file, char*** _word_array);
int text_to_array(char* text_file, char*** _word_array);
bst_node* build_BST(char* dict_file);
bst_node* build_tree(char** word_list, int self, int right, int left);
void free_bst(bst_node* root);
int is_misspelled(char* curr, bst_node* spell_dict);
word_stack* push_misspell(char* curr, word_stack* spell);
int count_misspell(word_stack* spell);
void print_spell(word_stack* spell);
char** get_words(char* tok_raw, int number_words);
int is_name(char** word_list, int position, int word_count);
Employee* push_name(char** word_list, int position, Employee* top_stack);
Employee* get_emp(char* name_search, Employee* stack);
int is_address(char** word_list, int position, int end_of_list);
void push_address(char** word_list, int position, int number_words, Employee* curr_emp);
int is_phone(char* curr);
void push_phone(char* phone_number, Employee* curr_emp);
int is_salary(char* curr);
void push_salary(char* curr, Employee* curr_emp);
int is_email(char* curr);
void push_email(char* curr, Employee* curr_emp);
void free_word_list(char** word_list, int number_words);
void free_emp_list(Employee* stack);
void free_word_stack(word_stack* stack);
int case_cmp(const char* first, const char* second);
char to_lower(char c_lower);
void no_punc_cpy(char* dest, char* source);
void write_emp_data(char* out_file_name, Employee* stack, word_stack* spell);

int count_words (char* raw, int raw_len){
	int result = 1;
	//Uses a copy of the raw, so as to leave it untouched for the calling function
	//to use to pull individual words from.
	char* raw_copy = malloc(sizeof(char) * raw_len);
	strcpy(raw_copy, raw);
	char* current = strtok(raw_copy, " \t\n");
	while(current != NULL){
		current = strtok(NULL, " \t\n");
		result++;
	}
	
	free(raw_copy);
	return result;
}

char** get_words (char* tok_raw, int number_words){
	char** word_list = malloc((number_words + 1) * sizeof(char*));
	int i, word_len;
	char* current = strtok(tok_raw, " \t\n");
	i = 0;
	//Gets the individual words from the raw array. Ignores whitespace.
	while(current != NULL){
		word_len = (strlen(current) + 1);
		*(word_list + i) = malloc(sizeof(char) * word_len);
		strcpy(*(word_list + i), current);
		current = strtok(NULL, " \t\n");
		i++;
	}
	return word_list;
}

//Returns the size of the array. Returns -1 on malloc fail.
//uses ***_word_array to pass back new word array to calling function.
int dict_to_array (char* dict_file, char*** _word_array){
	FILE* dict = fopen (dict_file, "r");
	if (dict == NULL) return -1;
	int number_words = 0;
	int word_size, i;
	char dummy[100];
	fpos_t last_pos, current_pos;
	fgetpos (dict, &last_pos);
	//Count number of words in the file.
	for (; fgets(dummy, 100, dict) != NULL; number_words++)
		;
	rewind (dict);
	
	//word_array is the working array, _word_array is the pointer to the array
	//that build_BST is using.
	char** word_array = malloc (sizeof(char*) * number_words);
	if(word_array == NULL) return -1;
	*_word_array = word_array;
	//Copies every word into an array
	for (i = 0; i < number_words - 1; i++){
		fgets(dummy, 100, dict);
		fgetpos(dict, &current_pos);
		word_size = (int) current_pos - (int) last_pos;
		word_array[i] = malloc (sizeof(char) * word_size);
		strcpy(word_array[i], dummy);
		
		//strcpy doesn't add null terminator.
		*(word_array[i] + word_size - 1) = '\0';
		last_pos = current_pos;
	}
	//Handles last word edge case: last word may not have any trailing white space.
	//With no trailing white space, last word* is one too short. The other option
	//is oversizing every other char* by one.
	fgets(dummy, 100, dict);
	fgetpos(dict, &current_pos);
	word_size = (int) current_pos - (int) last_pos;
	word_array[i] = malloc (sizeof(char) * ++word_size);
	strcpy(word_array[i], dummy);
	
	fclose (dict);
	return number_words;
}

//Returns the size of the array. Returns -1 on malloc fail.
//uses ***_word_array to pass back new text array to calling function.
int text_to_array (char* text_file, char*** _word_array){
	FILE* in_text = fopen (text_file, "r");
	if (in_text == NULL) return -1;
	
	//Get raw length of file, in chars.
	fseek(in_text, 0, SEEK_END);
	fpos_t end;
	fgetpos(in_text, &end);
	int number_chars = (int) end;
	fseek(in_text, 0, SEEK_SET);
	char* raw_data = malloc(sizeof(char) * (number_chars + 1));
	fread(raw_data, sizeof(char), number_chars, in_text);	
	
	int word_count = count_words(raw_data, (number_chars + 1));
	*(_word_array) = get_words(raw_data, word_count);
	fclose(in_text);
	free(raw_data);
	return word_count-1;
}

bst_node* build_BST (char* dict_file){
	char** word_array;
	int number_words;
	number_words = dict_to_array(dict_file, &word_array);
	if (number_words == -1) return (bst_node*) NULL;

	int i;
	
	//word_array is assumed to be properly sorted. This being the case,
	//recursively build a BST.
	int middle = number_words/2;
	bst_node* dict_root = build_tree (word_array, middle, 0, number_words-1);
	
	//Free the array allocated by dict_to_array.
	for (i = 0; i < number_words; i++){
		free(word_array[i]);
	}
	free(word_array);
	return dict_root;
}

bst_node* build_tree (char** word_list, int self, int left, int right){
	
	int word_len = strlen(word_list[self]);
	//Fancy tree math (wave hands)
	int next_left = left + ((self - left) / 2);
	int next_right = right - ((right - self) / 2);
	if (self - left == 1) next_left = self - 1;
	if (right - self == 1) next_right = self + 1;
	
	//Make new node for self, copy word into word_data.
	bst_node* new_node = malloc (sizeof(bst_node));
	char* word_data = malloc (sizeof(char) * (word_len + 1));
	if (word_data != NULL){
		strcpy (word_data, word_list[self]);
		new_node->word_data = word_data;
	}else printf ("\nProblems with memory.");
	
	//Build sub_trees
	if (self != left) new_node->left = build_tree (word_list, next_left, left, self-1);
	else new_node->left = NULL;
	if (self != right) new_node->right = build_tree (word_list, next_right, self+1, right);
	else new_node->right = NULL;
	return new_node;
}

void free_bst (bst_node* root){
	if (root == NULL) return;
	free_bst(root->left);
	free_bst(root->right);
	free(root->word_data);
	free(root);
}

int is_misspelled (char* curr, bst_node* root){
	
	//Whitespace is never misspelled, whitespace never precedes a word in this program.
	//This of course assumes Leprechauns aren't at work. Which is silly. Leprechauns
	//are always on the clock. Whitespace removed by word_list generator.
	if (isspace(*curr) || *curr == '\0') return 0;
	//case_cmp is used because a word may be capitalized in the text being checked.
	int cmp_result = case_cmp(curr, root->word_data);
	if (cmp_result == 0) return 0;
	if (cmp_result < 0 && root->left != NULL) return is_misspelled (curr, root->left);
	if (cmp_result > 0 && root->right != NULL) return is_misspelled (curr, root->right);
	return 1;
}

word_stack* push_misspell (char* curr, word_stack* spell){
	//If stack is NULL, malloc new node.
	if(spell == NULL){
		word_stack* new_node = malloc(sizeof(word_stack*));
		if (new_node == NULL){ 
			printf("\nAllocation problems in push_misspell.\n");
			return (word_stack*) NULL;
		}
		int word_len = strlen(curr);
		new_node->word = malloc(sizeof(char) * word_len);
		if (new_node->word == NULL){ 
			printf("\nAllocation problems in push_misspell.\n");
			return (word_stack*) NULL;
		}
		//Don't copy over punctuation.
		no_punc_cpy(new_node->word, curr);
		//Initialize next_record to NULL.
		new_node->next_record = (word_stack*) NULL;
		return new_node;
	}
	
	//If the next record is NULL, you're going to add a new node to the stack.
	//Then return this new node.
	if(spell->next_record == NULL){
		spell->next_record = push_misspell (curr, spell->next_record);
		return spell->next_record;
	}
	//If you try to access in the middle of a stack, it will always return
	//the new last record in the stack.
	return push_misspell (curr, spell->next_record);
}

int count_misspell (word_stack* spell){
	if (spell == NULL) return 0;
	return (count_misspell(spell->next_record) + 1);
}

void print_spell (word_stack* spell){
	if (spell == NULL) return;
	printf ("%s\n", spell->word);
	print_spell (spell->next_record);
}

int is_name (char** word_list, int position, int word_count){
	//Make sure you're not on the last word.
	if (position + 1 == word_count) return 0;
	static char buffer[100];
	char *first, *second;
	char first_letter;
	int yes_name = 1, i;
	
	//Get pointers to the words to check, remove any punctuation with no_punc_cpy.
	first = *(word_list + position);
	second = *(word_list + position + 1);
	no_punc_cpy(buffer, first);
	//Check if first word starts with majuscule.
	first_letter = *(buffer);
	if (first_letter >= 'A' && first_letter <= 'Z'){
		
		//Check each other letter in the word to be a miniscule
		for (i = 1; *(buffer + i) != '\0'; i++){
			if(*(buffer + i) < 'a' || *(buffer + i) > 'z'){
				return 0;
				break;
			}
		}
	}else{
		//Not a name if it doesn't start with a majuscule.
		return 0;
	}
	
	no_punc_cpy(buffer, second);
	//Check if second word starts with majuscule, and first word
	first_letter = *(buffer);	
	if (first_letter >= 'A' && first_letter <= 'Z'){
		
		//Check each other letter in the word to be a miniscule
		for (i = 1; *(buffer + i) != '\0'; i++){
			if (*(second + i) < 'a' || *(buffer + i) > 'z'){
				return 0;
				break;
			}
		}
	}else{
		//Again, not a name if it doesn't start with a majuscule. Like Neil deGrasse 
		//Tyson. deGrasse ain't no name.
		return 0;
	}
	
	return yes_name;
}

Employee* push_name (char** word_list, int position, Employee* top_stack){
	static char buffer[100], bufferf[50], bufferl[50];
	int i, j;
	
	//Get no_punc_copies into bufferf and bufferl.
	no_punc_cpy(bufferf, word_list[position]);
	no_punc_cpy(bufferl, word_list[position + 1]);
	
	//Copy those names into buffer. Make a space in the middle.
	for (i = 0; bufferf[i] != '\0'; i++) buffer[i] = bufferf[i];
	buffer[i++] = ' ';
	for (j = 0; bufferl[j] != '\0'; j++) buffer[i + j] = bufferl[j];
	buffer[i+j] = '\0';
	
	//get_emp returns an emp with that name or a new node for a new name.
	Employee* ret_record = get_emp(buffer, top_stack);
	
	//ret_record used by main to know the current emp. This is used for pushing
	//other information parsed from the file.
	return ret_record;
}

Employee* get_emp (char* search_name, Employee* stack){
	//If stack is NULL, malloc new node.
	if(stack == NULL){
		Employee* new_node = malloc(sizeof(Employee));
		if (new_node == NULL){ 
			printf("\nAllocation problems.\n");
			return (Employee*) NULL;
		}
		//Plus one to account for null term that strlen doesn't count.
		int name_len = strlen(search_name) + 1;
		new_node->name = malloc(sizeof(char) * name_len);
		strcpy(new_node->name, search_name);
		//Initialize all other data.
		new_node->next_record = (Employee*) NULL;
		
		new_node->phone = malloc(sizeof(char) * 14);
		new_node->address = malloc(sizeof(char) * 14);
		new_node->salary = malloc(sizeof(char) * 14);
		new_node->email = malloc(sizeof(char) * 14);
		if (new_node->phone == NULL || new_node->address == NULL || 
			new_node->salary == NULL || new_node->email == NULL){
				printf ("\nProblems allocating memory in get_emp\n");
				return (Employee*) NULL;
		}
		strcpy(new_node->phone, "Not available");
		strcpy(new_node->address, "Not available");
		strcpy(new_node->salary, "Not available");
		strcpy(new_node->email, "Not available");
		
		return new_node;
	}
	//strcmp returns 0 for match. Return current node if match.
	if (!strcmp(stack->name, search_name)){ 
		return stack;
	}
	
	//If the next record is NULL, you're going add a new node to the stack.
	//Set the current node next_record to this new node.
	if (stack->next_record == NULL){
		stack->next_record = get_emp (search_name, NULL);
		return stack->next_record;
	}
	return get_emp (search_name, stack->next_record);
}

//Returns 0 for not address. Returns number of words in address.
int is_address (char** word_list, int position, int end_of_list){
	//atoi(char*) returns 0 if not an int.
	//atof(char*) returns 0.0 if not a float.
	int number_words, end_value;
	char* start = word_list[position];
	
	//Checks if word is an int, checks to make sure it's not a float.
	//Also checks that it's not a phone number.
	if (atoi(start) > 0 && ((float) atoi(start) == atof(start)) && start[3] != '-'){
		//Iterates through word list, looking for 5 digit int.
		for (number_words = position + 1; number_words < end_of_list; number_words++){
			end_value = atoi(word_list[number_words]);
			if (end_value > 0 && end_value < 100000){
				if ((float) end_value == atof(word_list[number_words])){
					return (number_words - position + 1);
				}
			}
		}
	}
	
	//Default is return false.
	return 0;
}

void push_address (char** word_list, int position, int number_words, Employee* curr_emp){
	int i;
	static char buffer[100];
	static char zip_buff[6];
	buffer[0] = '\0';
	
	for (i = 0; i < number_words; i++){
		//If it is the last word, only copy 6 chars. This removes any trailing
		// punctuation from the zip code.
		if (number_words-1 == i){
			strncpy (zip_buff, word_list[position+i], 5);
			zip_buff[5] = '\0';
			strcat (buffer, zip_buff);
		}else{
			strcat (buffer, word_list[position + i]);
			strcat (buffer, " ");
		}
	}
	//If address data already present, replace.
	if (curr_emp->address != NULL) free(curr_emp->address);
	//Allocate memory for address string, copy over.
	int add_len = strlen(buffer);
	curr_emp->address = malloc (sizeof(char) * (add_len + 1));
	if (curr_emp->address == NULL){
		printf("\nMemory problems in push_address.\n");
		return;
	}
	strcpy (curr_emp->address, buffer);
}

//Returns 0 for not a phone number, 1 for is a phone number.
int is_phone (char* curr){
	int len = strlen(curr);
	int i;

	//Phone number must be 12 chars, not including null term. 13 characters
	//is allowed if there is trailing punctuation.
	if (len != 13 && len != 12) return 0;

	//See if the number positions are digits, and the dash positions are dashes.
	if (curr[3] == '-' && curr[7] == '-'){
		for(i = 0; i < 12; i++){
			
			//Skip dashes, check if current char is a digit.
			if (i == 3 || i == 7) i++;
			if (curr[i] < '0' || curr[i] > '9') return 0;
		}
	}else{
		return 0;
	}
	return 1;
}

void push_phone (char* phone_number, Employee* curr_emp){
	static char buffer[13];
	
	strncpy(buffer, phone_number, 12);
	buffer[12] = '\0';
	if (curr_emp->phone != NULL) free(curr_emp->phone);
	curr_emp->phone = malloc (sizeof(char) * 13);
	if (curr_emp->phone == NULL){
		printf ("\nProblems allocating memory in push_phone.\n");
		return;
	}
	strcpy(curr_emp->phone, buffer);
}

//Returns 0 for not salary. Returns position of last significant char if is_salary.
//Last significant char is two after the decimal place (24.45) <- 5 is last sig char.
int is_salary (char* curr){
	
	//Prevents malformed data like $430.2 from causing bad access.
	int word_len = strlen(curr);
	//First char has to be a '$'
	if (curr[0] != '$') return 0;
	
	//Second char has to be a digit
	if (curr[1] < '0' || curr[1] > '9') return 0;
	
	//Iterates to find the period. Stops before bad access
	int i;
	for (i = 2; curr[i] != '.' && i < word_len; i++);
	if (i + 3 > word_len) return 0;
	return i+2;
}

void push_salary (char* curr, Employee* curr_emp){
	int last_char = is_salary(curr);
	static char buffer[20];
	int i;
	
	for (i = 1; i <= last_char; i++){
		buffer[i-1] = curr[i];
	}
	buffer[last_char] = '\0';
	
	if (curr_emp->salary != NULL) free(curr_emp->salary);
	curr_emp->salary = malloc(sizeof(char) * (last_char + 1));
	if (curr_emp->salary == NULL){
		printf ("\nProblem allocating memory in push_salary.\n");
		return;
	}
	strcpy(curr_emp->salary, buffer);
}

//Returns 0 for not email, index of last char for is_email.
int is_email (char* curr){
	int curr_len = strlen(curr);
	int i, contains_at, contains_period, actual_end;
	contains_at = 0;
	contains_period = 0;
	
	//Email address must have alphanumeric at start.
	//If first NOT [a,z] || [A,Z] || [0,9] then return 0.
	char first = curr[0];
	if ( !( 
		(first >= 'a' && first <= 'z') || 
		(first >= 'A' && first <= 'Z') ||
		(first >= '0' && first <= '9')
		) )
	{
		return 0;
	}
	
	//If @ already found, look for period. If period found, set flag and break.
	//If period not found, continue.
	//If @ hasn't been found, look for at. If @ found, continue.
	for (i = 1; i < curr_len; i++){
		if (contains_at){
			if (curr[i] == '.'){
				//Must be at least one alpha after period.
				if (curr[i+1] >= 'a' && curr[i+1] <= 'z'){
					contains_period = 1;
				}
				break;
			}
			continue;
		}
		if (curr[i] == '@'){
			contains_at = 1;
			continue;
		}
	}
	
	//If @ wasn't found, or period wasn't found, return 0.
	if (!contains_at || !contains_period) return 0;
	
	//Trim trailing punctuation. Last char must be alpha. 
	actual_end = curr_len;
	while (curr[actual_end-1] < 'a' || curr[actual_end-1] > 'z'){
		actual_end--;
	}
	return actual_end;
}

void push_email (char* curr, Employee* curr_emp){
	int email_len = is_email(curr);
	static char buffer[50];
	strncpy(buffer, curr, email_len);
	buffer[email_len] = '\0';
	
	//Remove existing data.
	if (curr_emp->email != NULL) free(curr_emp->email);
	curr_emp->email = malloc (sizeof(char) * (email_len + 1));
	if (curr_emp->email == NULL){
		printf ("\nProblem allocating memory in push_email.\n");
		return;
	}
	strcpy(curr_emp->email, buffer);
}

void print_emp_list (Employee* stack){
	if(stack == NULL) return;
	printf ("\nEmployee: %s\n", stack->name);
	printf ("Salary: %s\n", stack->salary);
	printf ("Address: %s\n", stack->address);
	printf ("Email: %s\n", stack->email);
	printf ("Phone: %s\n", stack->phone);
	
	print_emp_list (stack->next_record);
}

void free_word_list (char** word_list, int number_words){
	int i;
	//Free the individual pointers first.
	for (i = 0; i < number_words; i++){
		free(*(word_list + i));
	}
	free(word_list);
}

void free_emp_stack (Employee* stack){
	if (stack == NULL) return;
	
	//Next record will be freed when stack is freed, so rec. call free_emp_stack first.
	free_emp_stack (stack->next_record);
	
	free(stack->name);
	free(stack->address);
	free(stack->email);
	free(stack->salary);
	free(stack->phone);
	free(stack);
}

void free_spell_stack (word_stack* stack){
	if (stack == NULL){
		return;
	}
	free_spell_stack (stack->next_record);
	free(stack->word);
	free(stack);
}

int case_cmp (const char* first, const char* second){
	int i = 0;
	
	//second comes from the dictionary, so it doesn't have any punctuation to remove.
	//first has any punctuation removed.
	char first_temp, second_temp;
	while (*(first + i) != '\0' && *(second + i) != '\0'){
		first_temp = to_lower(*(first + i));
		second_temp = to_lower(*(second + i));
		if (first_temp < second_temp) return -1;
		if (first_temp > second_temp) return 1;
		i++;
	}
	
	first_temp = to_lower(*(first + i));
	second_temp = to_lower(*(second + i));
	
	//If the next letter in first is a period or comma, truncate first.
	if (first_temp == '.' || first_temp == ',') first_temp = '\0';
	
	//Test substring partial match.
	//If first has ended, but second has not, first is "smaller" than second.
	//If second has ended, but first has not, second is "smaller" than first.
	if (first_temp == '\0' && second_temp != '\0') return -1;
	if (second_temp == '\0' && first_temp != '\0') return 1;
	
	//Otherwise they are a match
	return 0;
}

char to_lower (char c_lower){
	//Doesn't touch non-alpha chars.
	if (c_lower >= 'A' && c_lower <= 'Z') c_lower += 32;
	return c_lower;
}

void no_punc_cpy (char* dest, char* source){
	int i = 0;
	char current;
	
	if (dest == NULL) return;
	while(*(source + i) != '\0'){
		current = *(source + i);
		//Only copy alpha numerics. If it is NOT [a,z], [A,Z], [0,9] then truncate.
		if ( !(
				(current >= 'a' && current <= 'z') || 
				(current >='A' && current <= 'Z')  || 
				(current >= '0' && current <= '9') 
			) )
		{
				*(dest + i) = '\0';
				break;
		}else{
			*(dest + i) = *(source + i);
		}
		i++;
	}
	//Add null term at the end.
	*(dest + i) = '\0';
}

void write_data (char* out_file_name, Employee* stack, word_stack* spell){
	FILE* out_file = fopen(out_file_name, "w");
	if (out_file == NULL){
		printf ("\nCouldn't open file for writing.\n");
		return;
	}
	
	//Write emp_data first.
	while(stack != NULL){
		fprintf(out_file, "Employee: %s\n", stack->name);
		fprintf(out_file, "Salary: %s\n", stack->salary);
		fprintf(out_file, "Address: %s\n", stack->address);
		fprintf(out_file, "Email: %s\n", stack->email);
		fprintf(out_file, "Phone: %s\n\n", stack->phone);
		stack = stack->next_record;
	}
	
	//Then the misspells
	fprintf(out_file, "%i misspelled words:\n", count_misspell(spell));
	while(spell != NULL){
		fprintf(out_file, "%s\n", spell->word);
		spell = spell->next_record;
	}
	fclose(out_file);
}

int main (void){
	char** word_list;
	
	//Get the length of the email by word, point word_list to the array made by 
	//text_to_array.
	int word_count = text_to_array("input2.txt", &word_list);
	char buffer[100];
	int i, j;
	
	//Get the spelling dictionary.
	bst_node* spell_dict = build_BST ("words.txt");
	Employee* top_stack = (Employee*) NULL;
	Employee* curr_emp = (Employee*) NULL;
	word_stack* top_spell = (word_stack*) NULL;
	word_stack* curr_spell = (word_stack*) NULL;
	
	//Scan each word in the email. If it's a keyword, it will be pushed to proper
	// function.
	for(i = 0; i < word_count; i++){
		if (is_address(word_list, i, word_count)){
			int number_words = is_address(word_list, i, word_count);
			push_address (word_list, i, number_words, curr_emp);
			i += number_words - 1;
		}else if (is_phone(word_list[i])){
			push_phone(word_list[i], curr_emp);
		}else if (is_salary(word_list[i])){
			push_salary(word_list[i], curr_emp);
		}else if (is_email(word_list[i])){
			push_email(word_list[i], curr_emp);
		}else if (is_name(word_list, i, word_count)){
			if (top_stack == NULL){
				top_stack = push_name(word_list, i, NULL);
				curr_emp = top_stack;
			}else{
				curr_emp = push_name(word_list, i, top_stack);
			}
			i++;
		}else if (is_misspelled(word_list[i], spell_dict)){
			if (top_spell == NULL){
				top_spell = push_misspell(word_list[i], NULL);
				curr_spell = top_spell;
			}else{
				curr_spell = push_misspell(word_list[i], curr_spell);
			}
		}
	}
	
	print_emp_list(top_stack);
	write_data("data_output.txt", top_stack, top_spell);
	printf ("\n\n%i misspelled words: \n", count_misspell(top_spell));
	print_spell(top_spell);

	free_word_list(word_list, word_count);
	free_spell_stack(top_spell);
	free_emp_stack(top_stack);
	free_bst(spell_dict);
	return 1;
}