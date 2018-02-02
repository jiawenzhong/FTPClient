/**
    C++ client example using sockets.
    This programs can be compiled in linux and with minor modification in 
	   mac (mainly on the name of the headers)
    Windows requires extra lines of code and different headers
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment(lib, "Ws2_32.lib")
...
WSADATA wsaData;
iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
...

Name: Kaicie Messer, Jiawen Zhong, Eunice Chinchilla
*/
#include <iostream>    //cout
#include <string>
#include <stdio.h> //printf
#include <stdlib.h>
#include <string.h>    //strlen
#include <sys/socket.h>    //socket
#include <arpa/inet.h> //inet_addr
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <fstream>

#define BUFFER_LENGTH 2048
#define WAITING_TIME 1000000
using namespace std;

//! this create the connection with the FTP address
/*!
  \param host the ip address of the server
  \param port the port number to connect to 
*/
int create_connection(std::string host, int port)
{
    int s;
    struct sockaddr_in socketAddress;
    memset(&socketAddress,0, sizeof(socketAddress));
    s = socket(AF_INET,SOCK_STREAM,0);
    socketAddress.sin_family=AF_INET;
    socketAddress.sin_port= htons(port);

    int a1,a2,a3,a4;
    if (sscanf(host.c_str(), "%d.%d.%d.%d", &a1, &a2, &a3, &a4 ) == 4)
    {
        socketAddress.sin_addr.s_addr =  inet_addr(host.c_str());
    }
    else {
        hostent *record = gethostbyname(host.c_str());
        in_addr *addressptr = (in_addr *)record->h_addr;
        socketAddress.sin_addr = *addressptr;
    }
    if(connect(s,(struct sockaddr *)&socketAddress,sizeof(struct sockaddr))==-1)
    {
        perror("connection fail");
        exit(1);
        return -1;
    }
    
    return s;
}

//! request reply from server 
/*!
  \param sockpi the socket number 
  \param message the message sent to the server
*/
int request(int sock, std::string message)
{
    return send(sock, message.c_str(), message.size(), 0);
}

//! this gets the reply from the server 
/*!
  \param s the socket number 
*/
std::string reply(int s)
{
    std::string strReply;
    int count;
    char buffer[BUFFER_LENGTH+1];
    
    usleep(WAITING_TIME);
    
    do {
        count = recv(s, buffer, BUFFER_LENGTH, 0);
        buffer[count] = '\0';
        strReply += buffer;
        
    }while (count ==  BUFFER_LENGTH);

    return strReply;
}

//! this request reply from server
/*!
  \param s the socket number
  \param message the command for the server 
*/
std::string request_reply(int s, std::string message)
{
	if (request(s, message) > 0)
    {
    	return reply(s);
	}
	return "";
}

//! this connects the passive 
/*!
  \param sockpi the socket number 
*/
string passive(int sockpi){
    // wait for respond
    string strReply = request_reply(sockpi, "PASV \r\n");
    // parse string, find ( and )(130,179,16,134,144,252)
    string substr;
    substr = strReply.substr(strReply.find("("), strReply.find(")"));
    return substr;
}

int connect(string passiveReturn){
    // conver string to char
    char ports [passiveReturn.length()];
    strcpy(ports, passiveReturn.c_str());
    
    // parse string        
    int str [10];
    sscanf(ports,"(%d,%d,%d,%d,%d,%d)", 
        &str[0], &str[1], &str[2], &str[3], &str[4], &str[5]);

    // create host
    string host = to_string(str[0]) + "." + to_string(str[1]) + "." + to_string(str[2])
            + "." + to_string(str[3]);

    //! create port
    int a = str[4];
    int b = str[5];
    int port = (a << 8) | b;
    //cout << "Port: " << port << endl;
    // create connection
    int sockpi2 = create_connection(host, port);
    return sockpi2;
}

//! this closes the connection
/*!
  \param sockpi the socket number 
*/
void closeConnection(int sockpi){   
    //send bye, check return message
    string strReply = request_reply(sockpi, "QUIT \r\n");
    cout << strReply << endl;
}

//! this checks if all files are finish transfer
/*!
  \param sockpi the socket number 
*/
void checkFileTransfer(int sockpi){
    //get the status back when file is finish transmitting
    string replyStatus = reply(sockpi);
    //TODO: check status here -------
    string status = replyStatus.substr(0, replyStatus.find(" "));

    // Check status true
    if (status == "226"){
        cout << "Transfer complete" << endl;
    } else {// status when file is not valid
        cout << replyStatus.substr(replyStatus.find(" ") + 1, replyStatus.length());
    }
}

//! execute the command that the user has entered
/*!
  \param sockpi the socket number 
  \param sockpi2 the socket number that gets the file
  \param choice user input command
*/
void command(int sockpi, string choice){
    string status;/*!< store the status number from reply*/    
    string replyString, strReply;/*!< reply from command */

    // check command to decide the correct execution
    if(choice == "bye"){
        closeConnection(sockpi);//! close all connections
    }else{
        // wait for passive respond
        string passiveReturn = passive(sockpi);
        // create connection for commands
        int sockpi2 = connect(passiveReturn);

        string command = choice.substr(0, choice.find(" "));//! extract the return status
        string directory; // store the file or directory
        if ( command == "ls"){
            // list files
            directory = choice.substr(choice.find(" ") + 1, choice.length());
            if(directory == "ls") 
                directory = ".";
            // get reply from request
            strReply = request_reply(sockpi, "LIST " + directory + "\r\n");
            // get reply from connection
            replyString = reply(sockpi2);
            cout << replyString << endl;
            close(sockpi2);// close socket 2
            checkFileTransfer(sockpi);// check after the file has finish transfering
        } else if (command == "get"){
            // makes a copy
            directory = choice.substr(choice.find(" ") + 1);
            strReply = request_reply(sockpi, "RETR " + directory + "\r\n");
            
            //TODO: need to check the status before reply sockpi2 
            status = strReply.substr(0, strReply.find(" "));
            if (status == "150"){//! check if file is open correctly
                replyString = reply(sockpi2);
                // stream that create a file to store   
                ofstream savefile;
                savefile.open("NOTICE.txt");   
                if(savefile.is_open()){
                    savefile << replyString;//! store the text into the file
                    cout << "File saved" << endl;
                    savefile.close();// close the file
                }       
                close(sockpi2);// close sockpi2 connection
                checkFileTransfer(sockpi);//! check connection
            } else{
                // display error message
                cout << strReply.substr(strReply.find(" ") + 1, strReply.length()) << endl;
            }
        } else if(command == "cd"){
            directory = choice.substr(choice.find(" ") + 1, choice.length());
            strReply = request_reply(sockpi, "CWD " + directory + "\r\n");
            cout << strReply << endl;
        }else {
            cout << "Invalid Command" << endl;
        }
    }
}

int main(int argc , char *argv[])
{   
    string choice;// store user input
    int sockpi; // store socket number 
    std::string strReply;// store string reply from command 
    
    //TODO  arg[1] can be a dns or an IP address.
    if (argc > 2)
        sockpi = create_connection(argv[1], atoi(argv[2]));
    if (argc == 2)
        sockpi = create_connection(argv[1], 21);
    else
        sockpi = create_connection("130.179.16.134", 21);
    strReply = reply(sockpi);
    std::cout << strReply  << std::endl;
    
    // ask for username
    //getlin(cin, choice);
    //strReply = request_reply(sockpi, "USER " + choice + "\r\n");
    strReply = request_reply(sockpi, "USER anonymous\r\n");

    //TODO parse srtReply to obtain the status. 
    string status = strReply.substr(0, strReply.find(" "));

    //user log in
    if (status == "331"){
        cout << "Username found" << endl;
        // ask for password
        //getline(cin, choice);
        //strReply = request_reply(sockpi, "PASS " + choice + "\r\n");
        strReply = request_reply(sockpi, "PASS asa@asas.com\r\n");
        status = strReply.substr(0, strReply.find("-"));
        if (status == "230"){
            cout << "Login successfully." << endl;
    
 
            // Hint: implement a function that set the SP in passive mode and accept commands.
            do{
                cout << "enter your command: ls, cd, get, bye" << endl;
                getline(cin, choice);
                
                // execute the command 
                command(sockpi, choice);
            } while (choice != "bye");
        }
        else{
            cout << "Password incorrect.";
        }
    }
    else{
        cout << "Username incorrect.";
    }
	// Let the system act according to the status and display
    // friendly message to the user 
	// You can see the ouput using std::cout << strReply  << std::endl;
    
    cout << "Closed connection" << endl;
    return 0;
}
