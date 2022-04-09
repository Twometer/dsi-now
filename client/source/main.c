#include "main.h"

#include "miniz.h"
#include "picojpeg.h"

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

u8* current_jpeg_frame;
int current_jpeg_frame_len;
int current_jpeg_frame_offset;

int min(int a, int b) {
    return a < b ? a : b;
}

unsigned char
pjpeg_need_bytes_callback(unsigned char* pBuf, unsigned char buf_size, unsigned char* pBytes_actually_read, void* pCallback_data) {
    uint n = min(current_jpeg_frame_len - current_jpeg_frame_offset, buf_size);
    memcpy(pBuf, current_jpeg_frame + current_jpeg_frame_offset, n);
    *pBytes_actually_read = (unsigned char)n;
    current_jpeg_frame_offset += n;
    return 0;
}

inline u16 rgb15(int r, int g, int b) {
    return (u16)(((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3) | (1 << 15));
}

void drawJpeg(u8* dst, u8* frame, int frame_len) {
    // iprintf("Decoding %d bytes\n", frame_len);
    current_jpeg_frame = frame;
    current_jpeg_frame_len = frame_len;
    current_jpeg_frame_offset = 0;

    // picojpeg invocation
    pjpeg_image_info_t image_info;
    int status = pjpeg_decode_init(&image_info, pjpeg_need_bytes_callback, NULL, 0);
    if (status) {
        iprintf("JPEG decode_init failed %d\n", status);
        return;
    }

    int mcu_x = 0;
    int mcu_y = 0;

    for (;;) {
        status = pjpeg_decode_mcu();
        if (status == PJPG_NO_MORE_BLOCKS) {
            break;
        } else if (status != 0) {
            iprintf("JPEG decode_mcu failed %d\n", status);
            return;
        }

        if (mcu_y >= image_info.m_MCUSPerCol) {
            iprintf("something ain't right\n");
            break;
        }

        int dst_x = mcu_x * image_info.m_MCUWidth;
        int dst_y = mcu_y * image_info.m_MCUHeight;
        u16* pDst_row = backBuffer + (dst_y * 256 + dst_x);

        for (int y = 0; y < image_info.m_MCUHeight; y += 8) {
            const int by_limit = min(8, image_info.m_height - (mcu_y * image_info.m_MCUHeight + y));
            for (int x = 0; x < image_info.m_MCUWidth; x += 8) {
                const int bx_limit = min(8, image_info.m_width - (mcu_x * image_info.m_MCUWidth + x));

                // calc offset in decoder buffer
                uint src_ofs = (x * 8U) + (y * 16U);
                const uint8* pSrcR = image_info.m_pMCUBufR + src_ofs;
                const uint8* pSrcG = image_info.m_pMCUBufG + src_ofs;
                const uint8* pSrcB = image_info.m_pMCUBufB + src_ofs;

                // copy to screen buffer
                u16* pDst_block = pDst_row + x;
                for (int by = 0; by < by_limit; by++) {
                    u16* dst = pDst_block;
                    for (int bx = 0; bx < bx_limit; bx++) {
                        u8 r = *pSrcR++;
                        u8 g = *pSrcG++;
                        u8 b = *pSrcB++;
                        *dst = rgb15(r, g, b);
                        dst++;
                    }

                    pSrcR += (8 - bx_limit);
                    pSrcG += (8 - bx_limit);
                    pSrcB += (8 - bx_limit);
                    pDst_block += 256;
                }
            }
            pDst_row += 256 * 8;
        }

        mcu_x++;
        if (mcu_x == image_info.m_MCUSPerRow) {
            mcu_x = 0;
            mcu_y++;
        }
    }
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

    const char* request = "DVTP";

    if (send(sock, request, strlen(request), 0) == -1) {
        iprintf("Login failed.");
        waitForKeys();
        return;
    }

    // consoleClear();

    u8 recvbuf[BUF_SIZE];

    iprintf("DSi Now - Running...\n\nby Twometer\n");
    for (;;) {
        u32 keyState = keysCurrent();
        send(sock, &keyState, sizeof(u32), 0);

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

        target = (u8*)(backBuffer + 256 * 40);
        drawJpeg(target, recvbuf, frame_len);

        // iprintf("Frame!\n");

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
                consoleClear();
                iprintf("Connected.\n");
                iprintf("Enter server: ");
                scanf("%s", hostname);
                onConnected();
                return 0;
            }
        } else if (keysDown() & KEY_START) {
            iprintf("Leaving\n");
            return 0;
        } else if (keysDown() & KEY_B) {
            iprintf("Screen test mode\n");
            testScreen();
            return 0;
        }
    }
}