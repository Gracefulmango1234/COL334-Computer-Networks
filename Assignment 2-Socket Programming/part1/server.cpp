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

#define BUFFER_SIZE 1024

using namespace std;
using json = nlohmann::json;

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
    if (words.size() > 0 && words.back() == "\n") {
        words.pop_back();
    }
    return words;
}

int main() {
    // Load config from config.json using json
    json config;
    ifstream config_file("config.json", ifstream::binary);
    if (!config_file.is_open()) {
        cerr << "Error: Unable to open config.json" << endl;
        return 1;
    }
    config_file >> config;

    const string server_ip = config["server_ip"].get<string>();
    const int port = config["server_port"].get<int>();
    const string filename = config["input_file"].get<string>();
    int p = config["p"].get<int>();
    int k = config["k"].get<int>();
    int num_clients = config["num_clients"].get<int>();

    // Log server configuration
    cout << "Starting server on IP " << server_ip << " and port " << port << endl;
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
    server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());  // Use server_ip from config
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
    cout << "Server is listening on " << server_ip << ":" << port << endl;

    // Accept multiple clients (based on num_clients in config)
    while (true) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            cerr << "Error: Accept failed" << endl;
            continue;
        }
        cout << "Client connected" << endl;

        char buffer[BUFFER_SIZE];
        
        // Process client requests
        int cnt = 0;
        while (recv(client_fd, buffer, BUFFER_SIZE, 0) > 0) {
            buffer[strcspn(buffer, "\n")] = 0; // Remove newline character from the buffer
            string request(buffer);

            // Attempt to convert request to an integer (offset)
            int offset = stoi(request);

            cout << "Received request for offset: " << offset << endl;

            // Check if offset is within bounds
            if (offset >= words.size()) {
                cout << "Offset " << offset << " exceeds file size. Sending $$." << endl;
                send(client_fd, "$$\n", 3, 0);
            } 
            else {
                string response = "";
                int count = 0;

                for (int i = offset; i < offset + k && i < words.size(); i++) {
                    response += words[i] + ",";
                    count++;
                    cnt ++;
                    if (count >= p) {
                        if (i + 1 >= words.size()) {
                            response += "EOF";
                        }
                        response += "\n";
                        send(client_fd, response.c_str(), response.size(), 0);
                        count = 0; // Reset count
                        cout<<response;
                        response = "";
                    }
                }
                if (count > 0){
                    if (cnt + 1 >=  words.size()) response += "EOF";
                    response += "\n";
                    cout<<cnt<<endl;
                    cout<<response;
                    send(client_fd, response.c_str(), response.size(), 0);
                }
            }

            // Clear the buffer for the next iteration
            memset(buffer, 0, BUFFER_SIZE);  // Clear buffer
        }

        cout << "Client disconnected" << endl;
        close(client_fd);
    }

    close(server_fd);
    return 0;
}
