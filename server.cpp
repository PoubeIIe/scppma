#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#include <iostream>
#include <vector>
#include <algorithm>

std::vector<TCPsocket> clientSockets;
SDL_mutex* clientMutex;

int receiveMessages(void* data) {
    TCPsocket clientSocket = reinterpret_cast<TCPsocket>(data);
    char buffer[1024];

    while (true) {
        int bytesRead = SDLNet_TCP_Recv(clientSocket, buffer, sizeof(buffer));

        if (bytesRead <= 0) {
            std::cout << "Client disconnected." << std::endl;
            break;
        }

        buffer[bytesRead] = '\0';
        std::cout << "Received message from client: " << buffer << std::endl;
    }

    SDL_LockMutex(clientMutex);
    auto it = std::remove_if(clientSockets.begin(), clientSockets.end(),
                             [clientSocket](TCPsocket s) { return s == clientSocket; });
    clientSockets.erase(it, clientSockets.end());
    SDL_UnlockMutex(clientMutex);

    SDLNet_TCP_Close(clientSocket);

    return 0; // Return an integer value
}

int handleClient(void* data) {
    TCPsocket clientSocket = reinterpret_cast<TCPsocket>(data);

    SDL_LockMutex(clientMutex);
    clientSockets.push_back(clientSocket);
    SDL_UnlockMutex(clientMutex);

    std::cout << "Client connected." << std::endl;

    SDL_Thread* receiveThread = SDL_CreateThread(receiveMessages, "ReceiveThread", clientSocket);
    SDL_WaitThread(receiveThread, NULL);

    std::cout << "Client disconnected." << std::endl;

    return 0; // Return an integer value
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
