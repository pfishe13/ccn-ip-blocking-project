/* timeserv.c - a socket-based time of day server. Updated for IP blocking.
/* Parker Fisher, James Meyer, Riley Libens
 */
#include  <stdbool.h>
#include  <stdio.h>
#include  <unistd.h>
#include  <sys/types.h>
#include  <sys/socket.h>
#include  <netinet/in.h>
#include  <netdb.h>
#include  <time.h>
#include  <strings.h>
#include <string.h>
#include  <stdlib.h>
#include <arpa/inet.h>

#define   PORTNUM  13429   /* our time service phone number */
#define   HOSTLEN  256
#define   oops(msg)      { perror(msg) ; exit(1) ; }

int main(int ac, char *av[])
{
        struct  sockaddr_in   saddr, caddr;   /* build our address here */
        struct  hostent         *hp;   /* this is part of our    */
        char    hostname[HOSTLEN];     /* address                */
        int     sock_id,sock_fd;       /* line id, file desc     */
        FILE    *sock_fp;              /* use socket as stream   */
        char    *ctime();              /* convert secs to string */
        time_t  thetime;               /* the time we report     */
        struct in_addr temp;            /* added */

        /*
        * Step 0: Open and read the whitelist file
        */
        int lines = 0;
        int ch = 0;
        const char* fileName = av[1];
        FILE* file = fopen(fileName, "r");
        if(file == NULL)
                oops("Failure to open file");
        do {
                ch = fgetc(file);
                if (ch == '\n')
                lines++;
        } while (ch != EOF);

        rewind(file);

        char *whitelist[lines];
        int j = 0;
        size_t len = 0;
        for( j = 0; j < lines; j++ ) {
                whitelist[j] = NULL;
                len = 0;
                getline( &whitelist[j], &len, file );
        }

        /*
        // Print out IPs in the whitelist
        printf("\n");
        for( int i = 0; i < lines; i++ ){
                printf("%s",whitelist[i]);
                // printf("|");
        }

        fclose(file);
        */

      /*
       * Step 1: ask kernel for a socket
       */

        sock_id = socket( PF_INET, SOCK_STREAM, 0 );    /* get a socket */
        if ( sock_id == -1 )
                oops( "socket" );

      /*
       * Step 2: bind address to socket.  Address is host,port
       */

        bzero( (void *)&saddr, sizeof(saddr) ); /* clear out struct     */

        gethostname( hostname, HOSTLEN );       /* where am I ?         */
        hp = gethostbyname( hostname );         /* get info about host  */
                                                /* fill in host part    */
        bcopy( (void *)hp->h_addr, (void *)&saddr.sin_addr, hp->h_length);
        saddr.sin_port = htons(PORTNUM);        /* fill in socket port  */
        saddr.sin_family = AF_INET ;            /* fill in addr family  */

        if ( bind(sock_id, (struct sockaddr *)&saddr, sizeof(saddr)) != 0 )
               oops( "bind" );

      /*
       * Step 3: allow incoming calls with Qsize=1 on socket
       */

        if ( listen(sock_id, 1) != 0 )
                oops( "listen" );

      /*
       * main loop: accept(), write(), close()
       */
        int caddr_len = sizeof(caddr);
        while ( 1 ){

               sock_fd = accept(sock_id, (struct sockaddr *) &caddr, &caddr_len); /* wait for call */

                printf("Wow! got a call!\n");

                bool found = false;

                temp = caddr.sin_addr;
                char *ip;
                ip = inet_ntoa(temp);

                int len = sizeof(whitelist)/sizeof(whitelist[0]);
                int w;

                // need to add newline character to the IP string for the correct comparison
                char *ipCopy;
                ipCopy = malloc (sizeof (char) * _SS_SIZE);
                strcpy (ipCopy, ip);
                strcat (ipCopy, "\n");

                for(w = 0; w < len; ++w){
                        if(!strcmp(whitelist[w], ipCopy)) {
                                found = true;
                        }
                }


               if ( sock_fd == -1 ) {
                       oops( "accept" );       /* error getting calls  */
                }


               sock_fp = fdopen(sock_fd,"w");  /* we'll write to the   */
               if ( sock_fp == NULL ){          /* socket as a stream   */
                       oops( "fdopen" );       /* unless we can't      */
                }


               thetime = time(NULL);           /* get time             */
                                               /* and convert to strng */

                if ( found ) {
                        fprintf( sock_fp, "The time here is .." );
                        fprintf( sock_fp, "%s", ctime(&thetime) );
                        printf("Wow! Printing the time to the client!\n");
                } else {
                        printf("Client IP not in whitelist. Dropping connection\n");
                }

                fclose( sock_fp );              /* release connection   */


        }
}
