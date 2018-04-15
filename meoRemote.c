/*
 * meoRemote
 *
 * jml, 2018
 *
 */

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <SDL.h>
#include <SDL_gfxPrimitives.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>

#define SCREEN_WIDTH  121
#define SCREEN_HEIGHT 640
#define BPP 32

#define TITLE_X 30
#define TITLE_Y 10

#define FIELD_PADDING 1
#define BORDER_WIDTH 1
#define POINT_RADIUS 10

#define POINT_SPEED 3

#define COLOR_BORDER 0x0000FFFF
#define COLOR_FIELD  0x00FF00FF
#define COLOR_POINT  0xFF0000FF

char *destAddr = "192.168.1.64";
char *destMask = NULL;
int  destPort  = 8082;
int  destSock  = 0;

short int list_buttons = 0;
short int draw_buttons = 0;
short int draw_mark = 0;
short int one_shoot = -1;

#define MAX_NBOXES (4)
char *Box[MAX_NBOXES];

typedef struct {
  short int x, y;
  char *tag;
  char *desc;
  short int code;
} tButton;

#define MAX_BUTTONS (45)
tButton Button[MAX_BUTTONS];
int nButtons = 0;
int selectedButton = 0;

SDL_Surface *wallPaper = NULL;

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

static void checkSDLResult(int result) {
    if (!result) {
        fprintf(stderr, "Error: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
}

static SDL_Surface *createSurface() {
    SDL_Surface *screen;

    checkSDLResult(SDL_Init(SDL_INIT_VIDEO) == 0);
    atexit(SDL_Quit);

    screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, BPP, SDL_HWSURFACE);
    checkSDLResult(screen != 0);

    checkSDLResult(SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL) == 0);

    return screen;
}

void markButton(SDL_Surface *screen, int b) {
    aacircleColor(screen, Button[b].x, Button[b].y, POINT_RADIUS, COLOR_POINT);
}

void drawButtons(SDL_Surface *screen) {
    for( int i=0 ; i<nButtons ; i++ ) {
	aacircleColor(screen, Button[i].x, Button[i].y, POINT_RADIUS, COLOR_FIELD);
    }
}

void listButtons() {
    for( int i=0 ; i<nButtons ; i++ ) {
	printf("%2d %3d (%3d,%3d) \"%s\"\n",i, Button[i].code, Button[i].x, Button[i].y, Button[i].tag);
    }
}

static void screenDraw(SDL_Surface *screen) {
    // fill all screen with black color
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));

    //Apply the image
    SDL_BlitSurface( wallPaper, NULL, screen, NULL );

    if ( draw_buttons ) drawButtons(screen);

}

static void initialDraw(SDL_Surface *screen) {
    // fill all screen with black color
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
}

void sendCommand(int cmd) {
	if ( destSock<0 ) return;
	char msg[20];
	sprintf(msg, "key=%d\n", cmd);
    	/* send the message line to the server */
    	int n = write(destSock, msg, strlen(msg));
    	if (n < 0) error("ERROR writing to socket");
}

int findNextButton(int dx, int dy) {
    int best = selectedButton;
    int minDist = 1e8;
    for( int i=0 ; i<nButtons ; i++ ) {
	if ( i==selectedButton ) continue;
	int bdx = Button[i].x - Button[selectedButton].x; 
	int bdy = Button[i].y - Button[selectedButton].y;
	if ( bdx*dx<0 || bdy*dy<0 ) continue;
	int d = abs(bdx)/(2*dx*dx+1)+abs(bdy)/(2*dy*dy+1);
	if ( d<minDist ) {
	    minDist = d;
	    best = i;
	}
    }
    return best;
}

/* Returns 1 if quit. */
static int processKey(SDL_Surface *screen, SDLKey key) {
    // position change
    int dx = 0;
    int dy = 0;

    switch (key) {
        case SDLK_ESCAPE:
            return 1;
        case SDLK_UP:
            dy = -1;
            break;
        case SDLK_DOWN:
            dy = 1;
            break;
        case SDLK_LEFT:
            dx = -1;
            break;
        case SDLK_RIGHT:
            dx = 1;
            break;
	case SDLK_SPACE:
	case SDLK_RETURN:
	    sendCommand(Button[selectedButton].code);
	    break;
        default:
            break;
    }

    if (dx != 0 || dy != 0) {
        //movePoint(screen, point_x + dx * POINT_SPEED, point_y + dy * POINT_SPEED);
	selectedButton = findNextButton(dx,dy);
	screenDraw(screen);
	markButton(screen, selectedButton);
    }
    return 0;
}


void processMouseDown(SDL_Surface *screen, Uint8 button, Uint16 x, Uint16 y) {
    if (button != 1) {
        return;
    }
    int sel=-1;
    const int rad2 = POINT_RADIUS*POINT_RADIUS;
    // printf("x y = %d %d\n", x,y);
    for( int i=0 ; i<nButtons ; i++ ) {
	int dx = x - Button[i].x;
	int dy = y - Button[i].y;
	int dist = dx*dx + dy*dy;

	//printf("  dist = %d %d, %d %d\n", i, dist, Button[i].x, Button[i].y);
	if ( dist<=rad2 ) {
	    sel = i;
	    break;
	}
    }
    if ( sel>=0 ) {
	int cod = Button[sel].code;
        sendCommand(cod);
	//printf("sent = %d %d\n", cod, sel);
	selectedButton=sel;
	screenDraw(screen);
	if ( draw_mark ) markButton(screen, selectedButton);
    }
}


void addButton(int x, int y, int cod, char *tag) {
    if ( nButtons>=MAX_BUTTONS ) {
	return;
    } 
    //aacircleColor(screen, x, y, POINT_RADIUS, COLOR_FIELD);
    Button[nButtons].x = x;
    Button[nButtons].y = y;
    Button[nButtons].code = cod;
    Button[nButtons].tag = strdup(tag);
    nButtons++;
}

void addNumButton(int x, int y, int num) {
    char numTag[8];
    sprintf(numTag,"%d",num);
    addButton(x,y,num+48,numTag);
}

int defineButtons() {

    const int DX3 = 38;
    const int CENTER = 61;
    const int LEFT3  = CENTER-DX3;
    const int RIGHT3 = CENTER+DX3;

    const int BASE = 30;
    const int BASE5 = BASE+13;
    const int LINE = 30;
    const int BASE12 = BASE+13*LINE;

    const int QUAD1 = LEFT3-3;
    const int QUAD2 = CENTER-13;
    const int QUAD3 = CENTER+13;
    const int QUAD4 = RIGHT3+3;

    addButton(RIGHT3, BASE, 233, "Power");

    for( int y=1 ; y<4 ; y++ ) {
        for( int x=0 ; x<3 ; x++ ) {
	    addNumButton( x*38+23, y*LINE+BASE, nButtons );
 	}
    }
    addNumButton( CENTER, 4*LINE+BASE, 0 );

    addButton( LEFT3,  4*LINE+BASE,  0, "Backspace");   // code not set
    addButton( RIGHT3, 4*LINE+BASE, 43, "Enter");

    // vol    
    addButton( LEFT3,  5*LINE+BASE5,    175, "Vol+");
    addButton( LEFT3,  7*LINE+BASE5+17, 174, "Vol-");

    // p+/p-
    addButton( RIGHT3, 5*LINE+BASE5,    33, "Prog+");
    addButton( RIGHT3, 7*LINE+BASE5+17, 34, "Prog-");

    // +
    addButton( CENTER, 5*LINE+BASE5,    38, "Up");
    addButton( CENTER, 7*LINE+BASE5+17, 40, "Down");
    addButton( LEFT3,  6*LINE+BASE5+8,  37, "Left");
    addButton( RIGHT3, 6*LINE+BASE5+8,  39, "Right");

    addButton( CENTER, 6*LINE+BASE5+8,  13, "Ok");

    addButton( CENTER, 8*LINE+BASE5+12, 36, "Menu");  

    // nav
    addButton( QUAD1, 9*LINE+BASE5+5,   8, "Back");
    addButton( QUAD2, 9*LINE+BASE5+5,  27, "Esc");
    addButton( QUAD3, 9*LINE+BASE5+5, 112, "Guide");
    addButton( QUAD4, 9*LINE+BASE5+5, 114, "Club");

    // info    
    addButton( LEFT3,   10*LINE+BASE5+10, 159, "Info");
    addButton( CENTER,  10*LINE+BASE5+10, 156, "Swap");
    addButton( RIGHT3,  10*LINE+BASE5+10, 115, "Folder");
    // stop    
    addButton( LEFT3,   11*LINE+BASE5+10, 123, "Stop");
    addButton( CENTER,  11*LINE+BASE5+10, 119, "Pause/Play");
    addButton( RIGHT3,  11*LINE+BASE5+10, 226, "Rec");

    // moves
    addButton( QUAD1, BASE12, 117, "Prev");
    addButton( QUAD2, BASE12, 118, "Rewind");
    addButton( QUAD3, BASE12, 121, "Forward");
    addButton( QUAD4, BASE12, 122, "Next");

    // Colors
    addButton( QUAD1, BASE12+LINE-1, 38, "Red");
    addButton( QUAD2, BASE12+LINE-1, 40, "Green");
    addButton( QUAD3, BASE12+LINE-1, 37, "Yellow");
    addButton( QUAD4, BASE12+LINE-1, 39, "Blue");

    // bottom
    addButton( QUAD1, BASE12+2*LINE-1, 173, "Mute");
    addButton( QUAD2, BASE12+2*LINE-1,   0, "Sound");   // Code not set
    addButton( QUAD3, BASE12+2*LINE-1, 111, "Options");
    addButton( QUAD4, BASE12+2*LINE-1,   0, "TV/STB");	// Local. No remote function

    return nButtons;
}

int readConfigFile(const char *fname) {
    FILE *fcfg;
    fcfg = fopen(fname, "r");
    if ( ! fcfg ) return 0;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    static int nBox = 0;
    while ((read = getline(&line, &len, fcfg)) != -1) {
        printf("%s", line);
	char *tline = line;
	while( *tline == ' ' ) tline++;
	if ( strncasecmp(tline,"box",3)==0 ) {
	    tline += 3;
	    if ( *tline!='=' && *tline!=' ' ) {
		nBox = *tline - '0';
		tline++;
	    }
	    if ( nBox<0 || nBox>=MAX_NBOXES ) {
		fprintf(stderr,"Error: Invalid box number: %s:%s\n", fname, line);
		exit(1);
	    }
	    while( *tline==' ' || *tline=='=' ) tline++;
	    char *tlend;
	    tlend = strchr(tline,'\n');
	    if ( tlend ) *tlend = 0;
	    tlend = strchr(tline,'\r');
	    if ( tlend ) *tlend = 0;
	    Box[nBox] = strdup(tline);
	    nBox++;
	}
    }
    fclose(fcfg);
    free(line);
    return 1;
}

int readConfig() {
    int read = 0;
    read += readConfigFile("/etc/meoRemote.conf");
    read += readConfigFile("/usr/local/etc/meoRemote.conf");
    read += readConfigFile("~/.meoRemote.conf");
    read += readConfigFile("meoRemote.conf");
    return read;
}

int main(int argc, char **argv) {
    int quit;
    for( int i=0 ; i<MAX_NBOXES ; i++ )
	Box[i] = NULL;

    readConfig();
    int boxNum = 0;
    if (Box[boxNum]) destAddr = Box[boxNum];

    while( argc>1 ) {
	if ( argv[1][0]=='-' ) {
	    switch (argv[1][1]) {
		case 'b':
		    draw_buttons = 1;
		    break;
	 	case 'c':
		    draw_mark = 1;
		    break;
		case 'a':
		    if ( argc>2 ) {
		    	destAddr = argv[2];
			destMask = NULL;
		    	argc--;
		    	argv++;
		    }
		    break;
		case 'm':
		    if ( argc>2 ) {
		    	destMask = argv[2];
		    	argc--;
		    	argv++;
		    }
		    break;
		case 'p':
		    if ( argc>2 ) {
		    	destPort = atoi(argv[2]);
		    	argc--;
		    	argv++;
		    }
		    break;
		case 'l':
		    list_buttons = 1;
		    break;
		case '1':
		    one_shoot = atoi(argv[2]);
		    argc--;
		    argv++;
		    break;
	    }
	}
	else {
	    sendCommand(atoi(argv[2]));
	}
	argc--;
	argv++;
    } // while
		 
    
    defineButtons();
   
    SDL_Surface *screen = NULL;
    if ( one_shoot<0 ) {
	screen = createSurface();
    	initialDraw(screen);
    }

    if ( list_buttons ) {
	listButtons();
	exit(0);
    }

    if ( one_shoot<0 ) {
    	wallPaper = SDL_LoadBMP( "meo_remote.bmp" );
    	if( wallPaper == NULL )     {
            printf( "Unable to load image %s! SDL Error: %s\n", "meo_remote.bmp", SDL_GetError() );
    	}
    	else //Apply the image
            SDL_BlitSurface( wallPaper, NULL, screen, NULL );

    	if ( draw_buttons ) drawButtons(screen);
    }

    /* socket: create the socket */
    destSock = socket(AF_INET, SOCK_STREAM, 0);
    if (destSock < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    struct hostent *server = gethostbyname(destAddr);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as '%s'\n", destAddr);
        exit(0);
    }

    /* build the server's Internet address */
    struct sockaddr_in serveraddr;
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(destPort);

    /* connect: create a connection with the server */
    if (connect(destSock, &serveraddr, sizeof(serveraddr)) < 0) 
      perror("ERROR connecting");

    if ( one_shoot>=0 ) {
	sendCommand(one_shoot);
	exit(0);
    }

    quit = 0;
    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    quit = 1;
                    break;

                case SDL_KEYDOWN:
                    if (processKey(screen, event.key.keysym.sym)) {
                        quit = 1;
                    }
                    break;

                case SDL_MOUSEBUTTONDOWN: {
                    SDL_MouseButtonEvent be = event.button;
                    processMouseDown(screen, be.button, be.x, be.y);

                    break;
                }
            }
        }

        SDL_Flip(screen);
        SDL_Delay(33); // ~ 60 fps
        // screenDraw(screen);
    }

    close(destSock);

    for( int i=0 ; i<nButtons ; i++ )
	free(Button[i].tag);
    for( int i=0 ; i<MAX_NBOXES ; i++ )
	free(Box[i]);

    SDL_FreeSurface(wallPaper);
    SDL_FreeSurface(screen);
    SDL_Quit();

    return 0;
}

