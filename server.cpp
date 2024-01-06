#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>

std::vector<TCPsocket> clientSockets;
SDL_mutex* clientMutex;

void displayMessage(const char* message) {
    SDL_LockMutex(clientMutex);
    std::cout << message << std::endl;
    SDL_UnlockMutex(clientMutex);
}

int receiveMessages(void* data) {
    TCPsocket clientSocket = reinterpret_cast<TCPsocket>(data);
    char buffer[1024];

    // Receive the username from the client
    char username[256];
    ssize_t bytesRead = SDLNet_TCP_Recv(clientSocket, username, sizeof(username));

    if (bytesRead <= 0) {
        std::cerr << "Error receiving username." << std::endl;
        SDL_LockMutex(clientMutex);
        auto it = std::remove_if(clientSockets.begin(), clientSockets.end(),
                                 [clientSocket](TCPsocket s) { return s == clientSocket; });
        clientSockets.erase(it, clientSockets.end());
        SDL_UnlockMutex(clientMutex);

        SDLNet_TCP_Close(clientSocket);
        return 1;
    }
    username[bytesRead] = '\0';

    SDL_LockMutex(clientMutex);
    std::cout << "Client " << username << " connected!" << std::endl;
    SDL_UnlockMutex(clientMutex);

    while (true) {
        bytesRead = SDLNet_TCP_Recv(clientSocket, buffer, sizeof(buffer));

        if (bytesRead <= 0) {
            SDL_LockMutex(clientMutex);
            auto it = std::remove_if(clientSockets.begin(), clientSockets.end(),
                                     [clientSocket](TCPsocket s) { return s == clientSocket; });
            clientSockets.erase(it, clientSockets.end());
            SDL_UnlockMutex(clientMutex);

            SDLNet_TCP_Close(clientSocket);

            SDL_LockMutex(clientMutex);
            std::cout << "Client " << username << " disconnected." << std::endl;
            SDL_UnlockMutex(clientMutex);

            // Notify other clients about the disconnection
            char disconnectMsg[256];
            snprintf(disconnectMsg, sizeof(disconnectMsg), "Client %s disconnected.", username);
            SDL_LockMutex(clientMutex);
            for (TCPsocket otherClientSocket : clientSockets) {
                if (otherClientSocket != clientSocket) {
                    SDLNet_TCP_Send(otherClientSocket, disconnectMsg, strlen(disconnectMsg) + 1);
                }
            }
            SDL_UnlockMutex(clientMutex);

            break;
        }

        buffer[bytesRead] = '\0';

        // Notify other clients about the message
        char broadcastMsg[1280];  // Adjust the size as needed
        snprintf(broadcastMsg, sizeof(broadcastMsg), "[%s] : %s", username, buffer);

        SDL_LockMutex(clientMutex);
        for (TCPsocket otherClientSocket : clientSockets) {
            if (otherClientSocket != clientSocket) {
                SDLNet_TCP_Send(otherClientSocket, broadcastMsg, strlen(broadcastMsg) + 1);
            }
        }
        SDL_UnlockMutex(clientMutex);

        // Display the message in the server's console
        displayMessage(broadcastMsg);
    }

    return 0;
}

int handleClient(void* data) {
    TCPsocket clientSocket = reinterpret_cast<TCPsocket>(data);

    SDL_LockMutex(clientMutex);
    clientSockets.push_back(clientSocket);
    SDL_UnlockMutex(clientMutex);

    SDL_Thread* receiveThread = SDL_CreateThread(receiveMessages, "ReceiveThread", clientSocket);
    SDL_WaitThread(receiveThread, NULL);

    return 0;
}

int main() {
    if (SDL_Init(0) == -1 || SDLNet_Init() == -1) {
        fprintf(stderr, "SDL or SDL_net initialization failed: %s\n", SDL_GetError());
        return 1;
    }

    clientMutex = SDL_CreateMutex();

    IPaddress serverIP;
    TCPsocket serverSocket;

    if (SDLNet_ResolveHost(&serverIP, NULL, 12345) == -1) {
        fprintf(stderr, "SDLNet_ResolveHost: %s\n", SDLNet_GetError());
        SDLNet_Quit();
        SDL_Quit();
        return 2;
    }

    serverSocket = SDLNet_TCP_Open(&serverIP);
    if (!serverSocket) {
        fprintf(stderr, "SDLNet_TCP_Open: %s\n", SDLNet_GetError());
        SDLNet_Quit();
        SDL_Quit();
        return 3;
    }

    printf("The server is waiting for connections...\n");

    while (true) {
        TCPsocket clientSocket = SDLNet_TCP_Accept(serverSocket);

        if (clientSocket) {
            SDL_Thread* clientThread = SDL_CreateThread(handleClient, "ClientThread", clientSocket);
        }
    }

    SDLNet_TCP_Close(serverSocket);
    SDLNet_Quit();
    SDL_Quit();

    return 0;
}
