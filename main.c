#include "main.h"

#define ecran       images[0]
#define PORT1       5900
#define PORT2       6000
#define PORT3       6100

#define ping        15

static int LARGEUR,HAUTEUR,i = 0,j = 0,palette[] = {0,84,169,255},W = 0,H = 0;
static SOCKET csock,csock2,csock3;
static SDL_Surface* images[NB_IMG] = {NULL,NULL};
static SDL_Event event;
static pthread_t t1,t2,t3;
static Input in;
static unsigned char *data2 = NULL,*data = NULL;

void* f1(void* ptr)
{
    do
    {
        int rc1 = 0,rc2 = 0,sd1 = 0,sd2 = 0,taille_image = 0;

        rc1 = recv(csock,(char*)&taille_image,sizeof(int),0);
        sd1 = send(csock,(char*)&rc1,sizeof(int),0);
        rc2 = recv(csock,(char*)data2,taille_image,0);
        sd2 = send(csock,(char*)&rc2,sizeof(int),0);

        //printf("rc1 : %d\nsd1 : %d\nrc2 : %d\nsd2 : %d\n\n",rc1,sd1,rc2,sd2);

        if (err(rc1) || err(rc2) || err(sd1) || err(sd2))
        {
            printf("taille_image = %d\n",
               taille_image);
            in.fin = 1;
        }

        if (rc2 != taille_image)
            continue;

        RLEUncompress(data,data2,W*H);

        int d = 0;
        for (j = H ; j > -1; j--)
        {
            for (i = 0 ; i < W ; i++)
            {
                Uint32 pixel = palette[(data[d] >> 4 ) & 3]
                                         +(palette[(data[d] >> 2 ) & 3] << 8)
                                         +(palette[ data[d++]      & 3] << 16),
                       pixel2 = obtenirPixel(images[1],i,j);

                if (pixel != pixel2)
                    definirPixel(images[1],i,j,pixel);
            }
        }

        Stretch_Linear(images[1],ecran,W,H);
        SDL_Flip(ecran);
        Sleep(ping);

    }while(!in.fin);

    return NULL;
}

void* f2(void* ptr)
{
    Uint8 clic;
    int test = sizeof(Input);

    do
    {
        if (test != sizeof(Input))
            continue;

        int rc3 = 0,sd3 = 0;
        test = 0;

        clic = (event.type == SDL_MOUSEBUTTONDOWN);

        in.souris.x = event.motion.x;
        in.souris.y = event.motion.y;

        if (LARGEUR < W || HAUTEUR < H)
        {
            in.souris.x *= (double)((double)W/(double)LARGEUR);
            in.souris.y *= (double)((double)H/(double)HAUTEUR);
        }

        in.souris.x = htonl(in.souris.x);
        in.souris.y = htonl(in.souris.y);

        in.rmouse = clic && (event.button.button == SDL_BUTTON_RIGHT);
        in.lmouse = clic && (event.button.button == SDL_BUTTON_LEFT);

        in.fin = ((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_ESCAPE))
            || (event.type == SDL_QUIT);

        sd3 = send(csock2,(char*)&in,sizeof(Input),0);
        rc3 = recv(csock2,(char*)&test,sizeof(int),0);

        if (err(sd3) || err(rc3))
            in.fin = 1;

    }while(!in.fin);

    return NULL;
}

void* f3(void* ptr)
{
    Uint16 key;
    int test = 0;

    do
    {
        int rc4 = 0,sd4 = 0;

        if (event.type == SDL_KEYDOWN)
        {
            key = htons(event.key.keysym.unicode);

            while(event.type != SDL_KEYUP)
                Sleep(5);
        }
        else key = htons(0);

        sd4 = send(csock3,(char*)&key,sizeof(Uint16),0);
        rc4 = recv(csock3,(char*)&test,sizeof(int),0);

        if (err(sd4) ||err(rc4))
            in.fin = 1;

        Sleep(5);

    }while(!in.fin);

    return NULL;
}


int main(int argc,char** argv)
{
    freopen("CON","w",stdout);
    HWND hWndWindow = GetDesktopWindow();
    HDC hDcWindow = GetWindowDC(hWndWindow);
    LARGEUR = GetDeviceCaps(hDcWindow,HORZRES);
    HAUTEUR = GetDeviceCaps(hDcWindow,VERTRES);

    init3();
    csock  = create_socket(PORT1);
    csock2 = create_socket(PORT2);
    csock3 = create_socket(PORT3);


    images[0] = init(LARGEUR,HAUTEUR,"ecran",1);
    images[1] = LoadBMP("ecran.bmp",1);
    memset(&in,0,sizeof(Input));


    char ok = recv(csock,(char*)&W,sizeof(int),0) * recv(csock,(char*)&H,sizeof(int),0);
    W = ntohl(W); H  = ntohl(H);
    send(csock,(char*)&ok,sizeof(char),0);

    printf("LARGEUR : %d\nHAUTEUR : %d\n\n",W,H);

    data2 = s_malloc(32*sizeof(unsigned char)*W*H),
    data  = s_malloc(32*sizeof(unsigned char)*W*H);


    pthread_create(&t1,NULL,f1,NULL);
    pthread_create(&t2,NULL,f2,NULL);
    pthread_create(&t3,NULL,f3,NULL);

    /*pthread_join(t1,NULL);
    pthread_join(t2,NULL);*/

    do
    {
        SDL_PollEvent(&event);

        Sleep(5);
    }while(!in.fin);


    free(data);
    free(data2);
    shutdown(csock,2);
    shutdown(csock2,2);
    shutdown(csock3,2);
    WSACleanup();
    SDL_FreeSurface(ecran);
    //SDL_FreeSurface(images[1]);
    SDL_Quit();
    return EXIT_SUCCESS;
}
