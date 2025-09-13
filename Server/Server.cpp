
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <string>
#include <sstream>
#include <algorithm>

#pragma comment(lib, "Ws2_32.lib")

struct Todo {
    int id;
    std::string description;
    bool completed;
};

static std::vector<Todo> g_items;
static std::vector<SOCKET> g_clients;
static std::mutex g_mutex;
static int g_next_id = 1;
static const unsigned short SERVER_PORT = 5000;

std::string escape_json_string(const std::string& s) {
    std::ostringstream o;
    for (auto c : s) {
        switch (c) {
        case '\\': o << "\\\\"; break;
        case '\"': o << "\\\""; break;
        case '\b': o << "\\b";  break;
        case '\f': o << "\\f";  break;
        case '\n': o << "\\n";  break;
        case '\r': o << "\\r";  break;
        case '\t': o << "\\t";  break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                o << "\\u" << std::hex << (int)c;
            }
            else {
                o << c;
            }
        }
    }
    return o.str();
}

std::string build_full_list_json() {
    std::lock_guard<std::mutex> lock(g_mutex);
    std::ostringstream oss;
    oss << "{\"type\":\"FULL_LIST\",\"items\":[";
    for (size_t i = 0; i < g_items.size(); ++i) {
        const auto& it = g_items[i];
        oss << "{\"id\":" << it.id
            << ",\"description\":\"" << escape_json_string(it.description) << "\""
            << ",\"completed\":" << (it.completed ? "true" : "false")
            << "}";
        if (i + 1 < g_items.size()) oss << ",";
    }
    oss << "]}";
    oss << '\n'; // newline-terminated
    return oss.str();
}

void broadcast_full_list() {
    std::string msg = build_full_list_json();
    std::lock_guard<std::mutex> lock(g_mutex);
    std::vector<SOCKET> to_remove;
    for (SOCKET s : g_clients) {
        if (s == INVALID_SOCKET) continue;
        int ret = send(s, msg.c_str(), (int)msg.size(), 0);
        if (ret == SOCKET_ERROR) {
            to_remove.push_back(s);
        }
    }
    for (SOCKET s : to_remove) {
        closesocket(s);
        g_clients.erase(std::remove(g_clients.begin(), g_clients.end(), s), g_clients.end());
    }
}

void handle_line_from_client(SOCKET client, const std::string& line) {
    // protocol: GET | ADD:<desc> | TOGGLE:<id>
    if (line == "GET") {
        std::string msg = build_full_list_json();
        send(client, msg.c_str(), (int)msg.size(), 0);
        return;
    }
    if (line.rfind("ADD:", 0) == 0) {
        std::string desc = line.substr(4);
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            Todo t;
            t.id = g_next_id++;
            t.description = desc;
            t.completed = false;
            g_items.push_back(std::move(t));
            std::cout << "[+] Added item id=" << g_next_id - 1 << " desc=\"" << desc << "\"\n";
        }
        broadcast_full_list();
        return;
    }
    if (line.rfind("TOGGLE:", 0) == 0) {
        std::string idstr = line.substr(7);
        try {
            int id = std::stoi(idstr);
            bool changed = false;
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                for (auto& it : g_items) {
                    if (it.id == id) {
                        it.completed = !it.completed;
                        changed = true;
                        break;
                    }
                }
            }
            if (changed) {
                std::cout << "[~] Toggled item id=" << id << "\n";
                broadcast_full_list();
            }
        }
        catch (...) {
            // ignore malformed
        }
        return;
    }
    // unknown -> ignore
}

void client_thread(SOCKET clientSocket) {
    {
        // Add to clients list
        std::lock_guard<std::mutex> lock(g_mutex);
        g_clients.push_back(clientSocket);
    }
    // Send initial list
    std::string init = build_full_list_json();
    send(clientSocket, init.c_str(), (int)init.size(), 0);

    const int BUF_SIZE = 1024;
    char buf[BUF_SIZE];
    std::string readbuf;
    while (true) {
        int bytes = recv(clientSocket, buf, BUF_SIZE, 0);
        if (bytes <= 0) break; // disconnected or error
        readbuf.append(buf, buf + bytes);
        size_t pos;
        while ((pos = readbuf.find('\n')) != std::string::npos) {
            std::string line = readbuf.substr(0, pos);
            // trim CR if present
            if (!line.empty() && line.back() == '\r') line.pop_back();
            readbuf.erase(0, pos + 1);
            handle_line_from_client(clientSocket, line);
        }
    }

    // cleanup
    closesocket(clientSocket);
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_clients.erase(std::remove(g_clients.begin(), g_clients.end(), clientSocket), g_clients.end());
    }
    std::cout << "[i] Client disconnected\n";
}

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "socket() failed\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
    service.sin_port = htons(SERVER_PORT);

    if (bind(listenSocket, (sockaddr*)&service, sizeof(service)) == SOCKET_ERROR) {
        std::cerr << "bind() failed\n";
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "listen() failed\n";
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server listening on port " << SERVER_PORT << " ...\n";

    while (true) {
        sockaddr_in clientAddr;
        int addrlen = sizeof(clientAddr);
        SOCKET clientSocket = accept(listenSocket, (sockaddr*)&clientAddr, &addrlen);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "accept() failed\n";
            continue;
        }
        std::cout << "[i] Client connected\n";
        std::thread t(client_thread, clientSocket);
        t.detach();
    }

    // unreachable here
    closesocket(listenSocket);
    WSACleanup();
    return 0;
}