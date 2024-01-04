#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <vector>
#include <algorithm>

std::vector<int> clientSockets;

void broadcastMessage(int senderSocket, const char* message, const char* username) {
    for (int clientSocket : clientSockets) {
        // Envoyer le message formaté à tous les clients sauf l'expéditeur
        if (clientSocket != senderSocket) {
            std::string formattedMessage = username + std::string(":") + message;
            send(clientSocket, formattedMessage.c_str(), formattedMessage.size(), 0);
        }
    }
}

void handleClient(int clientSocket, struct sockaddr_in clientAddr) {
    // Ajouter le socket du client à la liste
    clientSockets.push_back(clientSocket);

    // Afficher l'adresse IP du client
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
    //std::cout << "Client " << clientIP << " connecté !" << std::endl;

    // Demander le nom d'utilisateur au client
    char username[256];
    ssize_t bytesRead = recv(clientSocket, username, sizeof(username), 0);
    if (bytesRead <= 0) {
        std::cerr << "Erreur de réception du nom d'utilisateur." << std::endl;
        close(clientSocket);
        return;
    }
    username[bytesRead] = '\0';

    // Envoyer un message de bienvenue au client avec son nom d'utilisateur
    // const char* welcomeMessage = "Bienvenue sur le serveur !";
    // send(clientSocket, welcomeMessage, strlen(welcomeMessage), 0);

    std::cout << "Client " << username << " connecté !" << std::endl;

    // Boucle de réception de messages du client
    char buffer[1024];
    while (true) {
        // Recevoir un message du client
        bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

        // Vérifier si la connexion a été fermée par le client
        if (bytesRead <= 0) {
            // Supprimer le socket du client de la liste
            auto it = std::remove_if(clientSockets.begin(), clientSockets.end(),
                                     [clientSocket](int s) { return s == clientSocket; });
            clientSockets.erase(it, clientSockets.end());

            std::cout << "Client " << username << " déconnecté." << std::endl;
            break;
        }

        // Afficher le message reçu du client
        buffer[bytesRead] = '\0';
        std::cout << "Message du client [" << username <<"] :" << buffer << std::endl;

        // Diffuser le message formaté à tous les autres clients
        broadcastMessage(clientSocket, buffer, username);
    }

    // Fermeture du socket du client
    //close(clientSocket);
}

int main() {
    // Création du socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Configuration de l'adresse du serveur
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345); // Port
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Adresse IP du serveur

    // Liaison du socket avec l'adresse
    bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    // Attente de connexions
    listen(serverSocket, 5);

    std::cout << "Le serveur attend des connexions..." << std::endl;

    while (true) {
        // Accepter une connexion
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

        // Créer un thread pour traiter la connexion du client
        std::thread(handleClient, clientSocket, clientAddr).detach();
    }

    // Le code ci-dessous ne sera jamais atteint, car la boucle est infinie
    close(serverSocket);

    return 0;
}
