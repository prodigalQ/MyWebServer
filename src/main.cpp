#include "../include/webserver.h"
//bugs in httprocess--parse
int main(){
    WebServer webserver(9999, 60000, 3, 8);
    webserver.run();
}