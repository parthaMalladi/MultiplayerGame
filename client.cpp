#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <netdb.h>

using namespace std;

struct gameState {
    int numPlayers;
    int currID;
    int hostId;
    bool readyToGo;
    bool gameStarted;
    bool gameOver;
    int winnerID;
    int randomNum;

    gameState() {
        numPlayers = 0;
        currID = -1;
        hostId = -1;
        readyToGo = false;
        gameStarted = false;
        gameOver = false;
        winnerID = -1;
        srand(time(nullptr));
        randomNum = rand() % 1000 + 1;
    }
};

int clientId = -1;

gameState receiveGameState(int socket) {
    gameState state;
    char buffer[sizeof(gameState)];
    ssize_t bytesRead = read(socket, buffer, sizeof(gameState));
    std::memcpy(&state, buffer, sizeof(gameState));
    return state;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <server> <port>" << endl;
        return 1;
    }

    char* serverName = argv[1];
    int serverPort = atoi(argv[2]);

    struct hostent* host = gethostbyname(serverName);
    if (!host) {
        cerr << "Error: Unable to resolve server address" << endl;
        return 1;
    }

    sockaddr_in serverAddr;
    bzero((char*)&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)*host->h_addr_list));
    serverAddr.sin_port = htons(serverPort);

    int clientSd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSd < 0) {
        cerr << "Error creating socket" << endl;
        return 1;
    }

    if (connect(clientSd, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Error connecting to server" << endl;
        return 1;
    }

    cout << "Connected to server. You can start typing messages." << endl;

    const unsigned int bufSize = 1500;
    char buffer[bufSize];
    bzero(buffer, bufSize);
    int rd = read(clientSd, buffer, bufSize - 1);
    buffer[rd] = '\0';
    cout << buffer << endl;
    clientId = buffer[rd - 1] - '0';

    string msg;
    gameState gs;
    while (true) {
        cout << "Enter message (or 'exit' to quit): ";
        getline(cin, msg);

        if (msg == "exit") {
            break;
        }

        int writeRequest = write(clientSd, msg.c_str(), msg.size());
        if (writeRequest < 0) {
            cerr << "Error sending message to server" << endl;
            break;
        }

        gs = receiveGameState(clientSd);

        if (gs.winnerID != -1) {
            if (gs.winnerID == clientId) {
                cout << "Congratulations, you guessed correctly and you won!" << endl;
            } else {
                cout << "Player " << gs.winnerID << " has won. Better luck next time." << endl;
            }
            break;
        }

        if (!gs.gameStarted && gs.readyToGo) {
            cout << gs.numPlayers << " in the lobby. ";
            if (clientId == gs.hostId) {
                cout << "Type 'start' to begin the game or wait for more players. ";
            } else {
                cout << "Waiting for the host to start the game. ";
            }
            cout << "Press enter to continue." << endl;
            continue;
        }

        if (gs.gameStarted) {
            if (gs.currID == clientId) {
                cout << "Guess the number between 1 and 1000." << endl;
            } else {
                cout << "Not your turn yet, press enter to continue." << endl;
            }
        }
    }

    close(clientSd);
    return 0;
}