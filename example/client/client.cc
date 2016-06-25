#include "../../client.h"
#include <unistd.h>

int main() {
    struct User u = {"daiki", "passwd"};
    struct Will w = {"w-topic", "w-message", false, 1};
    Client* c = new Client("GS-ID", &u, 7, &w);
    c->connect("127.0.0.1", 8883, false);
    usleep(1000000);
    return 0;
}
