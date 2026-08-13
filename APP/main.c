// test ffmpeg -> sdl3
// cmake --build APP/build
// execute -> ./APP/build/preview
#include <SDL3/SDL.h>
#include <hidapi/hidapi.h>

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VIDEO_WIDTH 640
#define VIDEO_HEIGHT 480
#define BYTES_PER_PIXEL 4
#define FRAME_SIZE (VIDEO_WIDTH * VIDEO_HEIGHT * BYTES_PER_PIXEL)

#define TARGET_VID 0x0483
#define TARGET_PID 0x5750
#define TARGET_USAGE_PAGE 0xFF00
#define TARGET_USAGE 0x01

#define KVM_REPORT_ID 0x10
#define KVM_PROTOCOL_VER 0x01

#define KVM_MSG_KEYBOARD 0x01
#define KVM_MSG_MOUSE 0x02
#define KVM_MSG_RELEASE_ALL 0x03
#define KVM_MSG_STATUS 0x80

#define REPORT_SIZE 64
#define PAYLOAD_SIZE 59
#define MAX_MOUSE_DELTA 127

static bool read_exact(FILE *pipe, uint8_t *buffer, size_t size){
    size_t offset = 0;
    while(offset < size){
        size_t n = fread(buffer + offset, 1, size - offset, pipe);
        if (n == 0){
            return false;
        }
        offset += n;
    }
    return true;
}

static uint8_t scancode_to_hid_usage(SDL_Scancode scancode){
    if ((scancode >= SDL_SCANCODE_A) && (scancode <= SDL_SCANCODE_Z)) {
        return (uint8_t)(0x04 + (scancode - SDL_SCANCODE_A));
    }
    if ((scancode >= SDL_SCANCODE_1) &&(scancode <= SDL_SCANCODE_9) ) {
        return (uint8_t)(0x1E + (scancode - SDL_SCANCODE_1));
    }
    if (scancode == SDL_SCANCODE_0) {
    return 0x27;
    }
    if (scancode == SDL_SCANCODE_RETURN) {
        return 0x28;
    }
    if (scancode == SDL_SCANCODE_ESCAPE) {
        return 0x29;
    }
    if (scancode == SDL_SCANCODE_SPACE) {
        return 0x2C;
    }
    if (scancode == SDL_SCANCODE_TAB) {
        return 0x2B;
    }
    if (scancode == SDL_SCANCODE_BACKSPACE) {
        return 0x2A;
    }
    if (scancode == SDL_SCANCODE_UP) {
        return 0x52;
    }
    if (scancode == SDL_SCANCODE_DOWN) {
        return 0x51;
    }
    if (scancode == SDL_SCANCODE_LEFT) {
        return 0x50;
    }
    if (scancode == SDL_SCANCODE_RIGHT) {
        return 0x4F;
    }
    return 0x00;
}

static hid_device *open_kvm_device(void){
    struct hid_device_info *devices = hid_enumerate(TARGET_VID, TARGET_PID);
    struct hid_device_info *cur = devices;
    hid_device *dev = NULL;

    while (cur != NULL) {
        if(cur->usage_page == TARGET_USAGE_PAGE && cur->usage == TARGET_USAGE){
            dev = hid_open_path(cur->path);
            break;
        }
        cur = cur->next;
    }
    hid_free_enumeration(devices);
    return dev;
}

static void put_int16le(uint8_t *dst, int16_t value){
    uint16_t raw = (uint16_t)value;
    dst[0] = (uint8_t)(raw & 0xFF);
    dst[1] = (uint8_t)((raw >> 8) & 0xFF);
}

static bool read_kvm_status(hid_device *dev, uint8_t seq, uint8_t request_type){
    uint8_t response[REPORT_SIZE] = {0};
    int n = hid_read_timeout(dev, response, sizeof(response), 1000);

    if (n < 8){
        fprintf(stderr, "No Ack\n");
        return false;
    }
    if (
        response[0] != KVM_REPORT_ID ||
        response[1] != KVM_PROTOCOL_VER ||
        response[2] != KVM_MSG_STATUS ||
        response[3] != seq ||
        response[5] != request_type
    ){
        fprintf(stderr, "Bad Ack\n");
        return false;
    }
    if (response[6] != 0) {
        fprintf(stderr, "KVM error: status=%u detail=%u\n", response[6], response[7]);
        return false;
    }
    return true;
}

static bool send_kvm_packet(hid_device *dev, uint8_t msg_type, uint8_t *seq,
                            const uint8_t *payload, uint8_t payload_len){
    uint8_t packet[REPORT_SIZE] = {0};
    uint8_t current_seq = (*seq)++;

    if(payload_len > PAYLOAD_SIZE){
        return false;
    }
    packet[0] = KVM_REPORT_ID;
    packet[1] = KVM_PROTOCOL_VER;
    packet[2] = msg_type;
    packet[3] = current_seq;
    packet[4] = payload_len;

    if (payload_len > 0 && payload != NULL) {
        memcpy(&packet[5], payload, payload_len);
    }

    int written = hid_write(dev, packet, sizeof(packet));
    if (written != REPORT_SIZE) {
        fprintf(stderr, "hid_write failed: written=%d\n", written);
        return false;
    }
    return read_kvm_status(dev, current_seq, msg_type);
}

static bool kvm_send_keyboard(hid_device *dev, uint8_t *seq, uint8_t modifiers, uint8_t key){
    uint8_t payload[8] = {0};
    payload[0] = modifiers;
    payload[1] = 0x00;
    payload[2] = key;

    return send_kvm_packet(dev, KVM_MSG_KEYBOARD, seq, payload, sizeof(payload));
}

static bool kvm_release_all(hid_device *dev, uint8_t *seq){
    return send_kvm_packet(dev, KVM_MSG_RELEASE_ALL, seq, NULL, 0);
}

static bool kvm_send_mouse(hid_device *dev, uint8_t *seq, int16_t dx, int16_t dy,
                            int8_t wheel, uint8_t buttons){
    uint8_t payload[7] = {0};
    payload[0] = buttons;
    put_int16le(&payload[1], dx);
    put_int16le(&payload[3], dy);
    payload[5] = (uint8_t)wheel;
    payload[6] = 0x00;

    return send_kvm_packet(dev, KVM_MSG_MOUSE, seq, payload, sizeof(payload));
}

static uint8_t sdl_mouse_button_mask(uint8_t button){
    if (button == SDL_BUTTON_LEFT) {
        return 0x01;
    }
    if (button == SDL_BUTTON_RIGHT) {
        return 0x02;
    }
    if (button == SDL_BUTTON_MIDDLE) {
        return 0x04;
    }
    return 0x00;
}

int main(void){
    const char *ffmpeg_cmd  =
        "ffmpeg -nostdin -hide_banner -loglevel error "
        "-f avfoundation "
        "-framerate 30 "
        "-video_size 640x480 "
        "-pixel_format nv12 "
        "-i \"0:none\" "
        "-f rawvideo "
        "-pix_fmt rgba "
        "-";

        FILE *camera_pipe = popen(ffmpeg_cmd, "r");
        if (camera_pipe == NULL) {
            fprintf(stderr, "Failed to start ffmpeg\n");
            return 1;
        }

        uint8_t *frame = malloc(FRAME_SIZE);

        if (frame == NULL){
            fprintf(stderr, "Failed to allocate frame buffer\n");
            pclose(camera_pipe);
            return 1;
        }

        if (!SDL_Init(SDL_INIT_VIDEO)){
            fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
            free(frame);
            pclose(camera_pipe);
            return 1;
        }

        SDL_Window *window = SDL_CreateWindow("Camera_Preview", VIDEO_WIDTH, VIDEO_HEIGHT, 0);
        
        if (window == NULL) {
            fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
            SDL_Quit();
            free(frame);
            pclose(camera_pipe);
            return 1;
        }
        SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);

        if (renderer == NULL) {
            fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
            SDL_DestroyWindow(window);
            SDL_Quit();
            free(frame);
            pclose(camera_pipe);
            return 1;
        }
        SDL_Texture *texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        VIDEO_WIDTH,
        VIDEO_HEIGHT
    );
    if (texture == NULL){
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        free(frame);
        pclose(camera_pipe);
        return 1;
    }
    if (hid_init() != 0) {
        fprintf(stderr, "hid_init failed\n");
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        free(frame);
        pclose(camera_pipe);
        return 1;
    }
    hid_device *kvm_dev = open_kvm_device();
    if (kvm_dev == NULL) {
        fprintf(stderr, "Can't open KVM device\n");
        hid_exit();
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        free(frame);
        pclose(camera_pipe);
        return 1;
    }
    uint8_t kvm_seq = 1;
    uint8_t mouse_buttons = 0;
    /* code is running or not*/
    bool running = true;
    bool control_mode = false; /* true = receive keyboard, mouse */
    uint8_t skip_mouse_motion = 0;
    while (running) {
        SDL_Event event;
        /* if event -> while */
        while (SDL_PollEvent(&event)) {
            /* if close window -> SDL_EVENT_QUIT */
            if (event.type == SDL_EVENT_QUIT){
                running = false;
            }
            /* If the mouse button is pressed within the SDL window and the window is not currently in control mode, 
            then control mode will be entered.*/
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (!control_mode) {
                    control_mode = true;
                    /*Mouse cursor is limited to the viewport. Relative movement (dx/dy) 
                    will not stop when the cursor reaches the edge of the screen.*/
                    SDL_SetWindowRelativeMouseMode(window, true);
                    skip_mouse_motion = 8;
                    printf("Control mode on\n");
                }
                else {
                    uint8_t mask = sdl_mouse_button_mask(event.button.button);
                    if (mask != 0 && (mouse_buttons & mask) == 0){
                        mouse_buttons |= mask;
                        kvm_send_mouse(kvm_dev, &kvm_seq, 0, 0, 0, mouse_buttons);
                    }
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP){
                if (control_mode){
                    uint8_t mask = sdl_mouse_button_mask(event.button.button);
                    if (mask != 0 && (mouse_buttons & mask) != 0){
                        mouse_buttons &= (uint8_t)~mask;
                        kvm_send_mouse(kvm_dev, &kvm_seq, 0, 0, 0, mouse_buttons);
                    }
                }
            }
            else if (event.type == SDL_EVENT_KEY_DOWN) {
                /* event.key.key -> key = what? */
                SDL_Keycode key = event.key.key;
                /* Which physical location on the keyboard? */
                SDL_Scancode scancode = event.key.scancode;
                uint8_t usage = scancode_to_hid_usage(scancode);
                if (event.key.repeat) {
                    continue;
                }
                /* exit control mode */
                if (key == SDLK_ESCAPE) {
                    control_mode = false;
                    SDL_SetWindowRelativeMouseMode(window, false);
                    skip_mouse_motion = 0;
                    printf("Control mode off\n");
                    printf("Release all\n");
                    kvm_release_all(kvm_dev, &kvm_seq);
                    mouse_buttons = 0;
                    continue;
                }
                
                if (control_mode && usage != 0) {
                    /* press A -> print KEY DOWN: key=A, scancode=4 */
                    printf("Key down: key=%s, usage=0x%02X\n", SDL_GetKeyName(key), usage);
                    kvm_send_keyboard(kvm_dev, &kvm_seq, 0x00, usage);
                }
            }
            else if (event.type == SDL_EVENT_KEY_UP) {
                SDL_Keycode key = event.key.key;
                SDL_Scancode scancode = event.key.scancode;
                uint8_t usage = scancode_to_hid_usage(scancode);

                if (control_mode && usage != 0) {
                    printf("key up: key=%s, usage=0x%02X\n", SDL_GetKeyName(key), usage);
                    kvm_send_keyboard(kvm_dev, &kvm_seq, 0x00, 0x00);
                }
            }
            else if (event.type == SDL_EVENT_MOUSE_MOTION) {
                if (control_mode) {
                    int16_t dx = (int16_t)event.motion.xrel;
                    int16_t dy = (int16_t)event.motion.yrel;

                    if (skip_mouse_motion > 0) {
                        skip_mouse_motion--;
                        continue;
                    }

                    if (dx == 0 && dy == 0) {
                        continue;
                    }

                    if (dx < -MAX_MOUSE_DELTA || dx > MAX_MOUSE_DELTA ||
                        dy < -MAX_MOUSE_DELTA || dy > MAX_MOUSE_DELTA) {
                        continue;
                    }

                    printf("Mouse: dx=%d, dy=%d\n", dx, dy);
                    kvm_send_mouse(kvm_dev, &kvm_seq, dx, dy, 0, mouse_buttons);
                }
            }
        }
        if (!read_exact(camera_pipe, frame, FRAME_SIZE)){
            break;
        }
        SDL_UpdateTexture(texture, NULL, frame, VIDEO_WIDTH * BYTES_PER_PIXEL);
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }
    kvm_release_all(kvm_dev, &kvm_seq);
    hid_close(kvm_dev);
    hid_exit();
    
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    free(frame);
    pclose(camera_pipe);

    return 0;
}
