#include <iostream>
#include <map>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "json.hpp"  // For nlohmann::json
#include <chrono>
#include <fstream>
#include <thread>
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
        cerr << "Error: Unable to open config file " << filename << endl;
        exit(EXIT_FAILURE);
    }
    json config;
    config_file >> config;
    return config;
}

// Function to count and log the number of words received by the client
void run_client(const string& server_ip, int server_port, int k, int p, int client_id) {
    // Create TCP socket
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "[CLIENT " << client_id << "] Socket creation error" << endl;
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, server_ip.c_str(), &serv_addr.sin_addr) <= 0) {
        cerr << "[CLIENT " << client_id << "] Invalid address/Address not supported" << endl;
        return;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "[CLIENT " << client_id << "] Connection Failed" << endl;
        return;
    }

    cout << "[CLIENT " << client_id << "] Connected to server at " << server_ip << ":" << server_port << endl;

    int offset = 0;
    map<string, int> word_count;
    bool done = false;
    auto start_time = chrono::high_resolution_clock::now();

    while (!done) {
        // Prepare and send the request
        string request = to_string(offset) + "\n";
        send(sock, request.c_str(), request.size(), 0);
        cout << "[CLIENT " << client_id << "] Sent request for offset: " << offset << endl;

        // Clear buffer and receive data
        memset(buffer, 0, BUFFER_SIZE);
        string response = "";
        
        while (true) {
            int valread = recv(sock, buffer, BUFFER_SIZE - 1, 0);
            if (valread <= 0) {
                // Connection closed or error
                break;
            }
            buffer[valread] = '\0';
            response += string(buffer);
        
            // Check if "EOF" or "$$" is in the response
            if (response.find("EOF\n") != string::npos || response.find("$$\n") != string::npos) {
                done = true;
                break;
            }

            // If the server sends less than BUFFER_SIZE, assume end of current packet
            if (valread < BUFFER_SIZE - 1) {
                break;
            }
        }

        if (response.empty()) {
            cerr << "[CLIENT " << client_id << "] Received an empty response. Terminating." << endl;
            break;
        }

        // Process the response
        istringstream stream(response);
        string word;
        while (getline(stream, word, ',')) {  // Split by commas
            word.erase(remove_if(word.begin(), word.end(), ::isspace), word.end());  // Trim whitespace
            if (!word.empty() && word != "EOF" && word != "$$") {  // Exclude EOF and empty words
                word_count[word]++;
            }
        }

        cout << "[CLIENT " << client_id << "] Received words from server." << endl;

        // Increment the offset by k for the next request
        offset += k;
    }

    // Calculate total number of words received
    int total_words = 0;
    for (const auto& entry : word_count) {
        total_words += entry.second;
    }

    cout << "[CLIENT " << client_id << "] Total words received: " << total_words << endl;

    // Write word count to a file
    string filename = "output" + to_string(client_id) + ".txt";
    ofstream outfile(filename);
    if (!outfile.is_open()) {
        cerr << "[CLIENT " << client_id << "] Error: Unable to open file " << filename << " for writing." << endl;
    } 
    else{
        for (const auto& entry : word_count) {
            outfile << entry.first << ", " << entry.second << endl;
        }
        outfile.close();
        cout << "[CLIENT " << client_id << "] Word frequency written to " << filename << endl;
    }

    // Close the socket
    close(sock);
    cout << "[CLIENT " << client_id << "] Connection closed." << endl;
}

int main() {
    // Load configuration
    json config = readConfig("config.json");
    string server_ip = config["server_ip"].get<string>();
    int server_port = config["server_port"].get<int>();
    int k = config["k"].get<int>();
    int p = config["p"].get<int>();
    int num_clients = config["num_clients"].get<int>();

    // Create threads for each client
    vector<thread> client_threads;

    for (int i = 0; i < num_clients; ++i) {
        client_threads.push_back(thread(run_client, server_ip, server_port, k, p, i + 1));
    }

    // Wait for all client threads to finish
    for (auto &t : client_threads) {
        t.join();
    }

    return 0;
}
