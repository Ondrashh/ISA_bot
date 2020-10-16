#include <openssl/ssl.h>
#include <stdio.h>
#include <memory.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <string.h>
#include <string>
#include <sstream>


using namespace std;
int main() {

    int err;
    struct sockaddr_in sa;
    SSL_CTX* ctx;
    SSL*     ssl;
    X509*    server_cert;
    char*    str;
    char     buf [100000];
    const SSL_METHOD *meth;

    struct hostent *lh = gethostbyname("discord.com");
    OpenSSL_add_ssl_algorithms();
    meth = TLSv1_2_client_method();
    SSL_load_error_strings();
    ctx = SSL_CTX_new (meth);  
    int sd = socket (AF_INET, SOCK_STREAM, 0); 
    if(sd == -1){
        printf("Chyba vytvoření socketu\n");
        exit(1);
    }

    memset(&sa, 0, sizeof(sa));
    sa.sin_family      = AF_INET;
    memcpy(&sa.sin_addr, lh->h_addr, lh->h_length);
    sa.sin_port        = htons     (443);          /* Server Port number */
  
    err = connect(sd, (struct sockaddr*) &sa,
		sizeof(sa));                   
    ssl = SSL_new (ctx);                          
    SSL_set_fd (ssl, sd);
    err = SSL_connect (ssl);                     
    
    string token_from_bot = "NzY0MTQ3MDc3NjgzODcxNzY1.X4CBbA.BJeeM1VuSY1sYbeEr-xO_zy61uw";
    stringstream stream;
    string config = "{\"content\":\"Ty zmrde\"}";
    stream << "POST /api/v6/channels/711233265578672179/messages HTTP/1.1\r\n"
       << "Content-Type: application/json\r\n"
       << "Host: discord.com\r\n"
       << "Authorization: Bot " << token_from_bot << "\r\n"
       << "Accept: application/json\r\n"
       << "Content-length: " << config.size() << "\r\n"
       << "Connection: close\r\n\r\n"
       << config << "\r\n\r\n";
    string request = stream.str();
    int len = 200;
    printf ("SSL connection using %s\n", SSL_get_cipher (ssl));
    err = SSL_write (ssl, request.c_str(), request.size());  
    string content;
    do{
        err = SSL_read (ssl, buf, 200);   
        buf[len] = 0;
        content.append(string(buf));          
        printf("%s\n",buf);
    } while (err > 0);        

    SSL_free (ssl);
    SSL_CTX_free (ctx);
}