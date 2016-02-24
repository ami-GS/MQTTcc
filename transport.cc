#include <stdint.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "transport.h"
#include "frame.h"


Transport::Transport(const std::string targetIP, int targetPort) {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    target.sin_family = AF_INET;
    target.sin_port = htons(targetPort);
    target.sin_addr.s_addr = inet_addr(targetIP.c_str());
}

int64_t Transport::sendMessage(Message* m) {
    uint64_t len = m->GetWire(writeBuff);
    if (len == -1) {
        return -1;
    }
    write(sock, writeBuff, len);
    return len;
}

int64_t Transport::readMessage(Message* m) {
    memset(readBuff, 0, sizeof(readBuff));
    int64_t status = read(sock, readBuff, sizeof(readBuff));

    if (status > 0) {
        /* Data read from the socket */
        int64_t len = GetMessage(readBuff, m);
        if (len == -1) {
            return -1;
        }
    } else if (status == -1) {
        /* Error, check errno, take action... */
    } else if (status == 0) {
        /* Peer closed the socket, finish the close */
        close( sock );
        /* Further processing... */
    }

    return status;
}