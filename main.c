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
#include <time.h>

#define NUM_OPTIONS 2
#define MAX_NAME_LEN 50
typedef struct
{
    char name[MAX_NAME_LEN];
    int effect;
    uint32_t color;
    uint32_t color2;
    int duration;
    char friendlyname[50];
    int maxeffects;
    int brightness;
    int trigger;
} LightSettings;

LightSettings lights[NUM_OPTIONS];
const char *lightnames[] = {
    "Central light", "L&R joysticks"};
#define NROF_TRIGGERS 12
const char *triggernames[] = {
    "B", "A", "Y", "X", "L", "R", "SELECT", "START", "MENU", "ALL", "LR", "DPAD"};

const char *effect_names[] = {
    "Linear", "Breathe", "Interval Breathe", "Static", "Blink 1", "Blink 2", "Blink 3", "Color Drift", // 1-8  native effects from LED driver
    "Twinkle", "Fire", "Glitter", "NeonGlow", "Firefly", "Aurora", "Reactive",                         // 9-15 effect logic managed by LED controller daemon
    "Battery Level", "CPU Speed", "CPU Temperature", "Ambilight", "Nothing",                           // 16-20 Effects from CrossMix
    "Rainbow Snake", "Rotation", "Rotation Mirror", "Directions"};                                     // 21-24 Effects requiring effect_rgb_hex_lr, exclusive to “lr” light.

int read_settings(const char *filename, LightSettings *lights, int max_lights)
{

    char diskfilename[256];
    snprintf(diskfilename, sizeof(diskfilename), "/mnt/SDCARD/System/etc/%s", filename);
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
        if (line[0] == '[')
        {
            // Section header
            char light_name[MAX_NAME_LEN];
            if (sscanf(line, "[%49[^]]]", light_name) == 1)
            {
                current_light++;
                if (current_light < max_lights)
                {
                    strncpy(lights[current_light].name, light_name, MAX_NAME_LEN - 1);
                    strncpy(lights[current_light].friendlyname, lightnames[current_light], MAX_NAME_LEN - 1);
                    lights[current_light].name[MAX_NAME_LEN - 1] = '\0'; // Ensure null-termination
                }
                else
                {
                    current_light = -1; // Reset if max_lights exceeded
                }
            }
        }
        else if (current_light >= 0 && current_light < max_lights)
        {
            int temp_value;
            uint32_t temp_color;

            if (sscanf(line, "effect=%d", &temp_value) == 1)
            {
                lights[current_light].effect = temp_value;
                continue;
            }
            if (sscanf(line, "color=%x", &temp_color) == 1)
            {
                lights[current_light].color = temp_color;
                continue;
            }
            if (sscanf(line, "color2=%x", &temp_color) == 1)
            {
                lights[current_light].color2 = temp_color;
                continue;
            }
            if (sscanf(line, "duration=%d", &temp_value) == 1)
            {
                lights[current_light].duration = temp_value;
                continue;
            }
            if (sscanf(line, "maxeffects=%d", &temp_value) == 1)
            {
                lights[current_light].maxeffects = temp_value;
                continue;
            }
            if (sscanf(line, "brightness=%d", &temp_value) == 1)
            {
                lights[current_light].brightness = temp_value;
                continue;
            }
            if (sscanf(line, "trigger=%d", &temp_value) == 1)
            {
                lights[current_light].trigger = temp_value;
                continue;
            }
        }
    }

    fclose(file);
    return 0;
}

int save_settings(const char *filename, LightSettings *lights, int max_lights)
{
    char diskfilename[256];
    snprintf(diskfilename, sizeof(diskfilename), "/mnt/SDCARD/System/etc/%s", filename);
    FILE *file = fopen(diskfilename, "w");
    if (file == NULL)
    {
        perror("Unable to open settings file for writing");
        return 1;
    }

    for (int i = 0; i < max_lights; ++i)
    {
        fprintf(file, "[%s]\n", lights[i].name);
        fprintf(file, "effect=%d\n", lights[i].effect);
        fprintf(file, "color=0x%06X\n", lights[i].color);
        fprintf(file, "color2=0x%06X\n", lights[i].color2);
        fprintf(file, "duration=%d\n", lights[i].duration);
        fprintf(file, "maxeffects=%d\n", lights[i].maxeffects);
        fprintf(file, "brightness=%d\n", lights[i].brightness);
        fprintf(file, "trigger=%d\n\n", lights[i].trigger);
    }

    fclose(file);
    return 0;
}

void handle_light_input(LightSettings *light, SDL_Event *event, int selected_setting)
{
    const uint32_t bright_colors[] = {
        // Blues
        0x000080, // Navy Blue
        0x0080FF, // Sky Blue
        0x00BFFF, // Deep Sky Blue
        0x8080FF, // Light Blue
        0x483D8B, // Dark Slate Blue
        0x7B68EE, // Medium Slate Blue

        // Cyan
        0x00FFFF, // Cyan
        0x40E0D0, // Turquoise
        0x80FFFF, // Light Cyan
        0x008080, // Teal
        0x00CED1, // Dark Turquoise
        0x20B2AA, // Light Sea Green

        // Green
        0x00FF00, // Green
        0x32CD32, // Lime Green
        0x7FFF00, // Chartreuse
        0x80FF00, // Lime
        0x80FF80, // Light Green
        0xADFF2F, // Green Yellow

        // Magenta
        0xFF00FF, // Magenta
        0xFF80C0, // Light Magenta
        0xEE82EE, // Violet
        0xDA70D6, // Orchid
        0xDDA0DD, // Plum
        0xBA55D3, // Medium Orchid

        // Purple
        0x800080, // Purple
        0x8A2BE2, // Blue Violet
        0x9400D3, // Dark Violet
        0x9B30FF, // Purple2
        0xA020F0, // Purple
        0x9370DB, // Medium Purple

        // Red
        0xFF0000, // Red
        0xFF4500, // Red Orange
        0xFF6347, // Tomato
        0xDC143C, // Crimson
        0xFF69B4, // Hot Pink
        0xFF1493, // Deep Pink

        // Yellow and Orange
        0xFFD700, // Gold
        0xFFA500, // Orange
        0xFF8000, // Orange Red
        0xFFFF00, // Yellow
        0xFFFF80, // Light Yellow
        0xFFDAB9, // Peach Puff

        // Others
        0xFFFFFF, // White
        0xC0C0C0, // Silver
        0x000000  // Black
    };

    const int num_bright_colors = sizeof(bright_colors) / sizeof(bright_colors[0]);

    switch (selected_setting)
    {

    case 0: // Effect
        if (event->key.keysym.sym == SDLK_RIGHT || event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
        {
            light->effect = (light->effect % light->maxeffects) + 1; // Increase effect (1 to 8)
        }
        else if (event->key.keysym.sym == SDLK_LEFT || event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
        {
            light->effect = (light->effect - 2 + light->maxeffects) % light->maxeffects + 1; // Decrease effect (1 to 8)
        }
        break;
    case 1: // Color
        if (event->key.keysym.sym == SDLK_RIGHT || event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
        {
            int current_index = -1;
            for (int i = 0; i < num_bright_colors; i++)
            {
                if (bright_colors[i] == light->color)
                {
                    current_index = i;
                    break;
                }
            }
            SDL_Log("saved settings to disk and shm %d", current_index);
            light->color = bright_colors[(current_index + 1) % num_bright_colors];
        }
        else if (event->key.keysym.sym == SDLK_LEFT || event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
        {
            int current_index = -1;
            for (int i = 0; i < num_bright_colors; i++)
            {
                if (bright_colors[i] == light->color)
                {
                    current_index = i;
                    break;
                }
            }
            light->color = bright_colors[(current_index - 1 + num_bright_colors) % num_bright_colors];
        }
        break;
    case 2: // Color2
        if (event->key.keysym.sym == SDLK_RIGHT || event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
        {
            int current_index = -1;
            for (int i = 0; i < num_bright_colors; i++)
            {
                if (bright_colors[i] == light->color2)
                {
                    current_index = i;
                    break;
                }
            }
            SDL_Log("saved settings to disk and shm %d", current_index);
            light->color2 = bright_colors[(current_index + 1) % num_bright_colors];
        }
        else if (event->key.keysym.sym == SDLK_LEFT || event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
        {
            int current_index = -1;
            for (int i = 0; i < num_bright_colors; i++)
            {
                if (bright_colors[i] == light->color2)
                {
                    current_index = i;
                    break;
                }
            }
            light->color2 = bright_colors[(current_index - 1 + num_bright_colors) % num_bright_colors];
        }
        break;
    case 3: // Duration
        if (event->key.keysym.sym == SDLK_RIGHT || event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
        {
            light->duration = (light->duration + 100) % 5000; // Increase duration
        }
        else if (event->key.keysym.sym == SDLK_LEFT || event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
        {
            light->duration = (light->duration - 100 + 5000) % 5000; // Decrease duration
        }
        break;
    case 4: // Brightness (synchronized between Central light and L&R joysticks)
    {
        int new_brightness = light->brightness;
        if (event->key.keysym.sym == SDLK_RIGHT || event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
        {
            new_brightness = (new_brightness + 1 > 60) ? 60 : new_brightness + 1;
        }
        else if (event->key.keysym.sym == SDLK_LEFT || event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
        {
            new_brightness = (new_brightness - 1 < -1) ? -1 : new_brightness - 1;
        }

        // Apply the same value to both lights
        lights[0].brightness = new_brightness;
        lights[1].brightness = new_brightness;
    }
    break;

    case 5: // trigger
        if (event->key.keysym.sym == SDLK_RIGHT || event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
        {
            light->trigger = (light->trigger % NROF_TRIGGERS) + 1; // Increase effect (1 to 8)
        }
        else if (event->key.keysym.sym == SDLK_LEFT || event->cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT)
        {
            light->trigger = (light->trigger - 2 + NROF_TRIGGERS) % NROF_TRIGGERS + 1; // Decrease effect (1 to 8)
        }
        break;
    }

    // Save settings after each change

    save_settings("led_daemon.conf", lights, NUM_OPTIONS);
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

char *read_effect_description(const char *effect_name)
{
    static char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    char path[256];
    snprintf(path, sizeof(path), "./effect_desc/%s.txt", effect_name);

    FILE *f = fopen(path, "r");
    if (!f)
    {
        snprintf(buffer, sizeof(buffer), "No description available.");
        return buffer;
    }

    fread(buffer, 1, sizeof(buffer) - 1, f);
    fclose(f);
    return buffer;
}

int get_mainui_brightness()
{
    static int cached_value = 60; // default value
    static time_t last_read = 0;

    time_t now = time(NULL);
    if (now - last_read >= 5)
    {
        FILE *fp = popen("/usr/trimui/bin/shmvar ledvalue", "r");
        if (fp)
        {
            char buffer[32];
            if (fgets(buffer, sizeof(buffer), fp))
            {
                int ui_value = atoi(buffer);
                cached_value = (ui_value * 60) / 10;
            }
            pclose(fp);
        }
        last_read = now;
    }

    return cached_value;
}

int main(int argc, char *argv[])
{

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

    TTF_Font *font = TTF_OpenFont("/mnt/SDCARD/System/resources/DejaVuSans.ttf", 40);   // Specify your font path
    TTF_Font *fontsm = TTF_OpenFont("/mnt/SDCARD/System/resources/DejaVuSans.ttf", 36); // Specify your font path
    if (!font || !fontsm)
    {
        SDL_Log("Unable to open font: %s", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Read initial settings
    if (read_settings("led_daemon.conf", lights, NUM_OPTIONS) != 0)
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

        if (SDL_WaitEventTimeout(&event, 50))
        {
            do
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
                        handle_light_input(&lights[selected_light], &event, selected_setting);
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
                        handle_light_input(&lights[selected_light], &event, selected_setting);
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
            } while (SDL_PollEvent(&event));
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Color color = {255, 255, 255, 255};        // Default white color
        SDL_Color darkcolor = {32, 36, 32, 255};       // Default white color
        SDL_Color highlight_color = {0, 0, 0, 255};    // Cyan color for the current setting
        SDL_Color selected_color = {255, 255, 0, 255}; // Yellow color for the selected option

        // Display light name
        char light_name_text[256];
        TTF_SetFontStyle(font, TTF_STYLE_BOLD);

        // Générer le texte
        snprintf(light_name_text, sizeof(light_name_text), "%s", lights[selected_light].friendlyname);
        SDL_Surface *surface = TTF_RenderText_Blended(font, light_name_text, color);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

        int text_width = surface->w;
        int text_height = surface->h;
        SDL_FreeSurface(surface);

        // Centered coordinates for text
        int center_x = (window_width - text_width) / 2;
        int center_y = 30;

        // Background rectangle: full width, height-adjusted
        SDL_Rect bg_rect = {
            .x = 0,
            .y = center_y - 5,
            .w = window_width,
            .h = text_height + 10};
        SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255); // Dark grey
        SDL_RenderFillRect(renderer, &bg_rect);

        // Display centered text
        SDL_Rect dstrect = {
            .x = center_x,
            .y = center_y,
            .w = text_width,
            .h = text_height};
        SDL_RenderCopy(renderer, texture, NULL, &dstrect);
        SDL_DestroyTexture(texture);

        // Reset style
        TTF_SetFontStyle(font, TTF_STYLE_NORMAL);

        // Display settings
        const char *settings_labels[6] = {"Effect", "Color1", "Color2", "Speed", "Brightness", "Trigger"};
        int settings_values[6] = {
            lights[selected_light].effect,
            lights[selected_light].color,
            lights[selected_light].color2,
            lights[selected_light].duration,
            lights[selected_light].brightness,
            lights[selected_light].trigger,
        };

        for (int j = 0; j < 6; ++j)
        {
            char setting_text[256];

            SDL_Color bgcolor = (j == selected_setting) ? color : highlight_color;

            if (j == 0)
            { // Display effect name instead of number
              // snprintf(setting_text, sizeof(setting_text), "%s: %s", settings_labels[j], selected_light == 3 ? lr_effect_names[settings_values[j] - 1] : selected_light == 2 ? topbar_effect_names[settings_values[j] - 1]
              //                                                                                                                                                                : effect_names[settings_values[j] - 1]);

                snprintf(setting_text, sizeof(setting_text), "%s: %s",
                         settings_labels[j],
                         effect_names[settings_values[j] - 1]);

                // Render the effect name
                SDL_Color current_color = (j == selected_setting) ? highlight_color : color;

                surface = TTF_RenderText_Solid(font, setting_text, current_color);
                texture = SDL_CreateTextureFromSurface(renderer, surface);

                text_width = surface->w;
                text_height = surface->h;

                SDL_SetRenderDrawColor(renderer, bgcolor.r, bgcolor.g, bgcolor.b, 255);
                SDL_Rect rect = {20, 112, text_width + 60, 68};
                SDL_RenderFillRect(renderer, &rect);

                SDL_FreeSurface(surface);

                // Calculate centered position
                dstrect = (SDL_Rect){50, 122, text_width, text_height};
                SDL_RenderCopy(renderer, texture, NULL, &dstrect);
                SDL_DestroyTexture(texture);
            }
            else if (j < 3)
            { // Display color as hex code
                snprintf(setting_text, sizeof(setting_text), "%s:", settings_labels[j]);

                // Render the "COLOR:" text
                SDL_Color current_color = (j == selected_setting) ? highlight_color : color; // Highlight color if selected
                surface = TTF_RenderText_Solid(font, setting_text, current_color);
                texture = SDL_CreateTextureFromSurface(renderer, surface);

                text_width = surface->w;
                text_height = surface->h;
                SDL_SetRenderDrawColor(renderer, bgcolor.r, bgcolor.g, bgcolor.b, 255);
                // draw_rounded_rect(renderer, 20, 112 + j * 82, text_width + 130, 68, 30);
                SDL_Rect rect = {20, 112 + j * 82, text_width + 130, 68};
                SDL_RenderFillRect(renderer, &rect);

                int cube_x = 30 + text_width + 30;
                int cube_y = 118 + j * 82;
                int cube_w = 56;
                int cube_h = 56;
                int corner_radius = 10;

                // Step 1: draw the slightly larger black background
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                draw_rounded_rect(renderer, cube_x - 1, cube_y - 1, cube_w + 2, cube_h + 2, corner_radius + 1);

                // Step 2: Draw the coloured cube on top
                SDL_Color color_cube = hex_to_sdl_color(settings_values[j]);
                SDL_SetRenderDrawColor(renderer, color_cube.r, color_cube.g, color_cube.b, color_cube.a);
                draw_rounded_rect(renderer, cube_x, cube_y, cube_w, cube_h, corner_radius);

                SDL_FreeSurface(surface);

                // Calculate text position
                dstrect = (SDL_Rect){50, 122 + j * 82, text_width, text_height};
                SDL_RenderCopy(renderer, texture, NULL, &dstrect);
                SDL_DestroyTexture(texture);
            }
            else if (j == 5)
            { // Display effect name instead of number
                snprintf(setting_text, sizeof(setting_text), "%s: %s", settings_labels[j], triggernames[settings_values[j] - 1]);

                SDL_Color current_color = (j == selected_setting) ? highlight_color : color;

                surface = TTF_RenderText_Solid(font, setting_text, current_color);
                texture = SDL_CreateTextureFromSurface(renderer, surface);

                text_width = surface->w;
                text_height = surface->h;
                SDL_SetRenderDrawColor(renderer, bgcolor.r, bgcolor.g, bgcolor.b, 255);
                // draw_rounded_rect(renderer, 20, 112 + j * 82, text_width + 60, 68, 30);
                SDL_Rect rect = {20, 112 + j * 82, text_width + 60, 68};
                SDL_RenderFillRect(renderer, &rect);
                SDL_FreeSurface(surface);

                // Calculate centered position
                dstrect = (SDL_Rect){50, 122 + j * 82, text_width, text_height};

                SDL_RenderCopy(renderer, texture, NULL, &dstrect);

                SDL_DestroyTexture(texture);
            }
            else
            {
                if (j == 4 && settings_values[j] == -1)
                {
                    int brightness_from_ui = get_mainui_brightness();
                    snprintf(setting_text, sizeof(setting_text), "%s: %d (MainUI)", settings_labels[j], brightness_from_ui);
                }
                else
                {
                    snprintf(setting_text, sizeof(setting_text), "%s: %d", settings_labels[j], settings_values[j]);
                }

                SDL_Color current_color = (j == selected_setting) ? highlight_color : color;

                surface = TTF_RenderText_Solid(font, setting_text, current_color);
                texture = SDL_CreateTextureFromSurface(renderer, surface);

                text_width = surface->w;
                text_height = surface->h;
                SDL_SetRenderDrawColor(renderer, bgcolor.r, bgcolor.g, bgcolor.b, 255);
                // draw_rounded_rect(renderer, 20, 112 + j * 82, text_width + 60, 68, 30);
                SDL_Rect rect = {20, 112 + j * 82, text_width + 60, 68};
                SDL_RenderFillRect(renderer, &rect);
                SDL_FreeSurface(surface);

                // Calculate centered position
                dstrect = (SDL_Rect){50, 122 + j * 82, text_width, text_height};

                SDL_RenderCopy(renderer, texture, NULL, &dstrect);

                SDL_DestroyTexture(texture);
            }
        }

        SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
        // draw_rounded_rect(renderer, 20, window_height - 90, 330, 80, 40);
        SDL_Rect rect = {20, window_height - 95, 350, 80};
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        // draw_rounded_rect(renderer, 30, window_height - 80, 100, 60, 30);
        rect = (SDL_Rect){30, window_height - 85, 100, 60};
        SDL_RenderFillRect(renderer, &rect);
        char button_text[256];
        snprintf(button_text, sizeof(button_text), "L/R");
        surface = TTF_RenderText_Solid(fontsm, button_text, darkcolor);
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        text_width = surface->w;
        text_height = surface->h;
        // Calculate centered position
        dstrect = (SDL_Rect){50, window_height - 76, text_width, text_height};
        SDL_RenderCopy(renderer, texture, NULL, &dstrect);
        SDL_DestroyTexture(texture);
        SDL_FreeSurface(surface);

        snprintf(button_text, sizeof(button_text), "Light select");
        surface = TTF_RenderText_Solid(fontsm, button_text, color);
        texture = SDL_CreateTextureFromSurface(renderer, surface);
        text_width = surface->w;
        text_height = surface->h;
        // Calculate centered position
        dstrect = (SDL_Rect){140, window_height - 78, text_width, text_height};
        SDL_RenderCopy(renderer, texture, NULL, &dstrect);
        SDL_DestroyTexture(texture);
        SDL_FreeSurface(surface);

        SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
        // draw_rounded_rect(renderer, window_width - 190, window_height - 90, 170, 80, 40);
        // SDL_Rect rect = {window_width - 190, window_height - 90, 170, 80};
        rect = (SDL_Rect){window_width - 190, window_height - 95, 170, 80};
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        // draw_rounded_rect(renderer, window_width - 180, window_height - 80, 60, 60, 30);
        // SDL_Rect rect = {window_width - 180, window_height - 80, 60, 60};
        rect = (SDL_Rect){window_width - 180, window_height - 85, 60, 60};

        SDL_RenderFillRect(renderer, &rect);

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

        // snprintf(button_text, sizeof(button_text), "By Robin Morgan :D");
        // surface = TTF_RenderText_Solid(fontsm, button_text, color);
        // texture = SDL_CreateTextureFromSurface(renderer, surface);
        // text_width = surface->w;
        // text_height = surface->h;

        // dstrect = (SDL_Rect){(window_width - text_width) / 2, 660, text_width, text_height};
        // SDL_RenderCopy(renderer, texture, NULL, &dstrect);
        // SDL_DestroyTexture(texture);
        // SDL_FreeSurface(surface);

        // =========== Display effect description ===========
        int effect_index = lights[selected_light].effect - 1;
        const char *effect_name = effect_names[effect_index];
        char *description = read_effect_description(effect_name);

        // Frame on right
        int box_x = window_width / 2 + 20;
        int box_y = 93;
        int box_w = window_width / 2 - 60;
        int box_h = window_height - 200;

        SDL_Rect desc_box = {box_x, box_y, box_w, box_h};
        SDL_SetRenderDrawColor(renderer, 32, 32, 32, 255);
        SDL_RenderFillRect(renderer, &desc_box);

        // Multiline text
        int line_height = 36;
        int max_lines = box_h / line_height;
        char *line = strtok(description, "\n");
        int line_num = 0;

        while (line && line_num < max_lines)
        {
            SDL_Surface *line_surface = TTF_RenderText_Blended(fontsm, line, (SDL_Color){255, 255, 255, 255});
            SDL_Texture *line_texture = SDL_CreateTextureFromSurface(renderer, line_surface);
            SDL_Rect line_dst = {
                .x = box_x + 10,
                .y = box_y + 10 + line_num * line_height,
                .w = line_surface->w,
                .h = line_surface->h};
            SDL_RenderCopy(renderer, line_texture, NULL, &line_dst);
            SDL_DestroyTexture(line_texture);
            SDL_FreeSurface(line_surface);
            line = strtok(NULL, "\n");
            line_num++;
        }
        // =========== End Displays the effect description ===========

        SDL_RenderPresent(renderer);
        usleep(50000);
    }
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
