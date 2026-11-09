# Winsock C++ Console Messenger

A multithreaded client-server console messenger built with C++ and WinSock API for Windows.

## Features
- Client-server architecture using TCP sockets.
- Multithreading for handling multiple clients simultaneously.
- Thread synchronization using mutexes.
- Command processing (e.g., `--exit`, `--menu`).
- Message routing by client ID.

## Technologies Used
- **Language:** C++ (C++17 or later)
- **Networking:** WinSock2
- **Concurrency:** `std::thread`, `std::mutex`
- **Build System:** MSVC / Visual Studio

## How to Build and Run
1. Clone the repository:
   git clone https://github.com/Enncryptedd/Winsock-cpp-console-messenger.git
   
3. Open the solution in Visual Studio (or compile with g++ using -lws2_32).

3. Run server.exe first.

4. Run client.exe (you can run multiple clients in separate terminals).

Future Improvements

  Cross-platform support (Linux using epoll/select).

  Graphical User Interface (GUI).

  Database integration for message history.

Author

Enncryptedd (2026)
