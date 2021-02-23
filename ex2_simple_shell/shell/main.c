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
void execute_FullCommand(char* cmd, int last_fdout, bool topipe);
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
			/*break;*/
		}
		if(strcmp(line, "exit") == 0) {
			/*printf("\n");*/
			/*break; exit*/
			exit(0);
		}
		/*printf("Line: %s\n", line);*/
		int pid = fork();     /*fork child*/
		if(pid==0){           /*Child*/
			if(strncmp(line, "cd", 2) == 0){
				argc = build_args(line, argv); /*build program argument*/
				set_program_path(path, bin, argv[0]);
				/*argc = build_args(line, argv); build program argument*/
				if(chdir(argv[1]) != 0)
					printf("Error in cd arguments\n");
			}
			else{
				/*printf("Inside Else, Line: %s\n", line);*/
				/*handel_program(line);*/
				command(line);
				/*execve(path, argv, 0); //if failed process is not replaced*/
				/* then print error message*/
				/*fprintf(stderr, "Child process could not do execve\n");*/
		
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
	printf("This is the start. Line: %s\n", cmd);
	if(strchr(cmd, '<') != NULL){
		printf("< is found in this command\n");
		if(strchr(cmd,'>') != NULL){
			printf("> is found in this command\n");
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
			printf("> is not found in this command\n");
			prog = strtok(cmd, "<");
			prog[strlen(prog)-1] = '\0';
			fin = strtok(NULL, "<");
			fin = fin + 1;
			isin = 1;
		}
	}else{
		printf("< is not found in this command\n");
		if(strchr(cmd, '>') != NULL){
			printf("> is found in this command\n");
			prog = strtok(cmd, ">");
			prog[strlen(prog)-1] = '\0'; /*to remove the last space in the program*/
			fout = strtok(NULL, ">"); fout = fout + 1; /*to remove first space*/
			isout = true;
			/*printf("Output File: %s\n", fout);*/
			printf("Programff: %s\n", prog);
			printf("Output File: %s\n", fout);
			printf("Hi Hi Captain. I don't hear you\n");
		}else{
			printf("> is not found in this command\n");
			prog = cmd;
			printf("CMD: %s\n", cmd);
			printf("Programgg: %s\n", prog);
		}
	}
	if(isin){
		fdin = open(fin, O_RDONLY);
		if(fdin <0){
			perror(fin);
			exit(1);
		}
		dup2(fdin, 0); /*duplicate the stdin in fdin*/
	}
	if(isout){
		printf("Outputing to a file, named: %s\n", fout);
		fdout = open(fout, O_CREAT|O_TRUNC|O_WRONLY, 0644);
		if(fdout < 0){
			perror(fout);
			exit(1);
		}
		printf("Before outputing to file\n");
		dup2(fdout, 1);
		printf("this should be inside the file\n");
	}
	argc = build_args(prog, argv); /*build program argument*/
	set_program_path(path, bin, argv[0]);
	execve(path, argv, 0); /*if failed process is not replaced*/
	/* then print error message*/
	fprintf(stderr, "Child process could not do execve\n");
	printf("Hi Hi, I should be inside the file\n");

}

void execute_FullCommand(char* cmd, int last_fdout, bool topipe){
	/*getting the number of commands */
	printf("Execution Begins\n");
	int n = 0;
	for(int i = 0; i < strlen(cmd)-1; i+=1)
		if(cmd[i] == '|') n += 1;
	char** programList = (char**)malloc((n+1)*sizeof(char*));/*initialize the array of commands*/
	
	/*constructing the array of commands*/
	int i = 0;
	char* program = strtok(cmd, "|");
	programList[i] = program;
	printf("First Program: %s\n", program);
	while(program != NULL){
		char* program = strtok(NULL, "|");
		if(program == NULL) break;
		programList[i][strlen(programList[i])-1] = '\0'; //to remove the last space 
		program += 1; /*just to remove the first*/
		i += 1;
		programList[i] = program; /*save the command inside the array*/
	}

	/*executing commands*/
	char* current_command;
	if(n==0){ /*this means it's only one program*/
		if(topipe){
			if(dup2(last_fdout, 1) == -1){
				perror("Error in assigning command output\n");
				exit(1);
			}
			printf("Mapped Successfully\n");
		}
		else{
			printf("Not Mapped there is a problem\n");
		}
		handel_program(cmd);

	}else{
		for(int i = 0; i < n; i+=1){
			current_command = programList[i]; /*this is the command of the parent*/
			int fd[2];
			pipe(fd); /*create a pipe*/
			int pid = fork();
			if(pid == 0){ 
				dup2(fd[1], 1); /*mapping child output to pipe's output socket*/
				/*for the child input. It's automatically inheritied from parent according to documentation*/
				handel_program(current_command); /*execute the command*/
				exit(1); /*just to exit this child*/
			}else{//parent process
				dup2(fd[0], 0); /*mapping parent's input to pipe input socket*/
				close(fd[1]);
				/*then continue to the loop for another forking*/
			}
		}
		if(topipe){
			if(dup2(last_fdout, 1) == -1){
				perror("Cannot assign command output\n");
				exit(1);
			}
			printf("Mapped Successfully\n");
		}
		else{
			printf("Not Mapped there is a problem\n");
		}
		handel_program(programList[n]);/*for the last program*/
		exit(1);
	
	}

}
void command(char* cmd){
	if(strchr(cmd, '=') != NULL){ /*assignment condition*/
		printf("= Found inside the command. Command: %s\n", cmd);
		char* cmdptr = strchr(cmd, '$');
		char* reciever = strtok(cmd, "=");
		printf("Reciever before: %s\n", reciever);
		if(reciever[strlen(reciever)-1] == ' ') reciever[strlen(reciever)-1] = '\0'; /*to remove spaces*/
		printf("Reciever: %s\n", reciever);
		char* sender = strtok(NULL, "=");
		if(sender[0] == ' ') sender += 1; /*to remove spaces*/
		printf("Sender: %s\n", sender);
		printf("Command Again: %s\n", cmd);
		if(cmdptr != NULL){ /*assigning a var to another*/
			printf("$ Found inside the command\n");
			char* var = strtok(sender, "$"); /*var = strtok(NULL, "$"); to get the string after the dollar sign*/
			char* value = getenv(var);
			setenv(reciever, value, 1);
		}else if(strchr(sender, '\"') != NULL){ /*if assigning output of a command */
			printf("Command impeded\n");
			if(sender[strlen(sender)-1] == '\"') sender[strlen(sender)-1] ='\0';
			if(sender[0] == '\"') sender += 1;
			int fd[2];
			pipe(fd); /*create a pipe*/
			int pid = fork();
			if(pid == 0){ /*in the child */
				execute_FullCommand(sender, fd[1], true);
				exit(1); /*just to exit this child*/
			}else{/*parent process*/
				/*close(fd[1]);*/
				char* k;
				read(fd[0], k, 1024);
				printf("Value is %s\n", k);
				setenv(reciever, k, 1);
				/*then continue to the loop for another forking*/
			}

		}else{/*simple assignment */
			printf("Simple Assignment\n");
			setenv(reciever, sender, 1);
		}	
	}else if(strchr(cmd, '$') != NULL && strstr(cmd, "echo") != NULL){ /*printing a var*/
		printf("Printing Environmental Variable's value\n");
		char* var = strtok(cmd, "$"); var = strtok(NULL, "$");
		char* value = getenv(var);
		printf("%s\n", value);
	}else{
		execute_FullCommand(cmd, NULL, false);
	}
}
