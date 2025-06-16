// Autor: Uros Stanisavljevic RA 175/2021
// Opis: 3D scena gradilišta sa kulom koja se gradi sloj po sloj

#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <random>

#include <GL/glew.h> 
#include <GLFW/glfw3.h>

//GLM biblioteke
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct TowerLayer {
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 color;
};

unsigned int compileShader(GLenum type, const char* source);
unsigned int createShader(const char* vsSource, const char* fsSource);
void generateCubeVertices(float* vertices);
static unsigned loadImageToTexture(const char* filePath);

// Globalne varijable za kameru
float cameraAngle = 0.0f;
float cameraRadius = 15.0f;
glm::vec3 towerCenter = glm::vec3(0.0f, 2.0f, 0.0f);

// FPS varijable
const double TARGET_FPS = 60.0;
const double FRAME_TIME = 1.0 / TARGET_FPS;
double lastFrameTime = 0.0;

// Globalne varijable za kulu
std::vector<TowerLayer> towerLayers;
const int MAX_LAYERS = 10;
const float CUBE_SIZE = 1.0f;
std::mt19937 rng(std::random_device{}());

// Funkcija za računanje ukupne visine svih slojeva
float calculateTotalHeight() {
    float totalHeight = 0.0f;
    for (const auto& layer : towerLayers) {
        totalHeight += layer.scale.y * CUBE_SIZE;
    }
    return totalHeight;
}

// Funkcija za inicijalizaciju početnih kocki
void initializeTower() {
    for (int i = 0; i < 4; i++) {
        TowerLayer layer;
        std::uniform_real_distribution<float> sizeDist(0.7f, 1.2f);
        float size = sizeDist(rng);
        layer.scale = glm::vec3(size, size, size);

        float yPosition = 0.0f;
        for (int j = 0; j < i; j++) {
            yPosition += towerLayers[j].scale.y * CUBE_SIZE;
        }
        yPosition += (layer.scale.y * CUBE_SIZE) * 0.5f;

        layer.position = glm::vec3(0.0f, yPosition, 0.0f);

        std::uniform_real_distribution<float> colorDist(0.3f, 1.0f);
        layer.color = glm::vec3(colorDist(rng), colorDist(rng), colorDist(rng));

        towerLayers.push_back(layer);
    }
}

int main(void)
{
    if (!glfwInit())
    {
        std::cout << "GLFW Biblioteka se nije ucitala! :(\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window;
    unsigned int wWidth = 800;
    unsigned int wHeight = 600;
    const char wTitle[] = "3D Gradiliste";
    window = glfwCreateWindow(wWidth, wHeight, wTitle, NULL, NULL);

    if (window == NULL)
    {
        std::cout << "Prozor nije napravljen! :(\n";
        glfwTerminate();
        return 2;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
    {
        std::cout << "GLEW nije mogao da se ucita! :'(\n";
        return 3;
    }

    // Depth testing setup
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDepthRange(0.0, 1.0);

    // Face culling setup
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Inicijalizacija početnih 4 kocke
    initializeTower();

    // Shader kreiranje
    unsigned int unifiedShader = createShader("basic.vert", "basic.frag");
    unsigned int textureShader = createShader("texture.vert", "texture.frag");

    // Vertices za kocku
    float cubeVertices[108];
    generateCubeVertices(cubeVertices);

    // Vertices za tlo
    float groundVertices[] = {
        -10.0f, -0.5f, -10.0f,
        -10.0f, -0.5f,  10.0f,
         10.0f, -0.5f,  10.0f,
        -10.0f, -0.5f, -10.0f,
         10.0f, -0.5f,  10.0f,
         10.0f, -0.5f, -10.0f
    };

    unsigned int stride = 3 * sizeof(float);

    // VAO i VBO za kocku
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    // VAO i VBO za tlo
    unsigned int groundVAO, groundVBO;
    glGenVertexArrays(1, &groundVAO);
    glGenBuffers(1, &groundVBO);

    glBindVertexArray(groundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(groundVertices), groundVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    // Vertices za teksturu (ISPRAVKA: normalizovane koordinate)
    float textureVertices[] = {
        // Pozicija (x,y)    // Texture koordinate (u,v)
        -1.0f,  0.85f,       0.0f, 0.0f,  // gornji levi
        -1.0f,  1.0f,       0.0f, 1.0f,  // donji levi  
        -0.3f,  1.0f,       1.0f, 1.0f,  // donji desni

        -1.0f,  0.85f,       0.0f, 0.0f,  // gornji levi
        -0.3f,  1.0f,       1.0f, 1.0f,  // donji desni
        -0.3f,  0.85f,       1.0f, 0.0f   // gornji desni
    };

    unsigned int textureVAO, textureVBO;
    glGenVertexArrays(1, &textureVAO);
    glGenBuffers(1, &textureVBO);

    glBindVertexArray(textureVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textureVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(textureVertices), textureVertices, GL_STATIC_DRAW);

    // Pozicija (location = 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Texture koordinate (location = 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Učitavanje teksture
    unsigned int authorTexture = loadImageToTexture("author.png");
    if (authorTexture == 0) {
        std::cout << "GRESKA: Tekstura nije ucitana!" << std::endl;
    }
    else {
        std::cout << "Tekstura je uspesno ucitana!" << std::endl;
    }

    // Konfiguracija teksture
    glBindTexture(GL_TEXTURE_2D, authorTexture);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Uniform lokacije
    unsigned int modelLoc = glGetUniformLocation(unifiedShader, "uM");
    unsigned int viewLoc = glGetUniformLocation(unifiedShader, "uV");
    unsigned int projectionLoc = glGetUniformLocation(unifiedShader, "uP");
    unsigned int colorLoc = glGetUniformLocation(unifiedShader, "uColor");
    unsigned int texLoc = glGetUniformLocation(textureShader, "uTex");

    // Perspektivna projekcija
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)wWidth / (float)wHeight, 0.1f, 100.0f);

    glClearColor(0.7f, 0.9f, 1.0f, 1.0f);

    // Varijable za praćenje stanja tastera
    bool wPressed = false, sPressed = false;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    while (!glfwWindowShouldClose(window))
    {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastFrameTime;

        if (deltaTime < FRAME_TIME) {
            continue;
        }

        lastFrameTime = currentTime;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }

        // Dodavanje sloja (W taster)
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && !wPressed)
        {
            if (towerLayers.size() < MAX_LAYERS)
            {
                TowerLayer newLayer;
                std::uniform_real_distribution<float> sizeDist(0.7f, 1.2f);
                float size = sizeDist(rng);
                newLayer.scale = glm::vec3(size, size, size);

                float yPosition = 0.0f;
                for (const auto& existingLayer : towerLayers) {
                    yPosition += existingLayer.scale.y * CUBE_SIZE;
                }
                yPosition += (newLayer.scale.y * CUBE_SIZE) * 0.5f;

                newLayer.position = glm::vec3(0.0f, yPosition, 0.0f);

                std::uniform_real_distribution<float> colorDist(0.0f, 1.0f);
                newLayer.color = glm::vec3(colorDist(rng), colorDist(rng), colorDist(rng));

                towerLayers.push_back(newLayer);
                std::cout << "Dodat novi sloj! Ukupno slojeva: " << towerLayers.size() << std::endl;
            }
            wPressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_RELEASE) {
            wPressed = false;
        }

        // Uklanjanje sloja (S taster)
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && !sPressed)
        {
            if (!towerLayers.empty() && towerLayers.size() > 4) {
                towerLayers.pop_back();
                std::cout << "Uklonjen sloj! Ukupno slojeva: " << towerLayers.size() << std::endl;
            }
            sPressed = true;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE) {
            sPressed = false;
        }

        // Rotacija kamere (A i D tasteri)
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            cameraAngle += 0.02f;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            cameraAngle -= 0.02f;
        }

        // Pozicija kamere
        glm::vec3 cameraPos;
        cameraPos.x = towerCenter.x + cameraRadius * cos(cameraAngle);
        cameraPos.z = towerCenter.z + cameraRadius * sin(cameraAngle);
        cameraPos.y = towerCenter.y + 5.0f;

        // View matrica
        glm::mat4 view = glm::lookAt(cameraPos, towerCenter, glm::vec3(0.0f, 1.0f, 0.0f));

        // Čišćenje buffer-a
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // === CRTANJE 3D SCENE ===
        glUseProgram(unifiedShader);
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        // Crtanje tla
        glm::mat4 groundModel = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(groundModel));
        glUniform4f(colorLoc, 0.6f, 0.6f, 0.6f, 1.0f);
        glBindVertexArray(groundVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Crtanje slojeva kule
        glBindVertexArray(cubeVAO);
        for (const auto& layer : towerLayers)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, layer.position);
            model = glm::scale(model, layer.scale);

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform4f(colorLoc, layer.color.r, layer.color.g, layer.color.b, 1.0f);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // === CRTANJE 2D TEKSTURE === (zameniti deo koda za crtanje teksture)
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        // Koristiti texture shader
        glUseProgram(textureShader);

        // Postaviti teksturu
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, authorTexture);
        glUniform1i(texLoc, 0); // Bind texture unit 0

        // Crtanje teksture
        glBindVertexArray(textureVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Ponovo omogućiti depth test za 3D objekte
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &groundVBO);
    glDeleteBuffers(1, &textureVBO);
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &groundVAO);
    glDeleteVertexArrays(1, &textureVAO);
    glDeleteTextures(1, &authorTexture);
    glDeleteProgram(unifiedShader);
    glDeleteProgram(textureShader);

    glfwTerminate();
    return 0;
}

void generateCubeVertices(float* vertices)
{
    float tempVertices[] = {
        // Prednja strana (z = 0.5)
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,

        // Zadnja strana (z = -0.5)
         0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,

         // Lijeva strana (x = -0.5)
         -0.5f, -0.5f, -0.5f,
         -0.5f, -0.5f,  0.5f,
         -0.5f,  0.5f,  0.5f,
         -0.5f, -0.5f, -0.5f,
         -0.5f,  0.5f,  0.5f,
         -0.5f,  0.5f, -0.5f,

         // Desna strana (x = 0.5)
          0.5f, -0.5f,  0.5f,
          0.5f, -0.5f, -0.5f,
          0.5f,  0.5f, -0.5f,
          0.5f, -0.5f,  0.5f,
          0.5f,  0.5f, -0.5f,
          0.5f,  0.5f,  0.5f,

          // Donja strana (y = -0.5)
          -0.5f, -0.5f, -0.5f,
           0.5f, -0.5f, -0.5f,
           0.5f, -0.5f,  0.5f,
          -0.5f, -0.5f, -0.5f,
           0.5f, -0.5f,  0.5f,
          -0.5f, -0.5f,  0.5f,

          // Gornja strana (y = 0.5)
          -0.5f,  0.5f,  0.5f,
           0.5f,  0.5f,  0.5f,
           0.5f,  0.5f, -0.5f,
          -0.5f,  0.5f,  0.5f,
           0.5f,  0.5f, -0.5f,
          -0.5f,  0.5f, -0.5f
    };

    for (int i = 0; i < 108; i++) {
        vertices[i] = tempVertices[i];
    }
}

unsigned int compileShader(GLenum type, const char* source)
{
    std::string content = "";
    std::ifstream file(source);
    std::stringstream ss;
    if (file.is_open())
    {
        ss << file.rdbuf();
        file.close();
        std::cout << "Uspješno pročitao fajl sa putanje \"" << source << "\"!" << std::endl;
    }
    else {
        ss << "";
        std::cout << "Greška pri čitanju fajla sa putanje \"" << source << "\"!" << std::endl;
    }
    std::string temp = ss.str();
    const char* sourceCode = temp.c_str();

    int shader = glCreateShader(type);

    int success;
    char infoLog[512];
    glShaderSource(shader, 1, &sourceCode, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        if (type == GL_VERTEX_SHADER)
            printf("VERTEX");
        else if (type == GL_FRAGMENT_SHADER)
            printf("FRAGMENT");
        printf(" shader ima grešku! Greška: \n");
        printf(infoLog);
    }
    return shader;
}

unsigned int createShader(const char* vsSource, const char* fsSource)
{
    unsigned int program;
    unsigned int vertexShader;
    unsigned int fragmentShader;

    program = glCreateProgram();

    vertexShader = compileShader(GL_VERTEX_SHADER, vsSource);
    fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSource);

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);
    glValidateProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success);
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(program, 512, NULL, infoLog);
        std::cout << "Objedinjeni shader ima grešku! Greška: \n";
        std::cout << infoLog << std::endl;
    }

    glDetachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    return program;
}

static unsigned loadImageToTexture(const char* filePath) {
    int TextureWidth;
    int TextureHeight;
    int TextureChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* ImageData = stbi_load(filePath, &TextureWidth, &TextureHeight, &TextureChannels, 0);
    if (ImageData != NULL)
    {
        GLint InternalFormat = -1;
        switch (TextureChannels) {
        case 1: InternalFormat = GL_RED; break;
        case 2: InternalFormat = GL_RG; break;
        case 3: InternalFormat = GL_RGB; break;
        case 4: InternalFormat = GL_RGBA; break;
        default: InternalFormat = GL_RGB; break;
        }

        unsigned int Texture;
        glGenTextures(1, &Texture);
        glBindTexture(GL_TEXTURE_2D, Texture);
        glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, TextureWidth, TextureHeight, 0, InternalFormat, GL_UNSIGNED_BYTE, ImageData);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(ImageData);
        return Texture;
    }
    else
    {
        std::cout << "Tekstura nije ucitana! Putanja texture: " << filePath << std::endl;
        stbi_image_free(ImageData);
        return 0;
    }
}