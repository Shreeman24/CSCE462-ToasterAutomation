
#define OpenGL3 true

#include "imgui.h"
#include "imgui_impl_glfw.h"
#if OpenGL3
#include "imgui_impl_opengl3.h"
#else
#include "imgui_impl_opengl2.h"
#endif
#include <GLFW/glfw3.h>
#include<stdio.h>
#include <pigpio.h>

#define SERVO_PIN 18

#define TEMP_CS_PIN 10
#define TEMP_CLK_PIN 11
#define TEMP_DATA_PIN 9
#define SETTLE_TIME 100
#define SERVO_OFF 700
#define SERVO_ON 2000

#define uint unsigned int
const uint WINDOW_WIDTH = 500;
const uint WINDOW_HEIGHT = 500;
GLFWwindow *window;

void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main() {
	printf("Hello Sailor!\n");
	
	if (gpioInitialise() < 0) {
        printf("Failed to initialize pigpio library\n");
        return -1;
    }
    // Set servo pin as output
    gpioSetMode(SERVO_PIN, PI_OUTPUT);

    // Set MAX6675 pins as input
    gpioSetMode(TEMP_CS_PIN, PI_INPUT);
    gpioSetMode(TEMP_CLK_PIN, PI_INPUT);
    gpioSetMode(TEMP_DATA_PIN, PI_INPUT);
    
	//GLFW setup
	glfwSetErrorCallback(glfw_error_callback);
	if(!glfwInit()) {
		return -1;
	}
	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Application", NULL, NULL);
    if (window == NULL)
        return 1;
	glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
	glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
	
	//ImGUI setup
	IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
#if OpenGL3
    ImGui_ImplOpenGL3_Init();
#else
    ImGui_ImplOpenGL2_Init();
#endif
    
    float startTime = 0.0;
    int last_second = int(ImGui::GetTime());
    int cur_temp;
    int cook_temp_slider = 80;
    const int low_temp_bound = 50;
    const int high_temp_bound = 150;
    gpioServo(SERVO_PIN, SERVO_OFF); // return to 0 degrees
    
	while(glfwWindowShouldClose(window) == 0) {
        
		glfwPollEvents();
		glClear(GL_COLOR_BUFFER_BIT);
		
		if (last_second != int(ImGui::GetTime())) {
            last_second = int(ImGui::GetTime());
            
            // Read temperature from MAX6675
            gpioWrite(TEMP_CS_PIN, 0); // Assert chip select
            gpioDelay(SETTLE_TIME); // Wait for chip select to settle
            int temperature = 0;

            for (int i = 11; i >= 0; i--) {
                gpioWrite(TEMP_CLK_PIN, 1); // Raise clock
                gpioDelay(SETTLE_TIME); // Wait for clock to settle
                temperature |= (gpioRead(TEMP_DATA_PIN) << i); // Read data                
                gpioWrite(TEMP_CLK_PIN, 0); // Lower clock
                gpioDelay(SETTLE_TIME); // Wait for clock to settle
            }
            gpioWrite(TEMP_CS_PIN, 1); // Deassert chip select
            
            // Convert temperature to Celsius
            cur_temp = int(temperature * 0.5);
		}
        
#if OpenGL3        
        ImGui_ImplOpenGL3_NewFrame();
#else
        ImGui_ImplOpenGL2_NewFrame();
#endif
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        {
            static int cook_temp_text = 80;
            static bool done = false;
            static float start_time_delta = 0;
            if (!done && startTime != 0)
                start_time_delta = ImGui::GetTime() - startTime;
                
            ImGui::Begin("Controls");
            
            ImGui::Text("This controls the temperature the bread will be cooked to.");
            ImGui::SliderInt("Temp Slider", &cook_temp_slider, low_temp_bound, high_temp_bound);
            if (ImGui::IsItemDeactivated())
                cook_temp_text = cook_temp_slider;
            ImGui::InputInt("Temp Input", &cook_temp_text);
            if (ImGui::IsItemDeactivated()) {
                if (cook_temp_text < low_temp_bound)
                    cook_temp_text = low_temp_bound;
                if (cook_temp_text > high_temp_bound)
                    cook_temp_text = high_temp_bound;
                cook_temp_slider = cook_temp_text;
            }
            ImGui::BeginTable("Temperature and Time", 2);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Temperature (C)");
            ImGui::TableNextColumn();
            ImGui::Text("Seconds Elapsed");
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%d", cur_temp);
            ImGui::TableNextColumn();
            ImGui::Text("%d", int(start_time_delta));
            ImGui::EndTable();
            
            // Check if temperature reaches threshold
            if (cur_temp >= cook_temp_slider && done == false) {
                done = true;
                printf("Exceeded Threshold\n");
                gpioServo(SERVO_PIN, SERVO_OFF); // return to 0 degrees
            }
            if (done) {
                ImGui::Text("Toast Complete");
                if (ImGui::Button("Reset")) {
                    startTime = 0;
                    start_time_delta = 0;
                    done = false;
                }
            } else {
                if (startTime == 0) {
                    if (ImGui::Button("START TOAST!")) {
                        gpioServo(SERVO_PIN, SERVO_ON);
                        startTime = ImGui::GetTime();
                    }
                } else {
                    ImGui::Text("Toasting...");
                }
            }
            
            ImGui::End();
        }
        
        ImGui::Render();
#if OpenGL3
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#else
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
#endif  //*/
		glFlush();
		glfwSwapBuffers(window);
	}
	//ImGUI cleanup
#if OpenGL3
    ImGui_ImplOpenGL3_Shutdown();
#else
	ImGui_ImplOpenGL2_Shutdown();
#endif
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
	
	//GLFW cleanup
    glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
