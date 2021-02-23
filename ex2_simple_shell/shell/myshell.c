#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

#define BUFFER_LEN 1024
#define WRITE_END 1
#define READ_END 0

size_t read_command(char* cmd);
int build_args(char* cmd, char** argv);
void set_program_path(char* path, char* bin, char* prog);
void handel_program(char* cmd);
void execute_FullCommand(char* cmd, char* filename);
void command(char* cmd);

int main(){
	char line[BUFFER_LEN]; /*get command line*/
	char* argv[100]; /*user command*/
	char* bin = "/bin/"; /*set path to bin*/
	char path[1024]; /*full file path*/
	char* homedir = getenv("HOME");
	int argc; /*arg count*/
	chdir(homedir); /*change current directory*/
	char* s[100]; /*used as a buffer for the get current directory*/
	while(1){
		printf("My shell$%s>> ", getcwd(s, 100));  /*print shell prompt*/
		if(read_command(line) == 0){
			printf("\n"); /*CRTL+D pressed*/
			exit(0);
		}
		if(strcmp(line, "exit") == 0) {
			exit(0);
		}
		/*printf("Line: %s\n", line);*/
		int pid = fork();     /*fork child*/
		if(pid==0){           /*Child*/
			if(strncmp(line, "cd", 2) == 0){
				argc = build_args(line, argv); /*build program argument*/
				set_program_path(path, bin, argv[0]);
				if(chdir(argv[1]) != 0)
					printf("Error in cd arguments\n");
			}
			else{
				command(line);
			}
		}
		else wait(NULL);    /*parent*/
	}
	return 0;
}

size_t read_command(char* cmd){
	if(!fgets(cmd, BUFFER_LEN, stdin)) /*get command and put it in line*/
		return 0;  /*if user hits CRTL+D break*/
	size_t length = strlen(cmd); /*get command length*/
	if(cmd[length-1] =='\n') cmd[length-1] = '\0';  /*clear new line*/
	return strlen(cmd); /*return length of the command read*/
}

int build_args(char* cmd, char** argv){
	char* token; /*split command into separate strings*/
	token = strtok(cmd, " ");
	int i = 0;
	while(token != NULL){  /*loop for all tokens*/
		argv[i] = token;  /*store token*/
		token = strtok(NULL, " "); /*get next token*/
		i++;  /*increment number of tokens*/
	}
	argv[i] = NULL; /*set last value to NULL for execvp*/
	return i;  /* return number of tokens*/
}
void set_program_path(char* path, char* bin, char* prog){
	memset(path, 0, 1024); /*initialize buffer*/
	strcpy(path, bin);  /*copy /bin/ to file path*/
	strcat(path, prog);  /*add program to path*/
	int i;
	for(i = 0; i < strlen(path); i++) /*delete newline*/
		if(path[i]=='\n') path[i]='\0';
}
void handel_program(char* cmd){
	char* prog = NULL, in = NULL, out = NULL;
	char* argv[100]; /*user command*/
	char* bin = "/bin/"; /*set path to bin*/
	char path[1024]; /*full file path*/
	int argc; /*arg count*/
	char* fout;
        char* fin;
	int fdout, fdin;
	bool isout = 0, isin = 0;
	if(strchr(cmd, '<') != NULL){
		if(strchr(cmd,'>') != NULL){
			prog = strtok(cmd, "<");
			prog[strlen(prog)-1] = '\0'; /* to remove unnecessary spaces*/
			char* temp = strtok(NULL, "<");
			fin = strtok(temp, ">"); 
			fin[strlen(fin)-1] = '\0'; /*to remove unnecessary spaces*/
			fin = fin +1; /*to remove unnessecary spaces*/
			fout = strtok(NULL, ">");
			fout = fout +1; /*to remove unnecessary spaces*/
			isin = 1;
			isout = 1;
		}else{
			prog = strtok(cmd, "<");
			prog[strlen(prog)-1] = '\0';
			fin = strtok(NULL, "<");
			fin = fin + 1;
			isin = 1;
		}
	}else{
		if(strchr(cmd, '>') != NULL){
			prog = strtok(cmd, ">");
			prog[strlen(prog)-1] = '\0'; /*to remove the last space in the program*/
			fout = strtok(NULL, ">"); fout = fout + 1; /*to remove first space*/
			isout = true;
		}else{
			prog = cmd;
		}
	}
	if(isin){
		fdin = open(fin, O_RDONLY);
		if(fdin <0){
			perror(fin);
			exit(1);
		}
		if(dup2(fdin, 0) == -1){/*duplicate the stdin in fdin*/
			perror("Error in redirecting file\n");
			exit(1);
		}
	}
	if(isout){
		fdout = open(fout, O_CREAT|O_TRUNC|O_WRONLY, 0644);
		if(fdout < 0){
			perror(fout);
			exit(1);
		}
		if(dup2(fdout, 1)==-1){
			perror("Error in redirecting file\n");
			exit(1);
		}
	}
	argc = build_args(prog, argv); /*build program argument*/
	set_program_path(path, bin, argv[0]);
	execve(path, argv, 0); /*if failed process is not replaced*/
	/* then print error message*/
	fprintf(stderr, "Child process could not do execve\n");
}

void execute_FullCommand(char* cmd, char* filename){
	/*getting the number of commands */
	int n = 0;
	int i;
	for(i = 0; i < strlen(cmd)-1; i+=1)
		if(cmd[i] == '|') n += 1;
	char** programList = (char**)malloc((n+1)*sizeof(char*));/*initialize the array of commands*/
	
	/*constructing the array of commands*/
	i = 0;
	char* program = strtok(cmd, "|");
	programList[i] = program;
	while(program != NULL){
		char* program = strtok(NULL, "|");
		if(program == NULL) break;
		programList[i][strlen(programList[i])-1] = '\0'; /*to remove the last space*/
		program += 1; /*just to remove the first*/
		i += 1;
		programList[i] = program; /*save the command inside the array*/
	}

	/*executing commands*/
	char* current_command;
	if(n==0){ /*this means it's only one program*/
		if(filename != NULL){	
			int fd_out = open(filename, O_CREAT|O_TRUNC|O_WRONLY, 0644);
			if(fd_out < 0){
				perror(filename);
				exit(1);
			}
			if(dup2(fd_out, 1) == -1){
				perror("Error in filename\n");
				exit(1);
			}
		}
		handel_program(cmd);

	}else{
		int i;
		for(i = 0; i < n; i+=1){
			current_command = programList[i]; /*this is the command of the parent*/
			int fd[2];
			pipe(fd); /*create a pipe*/
			int pid = fork();
			if(pid == 0){ 
				if(dup2(fd[1], 1)==-1){ /*mapping child output to pipe's output socket*/
				/*for the child input. It's automatically inheritied from parent according to documentation*/
					perror("Error in redirecting file descriptors\n");
					exit(1);
				}
				handel_program(current_command); /*execute the command*/
				exit(1); /*just to exit this child*/
			}else{/*parent process*/
				if(dup2(fd[0], 0) == -1){ /*mapping parent's input to pipe input socket*/
					perror("Error in redirecting this file descriptor\n");
					exit(1);
				}
				close(fd[1]);
				/*then continue to the loop for another forking*/
			}
		}
		if(filename != NULL){	
			int fd_out = open(filename, O_CREAT|O_TRUNC|O_WRONLY, 0644);
			if(fd_out < 0){
				perror(filename);
				exit(1);
			}
			if(dup2(fd_out, 1) == -1){
				perror("Error in filename\n");
				exit(1);
			}
		}
		handel_program(programList[n]);/*for the last program*/
		exit(1);
	
	}

}
void command(char* cmd){
	if(strchr(cmd, '=') != NULL){ /*assignment condition*/
		char* cmdptr = strchr(cmd, '$');
		char* reciever = strtok(cmd, "=");
		if(reciever[strlen(reciever)-1] == ' ') reciever[strlen(reciever)-1] = '\0'; /*to remove spaces*/
		char* sender = strtok(NULL, "=");
		if(sender[0] == ' ') sender += 1; /*to remove spaces*/
		if(cmdptr != NULL){ /*assigning a var to another*/
			char* var = strtok(sender, "$"); /*var = strtok(NULL, "$"); to get the string after the dollar sign*/
			char* value = getenv(var);
			setenv(reciever, value, 1);
		}else if(strchr(sender, '\"') != NULL){ /*if assigning output of a command */
			if(sender[strlen(sender)-1] == '\"') sender[strlen(sender)-1] ='\0';
			if(sender[0] == '\"') sender += 1;
			int processID = fork();
			char* fname = "temp__file__6785412563.txt";
			if(processID == 0){ /*child*/
				execute_FullCommand(sender, fname);
				exit(1);
			}else{ /*parent*/
				wait(NULL);
				char* cmd_output = 0;
				FILE* file = fopen(fname, "rb");
				if(file != NULL){
					/*get the length*/
					fseek(file, 0, SEEK_END);
					long len = ftell(file);
					fseek(file, 0, SEEK_SET);
					/*allocate space for this length*/
					cmd_output = malloc(len);
					fread(cmd_output, 1, len, file);
					fclose(file);
					remove(fname);
					setenv(reciever, cmd_output, 1);
					/*then continue to the loop for another forking*/
				}
			}	

		}else{/*simple assignment */
			setenv(reciever, sender, 1);
		}	
	}else if(strchr(cmd, '$') != NULL && strstr(cmd, "echo") != NULL){ /*printing a var*/
		char* var = strtok(cmd, "$"); var = strtok(NULL, "$");
		char* value = getenv(var);
		printf("%s\n", value);
	}else{
		execute_FullCommand(cmd, NULL);
	}
}
