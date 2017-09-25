#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/stat.h>
#include <syslog.h>
#include <string.h>
#define FILENAME "config.conf"
#define MAXBUF 1024 
#define DELIM "="

struct config
{
   char indexFilePath[MAXBUF];
   char error404FilePath[MAXBUF];
   char port[MAXBUF];
};

struct config get_config(char *filename) 
{
        struct config configstruct;
        FILE *file = fopen (filename, "r");

        if (file != NULL)
        { 
                char line[MAXBUF];
                int i = 0;

                while(fgets(line, sizeof(line), file) != NULL)
                {
                        char *cfline;
                        cfline = strstr((char *)line,DELIM);
                        cfline = cfline + strlen(DELIM);
    
                        if (i == 0){
                                memcpy(configstruct.indexFilePath,cfline,strlen(cfline));

                        } else if (i == 1){
                                memcpy(configstruct.error404FilePath,cfline,strlen(cfline));

                        } else if (i == 2){
                                memcpy(configstruct.port,cfline,strlen(cfline));
						}
                        
                        i++;
                } 
                fclose(file);
        } 
        
                
        
        return configstruct;

}
void daemon(){
        pid_t pid, sid;
        
        pid = fork();
        if (pid < 0) {
                exit(EXIT_FAILURE);
        }
        if (pid > 0) {
                exit(EXIT_SUCCESS);
        }
        umask(0);
                
        sid = setsid();
        if (sid < 0) {
                exit(EXIT_FAILURE);
        }

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
}


int main(int argc , char *argv[])
{
	daemon();
	struct config configstruct;
        
    configstruct = get_config(FILENAME);
	
	char str[100];
	FILE *file, *errorFile, *log;
	char line[256];
	
    int socket_desc , new_socket , c;
    struct sockaddr_in server , client;

     

    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
     

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(atoi(configstruct.port));
     

    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        return 1;
    }
     

    listen(socket_desc , 3);
     

    c = sizeof(struct sockaddr_in);
    while( (new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
 
		bzero( str, 100);
 
	    read(new_socket,str,100);
		log = fopen("logfile.log", "a+");
		fprintf(log,"%s", str);
		fclose(log);
		const char *str_split[3];
		char* words;
		int count = 0;
		char* str_copy = str;
		

		while ((words = strtok(str_copy, " ")) !=NULL)   
        {
			if(count > 2)
			{
				write(new_socket, "Bad gateway\n", 14);
				close(new_socket);
			}
			str_split[count] = words;
    		count++;
    		str_copy = NULL;  
        }
		if(count < 2)
		{
			write(new_socket, "Bad gateway\n", 14);
			close(new_socket);
		}

		if(strcmp(str_split[0], "GET"))
		{
			write(new_socket, "Bad gatewayget\n", 17);
			close(new_socket);
		}

		file = fopen(str_split[1], "r");
		if (file == NULL) 
		{
			errorFile = fopen("error404.html", "r");
			while (fgets(line, sizeof(line), errorFile)) {
				write(new_socket, line, strlen(line)); 
			}
		
			fclose (errorFile);
			close(new_socket);

		}
		
		if(file)
		{
			while (fgets(line, sizeof(line), file)) {
				write(new_socket, line, strlen(line)); 
			}
		
			fclose (file);
		}
		
		close(new_socket);

    }
     
    if (new_socket<0)
    {
        perror("accept failed");
        return 1;
    }
     
    return 0;
}
