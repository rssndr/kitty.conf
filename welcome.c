/*#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <termios.h>

const int width = 80;
const int height = 42;
const float theta_spacing = 0.07;
const float phi_spacing = 0.02;
const float R1 = 0.8;
const float R2 = 2;
const float K2 = -8;
const float K1 = width * K2 * 3 / (8 * (R1 + R2));
const int screen_center_x = width / 2;
const int screen_center_y = height / 2;
const float y_scale = 0.5;  // Aspect ratio correction

// Color cycling parameters
const float color_speed = 0.02;  // Speed of color change

// HSV to RGB conversion helper functions
void hsv_to_rgb(float h, float s, float v, int* r, int* g, int* b) {
    float c = v * s;
    float x = c * (1 - fabs(fmod(h / 60.0, 2) - 1));
    float m = v - c;
    
    float r_tmp, g_tmp, b_tmp;
    
    if(h >= 0 && h < 60) {
        r_tmp = c; g_tmp = x; b_tmp = 0;
    } else if(h >= 60 && h < 120) {
        r_tmp = x; g_tmp = c; b_tmp = 0;
    } else if(h >= 120 && h < 180) {
        r_tmp = 0; g_tmp = c; b_tmp = x;
    } else if(h >= 180 && h < 240) {
        r_tmp = 0; g_tmp = x; b_tmp = c;
    } else if(h >= 240 && h < 300) {
        r_tmp = x; g_tmp = 0; b_tmp = c;
    } else {
        r_tmp = c; g_tmp = 0; b_tmp = x;
    }
    
    *r = (int)((r_tmp + m) * 255);
    *g = (int)((g_tmp + m) * 255);
    *b = (int)((b_tmp + m) * 255);
}

void rotate_y(float x, float y, float z, float A, float* x_prime, float* z_prime) {
    *x_prime = x * cos(A) + z * sin(A);
    *z_prime = -x * sin(A) + z * cos(A);
}

void rotate_z(float x_prime, float y, float z_prime, float B, float* x_double_prime, float* y_double_prime) {
    *x_double_prime = x_prime * cos(B) - y * sin(B);
    *y_double_prime = x_prime * sin(B) + y * cos(B);
}

void project(float x, float y, float z, float* screen_x, float* screen_y) {
    *screen_x = screen_center_x + K1 * x / (z + K2);
    *screen_y = screen_center_y - K1 * y / (z + K2) * y_scale;
}

void handle_exit(int signal) {
    printf("\x1B[?25h");  // Show the cursor
    exit(signal);
}

float dot_product(float ax, float ay, float az, float bx, float by, float bz) {
    return ax * bx + ay * by + az * bz;
}

void normalize(float* x, float* y, float* z) {
    float length = sqrt((*x) * (*x) + (*y) * (*y) + (*z) * (*z));
    *x /= length;
    *y /= length;
    *z /= length;
}

void render_frames(float A, float B, const char* date_str, float color_angle) {
    // Output buffer for the frame
    char output[width * height];
    memset(output, ' ', width * height);
    float luminance_buffer[width * height];
    float z_buffer[width * height];
    
    for (int i = 0; i < width * height; i++) {
        z_buffer[i] = -1e9;
        luminance_buffer[i] = -1;
    }

    // First, put the welcome message into the output buffer
    const char welcome[] = "Welcome!";
    int welcome_start = (width - strlen(welcome)) / 2;  // Calculate center position
    for (int i = 0; i < strlen(welcome); i++) {
        output[welcome_start + i] = welcome[i];
    }
    
    // Add newlines and the date string
    int date_start = (width - strlen(date_str)) / 2;  // Calculate center position
    int date_offset = 2 * width;  // Keep two lines down
    for (int i = 0; i < strlen(date_str); i++) {
        output[date_offset + date_start + i] = date_str[i];
    }

    // Light source direction
    float light_dir_x = 1;
    float light_dir_y = 1;
    float light_dir_z = 0.5;
    normalize(&light_dir_x, &light_dir_y, &light_dir_z);

    // Loop over theta and phi
    for (float theta = 0; theta < 2 * M_PI; theta += theta_spacing) {
        for (float phi = 0; phi < 2 * M_PI; phi += phi_spacing) {
            float x = (R2 + R1 * cos(theta)) * cos(phi);
            float y = (R2 + R1 * cos(theta)) * sin(phi);
            float z = R1 * sin(theta);
            
            float x_prime, z_prime, x_double_prime, y_double_prime;
            rotate_y(x, y, z, A, &x_prime, &z_prime);
            rotate_z(x_prime, y, z_prime, B, &x_double_prime, &y_double_prime);

            float screen_x, screen_y;
            project(x_double_prime, y_double_prime, z_prime, &screen_x, &screen_y);
            
            int sx = (int)screen_x;
            int sy = (int)screen_y + 4;

            float nx = cos(phi) * cos(theta);
            float ny = sin(phi) * cos(theta);
            float nz = sin(theta);
            
            float nx_rot, ny_rot, nz_rot;
            rotate_y(nx, ny, nz, A, &nx_rot, &nz_rot);
            rotate_z(nx_rot, ny, nz_rot, B, &nx_rot, &ny_rot);

            float luminance = dot_product(nx_rot, ny_rot, nz_rot, light_dir_x, light_dir_y, light_dir_z);
            
            const char* lum_chars = ".,:;+=xX$&";
            int lum_index = (int)((luminance + 1) * 4.99);

            if (sx >= 0 && sx < width && sy >= 0 && sy < height) {
                int buffer_index = sy * width + sx;
                if (z_prime > z_buffer[buffer_index]) {
                    z_buffer[buffer_index] = z_prime;
                    output[buffer_index] = lum_chars[lum_index];
                    luminance_buffer[buffer_index] = luminance;
                }
            }
        }
    }

    printf("\x1B[H");
    
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int idx = i * width + j;
            if (i < 4) {
                printf("\033[97m%c\033[0m", output[idx]);
            } else if (output[idx] != ' ') {
                // Convert color angle to RGB
                int r, g, b;
                float luminance = luminance_buffer[idx];
                // Adjust saturation and value based on luminance
                float saturation = 0.8 + luminance * 0.2;  // Higher luminance = more saturated
                float value = 0.5 + luminance * 0.5;       // Higher luminance = brighter
                hsv_to_rgb(color_angle, saturation, value, &r, &g, &b);
                
                // Print colored character
                printf("\033[38;2;%d;%d;%dm%c\033[0m", r, g, b, output[idx]);
            } else {
                printf("%c", output[idx]);
            }
        }
        printf("\n");
    }
}

int main() {
    signal(SIGINT, handle_exit);
    signal(SIGTERM, handle_exit);

    printf("\x1B[?25l");
    fflush(stdout);
    
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date_str[64];
    strftime(date_str, sizeof(date_str), "%A, %B %d, %Y | %H:%M", tm);

    float A = 0;
    float B = 0;
    float color_angle = 0;

    struct termios old_tio, new_tio;
    unsigned char c;
    
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;
    new_tio.c_lflag &= (~ICANON & ~ECHO);
    new_tio.c_cc[VMIN] = 0;
    new_tio.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

    while (1) {
        render_frames(A, B, date_str, color_angle);
        fflush(stdout);
        
        A += 0.04;
        B += 0.02;
        color_angle = fmod(color_angle + color_speed * 360, 360);  // Cycle through hue
        
        if (read(STDIN_FILENO, &c, 1) > 0) {
            break;
        }
        
        usleep(30000);
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
    
    printf("\x1B[?25h");
    printf("\x1B[2J\x1B[H");
    fflush(stdout);

    return 0;
}*/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <termios.h>
#include <sys/ioctl.h>

// Fixed dimensions for the torus display
const int TORUS_WIDTH = 80;
const int TORUS_HEIGHT = 42;
// Terminal dimensions (will be detected)
int term_width;
int term_height;

const float theta_spacing = 0.07;
const float phi_spacing = 0.02;
const float R1 = 0.8;
const float R2 = 2;
const float K2 = -8;
const float K1 = TORUS_WIDTH * K2 * 3 / (8 * (R1 + R2));
const int screen_center_x = TORUS_WIDTH / 2;
const int screen_center_y = TORUS_HEIGHT / 2;
const float y_scale = 0.5;  // Aspect ratio correction

// Color cycling parameters
const float color_speed = 0.02;  // Speed of color change

void update_terminal_size() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    term_width = w.ws_col;
    term_height = w.ws_row;
}

void hsv_to_rgb(float h, float s, float v, int* r, int* g, int* b) {
    float c = v * s;
    float x = c * (1 - fabs(fmod(h / 60.0, 2) - 1));
    float m = v - c;
    
    float r_tmp, g_tmp, b_tmp;
    
    if(h >= 0 && h < 60) {
        r_tmp = c; g_tmp = x; b_tmp = 0;
    } else if(h >= 60 && h < 120) {
        r_tmp = x; g_tmp = c; b_tmp = 0;
    } else if(h >= 120 && h < 180) {
        r_tmp = 0; g_tmp = c; b_tmp = x;
    } else if(h >= 180 && h < 240) {
        r_tmp = 0; g_tmp = x; b_tmp = c;
    } else if(h >= 240 && h < 300) {
        r_tmp = x; g_tmp = 0; b_tmp = c;
    } else {
        r_tmp = c; g_tmp = 0; b_tmp = x;
    }
    
    *r = (int)((r_tmp + m) * 255);
    *g = (int)((g_tmp + m) * 255);
    *b = (int)((b_tmp + m) * 255);
}

void rotate_y(float x, float y, float z, float A, float* x_prime, float* z_prime) {
    *x_prime = x * cos(A) + z * sin(A);
    *z_prime = -x * sin(A) + z * cos(A);
}

void rotate_z(float x_prime, float y, float z_prime, float B, float* x_double_prime, float* y_double_prime) {
    *x_double_prime = x_prime * cos(B) - y * sin(B);
    *y_double_prime = x_prime * sin(B) + y * cos(B);
}

void project(float x, float y, float z, float* screen_x, float* screen_y) {
    *screen_x = screen_center_x + K1 * x / (z + K2);
    *screen_y = screen_center_y - K1 * y / (z + K2) * y_scale;
}

void handle_exit(int signal) {
    printf("\x1B[?25h");  // Show the cursor
    exit(signal);
}

float dot_product(float ax, float ay, float az, float bx, float by, float bz) {
    return ax * bx + ay * by + az * bz;
}

void normalize(float* x, float* y, float* z) {
    float length = sqrt((*x) * (*x) + (*y) * (*y) + (*z) * (*z));
    *x /= length;
    *y /= length;
    *z /= length;
}

void render_frames(float A, float B, const char* date_str, float color_angle) {
    // Update terminal size
    update_terminal_size();
    
    // Calculate centering offsets
    int x_offset = (term_width - TORUS_WIDTH) / 2;
    int y_offset = (term_height - TORUS_HEIGHT) / 2;
    
    if (x_offset < 0) x_offset = 0;
    if (y_offset < 0) y_offset = 0;

    // Allocate buffers for the fixed-size display
    char* output = (char*)malloc(TORUS_WIDTH * TORUS_HEIGHT * sizeof(char));
    float* luminance_buffer = (float*)malloc(TORUS_WIDTH * TORUS_HEIGHT * sizeof(float));
    float* z_buffer = (float*)malloc(TORUS_WIDTH * TORUS_HEIGHT * sizeof(float));
    
    memset(output, ' ', TORUS_WIDTH * TORUS_HEIGHT);
    
    for (int i = 0; i < TORUS_WIDTH * TORUS_HEIGHT; i++) {
        z_buffer[i] = -1e9;
        luminance_buffer[i] = -1;
    }

    // Center welcome message
    const char welcome[] = "Welcome!";
    int welcome_start = (TORUS_WIDTH - strlen(welcome)) / 2;
    for (int i = 0; i < strlen(welcome); i++) {
        output[welcome_start + i] = welcome[i];
    }
    
    // Center date string
    int date_start = (TORUS_WIDTH - strlen(date_str)) / 2;
    int date_offset = 2 * TORUS_WIDTH;
    for (int i = 0; i < strlen(date_str); i++) {
        output[date_offset + date_start + i] = date_str[i];
    }

    // Light source direction
    float light_dir_x = 1;
    float light_dir_y = 1;
    float light_dir_z = 0.5;
    normalize(&light_dir_x, &light_dir_y, &light_dir_z);

    // Torus calculation loop
    for (float theta = 0; theta < 2 * M_PI; theta += theta_spacing) {
        for (float phi = 0; phi < 2 * M_PI; phi += phi_spacing) {
            float x = (R2 + R1 * cos(theta)) * cos(phi);
            float y = (R2 + R1 * cos(theta)) * sin(phi);
            float z = R1 * sin(theta);
            
            float x_prime, z_prime, x_double_prime, y_double_prime;
            rotate_y(x, y, z, A, &x_prime, &z_prime);
            rotate_z(x_prime, y, z_prime, B, &x_double_prime, &y_double_prime);

            float screen_x, screen_y;
            project(x_double_prime, y_double_prime, z_prime, &screen_x, &screen_y);
            
            int sx = (int)screen_x;
            int sy = (int)screen_y + 4;  // Offset from text

            float nx = cos(phi) * cos(theta);
            float ny = sin(phi) * cos(theta);
            float nz = sin(theta);
            
            float nx_rot, ny_rot, nz_rot;
            rotate_y(nx, ny, nz, A, &nx_rot, &nz_rot);
            rotate_z(nx_rot, ny, nz_rot, B, &nx_rot, &ny_rot);

            float luminance = dot_product(nx_rot, ny_rot, nz_rot, light_dir_x, light_dir_y, light_dir_z);
            
            const char* lum_chars = ".,:;+=xX$&";
            int lum_index = (int)((luminance + 1) * 4.99);

            if (sx >= 0 && sx < TORUS_WIDTH && sy >= 0 && sy < TORUS_HEIGHT) {
                int buffer_index = sy * TORUS_WIDTH + sx;
                if (z_prime > z_buffer[buffer_index]) {
                    z_buffer[buffer_index] = z_prime;
                    output[buffer_index] = lum_chars[lum_index];
                    luminance_buffer[buffer_index] = luminance;
                }
            }
        }
    }

    // Clear screen and move to home position
    printf("\x1B[H");
    
    // Print top padding
    for (int i = 0; i < y_offset; i++) {
        printf("%*s\n", term_width, "");
    }
    
    // Print the torus display with centering
    for (int i = 0; i < TORUS_HEIGHT; i++) {
        // Print left padding
        printf("%*s", x_offset, "");
        
        for (int j = 0; j < TORUS_WIDTH; j++) {
            int idx = i * TORUS_WIDTH + j;
            if (i < 4) {  // Welcome text area
                printf("\033[97m%c\033[0m", output[idx]);
            } else if (output[idx] != ' ') {  // Torus
                int r, g, b;
                float luminance = luminance_buffer[idx];
                float saturation = 0.8 + luminance * 0.2;
                float value = 0.5 + luminance * 0.5;
                hsv_to_rgb(color_angle, saturation, value, &r, &g, &b);
                printf("\033[38;2;%d;%d;%dm%c\033[0m", r, g, b, output[idx]);
            } else {  // Empty space
                printf("%c", output[idx]);
            }
        }
        printf("\n");
    }

    free(output);
    free(luminance_buffer);
    free(z_buffer);
}

int main() {
    signal(SIGINT, handle_exit);
    signal(SIGTERM, handle_exit);

    // Get initial terminal size
    update_terminal_size();

    // Hide cursor
    printf("\x1B[?25l");
    fflush(stdout);
    
    // Get current time
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date_str[64];
    strftime(date_str, sizeof(date_str), "%A, %B %d, %Y | %H:%M", tm);

    float A = 0;
    float B = 0;
    float color_angle = 0;

    // Set up non-blocking input
    struct termios old_tio, new_tio;
    unsigned char c;
    
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;
    new_tio.c_lflag &= (~ICANON & ~ECHO);
    new_tio.c_cc[VMIN] = 0;
    new_tio.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

    // Animation loop
    while (1) {
        render_frames(A, B, date_str, color_angle);
        fflush(stdout);
        
        A += 0.04;
        B += 0.02;
        color_angle = fmod(color_angle + color_speed * 360, 360);
        
        if (read(STDIN_FILENO, &c, 1) > 0) {
            break;
        }
        
        usleep(30000);  // 30ms delay
    }

    // Cleanup
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
    
    printf("\x1B[?25h");  // Show cursor
    printf("\x1B[2J\x1B[H");  // Clear screen
    fflush(stdout);

    return 0;
}
