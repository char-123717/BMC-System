#include <iostream>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sstream>
#include <algorithm>
#include <vector>
#include <stdexcept>

// =========================
// Run shell script
// =========================
std::string runScript(const std::string& cmd) {
    char buffer[256];
    std::string result;

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";

    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }

    pclose(pipe);
    return result;
}

// =========================
// Safe trim
// =========================
std::string trim(std::string s) {
    s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());
    s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());
    return s;
}

// =========================
// Router (BMC core)
// =========================
std::string handleCommand(const std::string& cmdRaw) {

    std::string cmd = trim(cmdRaw);
    std::ostringstream json;

    try {

        // CPU
        if (cmd == "GET_CPU") {
            std::string cpu = trim(runScript("./scripts/get_cpu.sh"));

            json << "{"
                 << "\"cmd\":\"GET_CPU\","
                 << "\"value\":" << cpu << ","
                 << "\"unit\":\"%\""
                 << "}";
        }

        // MEM
        else if (cmd == "GET_MEM") {
            std::string mem = trim(runScript("bash ./scripts/get_mem.sh"));

            json << "{"
                 << "\"cmd\":\"GET_MEM\"," 
                 << "\"value\":" << (mem.empty() ? "null" : mem) << ","
                 << "\"unit\":\"%\""
                 << "}";
        }

        // TEMP
        else if (cmd == "GET_TEMP") {
            std::string temp = trim(runScript("bash ./scripts/get_temp.sh"));

            json << "{"
                 << "\"cmd\":\"GET_TEMP\",";

            if (temp.empty() || temp == "N/A") {
                json << "\"value\":\"N/A\",";
            } else {
                json << "\"value\":" << temp << ",";
            }

            json << "\"unit\":\"C\""
                 << "}";
        }

        // DISK
        else if (cmd == "GET_DISK") {
            std::string disk = trim(runScript("bash ./scripts/get_disk.sh"));

            json << "{"
                 << "\"cmd\":\"GET_DISK\"," 
                 << "\"value\":\"" << disk << "\""
                 << "}";
        }

        // UPTIME
        else if (cmd == "GET_UPTIME") {
            std::string up = trim(runScript("bash ./scripts/get_uptime.sh"));

            json << "{"
                 << "\"cmd\":\"GET_UPTIME\"," 
                 << "\"value\":\"" << up << "\""
                 << "}";
        }

        // UNKNOWN
        else {
            json << "{"
                 << "\"error\":\"UNKNOWN_COMMAND\""
                 << "}";
        }

    } catch (...) {
        json.str("");
        json << "{\"error\":\"EXCEPTION\"}";
    }

    return json.str();
}

// =========================
// Read line (IMPORTANT FIX)
// =========================
std::string readLine(int socket) {
    std::string cmd;
    char c;

    while (true) {
        int n = read(socket, &c, 1);
        if (n <= 0) break;

        if (c == '\n') break;
        cmd += c;
    }

    return cmd;
}

// =========================
// MAIN SERVER
// =========================
int main() {

    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8888);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);

    std::cout << "🔥 BMC Mini Server running on port 8888...\n";

    while (1) {

        new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);

        while (true) {

            std::string cmd = readLine(new_socket);

            if (cmd.empty()) break;

            std::string response = handleCommand(cmd);

            response += "\n";

            send(new_socket, response.c_str(), response.size(), 0);
        }

        close(new_socket);
    }

    return 0;
}