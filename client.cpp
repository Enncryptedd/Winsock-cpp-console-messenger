#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <windows.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <string>
#include <vector>
#include <unordered_map>
#include <ws2tcpip.h>
#include <list>
#include <cstdlib>
#include <chrono>
#include <conio.h>
#include <condition_variable>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

mutex console_mutex;
mutex messages_mutex;

mutex menu_mutex;
condition_variable menu_cv;
bool is_chatting = false;

atomic <bool> main_thread_is_alive = true;
atomic <bool> menu_thread_is_alive = true;
string client_id;

vector<pair<string, string>> incoming_messages;

string BuildMessage(string recipient_id){

    string message;
    cout << "[" << recipient_id << "]" << " >> ";
    getline(cin, message);

    return (message + client_id + recipient_id + "s");

}

void ShowCommands(){

   cout << endl;
   cout << "--------COMMANDS--------" << endl;
   cout << "--view_messages" << " <- view incoming messages" << endl;
   cout << "--exit" << " <- exit programm" << endl;
   cout << "--help" << " <- help" << endl;
   cout << "--menu" << " <- open menu" << endl;


   cout << "------------------------" << endl;
}



void Show_Messages() {
    lock_guard<mutex> msg_lock(messages_mutex);
    lock_guard<mutex> console_lock(console_mutex);

    cout << endl;
    cout << "_______________________________" << endl;
    cout << "             MESSAGES          " << endl;
    cout << "-------------------------------" << endl;

    for (const auto& pair : incoming_messages){
        cout << "[" << pair.first << "] -> " << pair.second << endl;
    }

    cout << "________________________________" << endl;
    cout << endl;
}

void ReceiveMessages(SOCKET client_socket){

    char buffer[1024];

    while (true){

        int bytes = recv(client_socket, buffer, sizeof(buffer), 0);
        if(bytes <= 0){

            cout << "Lost connection to server: [" <<  WSAGetLastError() << "]" << endl;
            break;
        }

        string message(buffer, bytes);
        
        std::lock_guard<std::mutex> lock(messages_mutex);
        
        if (message.length() >= 5) {
            string sender_id = message.substr(message.length() - 5); // Берем последние 5
            string text = message.substr(0, message.length() - 5); // Берем всё до них
            incoming_messages.push_back({sender_id, text});
        }

    }
}

string select_id() {
    string id;
    
    while (true) {

        cout << "Enter id >> ";
        getline(cin, id);

        if (id.length() == 5 && id != client_id) {
            return id;
        }
        else if (id == client_id){
            cout << "[ERROR] Not that fast - that's your ID" << endl;
        } else if (id.length() != 5) {
            cout << "[ERROR] Wrong id format, try again" << endl;
        }
   }
}

void Chat(SOCKET client_socket){

    string selected_id;

    while (true) {
        if (selected_id == "") {
            selected_id = select_id();
        }

        string message = BuildMessage(selected_id);
        
        if (message.find("--exit") != string::npos) {
            main_thread_is_alive = false;
            menu_thread_is_alive = false;
            
            // Будим меню, чтобы оно вышло из цикла while и завершило поток
            {
                lock_guard<mutex> lock(menu_mutex);
                is_chatting = false;
            }
            menu_cv.notify_all(); 
            return;
        }

        
        if (message.rfind("--menu", 0) == 0){
            // Сбрасываем флаг, чтобы меню проснулось
            {
                lock_guard<mutex> lock(menu_mutex);
                is_chatting = false;
            }
            menu_cv.notify_all();
            return;
        }

        if (message.rfind("--view_messages", 0) == 0) {
            Show_Messages();
            continue;
        }

        send(client_socket, message.c_str(), message.length(), 0);
    }
}

void MainMenu(SOCKET client_socket){

    while (menu_thread_is_alive) {
        // Ожидаем, пока is_chatting станет false
        {
            unique_lock<mutex> lock(menu_mutex);
            menu_cv.wait(lock, [] { return !is_chatting || !menu_thread_is_alive; });
        }
        
        // Если вышли из цикла из-за закрытия программы
        if (!menu_thread_is_alive) break;

        // Отрисовка меню
        
        cout << endl;
        cout << "--------MENU--------" << endl;
        cout << "[1] Start chatting" << endl;
        cout << "[2] View commands" << endl;
        cout << "[3] Exit " << endl;
        cout << "[4] View profile" << endl;
        cout << "--------------------" << endl;

        int choice; 
        cout << "Enter the number >> "; 
        cin >> choice;

        // Очиcтка потока ввода после выбора
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        switch (choice) {
            case 1: {
                {
                    lock_guard<mutex> lock(menu_mutex);
                    is_chatting = true; // Сон меню
                }
                thread ChatThread(Chat, client_socket);
                ChatThread.detach();
                break;
            }
            case 2:
                ShowCommands();
                break;
            case 3:
                main_thread_is_alive = false;
                menu_thread_is_alive = false;
                menu_cv.notify_all(); // Будим меню для выхода
                break;
            default:
                cout << "[ERROR] Wrong number" << endl;
                break;
        }
    }
}

int main(){

    char buffer[1024];

    WSADATA wsadata;
    int result = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (result != 0){

        cout << "An error occured with initializing WinSock, try restarting the programm: [" << WSAGetLastError() << "]" << endl;
        return 1;
    }

    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (client_socket == INVALID_SOCKET){

        cout << "An error occured with creating socket: [" << WSAGetLastError() << "]";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress{};

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);

    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);

    if (connect(client_socket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR){
        
        cout << "An error occured with connecting to server: [" << WSAGetLastError() << "]" << endl;
    }
    
    int bytes = recv(client_socket, buffer, sizeof(buffer), 0);
    string received(buffer, bytes);
    if (received.length() == 5) client_id = received;

    // Поток для приема сообщений от сервера
    thread receiveThread(ReceiveMessages, client_socket);
    receiveThread.detach();

    // Поток для отправки
    thread MenuThread(MainMenu, client_socket);
    MenuThread.detach();

    // Чтобы программа не закрылась сразу
    while(main_thread_is_alive) {
        this_thread::sleep_for(chrono::seconds(1));
    }   
}