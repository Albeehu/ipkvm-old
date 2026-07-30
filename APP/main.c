// test ffmpeg -> sdl3
// cmake --build /preview/build
// execute -> ./preview/build/preview
#include <SDL3/SDL.h>

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define VIDEO_WIDTH 640
#define VIDEO_HEIGHT 480
#define BYTES_PER_PIXEL 4
#define FRAME_SIZE (VIDEO_WIDTH * VIDEO_HEIGHT * BYTES_PER_PIXEL)

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
    /* code is running or not*/
    bool running = true;
    bool control_mode = false; /* true = receive keyboard, mouse */
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
                    printf("Control mode on\n");
                }
            }
            else if (event.type == SDL_EVENT_KEY_DOWN) {
                /* event.key.key -> key = what? */
                SDL_Keycode key = event.key.key;
                /* Which physical location on the keyboard? */
                SDL_Scancode scancode = event.key.scancode;
                uint8_t usage = scancode_to_hid_usage(scancode);
                /* press A -> print KEY DOWN: key=A, scancode=4 */
                printf("Key down: key=%s, usage=0x%02X\n", SDL_GetKeyName(key), usage);
                /* exit control mode */
                if (key == SDLK_ESCAPE) {
                    control_mode = false;
                    SDL_SetWindowRelativeMouseMode(window, false);
                    printf("Control mode off\n");
                    printf("Release all\n");
                }
            }
            else if (event.type == SDL_EVENT_KEY_UP) {
                SDL_Keycode key = event.key.key;
                SDL_Scancode scancode = event.key.scancode;
                uint8_t usage = scancode_to_hid_usage(scancode);

                printf("key up: key=%s, usage=0x%02X\n", SDL_GetKeyName(key), usage);
            }
            else if (event.type == SDL_EVENT_MOUSE_MOTION) {
                if (control_mode) {
                    printf("Mouse: dx=%f, dy=%f\n", event.motion.xrel, event.motion.yrel);
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
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    free(frame);
    pclose(camera_pipe);

    return 0;
}
