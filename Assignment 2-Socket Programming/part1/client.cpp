#include <iostream>
#include <map>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "json.hpp"
#include <chrono>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

#define BUFFER_SIZE 4096

using namespace std;
using json = nlohmann::json;

// Function to read config from a JSON file
json readConfig(const string& filename) {
    ifstream config_file(filename);
    if (!config_file.is_open()) {
        cerr << "[CLIENT] Failed to open config file: " << filename << endl;
        exit(EXIT_FAILURE);
    }
    json config;
    config_file >> config;
    return config;
}

// Function to dump word frequencies into a file
void dumpWordFrequencies(const map<string, int>& word_count, const string& filename) {
    ofstream out_file(filename);
    if (!out_file.is_open()) {
        cerr << "[CLIENT] Failed to open file for dumping word counts: " << filename << endl;
        return;
    }

    for (const auto& entry : word_count) {
        if (entry.first != "EOF" && entry.first != "$$") { // Exclude EOF and $$
            out_file << entry.first << ", " << entry.second << endl;
        }
    }

    out_file.close();
    cout << "[CLIENT] Word frequencies dumped into file: " << filename << endl;
}

int main() {
    cout << "[CLIENT] Starting client..." << endl;

    // Load configuration from config.json
    json config = readConfig("config.json");
    string server_ip = config["server_ip"];
    int server_port = config["server_port"];
    int k = config["k"];
    int p = config["p"];

    cout << "[CLIENT] Loaded configuration: server_ip=" << server_ip 
         << ", server_port=" << server_port 
         << ", k=" << k 
         << ", p=" << p << endl;

    string output_file = "output.txt";

    // Create TCP socket
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "[CLIENT] Socket creation error" << endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, server_ip.c_str(), &serv_addr.sin_addr) <= 0) {
        cerr << "[CLIENT] Invalid address/ Address not supported" << endl;
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "[CLIENT] Connection Failed" << endl;
        return -1;
    }

    cout << "[CLIENT] Connected to server at " << server_ip << ":" << server_port << endl;

    int offset = 0;
    map<string, int> word_count;
    bool done = false;
    auto start_time = chrono::high_resolution_clock::now();

    string leftover = "";  // To handle incomplete words from the previous chunk

    while (!done) {
        // Prepare and send the request with the current offset
        string request = to_string(offset) + "\n";
        send(sock, request.c_str(), request.size(), 0);
        cout << "[CLIENT] Sent request for offset: " << offset << endl;

        // Clear buffer and receive data
        memset(buffer, 0, BUFFER_SIZE);
        string response = "";

        // Receive words in chunks (p words at a time)
        int word_count_in_chunk = 0;
        while (true) {
            memset(buffer, 0, BUFFER_SIZE);
            int valread = recv(sock, buffer, BUFFER_SIZE - 1, 0);
            if (valread <= 0) {
                // Connection closed or error
                cerr << "[CLIENT] Connection closed by server or error occurred." << endl;
                done = true;
                break;
            }
            buffer[valread] = '\0';  // Null-terminate the received string
            cout<<string(buffer);
            response = string(buffer);
            istringstream stream(response);
            string word;
            

            // Keep track of leftover data
            leftover = "";

            while (getline(stream, word, ',')) {  // Split by commas
                word.erase(remove_if(word.begin(), word.end(), ::isspace), word.end());  // Trim whitespace
                if (!word.empty() && word != "EOF" && word != "$$") {  // Exclude EOF and $$
                    word_count[word]++;
                    word_count_in_chunk++;
                }
            }

            // If we don't have enough words in the current chunk, save the remaining response
            if (word_count_in_chunk >= k) {
                // leftover = word;  // Save the leftover word if it's incomplete
                
                break;
            }
            // Check if the response contains EOF or $$
            if (response.find("EOF") != string::npos) {
                done = true;
                break;
            }
            if (response.find("$$") != string::npos) {
                cout << "[CLIENT] Received $$, stopping processing for this chunk." << endl;
                done = true;
                break;  // Don't mark `done`, just break this inner loop to send the next request
            }            

            
        }

        if (response.empty()) {
            cerr << "[CLIENT] Received an empty response. Terminating." << endl;
            break;
        }

        cout << "[CLIENT] Received words from server." << endl;

        if (done) {
            cout << "[CLIENT] EOF received from server. Closing connection." << endl;
            break;
        }

        // Increment the offset by k for the next request
        offset += k;
    }

    auto end_time = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed_time = end_time - start_time;

    // Dump word frequencies into the output file
    dumpWordFrequencies(word_count, output_file);

    cout << "[CLIENT] Time taken for p = " << p << ": " << elapsed_time.count() << " seconds" << endl;

    // Close the socket
    close(sock);
    cout << "[CLIENT] Connection closed." << endl;

    return 0;
}
