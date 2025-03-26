#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"
#include "gb.h"
#include <stdio.h>
#include <stdlib.h>

u32 *frame_buffer;

bool load_bootrom(Gameboy *gb, char *filename)
{
}

void load_rom(Gameboy *gb, char *filename)
{
    FILE *f;
    fopen_s(&f, filename, "rb");

    if (f)
    {
        /* Find the ROM file size */
        fseek(f, 0L, SEEK_END);
        u64 rom_size = ftell(f);
        gb->memory.rom_size = rom_size;
        fseek(f, 0L, SEEK_SET);

        if (gb->memory.rom)
        {
            free(gb->memory.rom);
        }

        if (rom_size > 0)
        {
            gb->memory.rom = (u8 *)malloc(gb->memory.rom_size);
            memset(gb->memory.rom, 0xFF, gb->memory.rom_size);
            if (rom_size < 0x10000)
            {
                fread(gb->memory.rom + gb->cpu.reg.pc, 1, gb->memory.rom_size, f);
                fseek(f, 0L, SEEK_SET);
                fread(gb->memory.memory_map + gb->cpu.reg.pc, 1, gb->memory.rom_size, f);
            }
            else
            {
                /* Handle ROM too large for memory */
                printf("Error: ROM size exceeds memory bounds.\n");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            /* Handle empty ROM file */
            printf("Error: ROM file is empty.\n");
            exit(EXIT_FAILURE);
        }
        fclose(f);
    }
    else
    {
        /* Handle file opening error */
        printf("Error: Failed to open the ROM file.\n");
        exit(EXIT_FAILURE);
    }
}

void update_viewport(int w, int h, SDL_Window *window, SDL_Renderer *renderer, SDL_Texture *texture)
{
    int client_width;
    int client_height;
    SDL_GetRenderOutputSize(renderer, &client_width, &client_height);
    int x_factor = (int)(client_width / (r64)w);
    int y_factor = (int)(client_height / (r64)h);

    // Maintain aspect ratio
    if (x_factor > y_factor)
    {
        x_factor = y_factor;
    }
    else
    {
        y_factor = x_factor;
    }

    int scaled_width = x_factor * w;
    int scaled_height = y_factor * h;

    // Center render area
    int x = (client_width - scaled_width) / 2;
    int y = (client_height - scaled_height) / 2;
    SDL_Rect client_rect = {x, y, scaled_width, scaled_height};

    if (renderer)
    {
        SDL_SetRenderViewport(renderer, &client_rect);

        char *pixels;
        int pitch;

        SDL_LockTexture(texture, NULL, (void **)&pixels, &pitch);

        for (int i = 0, sp = 0, dp = 0; i < h; i++, dp += w, sp += pitch)
            memcpy(pixels + sp, frame_buffer + dp, w * 4);

        SDL_UnlockTexture(texture);
        SDL_RenderTexture(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        // SDL_Delay(1);
    }
}

void render(u64 ticks)
{
    for (int i = 0, c = 0; i < LCD_HEIGHT; i++)
    {
        for (int j = 0; j < LCD_WIDTH; j++, c++)
        {
            frame_buffer[c] = (int)(i * i - j * j + ticks) | 0xff000000;
        }
    }
}

int main(int argv, char **argc)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        return -1;
    }

    SDL_Window *window = SDL_CreateWindow("Gameboy Emulator", LCD_WIDTH, LCD_HEIGHT, SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        // TODO:
    }

    SDL_SetWindowMinimumSize(window, LCD_WIDTH, LCD_HEIGHT);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer)
    {
        // TODO:
    }

    SDL_Texture *texture =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, LCD_WIDTH, LCD_HEIGHT);
    if (!texture)
    {
        // TODO:
    }
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    char *bootrom = "../roms/dmg_boot.bin";
    char *rom_path = "R:/dmg/tests/roms/blargg/mem_timing/individual/01-read_timing.gb";

    Gameboy gb;
    gb_init(&gb);

    gb.cpu.reg.pc = 0x00;
    load_rom(&gb, rom_path);
    gb.cpu.reg.pc = 0x100;

    frame_buffer = calloc(LCD_WIDTH * LCD_HEIGHT, sizeof(u32));

    u64 cycles = 0;
    bool running = true;
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
                if (event.key.key == SDLK_ESCAPE)
                {
                    running = false;
                }
                switch (event.key.key)
                {
                case SDLK_1:
                    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
                    break;
                case SDLK_2:
                    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
                    break;
                case SDLK_3:
                    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_PIXELART);
                    break;
                }
                break;
            }
        }

        cycles += gb_run(&gb);
        update_viewport(LCD_WIDTH, LCD_HEIGHT, window, renderer, texture);
        render(SDL_GetTicks());
    }

    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyTexture(texture);
    SDL_Quit();

    return 0;
}
