#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#if defined (WIN32)
    #include <winsock2.h>
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

typedef struct Input Input;
struct Input
{
    POINT souris;
    char fin,rmouse,lmouse;
    int key;
};

const int i = 1;
BITMAPFILEHEADER m_BitmapFileHeader;
PBITMAPINFOHEADER m_pBitmapInfoHeader = '\0';
PBITMAPINFO m_pBitmapInfo = '\0';
LPBYTE m_pBitmapData = '\0';
HDC m_hDcBitmap = '\0';
HBITMAP hBitmap = '\0';
HWND hWndWindow;
HDC hDcWindow;
BITMAP Bitmap;
WORD   Couleur;
int ScreenX,ScreenY,n;

void init_format(char str[256]);
int init_capture(void);
void init2(SOCKET*,SOCKADDR_IN*,int);
void init3(void);
SOCKET create_socket2(char*,int);
int RLECompress(unsigned char*,unsigned char*,int);
FILE* s_fopen(const char*,const char*);
void* s_malloc(size_t);
int err(int);

void init_format(char str[256])
{
    memset(str,0,64);
    memset(str+64,1,64);
    memset(str+128,2,64);
    memset(str+192,3,64);
}

int init_capture(void)
{
    hWndWindow = GetDesktopWindow();
    hDcWindow = GetWindowDC(hWndWindow);
    ScreenX = GetDeviceCaps(hDcWindow,HORZRES);
    ScreenY = GetDeviceCaps(hDcWindow,VERTRES);

    m_hDcBitmap = CreateCompatibleDC(hDcWindow);
    if(!m_hDcBitmap) return 0;
    hBitmap = CreateCompatibleBitmap(hDcWindow,ScreenX,ScreenY);
    if(!hBitmap) return 0;
    if(!SelectObject(m_hDcBitmap,hBitmap)) return 0;

    if (!GetObject(hBitmap,sizeof(BITMAP),(LPSTR)&Bitmap)) return 0;

    Couleur = (WORD)(Bitmap.bmPlanes * Bitmap.bmBitsPixel);
    if (Couleur == 1)
        Couleur = 1;
    else if (Couleur <= 4)
        Couleur = 4;
    else if (Couleur <= 8)
        Couleur = 8;
    else if (Couleur <= 16)
        Couleur = 16;
    else if (Couleur <= 24)
        Couleur = 24;
    else Couleur = 32;

    if (Couleur != 24)
        m_pBitmapInfo = (PBITMAPINFO)s_malloc(sizeof(BYTE)*(sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1<< Couleur)));
    else
        m_pBitmapInfo = (PBITMAPINFO)s_malloc(sizeof(BYTE)*sizeof(BITMAPINFOHEADER));

    if(!m_pBitmapInfo) return 0;

    n = Couleur / 8;

    m_pBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    m_pBitmapInfo->bmiHeader.biWidth = Bitmap.bmWidth;
    m_pBitmapInfo->bmiHeader.biHeight = Bitmap.bmHeight;
    m_pBitmapInfo->bmiHeader.biPlanes = Bitmap.bmPlanes;
    m_pBitmapInfo->bmiHeader.biBitCount = Bitmap.bmBitsPixel;
    if (Couleur < 24)
        m_pBitmapInfo->bmiHeader.biClrUsed = (1<<Couleur);

    m_pBitmapInfo->bmiHeader.biCompression = BI_RGB;
    m_pBitmapInfo->bmiHeader.biSizeImage = ((m_pBitmapInfo->bmiHeader.biWidth * Couleur +31) & ~31) /8 * m_pBitmapInfo->bmiHeader.biHeight;
    m_pBitmapInfo->bmiHeader.biClrImportant = 0;
    m_pBitmapInfoHeader = (PBITMAPINFOHEADER) m_pBitmapInfo;
    m_pBitmapData = s_malloc(sizeof(BYTE)*m_pBitmapInfoHeader->biSizeImage);
    if (!m_pBitmapData) return 0;

    return 1;
}

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

SOCKET create_socket2(char* adresse_ip,int port)
{
    SOCKET sock;
    SOCKADDR_IN sin;

    sin.sin_addr.s_addr = inet_addr((const char*)adresse_ip);

    init2(&sock,&sin,port);

    int tentatives = 1;
    while ((connect(sock,(SOCKADDR*)&sin,sizeof(sin))) == SOCKET_ERROR)
    {
        printf("Tentative de connexion %d ...\r",tentatives);
        tentatives++;
    }

    printf("Connexion etablie sur le port %d.\n",port);
    return sock;
}

void* s_malloc(size_t s)
{
    void* ptr = NULL;

    if ((ptr = malloc(s)) == NULL)
        exit(EXIT_FAILURE);

    return ptr;
}

int RLECompress(unsigned char *output,unsigned char *input,int length)
{
   int count = 0,index,i,out = 0;
   unsigned char pixel;

   while (count < length)
   {
      index = count;
      pixel = input[index++];
      while (index < length && index - count < 127 && input[index] == pixel)
         index++;
      if (index - count == 1)
      {

         while (((index < length) && (index - count < 127)
               && (input[index] != input[index-1]))
               || ((index > 1) && (input[index] != input[index-2])))
            index++;

         while (index < length && input[index] == input[index-1])
            index--;
         output[out++] = (unsigned char)(count - index);
         for (i = count ; i < index ; i++)
            output[out++] = input[i];
      }
      else
      {
         output[out++] = (unsigned char)(index - count);
         output[out++] = pixel;
      }
      count = index;
   }
   return(out);
}

int err(int es)
{
    return (es == -1 || !es);
}

FILE* s_fopen(const char* file_name,const char* mode)
{
    FILE* file = NULL;

    if ((file = fopen(file_name,mode)) == NULL)
    {
        fprintf(stderr,"Impossible d'ouvrir le fichier %s en mode %s\n",file_name,mode);
        exit(EXIT_FAILURE);
    }

    return file;
}

#endif // MAIN_H_INCLUDED
