#include <stdio.h>
#include <conio.h>
#include <spectrum.h>
#include "zifi.h"

char host[80];
char port[6];

char nick[40];
char pass[40];
char channel[256];

char recvBuff[4096];
char sendBuff[1024];
char iBuff[64];
char iPos = 0;

char intBuff[80];
char msgOutBuff[1024];

static void myPutS(char *c);

static char* skip(char *s, char c) 
{
	while(*s != c && *s != '\0')
		s++;
	if(*s != '\0')
		*s++ = '\0';
	return s;
}

static void trim(char *s) 
{
	char *e;

	e = s + strlen(s) - 1;
	while(isspace(*e) && e > s)
		e--;
	*(e + 1) = '\0';
}


void readHostData()
{
    printf("Enter host: ");
    cgets(host);
 
    printf("Enter port(or keep empty for 6667):");
    cgets(port);
    if (!strcmp(port,"")) {
        strcpy(port, "6667");
    }
}

void readUserData()
{
    printf("Your nick:");
    cgets(nick);
    if (!strcmp(nick, "")) {
        strcpy(nick, "anon-spectrum");
    }

    printf("Your password(may keep empty): ");
    cgets(pass);
}

static void send()
{
    int i;
    for (i=0; sendBuff[i] != 0; i++)
        sendByte(sendBuff[i]);
    
    sendByte('\r');
    sendByte('\n');
}

static void recv()
{
    int i = 0;
    while (i < 4095) {
        recvBuff[i] = getByte();
        if (recvBuff[i] == 10) break;
        if (recvBuff[i]>31) i++;
    }

    recvBuff[i] = 0;
} 

void connect()
{
    if (strlen(pass) > 0) {
        sprintf(&sendBuff, "PASS %s", pass);
        send();
    }

    sprintf(&sendBuff, "NICK %s", nick);
    send();

    sprintf(&sendBuff, "USER %s localhost %s :%s", nick, host, nick);
    send();
}

static void privmsg(char *to, char *what) 
{
    if (to[0] == 0) {
        myPutS("No target to send message!");
        return;
    }

    if (what[0] == 0) return;

    sprintf(sendBuff, "PRIVMSG %s : %s", to, what);
    send();
    sprintf(intBuff, "<%s> %s", nick, what);    
    myPutS(intBuff);
} 

static void myPutS(char *c)
{
    char i;
    gotoxy(0, 23);
    for (i=0;i<strlen(iBuff) + 2;i++)
        printf(" ");

    gotoxy(0, 22);
    printf("%s\n\n", c);
}

static void parseIn()
{
    char *subStr, cnt;
    char argBuff[40];
    if (iBuff[0] == 0) return;
    if (iBuff[0] != '!') {
        privmsg(channel, iBuff);
        return;
    }

    if (iBuff[1] != 0 && iBuff[2] == ' ') {
        subStr = &iBuff + 3;
        switch (iBuff[1])
        {
        case 'j':
            if (channel[0] == 0) {
                sprintf(sendBuff, "JOIN %s", subStr);
                send();
                strcpy(channel, subStr);
            } else {
                myPutS("Please leave current channel before joining another");
            }
            break;
        case 'l':
            if (channel[0]) {
                sprintf(sendBuff, "PART %s :%s", channel, subStr);
                send();
                channel[0] = 0;

                return;
            }
        case 'm':
            for (cnt=0;(subStr[cnt] != ' ') && (subStr[cnt] != 0) ; argBuff[cnt] = subStr[cnt ++]);
            
            argBuff[cnt] = 0;

            if (subStr[cnt] == ' ') { 
                privmsg(argBuff, &subStr[++ cnt]);
            }
            else
                myPutS("No message specified!");
        default:
            break;
        }
    } else {
        if (iBuff[1] == 'l' && iBuff[2] == 0) {
            if (channel[0]) {
                sprintf(sendBuff, "PART %s", channel);
                send();
                channel[0] = 0;

                return;
            }
        } else {
            strcpy(sendBuff, &iBuff[1]);
            send();
        }
    }
}

static void parseSrv(char *cmd)
{
    char *usr, *par, *txt;

	usr = host;
	if(!cmd || !*cmd)
		return;

	if(cmd[0] == ':') {
		usr = cmd + 1;
		cmd = skip(usr, ' ');
		if(cmd[0] == '\0')
			return;
		skip(usr, '!');
	}
	skip(cmd, '\r');
	par = skip(cmd, ' ');
	txt = skip(par, ':');
	trim(par);
    if (!strcmp("PONG", cmd)) return;
    if(!strcmp("PRIVMSG", cmd)) {
        sprintf(msgOutBuff, "%s <%s> %s", par, usr, txt);
        myPutS(msgOutBuff);
        return;
    }

    if (!strcmp("PING", cmd)) {
        sprintf(sendBuff, "PONG %s", txt);
        send();
        return;
    }

    if (!strcmp("NICK", cmd)) {
        strcpy(nick, txt);
    }
    sprintf(msgOutBuff, "%s!%s(%s): %s", usr, cmd, par, txt);
    myPutS(msgOutBuff);
}    

static void iRoutine()
{
    char c;
    char i;
        
    gotoxy(0, 23);
    iBuff[iPos] = 0;
    c = getk();
    printf(">%s_", iBuff, c);
    
    if (c >= 32) iBuff[iPos ++] = c;
    if (c == 12) {
        gotoxy(0, 23);
        for (i=0;i<strlen(iBuff) + 2;i++) printf(" ");
        iPos --;
    }
    if (c == 13) { 
        gotoxy(0, 23);
        for (i=0;i<strlen(iBuff) + 2;i++) printf(" ");
        iPos = 0;
        parseIn();

    }
    if (iPos < 0) iPos = 0;
    if (iPos > 63) iPos = 63;
}

void main() 
{
    zx_border(INK_BLACK);
    zx_colour(PAPER_BLACK | INK_WHITE);
    clg();
    __asm
        ld a, 7
        ld (23695), a
        ld (23693), a
    __endasm;
    
    myPutS("Simple IRC Client for ZX-UNO v. 0.1");
    myPutS("(c) 2019 Nihirash");
    myPutS("Some parts based on Suckless Irc Client");
    myPutS("");

    readHostData();
    readUserData();
    initWifi();
    openTcp(host, port);
    myPutS("Connected!");
    recv();
    myPutS(recvBuff);
    connect();
    
    for(;;) {
        if (isAvail()) {
            recv();
            parseSrv(recvBuff);
        }
        iRoutine();
    }
}