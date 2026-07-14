#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <queue>
#include <atomic>
#include <cstring>
#include <map>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
#endif

struct OnionLayer {
    std::string nextHop;
    std::string encryptedData;
};

struct OnionPacket {
    std::string targetNode;
    std::queue<OnionLayer> layers; 
    std::string payload; 
};

class CyberVirtualMachine {
private:
    double tokens = 84.50;
    int vanishedObjects = 1;
    std::atomic<bool> running{true};
    std::string lastNetworkMessage = "Waiting for encrypted packet...";

    // نظام الملفات الافتراضي (Virtual File System)
    std::map<std::string, std::string> virtualFiles = {
        {"onion_config.sys", "NET_MODE=ONION-7\nTARGET_PORT=7000\nDEFAULT_HOP=node_FR"},
        {"private.key", "0x9F82C3D4A1B2E3F4... [ENCRYPTED AES-256]"},
        {"network_nodes.log", "node_FR: Active\nnode_DE: Active\nnode_IS: Active"}
    };

#ifdef _WIN32
    SOCKET listenSocket = INVALID_SOCKET;
#else
    int listenSocket = -1;
#endif

    std::string simulateDecrypt(const std::string& data) {
        std::string decrypted = data;
        for(char &c : decrypted) if(c != ' ') c--;
        return decrypted;
    }

public:
    std::string simulateEncrypt(const std::string& data) {
        std::string encrypted = data;
        for(char &c : encrypted) if(c != ' ') c++;
        return encrypted;
    }

    ~CyberVirtualMachine() {
        stopNetwork();
    }

    void renderUI() {
        std::cout << "\033[H\033[J"; 
        std::cout << "\033[1;36m┌────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐\033[0m\n";
        std::cout << "\033[1;36m│\033[0m ◉ AFNANDQ//RECEPTION      ⚙ TOKEN: " << std::fixed << std::setprecision(2) << tokens << " HK                ❖ NET: ONION-7 (PORT: 7000)                                \033[1;36m│\033[0m\n";
        std::cout << "\033[1;36m└────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘\033[0m\n";
        
        std::cout << "\033[1;36m┌───────────────────────┐┌──────────────────────────────────────────────┐┌──────────────────────────────────────────────┐\033[0m\n";
        std::cout << "\033[1;36m│\033[0m LOBBY                 \033[1;36m││\033[0m ROOM MANAGER                                 \033[1;36m││\033[0m SYS METRICS                                   \033[1;36m│\033[0m\n";
        std::cout << "\033[1;36m│\033[0m                       \033[1;36m││\033[0m                                              \033[1;36m││\033[0m                                              \033[1;36m│\033[0m\n";
        std::cout << "\033[1;36m│\033[0m ◉ room_svc            \033[1;36m││\033[0m ■ Public   ■ Private   ■ Paid                 \033[1;36m││\033[0m CPU [\033[32m████\033[0m░░░░░░] 40%                             \033[1;36m│\033[0m\n";
        std::cout << "\033[1;36m│\033[0m ○ corridor_a          \033[1;36m││\033[0m                                              \033[1;36m││\033[0m MEM [\033[32m██████\033[0m░░░░] 60%                             \033[1;36m│\033[0m\n";
        std::cout << "\033[1;36m│\033[0m                       \033[1;36m││\033[0m ROOM 0x4A2F                                  \033[1;36m││\033[0m ZVM [\033[32m██\033[0m░░░░░░░░] 20%                             \033[1;36m│\033[0m\n";
        std::cout << "\033[1;36m│───────────────────────││\033[0m TTL: 00:08:12                                \033[1;36m││\033[0m                                              \033[1;36m│\033[0m\n";
        std::cout << "\033[1;36m│\033[0m CORRIDORS             \033[1;36m││\033[0m Vanished Objects: " << vanishedObjects << "                          \033[1;36m││\033[0m HEAT 52°C                                     \033[1;36m│\033[0m\n";
        std::cout << "\033[1;36m│\033[0m ↳ FR → DE             \033[1;36m││\033[0m                                              \033[1;36m││\033[0m                                              \033[1;36m│\033[0m\n";
        std::cout << "\033[1;36m│\033[0m ↳ DE → IS             \033[1;36m││\033[0m                                              \033[1;36m││\033[0m LATEST LIVE INBOUND TRAFFIC:                  \033[1;36m│\033[0m\n";
        std::cout << "\033[1;36m│\033[0m                       \033[1;36m││\033[0m                                              \033[1;36m││\033[0m ↳ " << lastNetworkMessage.substr(0, 42) << "   \033[1;36m│\033[0m\n";
        std::cout << "\033[1;36m└───────────────────────┘└──────────────────────────────────────────────┘└──────────────────────────────────────────────┘\033[0m\n";
    }

    void executeOnionRoute(const std::string& finalMessage) {
        OnionPacket packet;
        packet.targetNode = "node_IS";
        
        std::string layer3_data = simulateEncrypt(finalMessage);
        std::string layer2_data = simulateEncrypt("FORWARD TO node_IS | DATA: " + layer3_data);
        std::string layer1_data = simulateEncrypt("FORWARD TO node_DE | DATA: " + layer2_data);

        packet.layers.push({ "node_FR", layer1_data });
        packet.layers.push({ "node_DE", layer2_data });
        packet.layers.push({ "node_IS", layer3_data });

        std::cout << "\n\033[1;33m[NET-CORE] Live Onion Mesh Packet Detected! Peeling layers...\033[0m\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(600));

        while(!packet.layers.empty()) {
            OnionLayer currentLayer = packet.layers.front();
            packet.layers.pop();

            std::cout << "\033[1;35m[DECRYPT] Processing hop at: " << currentLayer.nextHop << "\033[0m\n";
            std::cout << "          [RAW DATA]: " << currentLayer.encryptedData.substr(0, 45) << "...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            std::string peeledData = simulateDecrypt(currentLayer.encryptedData);
            std::cout << "          [DECRYPT LAYER OK] \033[1;32m✔\033[0m\n";
            
            if(packet.layers.empty()) {
                std::cout << "\033[1;32m[SUCCESS] Inbound network packet fully peeled! Payload: \"" << peeledData << "\"\033[0m\n";
                lastNetworkMessage = "Last Decrypted Payload: " + peeledData;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(400));
        }
    }

    void startNetworkListener() {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return;
#endif
        struct addrinfo hints, *result = NULL;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        if (getaddrinfo(NULL, "7000", &hints, &result) != 0) return;

#ifdef _WIN32
        listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
        listen(listenSocket, SOMAXCONN);
#else
        listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        int opt = 1; setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        bind(listenSocket, result->ai_addr, result->ai_addrlen);
        listen(listenSocket, 10);
#endif
        freeaddrinfo(result);

        std::thread([this]() {
            while (running) {
#ifdef _WIN32
                SOCKET clientSocket = accept(listenSocket, NULL, NULL);
                if (clientSocket == INVALID_SOCKET) break;
#else
                int clientSocket = accept(listenSocket, NULL, NULL);
                if (clientSocket < 0) break;
#endif
                char recvbuf[512] = {0};
                int iResult = recv(clientSocket, recvbuf, 511, 0);
                if (iResult > 0) {
                    std::string rawData(recvbuf);
                    
                    if(rawData.rfind("POST", 0) != 0 && rawData.rfind("GET", 0) != 0) {
                        std::cout << "\n\n\033[1;31m[ALERT] Inbound raw data stream intercepted on Port 7000!\033[0m\n";
                        executeOnionRoute(rawData);
                        std::cout << "\n[SYS] Press ENTER to return to active shell... ";
                    } else {
                        lastNetworkMessage = "Dropped insecure HTTP protocol attempt.";
                        std::string reply = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nOnion-7 Reject";
                        send(clientSocket, reply.c_str(), (int)reply.length(), 0);
                    }
                }
#ifdef _WIN32
                closesocket(clientSocket);
#else
                close(clientSocket);
#endif
            }
        }).detach();
    }

    void stopNetwork() {
        running = false;
#ifdef _WIN32
        if (listenSocket != INVALID_SOCKET) closesocket(listenSocket);
        WSACleanup();
#else
        if (listenSocket != -1) close(listenSocket);
#endif
    }

    void handleCommand(const std::string& input) {
        if (input.rfind("pay ", 0) == 0 && input.back() == ';') {
            size_t pos = input.find("HK");
            if (pos != std::string::npos) {
                double amount = std::stod(input.substr(4, pos - 4));
                tokens -= amount;
            }
        } else if (input == "vanish;") {
            vanishedObjects++;
        } else if (input.rfind("send ", 0) == 0 && input.back() == ';') {
            std::string msg = input.substr(5, input.length() - 6);
            executeOnionRoute(msg);
            std::cout << "\n[SYS] Press ENTER to refresh core UI...";
            std::cin.get();
        } 
        // الأوامر الجديدة المضافة للـ Shell
        else if (input == "ls;") {
            std::cout << "\n\033[1;32m[VFS] Listing Secure Virtual Storage:\033[0m\n";
            for (auto const& [name, content] : virtualFiles) {
                std::cout << "  ↳ " << std::left << std::setw(20) << name << " (" << content.length() << " bytes)\n";
            }
            std::cout << "\n[SYS] Press ENTER to return...";
            std::cin.get();
        } else if (input.rfind("cat ", 0) == 0 && input.back() == ';') {
            std::string filename = input.substr(4, input.length() - 5);
            std::cout << "\n\033[1;32m[VFS] Reading: " << filename << "\033[0m\n";
            if (virtualFiles.find(filename) != virtualFiles.end()) {
                std::cout << "----------------------------------------\n" << virtualFiles[filename] << "\n----------------------------------------\n";
            } else {
                std::cout << "\033[1;31m[ERROR] File not found in virtual storage.\033[0m\n";
            }
            std::cout << "\n[SYS] Press ENTER to return...";
            std::cin.get();
        } else if (input == "ps;") {
            std::cout << "\n\033[1;34m[SYS] Active Z-VM Process Threads:\033[0m\n";
            std::cout << "  PID    THREAD NAME          STATUS       PORT/RESOURCE\n";
            std::cout << "  1001   Core_UI_Render       RUNNING      Console_Stdout\n";
            std::cout << "  1002   Onion7_TCP_Listener  LISTENING    TCP/7000\n";
            std::cout << "  1003   VFS_Map_Manager      IDLE         RAM_Static\n";
            std::cout << "\n[SYS] Press ENTER to return...";
            std::cin.get();
        } else if (input == "help;") {
            std::cout << "\n\033[1;33m[HELP] Available Z-Shell Core Commands:\033[0m\n";
            std::cout << "  • pay [amount] HK;  - Deduct virtual network tokens\n";
            std::cout << "  • vanish;           - Increment vanished objects metric\n";
            std::cout << "  • send [message];   - Route a custom mock onion packet\n";
            std::cout << "  • ls;               - List files in Virtual File System (VFS)\n";
            std::cout << "  • cat [filename];   - View contents of a virtual file\n";
            std::cout << "  • ps;               - View status of active system threads\n";
            std::cout << "  • exit;             - Kill Z-VM session and close port\n";
            std::cout << "\n[SYS] Press ENTER to return...";
            std::cin.get();
        }
    }
};

int main() {
    CyberVirtualMachine zvm;
    zvm.startNetworkListener(); 
    std::string command;

    std::string sample = "Secret_Payload_Passed_Through_Network_Successfully";
    std::string encryptedSample = zvm.simulateEncrypt(sample);

    while (true) {
        zvm.renderUI();
        std::cout << "\033[1;33m[TEST GUIDE] To inject raw packet, run this in another CMD:\033[0m\n";
        std::cout << "  powershell \"\\\"" << encryptedSample << "\\\" | Out-String | nc 127.0.0.1 7000\"\n";
        std::cout << "────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────\n";
        std::cout << "concierge> ";
        std::getline(std::cin, command);
        if (command == "exit;") break;
        zvm.handleCommand(command);
    }
    return 0;
}