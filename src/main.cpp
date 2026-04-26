#include <iostream>
#include <cstdio>
#include <memory>
#include <array>

std::string runScript(const std::string& cmd) {
    std::array<char, 128> buffer;
    std::string result;

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "ERROR";

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }

    pclose(pipe);
    return result;
}

int main(int argc, char* argv[]) {

    // 檢查有沒有輸入指令
    if (argc < 2) {
        std::cout << "Usage: ./bmc <COMMAND>\n";
        return 1;
    }

    std::string cmd = argv[1];

    if (cmd == "GET_CPU") {
        std::string cpu = runScript("./scripts/get_cpu.sh");
        std::cout << "CPU Usage: " << cpu << "%\n";
    }
    else if (cmd == "GET_MEM") {
        std::string mem = runScript("free -m | grep Mem | awk '{print $3/$2 * 100.0}'");
        std::cout << "Memory Usage: " << mem << "%\n";
    }
    else {
        std::cout << "Unknown Command\n";
    }

    return 0;
}