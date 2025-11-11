#include "glad.h"
#include "glfw3.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "glm/glm/glm.hpp"
#include "glm/glm/gtc/matrix_transform.hpp"
#include "glm/glm/gtc/type_ptr.hpp"

#include "shader_m.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <random>

const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"uniform mat4 transform;\n"
"uniform vec3 uColor;\n"
"out vec3 vertexColor;\n"
"void main()\n"
"{\n"
"   gl_Position = transform * vec4(aPos, 1.0);\n"
"   vertexColor = uColor;\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"in vec3 vertexColor;\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(vertexColor, 1.0);\n"
"}\0";

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

struct Rectangle {
    glm::vec3 position;
    glm::vec3 color;
    float width;
    float height;
    bool isStationary;
};

struct RectangleVertex {
    std::vector<float> vertices;
};

// Generate a single rectangle
RectangleVertex generateRectangle(float width, float height) {
    RectangleVertex rect;
    
    float w = width / 2.0f;
    float h = height / 2.0f;
    
    // Triangle 1
    rect.vertices = {
        -w, -h, 0.0f,
         w, -h, 0.0f,
         w,  h, 0.0f
    };
    
    // Triangle 2
    rect.vertices.insert(rect.vertices.end(), {
        -w, -h, 0.0f,
         w,  h, 0.0f,
        -w,  h, 0.0f
    });
    
    return rect;
}

// Create all rectangles (stationary and moving)
std::vector<Rectangle> generateRectangles() {
    std::vector<Rectangle> rectangles;
    
    // 4 Stationary rectangles on ground with spacing - distinct colors
    std::vector<glm::vec3> stationaryColors = {
        glm::vec3(0.9f, 0.1f, 0.1f),  // Red
        glm::vec3(0.1f, 0.9f, 0.1f),  // Green
        glm::vec3(0.1f, 0.2f, 0.95f), // Blue
        glm::vec3(1.0f, 0.8f, 0.0f)   // Yellow
    };
    
    float spacing = 0.45f;
    for (int i = 0; i < 4; ++i) {
        Rectangle rect;
        rect.position = glm::vec3(-0.7f + i * spacing, -0.5f, 0.0f);
        rect.color = stationaryColors[i];
        rect.width = 0.12f;
        rect.height = 0.18f;
        rect.isStationary = true;
        rectangles.push_back(rect);
    }
    
    // 4 Moving rectangles (different colors) - come one by one, distinct colors
    std::vector<glm::vec3> movingColors = {
        glm::vec3(0.9f, 0.0f, 0.9f),  // Magenta
        glm::vec3(0.0f, 0.9f, 0.9f),  // Cyan
        glm::vec3(1.0f, 0.45f, 0.0f), // Orange
        glm::vec3(0.5f, 0.0f, 1.0f)   // Purple
    };
    
    for (int i = 0; i < 4; ++i) {
        Rectangle rect;
        rect.position = glm::vec3(-1.2f, 0.2f, 0.0f);
        rect.color = movingColors[i];
        rect.width = 0.12f;
        rect.height = 0.15f;
        rect.isStationary = false;
        rectangles.push_back(rect);
    }
    
    return rectangles;
}

// Update rectangle position based on time
glm::vec3 updateMovingRectanglePosition(const Rectangle& rect, float time, int rectIndex) {
    float cycleTime = 6.0f; // Total time for one complete cycle
    float delayBetweenRectangles = 1.5f; // Delay between each rectangle
    
    // Each rectangle starts with a delay
    float adjustedTime = time - (rectIndex * delayBetweenRectangles);
    
    // If time is negative, rectangle hasn't started yet
    if (adjustedTime < 0.0f) {
        return glm::vec3(-1.2f, 0.2f, 0.0f);
    }
    
    // Use modulo to repeat the cycle
    adjustedTime = fmod(adjustedTime, cycleTime);
    
    float baseY = 0.2f;
    
    // Horizontal movement: -1.2 to 1.2
    float x = -1.2f + (adjustedTime / cycleTime) * 2.4f;
    if (x > 1.2f) x = -1.2f;
    
    float y = baseY;
    
    // Jump over 4 stationary rectangles
    float jumpHeight = 0.35f;
    std::vector<float> stationaryPositions = {-0.7f, -0.25f, 0.2f, 0.65f};
    
    // Smooth jump using sine wave near stationary rectangles
    float jumpRadius = 0.12f;
    
    for (float stationaryX : stationaryPositions) {
        if (fabs(x - stationaryX) < jumpRadius) {
            float jumpFactor = (1.0f - fabs(x - stationaryX) / jumpRadius);
            y += jumpHeight * sin(jumpFactor * 3.14159f);
            break;
        }
    }
    
    return glm::vec3(x, y, 0.0f);
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "4 Moving Rectangles Jump Over 4 Stationary", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Compile shaders
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Generate rectangles
    std::vector<Rectangle> rectangles = generateRectangles();
    RectangleVertex rectTemplate = generateRectangle(0.12f, 0.18f);

    // Setup VAO and VBO (only positions, color supplied via uniform)
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, rectTemplate.vertices.size() * sizeof(float), rectTemplate.vertices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    int vertexCount = rectTemplate.vertices.size() / 3;

    // Get uniform locations once
    glUseProgram(shaderProgram);
    int transformLoc = glGetUniformLocation(shaderProgram, "transform");
    int colorLoc = glGetUniformLocation(shaderProgram, "uColor");

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        float time = (float)glfwGetTime();
        // dynamic background color (smoothly changing)
        float bgR = 0.15f + 0.35f * (0.5f + 0.5f * sin(time * 0.5f));
        float bgG = 0.12f + 0.35f * (0.5f + 0.5f * sin(time * 0.7f + 2.0f));
        float bgB = 0.2f  + 0.35f * (0.5f + 0.5f * sin(time * 0.9f + 4.0f));
        glClearColor(bgR, bgG, bgB, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        int movingRectIndex = 0;
        for (int i = 0; i < rectangles.size(); ++i) {
            const auto& rect = rectangles[i];
            glm::vec3 pos = rect.position;
            
            if (!rect.isStationary) {
                pos = updateMovingRectanglePosition(rect, time, movingRectIndex);
                movingRectIndex++;
            }

            glm::mat4 transform = glm::mat4(1.0f);
            transform = glm::translate(transform, pos);

            glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));

            // Set color per-rectangle using uniform
            glUniform3f(colorLoc, rect.color.x, rect.color.y, rect.color.z);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}
