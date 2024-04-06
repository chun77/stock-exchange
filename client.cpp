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
        cerr << "Could not open directory" << endl;
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

int main() {
    // Directory where XML files are located
    string directory = "/home/zw297/project4/erss-hwk4-hg161-zw297/test_resources";
    vector<string> files = getXmlFiles(directory);

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

    for (const string& file : files) {
        printf("Sending file: %s\n", file.c_str());
        string message = readFileContents(file);
        int bytes_sent = send(client_socket_fd, message.c_str(), message.length(), 0);
        if (bytes_sent == -1) {
            cerr << "Error: cannot send message to server" << endl;
            continue;
        }
        cout << "Message sent to server from file: " << file << endl;
    }

    close(client_socket_fd);

    return 0;
}
