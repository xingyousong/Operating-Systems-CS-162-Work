#include <stdio.h>
#include <stdlib.h>

struct counter{
	int lines;
	int words;
	int characters;

};

struct counter count_file(char *file_name){
	struct counter output_counter;

	FILE *f_p = fopen(file_name, "r");


	if (f_p == NULL){
		output_counter.lines = 0;
		output_counter.words = 0;
		output_counter.characters = 0;
		return output_counter;
	}

	output_counter.lines = 0;
	output_counter.words = 0;
	output_counter.characters = -1; // off by one management


	int temp_char = 0;
	int word_bool = 0;

	while(!feof(f_p)){

		temp_char = fgetc(f_p);
		output_counter.characters++; //add char

		// printf("%c\n", temp_char);

		if (temp_char == '\n'){
			output_counter.lines++; //add lines
		}

		
		if (word_bool == 1 && (temp_char == ' ' || temp_char == '\n' || temp_char == '\t')   ){
			word_bool = 0;
			output_counter.words++;
		}
		else if (word_bool == 0 && (temp_char != ' ' && temp_char != '\n' && temp_char != '\t')  ){
			word_bool = 1; 
		} // original word counter code

		// if (temp_char == ' '|| temp_char == '\n'){
		// 	output_counter.words++;
		// }




	}
	if (word_bool == 1){
		output_counter.words++;
	}
	return output_counter;

}


struct counter count_stdin(){
	struct counter output_counter;

	
	char temp_char = 0;
	/*if (input[0] == 0){
		output_counter.lines = 0;
		output_counter.words = 0;
		output_counter.characters = 0;
		return output_counter;
	}*/

	output_counter.lines = 0;
	output_counter.words = -1;
	output_counter.characters = -1; 


	int word_bool = 0;
	int i = 0;
	while(temp_char != EOF){

		temp_char = getchar();
		i++;
		output_counter.characters++; //add char

		// printf("%c\n", temp_char);

		if (temp_char == '\n'){
			output_counter.lines++; //add lines
		}

		
		if (word_bool == 1 && (temp_char == ' ' || temp_char == '\n' || temp_char == '\t' || temp_char == '\r' || temp_char == '\x00')   ){
			word_bool = 0;
			output_counter.words++;
		}
		else if (word_bool == 0 && (temp_char != ' ' && temp_char != '\n' && temp_char != '\t' && temp_char != '\r' && temp_char != '\x00')  ){
			word_bool = 1; 
		} // original word counter code

		// if (temp_char == ' '|| temp_char == '\n'){
		// 	output_counter.words++;
		// }




	}
	if (word_bool == 1){
		output_counter.words++;
	}
	return output_counter;

}



// test '234824093284 ' 423423 4 test
// test hard sllelelele ---------------------------- troll troll troll 
// adding new works sfwerwerwerwrwrewrw

int main(int argc, char *argv[]) {
	if (argv[1] == NULL){
		
		struct counter out = count_stdin();
		printf(" %d %d %d\n", out.lines, out.words, out.characters);
	} 
	else {
		struct counter out = count_file(argv[1]);
		printf(" %d %d %d %s\n", out.lines, out.words, out.characters, argv[1]);
	}
	//printf(" %d %d %d", out.lines, out.words, out.characters);
    return 0;
}
