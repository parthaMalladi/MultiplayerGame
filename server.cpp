#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <mutex>
#include <map>
#include <cstring>

using namespace std;

struct thread_data {
    int id;
    int sd;
};

struct gameState {
    // number of players in the lobby
    int numPlayers;

    // the id of the player who's turn it is
    int currID;

    // the id of the host
    int hostId;

    // boolean to see if atleast one player has readied up
    bool readyToGo;

    // boolean to see if the game has started
    bool gameStarted;

    // boolean to see if the game is still going or not
    bool gameOver;

    // id of the player who won
    int winnerID;

    // random number
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

// global variables
int id = 0;                         // id for the clients
map<int, int> connectedClients;     // vector of connected clients
const int bufSize = 1500;           // buffer size
const int maxClients = 3;           // max clients
gameState state;                    // initialize the game state

// other variables
map<int, bool> readyClients;        // map of clients who are ready
pthread_mutex_t gameLock;           // mutex lock

// function to compute game state
void decipherGameState(char* c, thread_data* d) {

}

// function to send the game state to the client
void sendGameState(int socket, const gameState& st) {
    char buffer[sizeof(gameState)];
    std::memcpy(buffer, &st, sizeof(gameState));
    ssize_t bytesSent = write(socket, buffer, sizeof(gameState));
}

// thread function
void* thread_server(void* ptr) {
    thread_data* data;
    data = (thread_data*) ptr;
    char buf[bufSize];

    // tell the client their id and ask them to join
    string readyMsg = "Type 'ready' to join game. Your id is " + to_string(data->id);
    write(data->sd, readyMsg.c_str(), readyMsg.size());

    while (true) {
        int bytesRead = read(data->sd, buf, bufSize);

        if (bytesRead > 0) {
            buf[bytesRead] = '\0';
            cout << "Received message from client: " << buf << endl;
            decipherGameState(buf, data);
            sendGameState(data->sd, state);
        } else {
            cerr << "Client disconnected or failed to read." << endl;
            break;
        }
    }

    close(data->sd);
    delete data;
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <port>" << endl;
        return 1;
    }

    int serverPort = atoi(argv[1]);
    pthread_mutex_init(&gameLock, nullptr);

    sockaddr_in serverAddr;
    bzero((char*)&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(serverPort);

    int serverSd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSd < 0) {
        cerr << "Error creating socket" << endl;
        return 1;
    }

    const int on = 1;
    setsockopt(serverSd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    if (bind(serverSd, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Error binding socket" << endl;
        return 1;
    }

    listen(serverSd, maxClients);

    cout << "Server is running and waiting for connections on port " << serverPort << endl;

    while (true) {
        // accept a client connection
        sockaddr_in clientAddr;
        socklen_t clientAddrSize = sizeof(clientAddr);
        int clientSd = accept(serverSd, (sockaddr*)&clientAddr, &clientAddrSize);

        if (clientSd < 0) {
            cerr << "Error accepting client connection" << endl;
            continue;
        }

        // add client to the map
        connectedClients[id] = clientSd;

        // create a thread struct with the id and socket
        thread_data* data = new thread_data;
        data->id = id;
        data->sd = clientSd;
        id += 1;

        // launch a new thread for each client
        pthread_t newThread;
        int iret1 = pthread_create(&newThread, NULL, thread_server, (void*)data);

        if (iret1 != 0) {
            cerr << "Error creating thread" << endl;
            close(clientSd);
        } else {
            pthread_detach(newThread);
        }
    }
}