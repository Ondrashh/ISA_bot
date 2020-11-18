/* Projekt do předmětu ISA
*  Discord Bot
*  Vypracoval: Ondřej Pavlacký
*  VUT FIT   3BIT 2020/21   */


#include <openssl/ssl.h>
#include <stdio.h>
#include <memory.h>
#include <errno.h>
#include <algorithm>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <string.h>
#include <string>
#include <sstream>
#include <tuple>
#include <vector>
#include <stdio.h>      
#include <stdlib.h>  
#include <unistd.h>  

using namespace std;


// Funkce na uzavření ssl připojení a socketu
void closeSSL(SSL* ssl, int sd, SSL_CTX* ctx){

    SSL_shutdown (ssl);
    close(sd);
    SSL_free (ssl);
    //Tohle na merlinu padalo
    //SSL_CTX_free (ctx);

}

// Funkce na vytvoření SSL připojení a socketu
// Vrací vytvořené a aktivní SSL spojení
SSL* initSSL(struct sockaddr_in sa, SSL_CTX* ctx, SSL* ssl, X509*    server_cert, char* str, char buf [4096], const SSL_METHOD *meth, int sd){
    

    // Převzato z: https://github.com/openssl/openssl/blob/691064c47fd6a7d11189df00a0d1b94d8051cbe0/demos/ssl/cli.cpp
    // Autoři: Sampo Kellomaki, Wade Scholine
    // Nastavení požadovaných parametrů pro úspěšné použití SSL
    struct hostent *lh = gethostbyname("discord.com");
    int err;
    OpenSSL_add_ssl_algorithms();
    meth = TLSv1_2_client_method();
    SSL_load_error_strings();
    ctx = SSL_CTX_new (meth);

    // Kontrola správnosti vytvoření spojení 
    if(!ctx){
        std::cout << "Chyba při vytváření SSL kontextu\n";
        exit(101);
    }  
    if(sd == -1){
        std::cout << "Chyba vytvoření socketu\n";
        exit(101);
    }

    memset(&sa, 0, sizeof(sa));  
    sa.sin_family      = AF_INET;
    memcpy(&sa.sin_addr, lh->h_addr, lh->h_length);
    sa.sin_port        = htons     (443); //Pro SSL se používá port 443
  

    err = connect(sd, (struct sockaddr*) &sa, sizeof(sa));
    if(err == -1){
        std::cout << "Nepodařilo se otevřít socket\n";
        exit(101);
    }

    // Vytvoření SSL spojení                  
    ssl = SSL_new (ctx);
    if(ssl == NULL){
        std::cout << "Nepodařilo se otevřít SSL spojení\n";
        exit(101);
    }
                        
    SSL_set_fd (ssl, sd);
    err = SSL_connect (ssl);
    if(err == -1){
        std::cout << "Nepodařilo připojit přes SSL\n";
        exit(101);
    }
    return ssl;
}

// Funkce na rozparsování zpráv co nám vrátí server
// Vrací list zpráv
vector<string> ParseMessages(string received_message){

    //Parsuje po indexech zprávy
    int help;
    string bot = "bot";
    vector<string> messages {};
    string content = "";
    string isa_bot = "";
    string message_from_user = "";
    for(int i =0; i < received_message.length(); i++){
        if(received_message[i] == 'n'){
            help = i;
            help++;
            if(received_message[help] == 't'){
                help++;
                if(received_message[help] == 'e'){
                    help++;
                    if(received_message[help] == 'n'){
                        help++;
                        if(received_message[help] == 't'){
                            help++;
                            if(received_message[help] == '"'){
                                help++;
                                if(received_message[help] == ':'){
                                    help = help+3;
                                    while(received_message[help] != '"'){
                                        //<channel> - <username>: <message>
                                        content.push_back(received_message[help]);
                                        help++;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        if(received_message[i] == 'r'){
            i++;
            if(received_message[i] == 'n'){
                i++;
                if(received_message[i] == 'a'){
                    i++;
                    if(received_message[i] == 'm'){
                        i++;
                        if(received_message[i] == 'e'){
                            i++;
                            if(received_message[i] == '"'){
                                i++;
                                if(received_message[i] == ':'){
                                    i = i+3;
                                    while(received_message[i] != '"'){
                                        //<channel> - <username>: <message>
                                        message_from_user.push_back(received_message[i]);
                                        i++;
                                    }
                                    if(message_from_user.find(bot) == std::string::npos){
                                        isa_bot.append(message_from_user);
                                        isa_bot.push_back(' ');
                                        isa_bot.push_back('-');
                                        isa_bot.push_back(' ');
                                        isa_bot.append(content);
                                        messages.push_back(isa_bot);
                                    }
                                    message_from_user = "";
                                    isa_bot = ""; 
                                    content = "";
                                    
                                }
                            }
                
                        }
                    }
                }
            }
        }
    }

    return messages;
}

// Funkce na získání id poslední zprávy
// Vrací string v podobě id poslední zprávy
string GetLastMessageId(string received_message){

    string last_message_id = "";
    for(int i =0; i < received_message.length(); i++){
        if(received_message[i] == '"'){
            i++;
            if(received_message[i] == 'i'){
                 i++;
                if(received_message[i] == 'd'){
                    i++;
                    if(received_message[i] == '"'){
                        i++;
                        if(received_message[i] == ':'){
                            i++;
                            if(received_message[i] == ' '){
                                i++;
                                if(received_message[i] == '"'){
                                    i++;
                                    while(received_message[i] != '"'){
                                        last_message_id.push_back(received_message[i]);
                                        i++;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return last_message_id;
}

// Tady už posloucchá a posílá zprávy zpět na server
// Nekonečný cyklus, které pořád naslouchá
void BotTalk(bool verbose, string bot_token, string last_message_id, string room_id, struct sockaddr_in sa, SSL_CTX* ctx, SSL* ssl, X509* server_cert, char* str, char buf [4096], const SSL_METHOD *meth, int sd){
    
    int err;
    stringstream http_request_get_message_after;
    string last_get_id = "";
    while (true){

        sd = socket (AF_INET, SOCK_STREAM, 0); 
        ssl = initSSL(sa,ctx,ssl,server_cert,str,buf,meth,sd);

        //cout<<last_message_id << "\n";

        // Dotaz na získání zpráv po poslední kterou jse zachitil
        http_request_get_message_after.str("");

        // Dotaz na získání zprávy od určité zprávy
        http_request_get_message_after << "GET /api/v6/channels/"+ room_id + "/messages?after="+ last_message_id + " HTTP/1.1\r\n"
               << "Content-Type: application/json; charset=utf-8\r\n"
               << "Host: discord.com\r\n"
               << "Authorization: Bot " << bot_token << "\r\n"
               << "Accept: application/json\r\n"
               << "Connection: close\r\n\r\n";

        // Převedení na string
        string request = http_request_get_message_after.str();

        // Uspání pro omezení dotazů na servery discordu
        sleep(1);

        // Poslání dotazu
        err = SSL_write (ssl, request.c_str(), request.size());  
        if(err == -1){
            std::cout << "Nepodařilo se odeslat SSL požadavek\n";
            exit(101);
        }
        // String ve kterém budu ukládat příchozí zprávu ze serveru
        string received_message;
       
        // Přectení celé zprávy a uložení odpovědi
        while (err > 0){
            err = SSL_read (ssl, buf, sizeof(buf)-1);   
            buf[err] = '\0';
            received_message.append(string(buf));          
            
        }

        // Uzavření spojení 
        closeSSL(ssl, sd, ctx);
        
        // Zahození hlavničky, kterou stejně nevyužiji
        int help = received_message.find("\r\n\r\n");
        string just_content = received_message.erase(0, (help + 4));

        // Pokud přišla nějaká nová zpráva
        if(strcmp(last_message_id.c_str(), last_get_id.c_str()) != 0){
            
            // Rozparsování contentu (zpráv co mi přijde)
            vector<string> parsed_messages {};
            parsed_messages = ParseMessages(just_content);
            

            // Vezmu zprávy od zadu, protože chci od starších po nejnovější
            for(int i = parsed_messages.size()-1; i >= 0; i--){


                //std::cout<< parsed_messages[i] << "\n";

                stringstream http_post_message;
                string concat_msg = "";
                string print_msg = "";

                // Sestavení contentu, který bot pošle
                string content = "{\"content\":\"echo: " + parsed_messages[i] + "\"}";

                // Znovu inicializace SSL připojení
                sd = socket (AF_INET, SOCK_STREAM, 0); 
                ssl = initSSL(sa,ctx,ssl,server_cert,str,buf,meth,sd);
                // POST kde posílám na server už samotný content co chci aby bot vypsal
                http_post_message << "POST /api/v6/channels/"+ room_id + "/messages HTTP/1.1\r\n"
                    << "Content-Type: application/json\r\n"
                    << "Host: discord.com\r\n"
                    << "Authorization: Bot " << bot_token << "\r\n"
                    << "Accept: application/json\r\n"
                    << "Content-length: " << content.size() << "\r\n"
                    << "Connection: close\r\n\r\n"
                    << content << "\r\n\r\n";
                content = "";

                // Pokud je zapnutý přepínač verbose
                if(verbose){
                    print_msg = "isa-bot - ";
                    //aprint_msg.append(parsed_messages[1]);
                    int nInx = parsed_messages[i].find('-');
                    parsed_messages[i].erase(nInx-1,1);
                    parsed_messages[i][nInx-1] = ':';
                    std::cout<< print_msg << parsed_messages[i] << "\n";
                }
                // Posunutí se indexu poslední zprávy
                last_message_id = GetLastMessageId(just_content);
                
                //Zaslání zprávy zpět na server
                request = http_post_message.str();
            
                err = SSL_write (ssl, request.c_str(), request.size());  
                if(err == -1){
                    std::cout << "Nepodařilo se odeslat SSL požadavek\n";
                    exit(101);
                }
                // Čtení 
                while (err > 0){
                    err = SSL_read (ssl, buf, sizeof(buf)-1);   
                    buf[err] = '\0';
                    received_message.append(string(buf)); 
                }
                // Zavření spojení
                closeSSL(ssl, sd, ctx); 
                int help = received_message.find("\r\n\r\n");
                string just_content = received_message.erase(0, (help + 4));

                concat_msg = "";
                content = "";

                // Čekám aby mě discord nezamítl připojit se
                sleep(2);
            }
        }
    }
}

// Funkce na hledání kanálu #isa-bot a poslední zprávy
// Vrací dvojici id místnosti a poslední zprávy
tuple<string, string> FindIsaBotChannel(string received_message){

    string last_message_id = "";
    string nazev_roomky = "";
    string id_roomky = "";
    for(int i = 0; received_message[i] != '\0'; i++){
        char pismenko = received_message[i];
        if(received_message[i] == '"'){
            i++;
            if(received_message[i] == 'i'){
                i++;
                if(received_message[i] == 'd'){
                    i++;
                    if(received_message[i] == '"'){
                        i++;
                        if(received_message[i] == ':'){
                            i++;
                            if(received_message[i] == ' '){
                                i++;
                                if(received_message[i] == '"'){
                                    i++;
                                    if(strcmp(id_roomky.c_str(), "") != 0){
                                        id_roomky = "";
                                    }
                                    while(received_message[i] != '"'){
                                        id_roomky.push_back(received_message[i]);
                                        i++;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        if(received_message[i] == 'a'){
            i++;
            if(received_message[i] == 'g'){
                i++;
                if(received_message[i] == 'e'){
                    i++;
                    if(received_message[i] == '_'){
                        i++;
                        if(received_message[i] == 'i'){
                            i++;
                            if(received_message[i] == 'd'){
                                i++;
                                if(received_message[i] == '"'){
                                    i++;
                                    if(received_message[i] == ':'){
                                        i++;
                                        if(received_message[i] == ' '){
                                            i++;
                                            if(received_message[i] == '"'){
                                                i++;
                                                while(received_message[i] != '"'){
                                                    last_message_id.push_back(received_message[i]);
                                                    i++;
                                                }
                                            } else if(received_message[i] == 'n'){
                                                last_message_id = "";
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
                if(received_message[i] == 'n'){
                    i++;
                    if(received_message[i] == 'a'){
                        i++;
                        if(received_message[i] == 'm'){
                            i++;
                            if(received_message[i] == 'e'){
                                i++;
                                if(received_message[i] == '"'){
                                    i++;
                                    if(received_message[i] == ':'){
                                        i++;
                                        if(received_message[i] == ' '){
                                            i++;
                                            if(received_message[i] == '"'){
                                                i++;
                                                while(received_message[i] != '"'){

                                                    nazev_roomky.push_back(received_message[i]);
                                                    i++;
                                                }
                                                if(strcmp(nazev_roomky.c_str(), "isa-bot") != 0){
                                                        nazev_roomky = "";
                                                        last_message_id = "";
                                                        id_roomky = "";
                                                }else {
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
        }
    return make_tuple(id_roomky, last_message_id);
}


// Funkce na zjištění serveru kde je místnost #isa-bot
// Vrací list všech id, které najde
vector<string> GetBotDiscordServers(string received_message){

    vector<string> id_list {};
    string id_serveru = "";
    // Musím si zjistit všechny id serverů, hledáma konkrétní posloupnost znaků, která značí id
    for(int i = 0; received_message[i] != '\0'; i++){
        char pismenko = received_message[i];
        //std::cout<< pismenko;
        if(pismenko == '['){
            i++;
            if(received_message[i] == '{'){
                i++;
                if(received_message[i] == '"'){
                    i++;
                    if(received_message[i] == 'i'){
                        i++;
                        if(received_message[i] == 'd'){
                            i++;
                            if(received_message[i] == '"'){
                                i++;
                                if(received_message[i] == ':'){
                                    i++;
                                    if(received_message[i] == ' '){
                                        i++;
                                        if(received_message[i] == '"'){
                                            i++;
                                            while(received_message[i] != '"'){
                                                id_serveru.push_back(received_message[i]);
                                                i++;
                                            }
                                            // Až najdu kompletní id, zkusím najít v serveru kanál #isa-bot
                                            id_list.push_back(id_serveru);
                                        } 
                                    } 
                                } 
                            } 
                        } 
                    }
                }  
            }
        }
    }

    return id_list;

}

// Funkce na inicializaci bota a následně na jeho kontrolu
void BotControl(bool verbose, const char* token){
 
    // Definování proměnných, které se vážkou k socketu/SSL
    int err;
    struct sockaddr_in sa;
    SSL_CTX* ctx;
    SSL*     ssl;
    X509*    server_cert;
    char*    str;
    char     buf [4096];
    const SSL_METHOD *meth;

    int sd = socket (AF_INET, SOCK_STREAM, 0); 
    ssl = initSSL(sa,ctx,ssl,server_cert,str,buf,meth,sd);

    // Vrací strukturu hostent která obsahuje informace o hostovi
    
    // Přednastavení zpráv 
    string bot_token = token;
    stringstream http_request;
    stringstream http_request1;

    // GET Request na získání serverů na kterých je bot
    http_request << "GET /api/v6/users/@me/guilds HTTP/1.1\r\n"
           << "Content-Type: application/json\r\n"
           << "Host: discord.com\r\n"
           << "Authorization: Bot " << bot_token << "\r\n"
           << "Accept: application/json\r\n"
           << "Connection: close\r\n\r\n";
           
    // Převedení na string
    string request = http_request.str();
    
    
    err = SSL_write (ssl, request.c_str(), request.size());  
    if(err == -1){
        std::cout << "Nepodařilo se odeslat SSL požadavek\n";
        exit(101);
    }

    // String ve kterém budu ukládat příchozí zprávu ze serveru
    string received_message;

    // Přectení celé zprávy a uložení odpovědi
    while (err > 0){
        err = SSL_read (ssl, buf, sizeof(buf)-1);   
        buf[err] = '\0';
        received_message.append(string(buf));          
        
    }

    closeSSL(ssl, sd, ctx);

    vector<string> server_ids {};
    server_ids = GetBotDiscordServers(received_message);
    string channel_id = "";
    string channels = "";

    sleep(2);

    // Kontrola jestli se lze připojit
    if(server_ids.size() == 0){
        printf("Nezle se připojit\n");
        exit(404);
    }
    for (int i = 0; i < server_ids.size(); i++){
        sd = socket (AF_INET, SOCK_STREAM, 0); 
        ssl = initSSL(sa,ctx,ssl,server_cert,str,buf,meth,sd);
        
        // GET na získání všech kanálů na serveru
        http_request1 << "GET /api/v6/guilds/" + server_ids[i] + "/channels HTTP/1.1\r\n"
           << "Content-Type: application/json\r\n"
           << "Host: discord.com\r\n"
           << "Authorization: Bot " << bot_token << "\r\n"
           << "Accept: application/json\r\n"
           << "Connection: close\r\n\r\n";
        request = http_request1.str();
        err = SSL_write (ssl, request.c_str(), request.size());  
        if(err == -1){
            std::cout << "Nepodařilo se odeslat SSL požadavek\n";
            exit(101);
        }

        while (err > 0){
        err = SSL_read (ssl, buf, sizeof(buf)-1);   
        buf[err] = '\0';
        channels.append(string(buf));          
        }

        string id_roomky;
        string last_message_id;
        tie(id_roomky, last_message_id) = FindIsaBotChannel(channels);

        sleep(3);
        closeSSL(ssl, sd, ctx);

        if(strcmp(id_roomky.c_str(), "") == 0){
            printf("Spojení špatně navázáno, navazuji znovu\n");
            BotControl(verbose,token);
        }

        //Tady se zavolá funkce která jede do konce 
        BotTalk(verbose, token, last_message_id, id_roomky,sa , ctx, ssl, server_cert, str, buf, meth, sd);
    } 
}


// Funkce zpracovává argumenty viz. zadání
// Vrací jestli byl použit přepínač verbose a token bota
tuple<bool, const char*> ArgumentParser(int argc, char **argv){

    const char* access_token = "";
    bool verbose = false;
    // Pokud nejsou zadány argumenty zobrazí se nápověda viz. zadání
    if(argc == 1){
        std::cout << "Discord BOT -Vypracoval Ondřej Pavlacký (xpavla15)\n\nNápověda pro použití:\n\n-h|--help : Vypíše nápovědu na standardní výstup.\n\n-v|--verbose : Bude zobrazovat zprávy, na které bot reaguje na standardní výstup ve formátu <channel> - <username>: <message>.\n\n-t <bot_access_token> : Zde je nutno zadat autentizační token pro přístup bota na Discord.\n\nUkončení programu proběhne zasláním signálu SIGINT (tedy například pomocí kombinace kláves Ctrl + c), do té doby bude bot vykonávat svou funkcionalitu.\n\nV případě dotazů mě neváhejte kontaktovat: xpavla15@vutbr.cz\n\n";
        exit(0);
    }else{
        // Projde další argumenty a zjistí jejich správnost a případně si je uloží
        for (int i = 1; i < argc; i++)
        {  
            if(strcmp(argv[i], "-h") ==  0 || strcmp(argv[i], "--help") == 0){
                std::cout << "Discord BOT -Vypracoval Ondřej Pavlacký (xpavla15)\n\nNápověda pro použití:\n\n-h|--help : Vypíše nápovědu na standardní výstup.\n\n-v|--verbose : Bude zobrazovat zprávy, na které bot reaguje na standardní výstup ve formátu <channel> - <username>: <message>.\n\n-t <bot_access_token> : Zde je nutno zadat autentizační token pro přístup bota na Discord.\n\nUkončení programu proběhne zasláním signálu SIGINT (tedy například pomocí kombinace kláves Ctrl + c), do té doby bude bot vykonávat svou funkcionalitu.\n\nV případě dotazů mě neváhejte kontaktovat: xpavla15@vutbr.cz\n\n";
                exit(0);
            }
            else if(strcmp(argv[i], "-v") ==  0 || strcmp(argv[i], "--verbose") == 0){
                verbose = true;
            }
            else if(strcmp(argv[i], "-t") ==  0){
                if(i+1 >= argc){
                    std::cout << "Není zadán token\n";
                    exit(101);
                }else {
                    access_token = argv[i+1];
                    i++;
                }
            }
            else{
                std::cout << "Chyba v přepínači!\nZkuste -h nebo --help pro nápovědu\n";
                exit(101);
            }
        }
        return make_tuple(verbose,access_token);
        
    }
}

//Spuštění programu
int main(int argc, char *argv[]) {

    //...
    // Funkce na zpracování argumentů
    bool verbose;
    const char* token;
    tie(verbose, token) = ArgumentParser(argc,argv);

    //Zavolání funkce na inicializaci a kontrolu bota
    BotControl(verbose,token);    

    return 0;
}