#include "main.h"

#define PORT1       5900
#define PORT2       6000

SOCKET sock,sock2;
Input in;
pthread_t t1,t2;
unsigned char *data = NULL,*data2 = NULL;
char str[256];
int rmouse = 0,lmouse = 0;

void* f1(void* ptr)
{
    do
    {
        if(!BitBlt(m_hDcBitmap,0,0,ScreenX,ScreenY,hDcWindow,0,0,SRCCOPY)) return 0;
        if (!GetDIBits(m_hDcBitmap,hBitmap,0,(WORD) m_pBitmapInfoHeader->biHeight,m_pBitmapData,m_pBitmapInfo,DIB_RGB_COLORS)) return 0;

        int i = 0,d = 0,rc1 = 0,rc2 = 0,sd1 = 0,sd2 = 0,taille_image = 0,test1 = 0,test2 = 0;

        for (i = 0 ; i < m_pBitmapInfoHeader->biSizeImage ; i += n)
            data[d++] = ((str[m_pBitmapData[i]]<<4) + (str[m_pBitmapData[i+1]]<<2) + str[m_pBitmapData[i+2]]);


        taille_image = RLECompress(data2,data,m_pBitmapInfoHeader->biSizeImage);

        sd1 = send(sock,(char*)&taille_image,sizeof(int),0);
        rc1 = recv(sock,(char*)&test1,sizeof(int),0);
        sd2 = send(sock,(char*)data2,taille_image,0);
        rc2 = recv(sock,(char*)&test2,sizeof(int),0);

        fprintf(stderr,"sd1 : %d\nrc1 : %d\nsd2 : %d\nrc2 : %d\n\n",sd1,rc1,sd2,rc2);

        if (err(rc1) || err(rc2)  || err(sd1) || err(sd2))
            return NULL;


    }while(!in.fin);

    return NULL;
}

void* f2(void* ptr)
{
    do
    {
        int rc3 = 0,sd3 = 0;

        rc3 = recv(sock2,(char*)&in,sizeof(Input),0);
        sd3 = send(sock2,(char*)&rc3,sizeof(int),0);

        if (!rc3 || !sd3)
            break;

        in.souris.x = ntohl(in.souris.x);
        in.souris.y = ntohl(in.souris.y);

        in.key = ntohl(in.key);

        if (in.key)
        {
            if (!isascii(in.key))
                break;
            if (islower(in.key))
                in.key = towupper(in.key);
            keybd_event(in.key,0,0,0);
            keybd_event(in.key,0,KEYEVENTF_KEYUP,0);
        }

        if ((in.souris.x > 0) && (in.souris.x < ScreenX) && (in.souris.y > 0) && (in.souris.y < ScreenY))
            SetCursorPos(in.souris.x,in.souris.y);


        if (in.rmouse && !rmouse)
        {
            mouse_event(MOUSEEVENTF_RIGHTDOWN,in.souris.x,in.souris.y,0,0);
            rmouse = 1;
        }

        if (!in.rmouse && rmouse)
        {
            mouse_event(MOUSEEVENTF_RIGHTUP,in.souris.x,in.souris.y,0,0);
            rmouse = 0;
        }

        if (in.lmouse && !lmouse)
        {
            mouse_event(MOUSEEVENTF_LEFTDOWN,in.souris.x,in.souris.y,0,0);
            lmouse = 1;
        }

        if (!in.lmouse && lmouse)
        {
            mouse_event(MOUSEEVENTF_LEFTUP,in.souris.x,in.souris.y,0,0);
            lmouse = 0;
        }

    }while(!in.fin);

    return NULL;
}

int main(void)
{
    init3();

    char IP[256];
    FILE* file_ip = s_fopen("IP.txt","r");

    if (fscanf(file_ip,"%s",IP) != 1)
    {
        fprintf(stderr,"Erreur : impossible de lire le fichier IP");
        return EXIT_FAILURE;
    }

    fclose(file_ip);

    sock  = create_socket2(IP,PORT1);
    sock2 = create_socket2(IP,PORT2);

    init_format(str);
    memset(&in,0,sizeof(in));

    if (!init_capture())
        return EXIT_FAILURE;

    data = s_malloc(4 * sizeof(unsigned char) * m_pBitmapInfoHeader->biSizeImage);
    data2 = s_malloc(4 * sizeof(unsigned char) * m_pBitmapInfoHeader->biSizeImage);

    int sx = htonl(ScreenX),sy = htonl(ScreenY);
    char ok;
    if (!send(sock,(char*)&sx,sizeof(int),0)) return EXIT_FAILURE;
    if (!send(sock,(char*)&sy,sizeof(int),0)) return EXIT_FAILURE;
    if (recv(sock,(char*)&ok,sizeof(char),0) != 1) return EXIT_FAILURE;

    pthread_create(&t1,NULL,f1,NULL);
    pthread_create(&t2,NULL,f2,NULL);

    do
    {
        Sleep(5);
    }while(!in.fin);

    printf("\nTermine");
    getchar();
    free(data);
    free(data2);
    free(m_pBitmapInfo);
    free(m_pBitmapData);
    DeleteDC(m_hDcBitmap);
    DeleteObject(hBitmap);
    closesocket(sock);
    closesocket(sock2);
    WSACleanup();
    return EXIT_SUCCESS;
}
