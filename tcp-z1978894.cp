
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <dirent.h>

using namespace std;

/**
 * Chomp off the extra crap like trailing newlines from a character array/string.
 *
 * @param *s a pointer to a character array/ string.
 ********************************************************************************/
void chomp(char *s) //< chomp to get rid of crap in string.
{
	for (char *p = s + strlen(s)-1; *p == '\r' || *p == '\n'; p--)
	{
 		*p = '\0'; // change \r or \n to \0
	}
}

/**
 * the main of the program.
 *
 * @param argc the amount of command line arguments.
 * @param *argv[] a array of the the command line arguments.
 *
 * @return return a 0 on success or something else on failure.
 ********************************************************************************/
int main(int argc, char *argv[])
{
	if(argc < 3 || argc > 3) //< mae sure server started with right amount of arguments.
	{
		perror("Please start the server with your socket and the path to your root directory");
		exit(1);
	}
	char buffer[257]; //< all the intializers.
	int sock;
	int newSock;
	int fd;
	ssize_t recieved;
	struct sockaddr_in httpserver;
	struct sockaddr_in httpclient;
	struct dirent *dirEntry;
	DIR *dirp;
	char erroMsg[] = {"An error was encountered please make sure you entered the correct path and the server was intialized with a proper directory.\n Make sure you make a request to the server by using GET /path\n"}; //< the error message.



	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) //< create socket.
	{
		perror("Failed to create socket");
		exit(EXIT_FAILURE);
	}
	memset(&httpserver, 0, sizeof(httpserver)); //< set all the stuff up for the server.
	httpserver.sin_family = AF_INET;
	httpserver.sin_addr.s_addr = INADDR_ANY;
	httpserver.sin_port = htons(atoi(argv[1]));

	socklen_t serverlen = sizeof(httpserver);
	if (bind(sock, (struct sockaddr *) &httpserver, serverlen) < 0) //< bind the socket.
	{
		perror("Failed to bind server socket");
		exit(EXIT_FAILURE);
	}
	if(listen(sock, 64) == -1) //< make the server listen.
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}

	socklen_t clientlen = sizeof(httpclient);
	while((newSock = accept(sock, (struct sockaddr *) &httpclient, &clientlen)) != -1) //< accept data from client.
	{
		if(fork()) //< set up the fork so server can take multiple requests.
		{
			close(newSock);
		}
		else
		{
			cerr << "Client connected: " << inet_ntoa(httpclient.sin_addr) << "\n";
			if((dirp = opendir(argv[2])) == 0) //< make sure root directory exsists.
			{
				perror("cant open root directory");
				close(newSock);
				exit(1);		
			}
			closedir(dirp);
		
			if ((recieved = read(newSock, buffer, 256)) == -1) //< get the data from the client.
			{
				write(newSock, erroMsg, sizeof(erroMsg));
				close(newSock);
				exit(1);
			}
			if(recieved < 4) //< make sure data from client is valid.
			{
				write(newSock, erroMsg, sizeof(erroMsg));
				close(newSock);
				exit(1);			
			}

			string path; //< this chunks creates thhe path that will be used in the program.
			path += argv[2];
			string str = buffer;
			string sub = str.substr(4);
			path += sub;
			int length = path.length();
			char arrPath[length + 1];
			strcpy(arrPath, path.c_str());
			chomp(arrPath);			

			
			dirp = opendir(arrPath); //< try to open the final path as directory.
			if(dirp == 0) //< if path is not a directory check if it's a file.
			{
				char buff[BUFSIZ];
				int m;
				if((fd = open(arrPath, O_RDONLY)) == -1) //< open file at end of path.
				{
			 		write(newSock, erroMsg, sizeof(erroMsg));
					close(newSock);
					exit(1);
				}
				while ((m = read(fd, buff, BUFSIZ)) > 0) //< read and write to socket as long as read gets data from file.
				{
					if (write(newSock, buff, m) != m)
					{	
						perror("write error");
						close(fd);
						close(newSock);
						exit(1);
					}
				}
				close(fd);

							
			}
			else
			{

				bool found = false;
				char dot[] = {". "};
				string allNames;
				while ((dirEntry = readdir(dirp)) != NULL) //< if end of path is directory then check all the file names and put them in a string.
				{
					string Names = dirEntry->d_name;
					int v = Names.length();
   						  
    					char arrNames[v + 1];
	
   						
					strcpy(arrNames, Names.c_str());
					chomp(arrNames);
					
					if(arrNames[0] != dot[0]) //< keep out files in direcotry if they start with a .
					{
						allNames += arrNames;
						allNames += "  ";
					}
					
					if(Names == "index.html") //< check for a file named index.html if it exists flip found to true.
					{
						found = true;
					}
				}				
				if(found == true) //< if found is true then add index.html to end of path and open that file and read and write it's contents.
				{
					 char buf[BUFSIZ];
					 string newPath; 
					 newPath += arrPath;
					 int pathSize = sizeof(arrPath);
					 char dash[] {"/ "};
					 if(arrPath[pathSize - 1] == dash[0])
					 {
						newPath += "index.html";
					 }
					 else
					 {
						 newPath += "/index.html";
					 }
					 int u = newPath.length();
					 char indexP[u + 1];

					 strcpy(indexP, newPath.c_str());
					 chomp(indexP);

					 int a; 
					 if((fd = open(indexP, O_RDONLY)) == -1)
					 {
						 perror("Can't open index.html");
						 closedir(dirp);
						 close(newSock);
						 close(fd);
						 exit(1);
					 }
					 while ((a = read(fd, buf, BUFSIZ)) > 0)
					 {
					 	if(write(newSock, buf, a) != a)
						{
							perror("Write error in index write");
							closedir(dirp);
							close(newSock);
							close(fd);
							exit(1);
						}
					 }
					 close(fd);
				}
				else //< if a file named index.html does not exsist then print out all the files in the directory.
				{	
					int n = allNames.length();
					char charNames[n + 1];
					strcpy(charNames, allNames.c_str());
					chomp(charNames);

					if(write(newSock, charNames, sizeof(charNames)) != sizeof(charNames))
					{
						perror("write error in printing directories");
						closedir(dirp);
						close(newSock);
						exit(1);

					}

					char nl[] = {"\n"}; //< add new line.
					if(write(newSock, nl, sizeof(nl)) != sizeof(nl))
					{
						perror("Can't write new line");
						closedir(dirp);
						close(newSock);
						exit(1);
					}				
				}
			}
				
			closedir(dirp);
			close(newSock);
			exit(0);
		}

	}
}
