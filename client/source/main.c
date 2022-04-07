#include "main.h"

#include "miniz.h"

int bg = 0;
u16* backBuffer = NULL;
char hostname[256];

#define BUF_SIZE  256 * 144 * 2
#define WIFI_SSID "DsiNow"

void keyPressed(int c) {
    if (c > 0)
        iprintf("%c", c);
}

void swapBuffers() {
    backBuffer = (u16*)bgGetGfxPtr(bg);
    if (bgGetMapBase(bg) == 8)
        bgSetMapBase(bg, 0);
    else
        bgSetMapBase(bg, 8);
}

void waitForKeys() {
    while (true) {
        scanKeys();
        swiWaitForVBlank();
        if (keysDown() != 0) return;
    }
}

void drawZlib(u8* dst, u8* frame, int frame_len) {
    mz_ulong uncomp_size = BUF_SIZE;
    int err = mz_uncompress(dst, &uncomp_size, frame, frame_len);
    if (err != 0) {
        iprintf("zlib err %d\n", err);
    }
}

void drawJpeg(u8* dst, u8* frame, int frame_len) {
    // TODO https://github.com/Bodmer/JPEGDecoder
}

void onConnected() {
    consoleClear();
    iprintf("Resolving %s...\n", hostname);

    struct hostent* host = gethostbyname(hostname);
    iprintf("IP: %s\n", inet_ntoa(*(struct in_addr*)host->h_addr_list[0]));

    iprintf("Connecting to %s...\n", hostname);
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    int a = 16384;
    if ((setsockopt(sock, 0, SO_RCVBUF, &a, sizeof(int))) < 0) {
        iprintf("Error setting sock opts..\n");
    }

    struct sockaddr_in sain;
    sain.sin_family = AF_INET;
    sain.sin_port = htons(34221);
    sain.sin_addr.s_addr = *((unsigned long*)(host->h_addr_list[0]));
    if (connect(sock, (struct sockaddr*)&sain, sizeof(sain)) == -1) {
        iprintf("Connection failed. Press any key to quit.");
        waitForKeys();
        return;
    }
    iprintf("Connected to server!\n");

    const char* request_text =
        "LOGIN DVTP/0.1\n\n";

    if (send(sock, request_text, strlen(request_text), 0) == -1) {
        iprintf("Login failed.");
        waitForKeys();
        return;
    }

    // consoleClear();

    u8 recvbuf[BUF_SIZE];

    iprintf("DSi Now - Running...\n\nby Twometer");
    for (;;) {
        if (keysDown() & KEY_START) {
            shutdown(sock, 0);
            closesocket(sock);
            return;
        }

        int recvd;
        int remain = 4;
        int frame_len = 0;
        u8* target = (u8*)&frame_len;
        while ((recvd = recv(sock, target, remain, 0)) > 0) {
            remain -= recvd;
            target += recvd;
            if (remain <= 0) break;
        }

        target = (u8*)recvbuf;
        remain = frame_len;

        while ((recvd = recv(sock, target, remain, 0)) > 0) {
            remain -= recvd;
            target += recvd;
            if (remain <= 0) break;
        }

        target = (u8*)backBuffer + 256 * 40;
        drawJpeg(target, recvbuf, frame_len);

        swiWaitForVBlank();
        scanKeys();
        swapBuffers();
    }
}

void testScreen() {
    for (;;) {
        // check keys
        if (keysDown() & KEY_START) {
            return;
        }

        // Draw
        for (int iy = 60; iy < 196 - 60; iy++)
            for (int ix = 60; ix < 256 - 60; ix++)
                backBuffer[iy * 256 + ix] = (rand() & 0xFFFF) | BIT(15);

        // update
        swiWaitForVBlank();
        scanKeys();
        swapBuffers();
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

    Keyboard* kb = keyboardDemoInit();
    kb->OnKeyPressed = keyPressed;

    consoleClear();
    iprintf("\nDSi Now! by Twometer - Main Menu\n\n");
    iprintf("START - Exit\n");
    iprintf("A     - WiFi connect\n");
    iprintf("B     - Screen test\n");

    while (true) {
        waitForKeys();
        if (keysDown() & KEY_A) {
            iprintf("Connecting (A to cancel)...\n");
            if (!wifiConnect()) {
                iprintf("Failed to connect!\n");
            } else {
                iprintf("Connected.\n");
                iprintf("Enter server: ");
                scanf("%s", hostname);
                onConnected();
                return 0;
            }
        } else if (keysDown() & KEY_START) {
            iprintf("EXIT\n");
            return 0;
        } else if (keysDown() & KEY_B) {
            iprintf("Screen test mode\n");
            testScreen();
            return 0;
        }
    }
}