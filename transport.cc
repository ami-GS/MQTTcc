#include <stdint.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "transport.h"
#include "frame.h"


Transport::Transport(int sock, struct sockaddr_in* target) {
    memset(readBuff, 0, 65535);
    memset(writeBuff, 0, 65535);
    this->sock = sock;
    this->target = target;
}

Transport::Transport(const std::string targetIP, const int targetPort) {
    memset(readBuff, 0, 65535);
    memset(writeBuff, 0, 65535);
    this->sock = socket(AF_INET, SOCK_STREAM, 0);
    this->target->sin_family = AF_INET;
    this->target->sin_port = htons(targetPort);
    this->target->sin_addr.s_addr = inet_addr(targetIP.c_str());
}

int64_t Transport::sendMessage(Message* m) {
    uint64_t len = m->getWire(writeBuff);
    if (len == -1) {
        return -1;
    }
    write(sock, writeBuff, len);
    delete m; // TODO: this is little fast, need to wait ack
    return len;
}

MQTT_ERROR Transport::readMessage() {
     int64_t status = read(sock, readBuff, sizeof(readBuff));
    if (status == -1) {
        /* Error, check errno, take action... */
    } else if (status == 0) {
        /* Peer closed the socket, finish the close */
        close( sock );
        /* Further processing... */
        //return EOF;
    }
    return NO_ERROR;
}
