#include <iostream>
#include <fstream>
#include <vector>
#include <dirent.h> // For directory operations
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>

using namespace std;

vector<string> getXmlFiles(const string& directory) {
    vector<string> files;
    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(directory.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            string file = ent->d_name;
            if (file.find(".xml") != string::npos) {
                files.push_back(directory + "/" + file);
            }
        }
        closedir(dir);
    } else {
        perror("opendir"); // This will print why opendir failed
        cerr << "Could not open directory: " << strerror(errno) << endl;

        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
        fprintf(stdout, "Current working dir: %s\n", cwd);
        } else {
        perror("getcwd");
        }
    }
    return files;
}

string readFileContents(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Could not open file: " << filename << endl;
        return "";
    }
    return string((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
}

string recvMessage(int client_socket_fd) {
    const int BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];

    int bytes_received = recv(client_socket_fd, buffer, BUFFER_SIZE, 0);
    if (bytes_received == -1) {
        cerr << "Error: cannot receive message from client" << endl;
        exit(-1);
    }

    string message(buffer, bytes_received);

    size_t newline_pos = message.find('\n');
    if (newline_pos == string::npos) {
        cerr << "Error: invalid message format from client" << endl;
        exit(-1);
    }
    string length_str = message.substr(0, newline_pos);
    unsigned int length = stoi(length_str);

    string xml_message = message.substr(newline_pos + 1, length);

    return xml_message;
}

int main() {
    // Directory where XML files are located
    string directory = "test_resources";
    vector<string> files = getXmlFiles(directory);

    for (const string& file : files) {
        int client_socket_fd;
        struct sockaddr_in server_addr;

        client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket_fd == -1) {
            cerr << "Error: cannot create socket" << endl;
            return 1;
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(12345); 
        inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
        if (connect(client_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            cerr << "Error: cannot connect to server" << endl;
            close(client_socket_fd);
            return 1;
        }
        printf("Sending file: %s\n", file.c_str());
        string message = readFileContents(file);
        int bytes_sent = send(client_socket_fd, message.c_str(), message.length(), 0);
        if (bytes_sent == -1) {
            cerr << "Error: cannot send message to server" << endl;
            continue;
        }
        cout << "Message sent to server from file: " << file << endl;

        string xmlResponse = recvMessage(client_socket_fd);
        cout << xmlResponse;
        close(client_socket_fd);
    }

    
    return 0;
}
