#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <pthread.h>

#define NB_IMG      2

#if defined (WIN32)
    #include <winsock2.h>
    #include <windows.h>
    typedef int socklen_t;
#elif defined (linux)
    #include <sys/types.h>
    #include <sys/sysctl.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <sys/param.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define closesocket(s) close(s)
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
	typedef int SOCKET;
    typedef struct sockaddr_in SOCKADDR_IN;
    typedef struct sockaddr SOCKADDR;
#endif


SDL_Rect pos = {0,0,0,0};
typedef struct Input Input;
struct Input
{
    POINT souris;
    char fin,rmouse,lmouse;
};

SDL_Surface* LoadBMP(char*,int);
unsigned char GetPixelComp32(SDL_Surface*,int,int,int);
void PutPixelComp32(SDL_Surface*,int,int,int,unsigned char);
void RLEUncompress(unsigned char*,unsigned char*,int);
void init3(void);
void init2(SOCKET*,SOCKADDR_IN*,int);
SOCKET create_socket(int);
SDL_Surface* init(int,int,const char*,int);
FILE* s_fopen(const char*,const char*);
void* s_malloc(size_t);
SDL_Surface* s_IMG_Load(const char*);
void cleanup(SDL_Surface**);
size_t sizeofFile(const char*);
void pause(void);
Uint32 obtenirPixel(SDL_Surface*,int,int);
void definirPixel(SDL_Surface*,int,int,Uint32);
int minn(int,int);
int err(int);

void init3(void)
{
    #if defined (WIN32)
        WSADATA WSAData;
        int erreur = WSAStartup(MAKEWORD(2,2),&WSAData);
    #else
        int erreur = 0;
    #endif

    if (erreur)
    {
        perror("");
        exit(EXIT_FAILURE);
    }
}

void init2(SOCKET* sock,SOCKADDR_IN* sin,int port)
{
    *sock = socket(AF_INET,SOCK_STREAM,0);

    if(*sock == INVALID_SOCKET)
    {
        perror("Erreur socket 1 ");
        printf("\n%d",WSAGetLastError());
        exit(EXIT_FAILURE);
    }

    printf("La socket %d est maintenant ouverte en mose TCP/IP\n",*sock);
    sin->sin_family = AF_INET;
    sin->sin_port = htons(port);
}

SOCKET create_socket(int port)
{
    SOCKET sock,csock;
    SOCKADDR_IN sin,csin;
    socklen_t recsize = sizeof(csin);
    int sock_err;

    init2(&sock,&sin,port);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

    if ((sock_err = bind(sock,(SOCKADDR*)&sin,sizeof(sin))) == SOCKET_ERROR)
    {
        perror("Erreur socket 2 ");
        exit(EXIT_FAILURE);
    }

    sock_err = listen(sock,5);
    printf("Listage du port %d...\n",port);

    if(sock_err == SOCKET_ERROR)
    {
        perror("Erreur connexion ");
        return EXIT_FAILURE;
    }

    printf("Patientez pendant que le client se connecte sur le port %d...\n",port);
    csock = accept(sock,(SOCKADDR*)&csin,&recsize);
    printf("Un client se connecte avec la socket  %d de %s : %d.\n",csock,inet_ntoa(csin.sin_addr),htons(csin.sin_port));

    return csock;
}

SDL_Surface* init(int x,int y,const char* titre,int fullscreen)
{
    SDL_Surface* screen = NULL;

    fullscreen = (fullscreen ? 1 : 0);

    if (SDL_Init(SDL_INIT_VIDEO) == -1)
        exit(EXIT_FAILURE);

    if ((screen =
         SDL_SetVideoMode(x,y,32,SDL_DOUBLEBUF | SDL_HWSURFACE | (SDL_FULLSCREEN*fullscreen))) == NULL)
        exit(EXIT_FAILURE);


    SDL_EnableUNICODE(1);
    SDL_EnableKeyRepeat(500,10);

    return screen;
}

void cleanup(SDL_Surface** images)
{
    int img;
    for (img = 0 ; img < NB_IMG ; img++)
        SDL_FreeSurface(images[img]);
    SDL_Quit();
}

FILE* s_fopen(const char* cfile,const char* mode)
{
    FILE* file = NULL;

    if ((file = fopen(cfile,mode)) == NULL)
    {
        fprintf(stderr,"%s ",cfile);
        perror("-> erreur ");
        exit(EXIT_FAILURE);
    }

    return file;
}

void* s_malloc(size_t size)
{
    void* ptr = NULL;

    if ((ptr = malloc(size)) == NULL)
    {
        perror("Erreur memoire ");
        exit(EXIT_FAILURE);
    }

    return ptr;
}

SDL_Surface* s_IMG_Load(const char* cimage)
{
    SDL_Surface* surface = NULL;

    if ((surface = IMG_Load(cimage)) == NULL)
    {
        fprintf(stderr,"Impossible de charger l'image \'%s\' !",cimage);
        exit(EXIT_FAILURE);
    }

    return surface;
}

size_t sizeofFile(const char* cfile)
{
    size_t size;
    FILE* file = s_fopen(cfile,"rb");
    fseek(file,0,SEEK_END);
    size = ftell(file);
    rewind(file);
    fclose(file);
    return size;
}

void pause(void)
{
    int continuer = 1;
    SDL_Event event;

    while (continuer)
    {
        SDL_PollEvent(&event);
        switch(event.type)
        {
            case SDL_QUIT:
                continuer = 0;
                break;
            case SDL_KEYDOWN:
                if(event.key.keysym.sym == SDLK_RETURN
                || event.key.keysym.sym == SDLK_ESCAPE)
                    continuer = 0;
                break;
            default:
                break;
        }
    }
}

Uint32 obtenirPixel(SDL_Surface *surface,int x,int y)
{
    int nbOctetsParPixel = surface->format->BytesPerPixel;
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * nbOctetsParPixel;

    switch(nbOctetsParPixel)
    {
        case 1:
            return *p;

        case 2:
            return *(Uint16 *)p;

        case 3:
            if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
                return p[0] << 16 | p[1] << 8 | p[2];
            else
                return p[0] | p[1] << 8 | p[2] << 16;

        case 4:
            return *(Uint32 *)p;

        default:
            return 0;
    }
}

void definirPixel(SDL_Surface *surface,int x,int y,Uint32 pixel)
{
    if (x >= surface->w || y >= surface->h) return;

    int nbOctetsParPixel = surface->format->BytesPerPixel;
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * nbOctetsParPixel;

    switch(nbOctetsParPixel)
    {
        case 1:
            *p = pixel;
            break;

        case 2:
            *(Uint16 *)p = pixel;
            break;

        case 3:
            if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            {
                p[0] = (pixel >> 16) & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = pixel & 0xff;
            }
            else
            {
                p[0] = pixel & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = (pixel >> 16) & 0xff;
            }
            break;
        case 4:
        default:
            *(Uint32 *)p = pixel;
            break;
    }
}

void RLEUncompress(unsigned char *output,unsigned char *input,int length)
{
   signed char count;

   while (length > 0)
    {
      count = (signed char)*input++;
      if (count > 0)
      {
         memset(output,*input++,count);
      }
      else if (count < 0)
      {
         count = (signed char) - count;
         memcpy(output,input,count);
         input += count;
      }
      output += count;
      length -= count;
   }
}

SDL_Rect Rect(int x,int y,int w,int h)
{
    SDL_Rect r;
    r.x = x;
    r.y = y;
    r.w = w;
    r.h = h;
    return r;
}

SDL_Surface* LoadBMP(char* fichier,int vram)
{
    SDL_Rect R;
    SDL_Surface *r,*f = SDL_LoadBMP(fichier);
    if (!f)
    {
        printf("Echec chargement %s\n",fichier);
        SDL_Quit();
        exit(EXIT_FAILURE);
    }
    r = NULL;
    if (vram)
        r = SDL_CreateRGBSurface(SDL_HWSURFACE,f->w,f->h,32,0,0,0,0);
    if (r == NULL) vram = 0;
    if (!vram)
        r = SDL_CreateRGBSurface(SDL_SWSURFACE,f->w,f->h,32,0,0,0,0);
    R = Rect(0,0,f->w,f->h);
    SDL_BlitSurface(f,NULL,r,&R);
    SDL_FreeSurface(f);
    return r;
}

unsigned char GetPixelComp32(SDL_Surface* surface,int x,int y,int c)
{
    unsigned char *p = ((unsigned char*)surface->pixels) + y * surface->pitch + x * 4;
    return p[c];
}

void PutPixelComp32(SDL_Surface* surface,int x,int y,int c,unsigned char val)
{
    unsigned char *p = ((unsigned char*)surface->pixels) + y * surface->pitch + x * 4;
    p[c] = val;
}

void Stretch_Linear(SDL_Surface* src,SDL_Surface* dest,int src_w,int src_h)
{
    if ((src_w <= dest->w) && (src_h <= dest->h))
    {
        SDL_BlitSurface(src,NULL,dest,&pos);
        return;
    }

    int i,j,k;
    double rx,ry;
    rx = dest->w*1.0/src_w;
    ry = dest->h*1.0/src_h;
    for(i = 0 ; i < dest->w ; i++)
        for(j = 0 ; j < dest->h ; j++)
        {
            unsigned char pix;
            double valx,valy,fx,fy;
            int minx,miny,maxx,maxy;
            valx = i/rx;
            valy = j/ry;
            minx = (int)valx;
            miny = (int)valy;
            maxx = minx + 1;
            if (maxx >= src_w)
                maxx--;
            maxy = miny+1;
            if (maxy >= src_h)
                maxy--;
            fx = valx-minx;
            fy = valy-miny;
            for(k = 0 ; k < 3 ; k++)
            {
                pix = (unsigned char)(GetPixelComp32(src,minx,miny,k)*(1-fx)*(1-fy) + GetPixelComp32(src,maxx,miny,k)*fx*(1-fy)
                    + GetPixelComp32(src,minx,maxy,k)*(1-fx)*fy + GetPixelComp32(src,maxx,maxy,k)*fx*fy);
                PutPixelComp32(dest,i,j,k,pix);
            }
        }
}

int minn(int a,int b)
{
    if (a < b)
        return a;
    return b;
}

int err(int es)
{
    return (es == -1 || !es);
}

#endif // MAIN_H_INCLUDED
