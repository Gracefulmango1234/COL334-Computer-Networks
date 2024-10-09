#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <cstring>
#include "json.hpp"
#include <thread>
#include <atomic>

#define BUFFER_SIZE 1024
using json = nlohmann::json;
using namespace std;

atomic<int> client_count(0);  // Atomic counter for client numbers

// Function to split comma-separated words
vector<string> split_words(const string &str) {
    vector<string> words;
    size_t start = 0, end = 0;
    while ((end = str.find(',', start)) != string::npos) {
        words.push_back(str.substr(start, end - start));
        start = end + 1;
    }
    if (start < str.size()) {
        words.push_back(str.substr(start));
    }
    if (!words.empty() && words.back() == "\n") {
        words.pop_back();
    }
    return words;
}

// Function to handle each client
void handle_client(int client_fd, const vector<string>& words, int k, int p, int client_number) {
    char buffer[BUFFER_SIZE];
    
    cout << "Client #" << client_number << " connected." << endl;
    
    while (recv(client_fd, buffer, BUFFER_SIZE, 0) > 0) {
        buffer[strcspn(buffer, "\n")] = 0; // Remove newline character
        string request(buffer);

        // Attempt to convert request to an integer (offset)
        int offset = 0;
        try {
            offset = stoi(request);
        } catch (invalid_argument&) {
            cerr << "Client #" << client_number << " sent an invalid offset: " << request << endl;
            send(client_fd, "Invalid offset\n", 15, 0);
            continue;
        }

        cout << "Client #" << client_number << " requested offset: " << offset << endl;

        if (offset >= words.size()) {
            cout << "Client #" << client_number << " offset " << offset << " exceeds file size. Sending $$." << endl;
            send(client_fd, "$$\n", 3, 0);
        } else {
            string response = "";
            int count = 0;

            for (int i = offset; i < offset + k && i < words.size(); i++) {
                response += words[i] + ",";
                count++;
                if (count >= p) {
                    response += "\n";  // Add a newline after p words
                    count = 0;
                }
            }

            if (offset + k >= words.size()) {
                response += "EOF\n";
                cout << "Client #" << client_number << ": End of file reached. Sending EOF." << endl;
            }

            // Send the response
            send(client_fd, response.c_str(), response.size(), 0);
        }

        // Clear the buffer for the next iteration
        memset(buffer, 0, BUFFER_SIZE);  
    }

    cout << "Client #" << client_number << " disconnected." << endl;
    close(client_fd);
}

int main() {
    // Load config from config.json using nlohmann::json
    ifstream config_file("config.json");
    if (!config_file.is_open()) {
        cerr << "Error: Unable to open config.json" << endl;
        return 1;
    }

    json config;
    config_file >> config;

    int port = config["server_port"];
    string filename = config["input_file"];
    int p = config["p"];
    int k = config["k"];

    // Log server configuration
    cout << "Starting server on port " << port << endl;
    cout << "Serving file: " << filename << endl;
    cout << "Config: k = " << k << ", p = " << p << endl;

    // Read the file and split it into words
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Unable to open file " << filename << endl;
        return 1;
    }

    string file_content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    vector<string> words = split_words(file_content);
    cout << "File read successfully, total words: " << words.size() << endl;

    // Setup TCP socket
    int server_fd, client_fd;
    struct sockaddr_in server_addr;

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        cerr << "Error: Socket creation failed" << endl;
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Error: Binding failed" << endl;
        close(server_fd);
        return 1;
    }

    // Start listening
    if (listen(server_fd, 10) < 0) {
        cerr << "Error: Listening failed" << endl;
        close(server_fd);
        return 1;
    }
    cout << "Server is listening on port " << port << endl;

    while (true) {
        // Accept connection from client
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            cerr << "Error: Accept failed" << endl;
            continue;
        }
        
        int client_number = ++client_count;  // Increment and get client number
        
        // Spawn a new thread to handle each client
        thread client_thread(handle_client, client_fd, words, k, p, client_number);
        client_thread.detach();  // Detach thread to allow concurrent handling
    }

    close(server_fd);
    return 0;
}
