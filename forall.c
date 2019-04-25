#include<stdio.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<unistd.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
#include<signal.h>

int signalNum;
int tempstdout;

static void signalHandler(int sig, siginfo_t *si, void *ignore) {
	dup2(tempstdout, 1);
	if (sig == 2) {
		dprintf(1, "Signaling %d\n", getpid());
		signalNum = 1;
	}
	else if (sig == 3) {
		dprintf(1, "Exiting due to quit signal\n");
		kill(0, SIGTERM);
		signalNum = -1;
	}
	else {
		signalNum = 0;
	}
}

int main(int argc, char **argv) {
	char* pointer;
	char filename[FILENAME_MAX];
	int count = 1;
	int status;

	struct sigaction sa;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = signalHandler;
	//cataches sigint error
	if(sigaction(SIGINT, &sa, NULL) == -1) {
		perror("sigaction SIGINT");
		exit(errno);
	}
	//catches sigquit error
	if(sigaction(SIGQUIT, &sa, NULL) == -1) {
		perror("sigaction SIGQUIT");
		exit(errno);
	}

	for(int i = 2; i < argc; i++) {
		sprintf(filename, "%d.out", count); //https://www.geeksforgeeks.org/sprintf-in-c/
		pointer = filename; //sets it pointer to the file name
		tempstdout = dup(1);
		close(1);
		close(2);
		int filed = open(pointer, O_CREAT|O_TRUNC|O_WRONLY, S_IWUSR|S_IRUSR);
		dup2(filed, 1); //copies stdout
		//dup2(filed, 2); //copies stderr
		pid_t pid = fork();
		if(pid == 0) {
			sigaction(SIGINT, &sa, NULL);
			sigaction(SIGQUIT, &sa, NULL);
			execlp(argv[1], argv[1], argv[i], NULL);
			//handle exec error here:
			perror("Exec error");
			exit(errno);
		}

		else if(pid < 0) { //handles fork error
			perror("Fork failed\n");
		}

		else { //parent process
			dprintf(1, "Executing %s %s\n", argv[1], argv[i]);
			waitpid(pid, &status, 0);
			//print to file
			dup2(filed, 2);
			if(signalNum == 1) {

                                dprintf(2, "Stopped executing %s %s signal = %d\n", argv[1], argv[i], SIGINT);
                        }
			else if(signalNum == 0) {
				dprintf(2, "Finished executing %s %s exit code = %d\n", argv[1], argv[i], status);
			}

		}
		count += 1;
	}
	return 0;
}

