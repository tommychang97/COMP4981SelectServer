#include "Server.hpp"
#include <iostream>
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <strings.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include "ConnectivityManager.hpp"
#include "rapidjson/filewritestream.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/reader.h"
#include "rapidjson/document.h"
#include <thread>
#include <algorithm>
#include <unistd.h>
#include <array>

#define SERVER_TCP_PORT 7000	// Default port
#define BUFLEN	8096	//Buffer length
#define TRUE	1
#define LISTENQ	5
#define MAXLINE 4096
#define  CREATE 0
#define  DESTROY 1
#define  GET_ALL 2
#define  JOIN 3
#define  LEAVE 4

using namespace std;
using namespace rapidjson;
// Function Prototypes
static void SystemFatal(const char* );

volatile int UDP_PORT = 12500;
LobbyManager * lobbyManager = new LobbyManager();
Document document;
std::vector<Client*>clientList;

int validateJSON(char * buffer) {


	if (document.Parse(buffer).HasParseError()) {
		cout << "Parse error" << endl;
		return 0;
	}
	Value::ConstMemberIterator itr = document.FindMember("messageType");
	if (itr == document.MemberEnd()) {
		cout << "Cannot find message type" << endl;
		return 0;
	}
	return 1;
	
	// string str(buffer);
	// str.erase(std::remove(str.begin(), str.end(), '\\'), str.end());
	// str.erase(0,1);
	// str.pop_back();

	// int n = sizeof(str);
	// char temp[n];
	// strcpy(temp, str.c_str());

	// if (document.Parse(temp).HasParseError()) {
	// 	cout << "Parse error" << endl;
	// 	return 0;
	// }
	// Value::ConstMemberIterator itr = document.FindMember("messageType");
	// if (itr == document.MemberEnd()) {
	// 	cout << "Cannot find message type" << endl;
	// 	return 0;
	// }
	// return 1;
	
}
string connectResponse(Client *client) {
	// const char * json = "{\"userID\":0,\"UDPPort\":0,\"statusCode\":200,\"response\":{\"docs\":[{\"eircode\":\"D02 YN32\"}]}}";
	const char * json = "{\"statusCode\":200,\"userID\":0,\"UDPPort\":0}";
	Document ClientInfo;
	ClientInfo.Parse(json);
	Value & id = ClientInfo["userID"];
	Value & port = ClientInfo["UDPPort"];
	id.SetInt(client->getPlayer_Id());
	port.SetInt(client->getUDPPort());
	StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    ClientInfo.Accept(writer);
	return buffer.GetString();
}

int sendResponse(int socket, string response) {
    std::cout << "Sending back response: " << response.c_str() << endl;
	int n = send(socket, response.c_str(), response.size(), 0);
    return n;
}

Client * getClient(int playerId) {
    for (int j = 0; j < clientList.size(); j++) {
        if (clientList[j]->getPlayer_Id() == playerId) {
            return clientList[j];
            }
    }
    return NULL;
}

int main (int argc, char **argv)
{
    char buffer[BUFLEN];
	int i, maxi, nready, bytes_to_read, arg;
	int listen_sd, new_sd, sockfd, client_len, port, maxfd, client[FD_SETSIZE];
	struct sockaddr_in server, client_addr;
	char *bp, buf[BUFLEN];
   	ssize_t n;
   	fd_set rset, allset;

	switch(argc)
	{
		case 1:
			port = SERVER_TCP_PORT;	// Use the default port
		break;
		case 2:
			port = atoi(argv[1]);	// Get user specified port
		break;
		default:
			fprintf(stderr, "Usage: %s [port]\n", argv[0]);
			exit(1);
	}

	// Create a stream socket
	if ((listen_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		SystemFatal("Cannot Create Socket!");
	
	// set SO_REUSEADDR so port can be resused imemediately after exit, i.e., after CTRL-c
        arg = 1;
        if (setsockopt (listen_sd, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1)
                SystemFatal("setsockopt");

	// Bind an address to the socket
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections from any client

	if (bind(listen_sd, (struct sockaddr *)&server, sizeof(server)) == -1)
		SystemFatal("bind error");
	
	// Listen for connections
	// queue up to LISTENQ connect requests
	listen(listen_sd, 20);

	maxfd = listen_sd;	// initialize
   	maxi  = -1;		// index into client[] array

	for (i = 0; i < FD_SETSIZE; i++) {
           	client[i] = -1;  // -1 indicates available entry      
    } 
 	FD_ZERO(&allset);
   	FD_SET(listen_sd, &allset);
    int UDP_PORT = 12000;
	while (TRUE)
	{
   		rset = allset;               // structure assignment
		nready = select(maxfd + 1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(listen_sd, &rset)) {// new client connection
            client_len = sizeof(client_addr);
            if ((new_sd = accept(listen_sd, (struct sockaddr *) &client_addr, &client_len)) == -1)
				SystemFatal("accept error");
                printf(" Remote Address:  %s\n", inet_ntoa(client_addr.sin_addr));
            for (i = 0; i < FD_SETSIZE; i++)
			if (client[i] < 0) {
				client[i] = new_sd;	// save descriptor
				break;
            }
			if (i == FD_SETSIZE) {
				printf ("Too many clients\n");
            	exit(1);
    		}
			FD_SET (new_sd, &allset);     // add new descriptor to set
			if (new_sd > maxfd) {
				maxfd = new_sd;	// for select
            }

			if (i > maxi) {
				maxi = i;	// new max index in client[] array
            }

			if (--nready <= 0)
				continue;	// no more readable descriptors
     	}

		for (i = 0; i <= maxi; i++)	{// check all clients for data
			if ((sockfd = client[i]) < 0)
				continue;

        if (FD_ISSET(sockfd, &rset)) {
            bp = buf;
            bytes_to_read = BUFLEN;
            n = recv (sockfd, buffer, bytes_to_read, 0);
    		if (n < 0) {
    			printf("didnt recieve anything, recv error\n");
    		}
        }
		try {

            // Validate the JSON object received
			if (!validateJSON(buffer)) {
				throw std::invalid_argument("bad json object");
			}

            // Check the message type, can either be connect or a lobby request
			string request = document["messageType"].GetString();

			if (request == "connect") {
				cout << "A new client has connected to the server!" << endl;
                //  Create a new client, add it to the list, and return its id and UDP Port number.
                Client * newClient = new Client("default", 0, new_sd, UDP_PORT++, atoi(inet_ntoa(client_addr.sin_addr)));
		        clientList.push_back(newClient);
				Value::ConstMemberIterator itr = document.FindMember("username");
				if (itr == document.MemberEnd()) {
					throw std::invalid_argument("bad json object");
				}
				string username = document["username"].GetString();
				newClient->setPlayer_name(username);
				newClient->setStatus("false");
				string response = connectResponse(newClient);
                if ((n = sendResponse(sockfd, response)) < 0)
					cout << "Failed to send!" << endl;
			} 
			else {
                // Ensure that the id is part of the request.
                Value::ConstMemberIterator itr = document.FindMember("id");
				if (itr == document.MemberEnd()) {
					throw std::invalid_argument("bad json object");
				}
                int playerId = document["id"].GetInt();
                Client * clientObj = getClient(playerId);
                if (clientObj == NULL) {
                    cout << "Couldn't retrieve client based on Id" << endl;
                    continue;
                }
			    itr = document.FindMember("action");
				if (itr == document.MemberEnd()) {
					throw std::invalid_argument("bad json object");
				}
				int action = document["action"].GetInt();
				int lobbyID;
				string lobbyResponse;
				if (request == "lobbyRequest") {	
					switch(action) {
						case CREATE:
							cout << "Received client request to create lobby!" << endl;
							//create lobby, send lobby back
							lobbyID = lobbyManager->createLobby(clientObj);
							lobbyResponse = lobbyManager->getLobby(lobbyID);
							if ((n = sendResponse(sockfd, lobbyResponse)) < 0)
					            cout << "Failed to send!" << endl;
							break;
						case DESTROY:
							{
								cout << "Request received to destroy the lobby!" << endl;
								lobbyID = lobbyManager->verifyLobbyOwner(clientObj);
								if (lobbyID == -1)
								{
									cout << "The current client is not a Lobby owner!" << endl;
									break;
								}
								Lobby* lobby1 = lobbyManager->getLobbyObject(lobbyID);
								lobby1->removeAllClients();
								lobbyManager->deleteLobby(lobbyID);
								lobbyResponse = lobbyManager->getLobbyList();
								if ((n = sendResponse(sockfd, lobbyResponse)) < 0)
					            cout << "Failed to send!" << endl;
							}
							break;
						case GET_ALL:
							cout << "Received client request for lobby list!" << endl;
							lobbyResponse = lobbyManager->getLobbyList();
                            if ((n = sendResponse(sockfd, lobbyResponse)) < 0)
					            cout << "Failed to send!" << endl;
							break;
						case JOIN:
							{
								Value::ConstMemberIterator itr = document.FindMember("lobbyId");
								if (itr == document.MemberEnd()) {
									throw std::invalid_argument("bad json object");
								}
								lobbyID = document["lobbyId"].GetInt();
								Lobby * lobby = lobbyManager->getLobbyObject(lobbyID);
								lobby->addClient(clientObj);
								lobbyResponse = lobbyManager->getLobby(lobbyID);
								if ((n = sendResponse(sockfd, lobbyResponse)) < 0)
					            cout << "Failed to send!" << endl;
							}
							break;
						case LEAVE:
							{
								Value::ConstMemberIterator itr = document.FindMember("lobbyId");
								if (itr == document.MemberEnd()) {
									throw std::invalid_argument("bad json object");
								}
								lobbyID = document["lobbyId"].GetInt();
								Lobby * lobby = lobbyManager->getLobbyObject(lobbyID);
								lobby->removeClient(clientObj);
                                lobbyResponse = lobbyManager->getLobby(lobbyID);
								if ((n = sendResponse(sockfd, lobbyResponse)) < 0)
					            cout << "Failed to send!" << endl;
							}
							break;
					}
				}
				else if (request == "switchUserSide") {
					clientObj->getTeam() == 0? clientObj->setTeam(1) : clientObj->setTeam(0);
					// send response
				}
				else if (request == "switchStatusReady") {
					clientObj->getStatus() == "true"? clientObj->setStatus("false") : clientObj->setStatus("true");
					//send response
				}
				else if (request == "switchPlayerClass") {

				}
            }
		} catch (...) {
			printf("Catching an exception");
			string errorResponse = "{\"statusCode\":500}";
			send(sockfd, errorResponse.c_str(), errorResponse.size(), 0);
		}
				
        if (n == 0) // connection closed by client
        {
            printf(" Remote Address:  %s closed connection\n", inet_ntoa(client_addr.sin_addr));
            close(sockfd);
            FD_CLR(sockfd, &allset);
            client[i] = -1;
        }
                                                        
        if (--nready <= 0)
            break;        // no more readable descriptors
        }
    }
	return(0);
}


// Prints the error stored in errno and aborts the program.
static void SystemFatal(const char* message)
{
    perror (message);
    exit (EXIT_FAILURE);
}