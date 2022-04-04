#include "main.h"

int bg = 0;
u16* backBuffer = NULL;
char hostname[256];

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

void onConnected() {
    consoleClear();
    iprintf("Resolving %s...\n", hostname);

    struct hostent* host = gethostbyname(hostname);
    iprintf("IP: %s\n", inet_ntoa(*(struct in_addr*)host->h_addr_list[0]));

    iprintf("Connecting to %s...\n", hostname);
    int sock = socket(AF_INET, SOCK_STREAM, 0);

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
    iprintf("Receiving video...\n");

    waitForKeys();
    if (keysDown() & KEY_START) return;
}

int main(void) {
    videoSetMode(MODE_5_2D);
    vramSetPrimaryBanks(VRAM_A_MAIN_BG_0x06000000, VRAM_B_MAIN_BG_0x06020000,
                        VRAM_C_SUB_BG, VRAM_D_LCD);

    consoleDemoInit();

    bg = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    backBuffer = (u16*)bgGetGfxPtr(bg) + 256 * 256;

    Keyboard* kb = keyboardDemoInit();
    kb->OnKeyPressed = keyPressed;

    consoleClear();
    iprintf("\nDSi Now! by Twometer - Main Menu\n\n");
    iprintf("START - Exit\n");
    iprintf("A     - WiFi connect\n");

    while (true) {
        waitForKeys();
        if (keysDown() & KEY_A) {
            iprintf("Connecting to WiFi...\n");
            if (!Wifi_InitDefault(WFC_CONNECT)) {
                iprintf("Failed to connect!\n");
            } else {
                iprintf("Connected.\n");
                iprintf("Enter server: ");
                scanf("%s", hostname);
                onConnected();
                return 0;
            }
        } else if (keysDown() & KEY_START)
            break;
    }
}