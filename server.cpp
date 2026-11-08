#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <mutex>
#include <fstream>
#include <cstdlib>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <list>
#include <atomic>
#include <thread>
#include <unordered_map>
#include <iomanip>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

mutex console_mutex;
mutex clients_mutex;

unordered_map<string, SOCKET> clients;

string generate_id()
{
    stringstream ss;

    ss << setfill('0') << setw(5) << clients.size();

    return ss.str();
}

//Обработка сообщения
unordered_map<string, string> GetMessageData(string message){

    unordered_map<string, string> result;

    string flag = message.substr(message.length() - 1);
    string recipient_id = message.substr(message.length() - 6, 5);
    string sender_id = message.substr(message.length() - 11, 5);
    string text = message.substr(0, message.length() - 11);

    result["text"] = text;
    result["sender_id"] = sender_id;
    result["recipient_id"] = recipient_id;
    result["flag"] = flag;
    
    return result;
}

void SendMessageToClient(string recipient_id, string sender_id, string text, SOCKET client_socket){

    const string message = text + sender_id;
    send(client_socket, message.c_str(), message.length(), 0);

}

//Отправка сообщения другому клиенту
void HandleMessage(string message){

    unordered_map<string, string> data = GetMessageData(message);
    string text = data["text"];
    string sender_id = data["sender_id"];
    string recipient_id = data["recipient_id"];
    string flag = data["flag"];

    auto it = clients.find(recipient_id);
    SOCKET client_socket;

    if(it != clients.end()) client_socket = it->second;
    else cout << "[ERROR] Could not find client with ID : [" << recipient_id << "]" << endl;

    if (flag == "s"){
        SendMessageToClient(recipient_id, sender_id, text, client_socket); 
    }
}

//Хэндлер клиентов
void ClientHandler(SOCKET client_socket) {

    string id = generate_id();
    send(client_socket, id.c_str(), id.length(), 0);

    {
        lock_guard<mutex> lock(clients_mutex);
        clients.insert({ id, client_socket });
    }

    vector<pair<string, SOCKET>> active_clients;

    {
        lock_guard<mutex> lock(clients_mutex);

        for (const auto& i : clients) {
            active_clients.push_back(i);
        }
    }

    
    {
        lock_guard<mutex> lock(console_mutex);

        cout << "[SERVER] Active clients:" << endl;

        for (const auto& i : active_clients) {
            cout << "SOCKET [" << i.second
                 << "] : ID [" << i.first << "]" << endl;
        }
    }

    char buffer[1024];

    while (true) {

        int bytes = recv(client_socket, buffer, sizeof(buffer), 0);

        if (bytes <= 0) {
            break;
        }

        string client_message(buffer, bytes);

        {
            lock_guard<mutex> lock(console_mutex);
            cout << "Received from client: "
                 << client_message << endl;
        }

        
        HandleMessage(client_message);
    }

    closesocket(client_socket);

    {
        lock_guard<mutex> lock(console_mutex);
        cout << "Client disconnected: " << id << endl;
    }

    {
        lock_guard<mutex> lock(clients_mutex);
        clients.erase(id);
    }
}


int main(){

    //Инициализация WinSock
    WSADATA wsadata;

    int result = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (result != 0) {
        
        cout << "An error occured with WSAStartup: " << result << endl; 
        return 1;
    }

    //Создание серверного сокета
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) cout << "An error occured with initalizing server socket: " << WSAGetLastError() << endl;

    //Параметры сокета
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    //Бинд сокета
    if (bind(server_socket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR){
        cout << "An error occured with binding socket: " << WSAGetLastError() << endl;
    }

    //Слушает на порту 
    listen(server_socket, SOMAXCONN);

    cout << "Listening on port 8080..." << endl;

    //Цикл подключения клиентов -> вынос клиента в отдельный поток
    while (true){

        SOCKET client_socket = accept(server_socket, nullptr, nullptr);

        if (client_socket == INVALID_SOCKET){

            cout << "An error occured with accepting client: " << WSAGetLastError() << endl;
            continue;
        }

        thread Client_Thread(ClientHandler, client_socket);
        Client_Thread.detach();

    }

    closesocket(server_socket);
    WSACleanup();
}