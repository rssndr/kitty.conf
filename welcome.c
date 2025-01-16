#include <stdio.h>
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

void render_frames(float A, float B, const char* date_str) {
    // Output buffer for the frame
    char output[width * height];
    memset(output, ' ', width * height);
    
    // Z buffer used to handle depth
    float z_buffer[width * height];
    for (int i = 0; i < width * height; i++) {
        z_buffer[i] = -1e9;
    }

    // First, put the welcome message into the output buffer
    const char welcome[] = "Welcome!";
    for (int i = 0; i < strlen(welcome); i++) {
        output[i] = welcome[i];
    }
    
    // Add newlines and the date string
    int date_offset = 2 * width;  // Move down 2 lines
    for (int i = 0; i < strlen(date_str); i++) {
        output[date_offset + i] = date_str[i];
    }

    // Light source direction
    float light_dir_x = 1;
    float light_dir_y = 1;
    float light_dir_z = 0.5;
    normalize(&light_dir_x, &light_dir_y, &light_dir_z);

    // Loop over theta and phi
    for (float theta = 0; theta < 2 * M_PI; theta += theta_spacing) {
        for (float phi = 0; phi < 2 * M_PI; phi += phi_spacing) {
            // Calculate the 3D coordinates
            float x = (R2 + R1 * cos(theta)) * cos(phi);
            float y = (R2 + R1 * cos(theta)) * sin(phi);
            float z = R1 * sin(theta);
            
            // Apply rotations
            float x_prime, z_prime, x_double_prime, y_double_prime;
            rotate_y(x, y, z, A, &x_prime, &z_prime);
            rotate_z(x_prime, y, z_prime, B, &x_double_prime, &y_double_prime);

            // Project to 2D
            float screen_x, screen_y;
            project(x_double_prime, y_double_prime, z_prime, &screen_x, &screen_y);
            
            // Convert to integer screen coordinates
            int sx = (int)screen_x;
            int sy = (int)screen_y + 4;  // Offset for welcome message space

            // Calculate surface normal in world coordinates
            float nx = cos(phi) * cos(theta);
            float ny = sin(phi) * cos(theta);
            float nz = sin(theta);
            
            // Rotate normal to match the torus orientation
            float nx_rot, ny_rot, nz_rot;
            rotate_y(nx, ny, nz, A, &nx_rot, &nz_rot);
            rotate_z(nx_rot, ny, nz_rot, B, &nx_rot, &ny_rot);

            // Calculate luminance
            float luminance = dot_product(nx_rot, ny_rot, nz_rot, light_dir_x, light_dir_y, light_dir_z);
            
            // Map luminance to characters
            const char* lum_chars = ".,:;+=xX$&";
            int lum_index = (int)((luminance + 1) * 4.99);  // Map luminance [-1, 1] to [0, 9]

            // Ensure the coordinates are within screen bounds
            if (sx >= 0 && sx < width && sy >= 0 && sy < height) {
                int buffer_index = sy * width + sx;
                if (z_prime > z_buffer[buffer_index]) {
                    z_buffer[buffer_index] = z_prime;
                    output[buffer_index] = lum_chars[lum_index];
                }
            }
        }
    }

    // Move cursor to home position and print the entire frame at once
    printf("\x1B[H");
    
    // Print the entire buffer with appropriate color codes
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int idx = i * width + j;
            // If we're in the welcome message area (first 6 lines)
            if (i < 4) {
                printf("\033[97m%c\033[0m", output[idx]);
            } else {
                printf("%c", output[idx]);
            }
        }
        printf("\n");
    }
}

int main() {
    // Set up signal handler to show cursor on exit
    signal(SIGINT, handle_exit);
    signal(SIGTERM, handle_exit);

    // Hide the cursor
    printf("\x1B[?25l");
    fflush(stdout);
    
    // Get current time
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date_str[64];
    strftime(date_str, sizeof(date_str), "%A, %B %d, %Y | %H:%M", tm);

    float A = 0;
    float B = 0;

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
        // Render everything in one go
        render_frames(A, B, date_str);
        fflush(stdout);
        
        A += 0.04;
        B += 0.02;
        
        // Non-blocking check for keypress
        if (read(STDIN_FILENO, &c, 1) > 0) {
            break;
        }
        
        usleep(30000);  // 30ms delay
    }

    // Restore terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
    
    // Show cursor and clear screen
    printf("\x1B[?25h");
    printf("\x1B[2J\x1B[H");
    fflush(stdout);

    return 0;
}

