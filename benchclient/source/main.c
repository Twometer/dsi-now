#include "main.h"

int bg = 0;
u16* backBuffer = NULL;
char hostname[256];

#define MAXRECVSZ 65000
#define HOSTNAME  "10.0.2.140"
#define PORT      34221
#define WIFI_SSID "DsiNow"

void hang() {
    while (1) {
        scanKeys();
        swiWaitForVBlank();
        if (keysDown() & KEY_START) break;
    }
}

void onConnected() {
    consoleClear();
    struct hostent* host = gethostbyname(HOSTNAME);
    iprintf("IP: %s\n", inet_ntoa(*(struct in_addr*)host->h_addr_list[0]));

    int sockfd_in;
    int sockfd_out;
    if ((sockfd_in = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        iprintf("Failed to create IN socket\n");
        hang();
        return;
    }
    if ((sockfd_out = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        iprintf("Failed to create OUT socket\n");
        hang();
        return;
    }

    // REMOTE
    struct sockaddr_in addr_remote;
    memset(&addr_remote, 0, sizeof(addr_remote));
    addr_remote.sin_family = AF_INET;
    addr_remote.sin_port = htons(PORT);
    addr_remote.sin_addr.s_addr = *((unsigned long*)(host->h_addr_list[0]));

    // LOCAL
    struct sockaddr_in addr_local;
    memset(&addr_local, 0, sizeof(addr_local));
    addr_local.sin_family = AF_INET;
    addr_local.sin_port = htons(PORT);
    addr_local.sin_addr.s_addr = INADDR_ANY;

    const char* request_text =
        "LOGIN DVTP/0.1\n\n";

    if (bind(sockfd_in, (struct sockaddr*)&addr_local, sizeof(addr_local)) < 0) {
        iprintf("Failed to bind socket\n");
        hang();
        return;
    } else {
        iprintf("Bound. ");
    }

    int i = 1;
    ioctl(sockfd_in, FIONBIO, &i);
    iprintf("IOCTL'd. \n");

    int sent = sendto(sockfd_out, request_text, strlen(request_text), 0, (struct sockaddr*)&addr_remote, sizeof(addr_remote));
    iprintf("Sent %d bytes\n", sent);

    int recv_addr_len = sizeof(addr_remote);
    uint8_t buffer[MAXRECVSZ];
    for (;;) {
        int recvd = recvfrom(sockfd_in, buffer, MAXRECVSZ, 0, (struct sockaddr*)&addr_remote, &recv_addr_len);
        if (recvd != -1) {
            iprintf("Received %d bytes\n", recvd);
        }

        swiWaitForVBlank();
        scanKeys();
    }
}

bool wifiConnect() {
    Wifi_ScanMode();
    do {
        int num = Wifi_GetNumAP();
        for (int i = 0; i < num; i++) {
            Wifi_AccessPoint ap;
            Wifi_GetAPData(i, &ap);

            if (strcmp(ap.ssid, WIFI_SSID) == 0) {
                iprintf("Connecting to %s\n", ap.ssid);
                Wifi_SetIP(0, 0, 0, 0, 0);
                Wifi_ConnectAP(&ap, WEPMODE_NONE, 0, 0);
                int status = ASSOCSTATUS_DISCONNECTED;

                while (status != ASSOCSTATUS_ASSOCIATED && status != ASSOCSTATUS_CANNOTCONNECT) {
                    status = Wifi_AssocStatus();
                    int len = strlen(ASSOCSTATUS_STRINGS[status]);
                    iprintf("\x1b[0;0H\x1b[K");
                    iprintf("\x1b[0;%dH%s", (32 - len) / 2, ASSOCSTATUS_STRINGS[status]);

                    scanKeys();
                    if (keysDown() & KEY_A) return false;
                    swiWaitForVBlank();
                }
                onConnected();
                return true;
            }
        }
        swiWaitForVBlank();
        scanKeys();
    } while (!(keysDown() & KEY_A));
    iprintf("Failed to find AP\n");
    return false;
}

int main(void) {
    videoSetMode(MODE_5_2D);
    vramSetPrimaryBanks(VRAM_A_MAIN_BG_0x06000000, VRAM_B_MAIN_BG_0x06020000,
                        VRAM_C_SUB_BG, VRAM_D_LCD);

    Wifi_InitDefault(false);

    consoleDemoInit();

    bg = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    backBuffer = (u16*)bgGetGfxPtr(bg) + 256 * 256;

    wifiConnect();
    return 0;
}