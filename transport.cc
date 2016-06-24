#include <stdint.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "transport.h"
#include "frame.h"


Transport::Transport(int sock, struct sockaddr_in* target) {
    memset(this->readBuff, 0, 65535);
    memset(this->writeBuff, 0, 65535);
    this->sock = sock;
    this->target = target;
}

Transport::Transport(const std::string targetIP, const int targetPort) {
    memset(this->readBuff, 0, 65535);
    memset(this->writeBuff, 0, 65535);
    this->sock = socket(AF_INET, SOCK_STREAM, 0);
    this->target = new sockaddr_in();
    memset(&this->target->sin_addr, 0, sizeof(struct in_addr));
    this->target->sin_family = AF_INET;
    this->target->sin_port = htons(targetPort);
    this->target->sin_addr.s_addr = inet_addr(targetIP.c_str());
}

void Transport::connectTarget() {
    // TODO : do more detail
    connect(this->sock, (struct sockaddr *)this->target, sizeof(sockaddr));
}

MQTT_ERROR Transport::sendMessage(Message* m) {
    uint64_t len = m->getWire(this->writeBuff);
    if (len == -1) {
        // TODO : more detalied error
        return SEND_ERROR;
    }
    write(this->sock, this->writeBuff, len);
    if (m->fh->packetID > 0) {
        delete m;
    }
    return NO_ERROR;
}

MQTT_ERROR Transport::readMessage() {
     int64_t status = read(this->sock, this->readBuff, sizeof(this->readBuff));
    if (status == -1) {
        /* Error, check errno, take action... */
    } else if (status == 0) {
        /* Peer closed the socket, finish the close */
        close( this->sock );
        /* Further processing... */
        //return EOF;
    }
    return NO_ERROR;
}
