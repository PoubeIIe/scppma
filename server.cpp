#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <winsock2.h>  // Include for inet_addr and htons on Windows
#include <iostream>
#include <thread>

TCPsocket clientSocket;

int receiveMessages(void* data) {
    char buffer[1024];
    while (true) {
        int bytesRead = SDLNet_TCP_Recv(clientSocket, buffer, sizeof(buffer));

        if (bytesRead <= 0) {
            std::cout << "Connection to the server closed." << std::endl;
            break;
        }

        buffer[bytesRead] = '\0';
        std::cout << buffer << std::endl;
    }

    return 0;
}

void stopClient() {
    SDLNet_TCP_Close(clientSocket);
}


int main() {
    // Demander à l'utilisateur l'adresse IP du serveur
    std::string IPaddr;
    std::cout << "Entrez l'adresse IP du serveur : ";
    std::cin >> IPaddr;
    
    // Demander à l'utilisateur le port du serveur
    int serverPort;
    std::cout << "Entrez le port du serveur : ";
    std::cin >> serverPort;
    
    std::string username;
    std::cout << "Entrez votre nom d'utilisateur : ";
    std::cin>>username;
    std::cout << "UN entré" << std::endl;


    IPaddress serverIP;
    std::cout << "instanctaiton de serverIP" << std::endl;
    // Include <winsock2.h> instead of arpa/inet.h
    serverIP.host = inet_addr(IPaddr.c_str()); // Server IP address
    serverIP.port = htons(serverPort); // Server port
    std::cout << "trucs d'ip" << std::endl;

    // Connect to the server
    clientSocket = SDLNet_TCP_Open(&serverIP);
    if (!clientSocket) {
        std::cerr << "Connection to the server failed." << std::endl;
        stopClient();
        return 1;
    }

    // Send the username to the server
    SDLNet_TCP_Send(clientSocket, username.c_str(), username.size() + 1);
    std::cout << "Connected to the server!" << std::endl;

    // Create a thread to receive messages from the server
    SDL_Thread* receiveThread = SDL_CreateThread(receiveMessages, "ReceiveThread", NULL);
    if (!receiveThread) {
        std::cerr << "Failed to create receive thread: " << SDL_GetError() << std::endl;
        stopClient();
        return 2;
    }

    // Boucle d'envoi de messages au serveur
    std::string userInput;
    while (true) {
        std::getline(std::cin, userInput);
    
        SDLNet_TCP_Send(clientSocket, userInput.c_str(), userInput.size() +1);
    }

    // Le code ci-dessous ne sera jamais atteint, car la boucle est infinie

    // The code below will never be reached, as the thread is detached

    return 0;
}
