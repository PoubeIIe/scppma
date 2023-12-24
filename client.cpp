#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>

void receiveMessages(int clientSocket) {
    char buffer[1024];
    while (true) {
        // Recevoir un message du serveur
        ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

        // Vérifier si la connexion a été fermée par le serveur
        if (bytesRead <= 0) {
            std::cout << "Connexion au serveur fermée." << std::endl;
            break;
        }

        // Afficher le message reçu du serveur
        buffer[bytesRead] = '\0';
        std::cout << buffer << std::endl;
    }
}

int main() {
    // Demander à l'utilisateur l'adresse IP du serveur
    std::string serverIP;
    std::cout << "Entrez l'adresse IP du serveur : ";
    std::cin >> serverIP;

    // Demander à l'utilisateur le port du serveur
    int serverPort;
    std::cout << "Entrez le port du serveur : ";
    std::cin >> serverPort;

    std::string username;
    std::cout << "Entrez votre nom d'utilisateur : ";
    std::cin>>username;

    // Création du socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    std::cout<<"Tentative de connection au serveur : \""<<serverIP<<": "<<serverPort<<"\"..."<<std::endl;
    // Configuration de l'adresse du serveur
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort); // Port
    serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str()); // Adresse IP du serveur

    // Connexion au serveur
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "La connexion au serveur a échoué." << std::endl;
        close(clientSocket);
        return 1;
    }
    std::cout << "Connecté au serveur !" << std::endl;

    // Créer un thread pour recevoir les messages du serveur
    std::thread(receiveMessages, clientSocket).detach();

    // Boucle d'envoi de messages au serveur
    std::string userInput;
    while (true) {
        std::getline(std::cin, userInput);

        // Envoyer le message au serveur avec le nom d'utilisateur
        std::string message = "[" + username + "]" + " : " + userInput;
        send(clientSocket, message.c_str(), message.size(), 0);
    }

    // Le code ci-dessous ne sera jamais atteint, car la boucle est infinie
    close(clientSocket);

    return 0;
}
