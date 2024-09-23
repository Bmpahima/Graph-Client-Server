#include "std_lib_facilities.h"
#include <queue>
#include <unordered_set>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * main
 * Entry point of the program. This program is a client that connects to a server
 * to request the shortest path between two vertices using a graph.
 * 
 * Parameters:
 *  - argc: The number of command-line arguments (should be 5: server IP, port, vertex1, vertex2).
 *  - argv: An array of command-line arguments.
 * 
 * The client sends the two vertices to the server, waits for a response, and prints the shortest path.
 * 
 * Returns:
 *  - int: Returns 0 upon successful execution, or 1 in case of an error.
 */
int main(int argc, char* argv[]) {

    // Extract server IP, port, and vertices from command-line arguments
    string server_ip = argv[1];
    int server_port = stoi(argv[2]);
    int vertex1 = stoi(argv[3]);
    int vertex2 = stoi(argv[4]);

    /*
     * Create a client socket
     * AF_INET: Address family for IPv4
     * SOCK_STREAM: Socket type for TCP
     * 0: Protocol type (default for TCP)
     */
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        std::cerr << "Error: Could not create socket\n";
        return 1;
    }

    /*
     * Prepare the server address structure
     * server_addr: Stores the server's IP address and port.
     * inet_addr: Converts the server IP string to a network address.
     * htons: Converts the port number to network byte order.
     */
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
    server_addr.sin_port = htons(server_port);

    /*
     * Connect to the server
     * The client attempts to establish a connection to the specified server IP and port.
     * If the connection fails, an error message is printed and the socket is closed.
     */
    if (connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error: Connection failed\n";
        close(client_socket);
        return 1;
    }

    /*
     * Prepare the message to send
     * The message contains the two vertices (vertex1 and vertex2) separated by a space.
     * This message is sent to the server to request the shortest path between the vertices.
     */
    string message_to_send = to_string(vertex1) + " " + to_string(vertex2);
    write(client_socket, message_to_send.c_str(), message_to_send.length());

    /*
     * Read the response from the server
     * The response contains the shortest path between the two vertices (if exists).
     * The response is stored in the arr buffer and printed to the console.
     */
    char arr[256];
    int bytes_received = read(client_socket, arr, sizeof(arr));

    // Null-terminate the received data to ensure it's a valid string
    arr[bytes_received] = '\0';

    // Output the result (the shortest path or error message)
    cout << arr << endl;

    // Close the socket to release resources
    close(client_socket);

    return 0;
}
