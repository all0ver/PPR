#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/server.h>
#include <xmlrpc-c/server_abyss.h>
#include <openssl/md5.h>

// funkcja wywoływana przez php w ramach xml-rpc
static xmlrpc_value * data_transfer(
            xmlrpc_env * const envP,
            xmlrpc_value * const paramArrayP, // tablica argumentów od php
            void * const serverInfo,
            void * const channelInfo) {

    const char * b64_string; // wskaźnik na adres pod którym zostaną zapisane dane (* bo nie ma string)

    // zmienne socket/tcp - p3
    int     port   = 5000;         // Port procesu III
    char    host[] = "127.0.0.1";  // Adres procesu III
    int     fd, n; // fd - deskryptor, n - ilość bajtów
    struct  sockaddr_in serv_addr; // struktura do konfiguracji socket/tcp
    struct  hostent *server; // struktura z danymi serwera

    // rozpakowanie tablicy parametrów z formatu xml-rpc do zwyklych zmiennych - dane zapisane jako tablica z jednym stringiem (s) - zapisanie danych do b64_string
    xmlrpc_decompose_value( envP, paramArrayP, "(s)", &b64_string );
    if (envP->fault_occurred) return NULL; // w przypadku błędu

    printf("Proces II (C): Odebrano %zu znakow. Lacze z Pythonem...\n", strlen(b64_string));
    //dodatkowe - md5
    unsigned char digest[MD5_DIGEST_LENGTH];
	MD5((unsigned char*)b64_string, strlen(b64_string), digest);

	for(int i = 0; i < 16; i++) {
		printf("%02x", digest[i]); // wypisujemy bajty jako HEX
	}
	printf("\n");
	// dodatkowe

    // utworzenie gniazda - ipv4, tcp
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket()");
        return xmlrpc_build_value(envP, "s", "Blad: socket()");
    }
  
    // pobranie informacji o serwerze - przetłumaczenie adresu
    server = gethostbyname( host );
    if( server == NULL ) {
        perror("gethostbyname()");
        return xmlrpc_build_value(envP, "s", "Blad: gethostbyname()");
    }
    
    // wyczyszczenie pamięci struktury
    bzero((char *) &serv_addr, sizeof(serv_addr));
    
    serv_addr.sin_family = AF_INET; //ipv4
    serv_addr.sin_port = htons(port); // zapis portu w jednolity sposób
    // przekopiowanie adresu IP do głównej struktury konfiguracyjnej serv_addr
    bcopy( (char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length );
    
    // nawiązanie połaczenia wykorzystując gniazdo i konfigurację serv_addr
    if( connect( fd, (struct sockaddr *) &serv_addr, sizeof( serv_addr ) ) < 0 ) {
        perror("connect()");
        return xmlrpc_build_value(envP, "s", "Blad: connect() - Python nie dziala?");
    }
    
	// wysłanie danych do python
    n = write( fd, b64_string, strlen(b64_string) );
    if (n < 0) {
        perror("write()");
        return xmlrpc_build_value(envP, "s", "Blad: write()");
    }
    
    // zamknięcie socketu
    close(fd);
    
    // zwrot informacji o wysłaniu do php jako string
    return xmlrpc_build_value(envP, "s", "OK");
}

//======================================================================
int main( void ){

    int  port   = 12345; // Port, na ktorym C slucha PHP (xml-rpc)
    // struktura, która przechowuje info o funkcji dla xml-rpc
    struct xmlrpc_method_info3 const methodInfo = {
        /* .methodName     = */ "data.transfer",
        /* .methodFunction = */ &data_transfer,
    };
    
    // zmienne serwera abbys
    xmlrpc_server_abyss_parms serverparm;
    xmlrpc_registry * registryP;
    xmlrpc_env env;

	// 
    xmlrpc_env_init(&env);

	// utworzenie rejestru funkcji dla xml-rpc
    registryP = xmlrpc_registry_new(&env);
    if (env.fault_occurred){
        printf( "xmlrpc_registry_new(): %s\n", env.fault_string);
        exit(1);
    }
	// dodanie zdefiniowanej funkcji (methodInfo) do rejestru funkcji 
    xmlrpc_registry_add_method3( &env, registryP, &methodInfo );
    if (env.fault_occurred) {
        printf( "xmlrpc_registry_add_method3(): %s\n", env.fault_string );
        exit(1);
    }

	// konfiguracja serwera abyss
    serverparm.config_file_name = NULL;
    serverparm.registryP        = registryP;
    serverparm.port_number      = port;
    serverparm.log_file_name    = "/tmp/xmlrpc_log";

    // printf("Proces II (C): Oczekiwanie na XML-RPC (port %d)...\n", port);


	// uruchomienie serwera abyss
    xmlrpc_server_abyss(&env, &serverparm, XMLRPC_APSIZE(log_file_name));
    if( env.fault_occurred ){
        printf("xmlrpc_server_abyss(): %s\n", env.fault_string);
        exit(1);
    }

    return 0;
}
