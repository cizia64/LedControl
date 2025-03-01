#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>

#define NUM_OPTIONS 3
#define MAX_NAME_LEN 50
typedef struct
{
    int font;
    uint32_t color1;
    uint32_t color2;
    uint32_t color3;
} MinUISettings;

#define fontcount 2
const char *fontnames[] = {
    "Next", "OG"};
MinUISettings settings;

int read_settings(const char *filename)
{

    char diskfilename[256];
    snprintf(diskfilename, sizeof(diskfilename), "/mnt/SDCARD/.userdata/%s", filename);
    FILE *file = fopen(diskfilename, "r");
    if (file == NULL)
    {
        perror("Unable to open settings file");
        return 1;
    }

    char line[256];
    int current_light = -1;
    while (fgets(line, sizeof(line), file))
    {
        
            int temp_value;
            uint32_t temp_color;

            if (sscanf(line, "font=%i", &temp_value) == 1)
            {
                settings.font = temp_value;
                continue;
            }
            if (sscanf(line, "color1=%x", &temp_color) == 1)
            {
                settings.color1 = temp_color;
                continue;
            }
            if (sscanf(line, "color2=%x", &temp_color) == 1)
            {
                settings.color2 = temp_color;
                continue;
            }
            if (sscanf(line, "color3=%x", &temp_color) == 1)
            {
                settings.color3 = temp_color;
                continue;
            }
        
    }

    fclose(file);
    return 0;
}

int save_settings(const char *filename)
{
    char diskfilename[256];
    snprintf(diskfilename, sizeof(diskfilename), "/mnt/SDCARD/.userdata/%s", filename);
    FILE *file = fopen(diskfilename, "w");
    if (file == NULL)
    {
        perror("Unable to open settings file for writing");
        return 1;
    }


    fprintf(file, "font=%i\n", settings.font);
    fprintf(file, "color1=0x%06X\n", settings.color1);
    fprintf(file, "color2=0x%06X\n", settings.color2);
    fprintf(file, "color3=0x%06X\n", settings.color3);
    

    fclose(file);
    return 0;
}



void handle_light_input(SDL_Event *event, int selected_setting)
{
    const uint32_t bright_colors[] = {
        // Blues
        0x000022, 0x000044, 0x000066, 0x000088, 0x0000AA, 0x0000CC, 0x3366FF, 0x4D7AFF, 0x6699FF, 0x80B3FF, 0x99CCFF, 0xB3D9FF,
        // Cyan
        0x002222, 0x004444, 0x006666, 0x008888, 0x00AAAA, 0x00CCCC, 0x33FFFF, 0x4DFFFF, 0x66FFFF, 0x80FFFF, 0x99FFFF, 0xB3FFFF,
        // Green
        0x002200, 0x004400, 0x006600, 0x008800, 0x00AA00, 0x00CC00, 0x33FF33, 0x4DFF4D, 0x66FF66, 0x80FF80, 0x99FF99, 0xB3FFB3,
        // Magenta
        0x220022, 0x440044, 0x660066, 0x880088, 0xAA00AA, 0xCC00CC, 0xFF33FF, 0xFF4DFF, 0xFF66FF, 0xFF80FF, 0xFF99FF, 0xFFB3FF,
        // Purple
        0x110022, 0x220044, 0x330066, 0x440088, 0x5500AA, 0x6600CC, 0x8833FF, 0x994DFF, 0xAA66FF, 0xBB80FF, 0xCC99FF, 0xDDB3FF,
        // Red
        0x220000, 0x440000, 0x660000, 0x880000, 0xAA0000, 0xCC0000, 0xFF3333, 0xFF4D4D, 0xFF6666, 0xFF8080, 0xFF9999, 0xFFB3B3,
        // Yellow
        0x222200, 0x444400, 0x666600, 0x888800, 0xAAAA00, 0xCCCC00, 0xFFFF33, 0xFFFF4D, 0xFFFF66, 0xFFFF80, 0xFFFF99, 0xFFFFB3,
        // Orange
        0x221100, 0x442200, 0x663300, 0x884400, 0xAA5500, 0xCC6600, 0xFF8833, 0xFF994D, 0xFFAA66, 0xFFBB80, 0xFFCC99, 0xFFDDB3,
        // White to Black Gradient
        0x000000, 0x141414, 0x282828, 0x3C3C3C, 0x505050, 0x646464, 0x8C8C8C, 0xA0A0A0, 0xB4B4B4, 0xC8C8C8, 0xDCDCDC, 0xFFFFFF
    };
    

    const int num_bright_colors = sizeof(bright_colors) / sizeof(bright_colors[0]);

    switch (selected_setting)
    {

    case 0: // Effect
        if (event->key.keysym.sym == SDLK_RIGHT || event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
        {
            settings.font = (settings.font % fontcount) + 1; // Increase effect (1 to 8)
        }
        else if (event->key.keysym.sym == SDLK_LEFT || event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
        {
            settings.font = (settings.font - 2 + fontcount) % fontcount + 1; // Decrease effect (1 to 8)
        }
        break;
    case 1: // Color
        if (event->key.keysym.sym == SDLK_RIGHT || event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
        {
            int current_index = -1;
            for (int i = 0; i < num_bright_colors; i++)
            {
                if (bright_colors[i] == settings.color1)
                {
                    current_index = i;
                    break;
                }
            }
            settings.color1 = bright_colors[(current_index + 1) % num_bright_colors];
        }
        else if (event->key.keysym.sym == SDLK_LEFT || event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
        {
            int current_index = -1;
            for (int i = 0; i < num_bright_colors; i++)
            {
                if (bright_colors[i] == settings.color1)
                {
                    current_index = i;
                    break;
                }
            }
            settings.color1 = bright_colors[(current_index - 1 + num_bright_colors) % num_bright_colors];
        }
        break;
    case 2: // Color
        if (event->key.keysym.sym == SDLK_RIGHT || event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
        {
            int current_index = -1;
            for (int i = 0; i < num_bright_colors; i++)
            {
                if (bright_colors[i] == settings.color2)
                {
                    current_index = i;
                    break;
                }
            }
            settings.color2 = bright_colors[(current_index + 1) % num_bright_colors];
        }
        else if (event->key.keysym.sym == SDLK_LEFT || event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
        {
            int current_index = -1;
            for (int i = 0; i < num_bright_colors; i++)
            {
                if (bright_colors[i] == settings.color2)
                {
                    current_index = i;
                    break;
                }
            }
            settings.color2 = bright_colors[(current_index - 1 + num_bright_colors) % num_bright_colors];
        }
        break;
    case 3: // Color
        if (event->key.keysym.sym == SDLK_RIGHT || event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
        {
            int current_index = -1;
            for (int i = 0; i < num_bright_colors; i++)
            {
                if (bright_colors[i] == settings.color3)
                {
                    current_index = i;
                    break;
                }
            }
            settings.color3 = bright_colors[(current_index + 1) % num_bright_colors];
        }
        else if (event->key.keysym.sym == SDLK_LEFT || event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
        {
            int current_index = -1;
            for (int i = 0; i < num_bright_colors; i++)
            {
                if (bright_colors[i] == settings.color3)
                {
                    current_index = i;
                    break;
                }
            }
            settings.color3 = bright_colors[(current_index - 1 + num_bright_colors) % num_bright_colors];
        }
        break;
   
    }

    // Save settings after each change

    save_settings("minuisettings.txt");
}

void draw_filled_circle(SDL_Renderer *renderer, int x, int y, int radius)
{
    for (int w = 0; w < radius * 2; w++)
    {
        for (int h = 0; h < radius * 2; h++)
        {
            int dx = radius - w; // horizontal offset
            int dy = radius - h; // vertical offset
            if ((dx * dx + dy * dy) <= (radius * radius))
            {
                SDL_RenderDrawPoint(renderer, x + dx, y + dy);
            }
        }
    }
}

// Function to draw a rounded rectangle
void draw_rounded_rect(SDL_Renderer *renderer, int x, int y, int w, int h, int radius)
{
    // Draw the central part of the rectangle
    SDL_Rect rect = {x + radius, y, w - 2 * radius, h};
    SDL_RenderFillRect(renderer, &rect);

    rect.x = x;
    rect.y = y + radius;
    rect.w = w;
    rect.h = h - 2 * radius;
    SDL_RenderFillRect(renderer, &rect);

    // Draw the corners
    draw_filled_circle(renderer, x + radius, y + radius, radius);                 // Top-left corner
    draw_filled_circle(renderer, x + w - radius - 1, y + radius, radius);         // Top-right corner
    draw_filled_circle(renderer, x + radius, y + h - radius - 1, radius);         // Bottom-left corner
    draw_filled_circle(renderer, x + w - radius - 1, y + h - radius - 1, radius); // Bottom-right corner
}

char last_button_pressed[50] = "None";

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    if (TTF_Init() == -1)
    {
        SDL_Log("Unable to initialize SDL_ttf: %s", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0)
    {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Options Example",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          1024, 768, SDL_WINDOW_SHOWN);

    // SDL_Window *window = SDL_CreateWindow("Options Example",
    //                                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    //                                       800, 600, SDL_WINDOW_FULLSCREEN_DESKTOP);

    if (!window)
    {
        SDL_Log("Unable to create window: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        SDL_Log("Unable to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    TTF_Font *font = TTF_OpenFont("main.ttf", 50);   // Specify your font path
    TTF_Font *fontsm = TTF_OpenFont("main.ttf", 36); // Specify your font path
    if (!font || !fontsm)
    {
        SDL_Log("Unable to open font: %s", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Read initial settings
    if (read_settings("minuisettings.txt") != 0)
    {
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_GameController *controller = NULL;
    for (int i = 0; i < SDL_NumJoysticks(); ++i)
    {
        if (SDL_IsGameController(i))
        {
            controller = SDL_GameControllerOpen(i);
            if (controller)
            {
                SDL_Log("Game Controller %s connected", SDL_GameControllerName(controller));
                break;
            }
        }
    }

    if (!controller)
    {
        SDL_Log("No game controller available");
    }

    int selected_light = 0;
    int selected_setting = 0;
    bool running = true;
    SDL_Event event;

    SDL_Color hex_to_sdl_color(uint32_t hex)
    {
        SDL_Color color;
        color.r = (hex >> 16) & 0xFF;
        color.g = (hex >> 8) & 0xFF;
        color.b = hex & 0xFF;
        color.a = 255;
        return color;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    // Get the window size
    int window_width, window_height;
    SDL_GetWindowSize(window, &window_width, &window_height);
    while (running)
    {

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_DOWN:
                    selected_setting = (selected_setting + 1) % 6;
                    break;
                case SDLK_UP:
                    selected_setting = (selected_setting - 1 + 6) % 6;
                    break;
                case SDLK_TAB:
                    selected_light = (selected_light - 1 + NUM_OPTIONS) % NUM_OPTIONS;
                    break;
                case SDLK_RIGHT:
                case SDLK_LEFT:
                    handle_light_input(&event, selected_setting);
                    break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    // SDL_Log("Selected: %s -> brightness: %d, effect: %d, color: 0x%06X, duration: %d",
                    //         lights[selected_light].name,
                    //         lights[selected_light].brightness,
                    //         lights[selected_light].effect,
                    //         lights[selected_light].color,
                    //         lights[selected_light].duration);
                    break;
                }
            }
            else if (event.type == SDL_CONTROLLERBUTTONDOWN)
            {
                switch (event.cbutton.button)
                {
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                    selected_setting = (selected_setting + 1) % 6;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_UP:
                    selected_setting = (selected_setting - 1 + 6) % 6;
                    break;
                case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                    selected_light = (selected_light - 1 + NUM_OPTIONS) % NUM_OPTIONS;
                    break;
                case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                    selected_light = (selected_light + 1) % NUM_OPTIONS;
                    break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                    handle_light_input(&event, selected_setting);
                    break;
                case SDL_CONTROLLER_BUTTON_B:
                    // strcpy(last_button_pressed, "DPAD Down");
                    // SDL_Log("Selected: %s -> brightness: %d, effect: %d, color: 0x%06X, duration: %d",
                    //         lights[selected_light].name,
                    //         lights[selected_light].brightness,
                    //         lights[selected_light].effect,
                    //         lights[selected_light].color,
                    //         lights[selected_light].duration);
                    break;
                case SDL_CONTROLLER_BUTTON_A:
                    SDL_Quit();
                    break;
                    // Add more cases for other buttons as needed
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Color color = {255, 255, 255, 255};        // Default white color
        SDL_Color darkcolor = {32, 36, 32, 255};       // Default white color
        SDL_Color highlight_color = {0, 0, 0, 255};    // Cyan color for the current setting
        SDL_Color selected_color = {255, 255, 0, 255}; // Yellow color for the selected option


        // Display settings
        const char *settings_labels[4] = {"Font", "Color1", "Color2", "Color3"};
        int settings_values[4] = {
            settings.font,
            settings.color1,
            settings.color2,
            settings.color3,
        };

     // Display light name
     char light_name_text[256];
     snprintf(light_name_text, sizeof(light_name_text), "%s", "MinUI Next Settings");
     SDL_Surface *surface = TTF_RenderText_Solid(font, light_name_text, color);
     SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

     int text_width = surface->w;
     int text_height = surface->h;
     SDL_FreeSurface(surface);

     // Calculate centered position
     SDL_Rect dstrect = (SDL_Rect){50, 30, text_width, text_height};
     SDL_RenderCopy(renderer, texture, NULL, &dstrect);
     SDL_DestroyTexture(texture);
        for (int j = 0; j < 4; ++j)
        {
            char setting_text[256];

            SDL_Color bgcolor = (j == selected_setting) ? color : highlight_color;

            // saving this for font names
            if (j == 0)
            { // Display font name instead of number
         
                snprintf(setting_text, sizeof(setting_text), "%s: %s", settings_labels[j], fontnames[settings_values[j] - 1]);

                SDL_Color current_color = (j == selected_setting) ? highlight_color : color;

                surface = TTF_RenderText_Solid(font, setting_text, current_color);
                texture = SDL_CreateTextureFromSurface(renderer, surface);

                text_width = surface->w;
                text_height = surface->h;
                SDL_SetRenderDrawColor(renderer, bgcolor.r, bgcolor.g, bgcolor.b, 255);
                draw_rounded_rect(renderer, 20, 115 + j * 92, text_width + 60, 88, 40);
                SDL_FreeSurface(surface);

                // Calculate centered position
                dstrect = (SDL_Rect){50, 122 + j * 92, text_width, text_height};

                SDL_RenderCopy(renderer, texture, NULL, &dstrect);

                SDL_DestroyTexture(texture);
            }
            else if (j < 5)
            { // Display color as hex code
                snprintf(setting_text, sizeof(setting_text), "%s:", settings_labels[j]);

                // Render the "COLOR:" text
                SDL_Color current_color = (j == selected_setting) ? highlight_color : color; // Highlight color if selected
                surface = TTF_RenderText_Solid(font, setting_text, current_color);
                texture = SDL_CreateTextureFromSurface(renderer, surface);

                text_width = surface->w;
                text_height = surface->h;
                SDL_SetRenderDrawColor(renderer, bgcolor.r, bgcolor.g, bgcolor.b, 255);
                draw_rounded_rect(renderer, 20, 115 + j * 92, text_width + 130, 88, 40);
                // Draw color cube
                SDL_Color color_cube = hex_to_sdl_color(settings_values[j]);
                SDL_Rect color_rect = {30 + text_width + 30, 122 + j * 92, 50, 55}; // Cube size 50x50, adjust x position as needed
                SDL_SetRenderDrawColor(renderer, color_cube.r, color_cube.g, color_cube.b, color_cube.a);
                // SDL_RenderFillRect(renderer, &color_rect);
                draw_rounded_rect(renderer, 30 + text_width + 30, 130 + j * 92, 56, 56, 10);

                SDL_FreeSurface(surface);

                // Calculate text position
                dstrect = (SDL_Rect){50, 122 + j * 92, text_width, text_height};
                SDL_RenderCopy(renderer, texture, NULL, &dstrect);
                SDL_DestroyTexture(texture);
            }
           
            else
            {
                snprintf(setting_text, sizeof(setting_text), "%s: %d", settings_labels[j], settings_values[j]);

                SDL_Color current_color = (j == selected_setting) ? highlight_color : color;

                surface = TTF_RenderText_Solid(font, setting_text, current_color);
                texture = SDL_CreateTextureFromSurface(renderer, surface);

                text_width = surface->w;
                text_height = surface->h;
                SDL_SetRenderDrawColor(renderer, bgcolor.r, bgcolor.g, bgcolor.b, 255);
                draw_rounded_rect(renderer, 20, 115 + j * 92, text_width + 60, 88, 40);
                SDL_FreeSurface(surface);

                // Calculate centered position
                dstrect = (SDL_Rect){50, 122 + j * 92, text_width, text_height};

                SDL_RenderCopy(renderer, texture, NULL, &dstrect);

                SDL_DestroyTexture(texture);
            }
        }
        SDL_SetRenderDrawColor(renderer, 32, 36, 32, 255);
        draw_rounded_rect(renderer, window_width - 190, window_height - 90, 170, 80, 40);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        draw_rounded_rect(renderer, window_width - 180, window_height - 80, 60, 60, 30);
        char button_text[256];
        snprintf(button_text, sizeof(button_text), "B");
        surface = TTF_RenderText_Solid(fontsm, button_text, darkcolor);
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        text_width = surface->w;
        text_height = surface->h;
        // Calculate centered position
        dstrect = (SDL_Rect){window_width - 160, window_height - 78, text_width, text_height};
        SDL_RenderCopy(renderer, texture, NULL, &dstrect);
        SDL_DestroyTexture(texture);
        SDL_FreeSurface(surface);

        snprintf(button_text, sizeof(button_text), "Quit");
        surface = TTF_RenderText_Solid(fontsm, button_text, color);
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        text_width = surface->w;
        text_height = surface->h;
        // Calculate centered position
        dstrect = (SDL_Rect){window_width - 110, window_height - 78, text_width, text_height};
        SDL_RenderCopy(renderer, texture, NULL, &dstrect);
        SDL_DestroyTexture(texture);
        SDL_FreeSurface(surface);

        SDL_RenderPresent(renderer);
    }
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
