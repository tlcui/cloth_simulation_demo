#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "cloth.h"

#include <iostream>

#define ASSERT(x) if(!(x)) __debugbreak();
#define GLCALL(x) gl_clear_error();\
    x;\
    ASSERT(gl_log_error(#x, __FILE__, __LINE__))

static void gl_clear_error()
{
    while (glGetError() != GL_NO_ERROR);
}

static bool gl_log_error(const char* function, const char* file, int line)
{
    while (GLenum errorcode = glGetError())
    {
        std::string error("");
        switch (errorcode)
        {
        case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
        case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
        case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
        //case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
        //case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
        case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        default:break;
        }
        std::cout << "opengl error (" << errorcode << "): " << error << ", " << function << ", " << file << ": " << line << std::endl;
        return false;
    }
    return true;
}
//code above is used for debugging

static constexpr int n = 128;
static constexpr float quad_size = 1.0 / n;
static constexpr float dt = 4e-2 / n;
static constexpr int substeps = static_cast<int>(1.0 / 60 / dt);

static constexpr int ball_number = 5;
static constexpr float ball_radius = 0.5 / ball_number;
static constexpr int ball_mesh_resolution_x = 60;
static constexpr int ball_mesh_resolution_y = 60;

const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aColor;\n"
"layout (location = 2) in vec3 aNormal;\n"
"out vec3 objectColor;\n"
"out vec3 Normal;\n"
"out vec3 FragPos;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"void main()\n"
"{\n"
"   FragPos = vec3(model * vec4(aPos, 1.0f));\n"
"   gl_Position = projection * view * model * vec4(aPos, 1.0f);\n"
"   objectColor = aColor;\n"
"   Normal = aNormal;\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"in vec3 Normal;\n"
"in vec3 FragPos;\n"
"in vec3 objectColor;\n"
"uniform vec3 lightPos;\n"
"uniform vec3 viewPos;\n"
"uniform vec3 lightColor;\n"
"void main()\n"
"{\n"
"    float ambientStrength = 0.1;\n"
"    vec3 ambient = ambientStrength * lightColor;\n"
"    vec3 norm = normalize(Normal);\n"
"    vec3 offset = lightPos - FragPos;\n"
"    float distance_square = dot(offset, offset);\n"
"    vec3 lightDir = normalize(offset);\n"
"    float diff = max(dot(norm, lightDir), 0.0);\n"
"    vec3 diffuse = diff * lightColor;\n"
"    float specularStrength = 0.5;\n"
"    vec3 viewDir = normalize(viewPos - FragPos);\n"
"    vec3 halfwayDir = normalize(lightDir + viewDir);\n"
"    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32);\n"
"    vec3 specular = specularStrength * spec * lightColor;\n"
"    vec3 result = 6*(ambient + diffuse/distance_square + specular/distance_square) * objectColor;\n"
"    FragColor = vec4(result, 1.0);\n"
"}\n\0";

const char* ball_vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aNormal;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"out vec3 Normal;\n"
"out vec3 FragPos;\n"
"void main()\n"
"{\n"
"   FragPos = vec3(model * vec4(aPos, 1.0f));\n"
"   Normal = mat3(transpose(inverse(model))) * aNormal;\n"
"   gl_Position = projection * view * model * vec4(aPos, 1.0f);\n"
"}\0";

const char* ball_fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"in vec3 Normal;\n"
"in vec3 FragPos;\n"
"uniform vec3 lightPos;\n"
"uniform vec3 viewPos;\n"
"uniform vec3 objectColor;\n"
"uniform vec3 lightColor;\n"
"void main()\n"
"{\n"
"    float ambientStrength = 0.1;\n"
"    vec3 ambient = ambientStrength * lightColor;\n"
"    vec3 norm = normalize(Normal);\n"
"    vec3 offset = lightPos - FragPos;\n"
"    float distance_square = dot(offset, offset);\n"
"    vec3 lightDir = normalize(offset);\n"
"    float diff = max(dot(norm, lightDir), 0.0);\n"
"    vec3 diffuse = diff * lightColor;\n"
"    float specularStrength = 0.5;\n"
"    vec3 viewDir = normalize(viewPos - FragPos);\n"
"    vec3 halfwayDir = normalize(lightDir + viewDir);\n"
"    float spec = pow(max(dot(norm, halfwayDir), 0.0), 20);\n"
"    vec3 specular = specularStrength * spec * lightColor;\n"
"    vec3 result = 3*(ambient + diffuse/distance_square + specular/distance_square) * objectColor;\n"
"    FragColor = vec4(result, 1.0);\n"
"}\n\0";

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

// settings
static constexpr unsigned int SCR_WIDTH = 1024;
static constexpr unsigned int SCR_HEIGHT = 1024;

// build and compile our shader program
unsigned int generate_shader(const char* vertexShaderSource, const char* fragmentShaderSource)
{
    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

int main()
{
    Cloth<n, n> cloth(quad_size);
    cloth.initialize();

    Balls<ball_number> balls(ball_radius);
    balls.initialize();

    Cloth_mesh<n, n> mesh;
    mesh.update_vertices(cloth);

    Balls_mesh<ball_number,ball_mesh_resolution_x, ball_mesh_resolution_y> balls_mesh;
    balls_mesh.update_vertices(balls);

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "C++ cloth simulation", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    unsigned int shaderProgram = generate_shader(vertexShaderSource, fragmentShaderSource);
    unsigned int ball_shaderProgram = generate_shader(ball_vertexShaderSource, ball_fragmentShaderSource);

    // ---for cloth_mesh---
    unsigned int VBO, VAO, EBO; 
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*9*n*n, mesh.vertices, GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*6*(n-1)*(n-1), mesh.indices, GL_STREAM_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // normal vector attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // ---for balls_mesh---
    unsigned int VBO_balls, VAO_balls, EBO_balls;
    glGenVertexArrays(1, &VAO_balls);
    glGenBuffers(1, &VBO_balls);
    glGenBuffers(1, &EBO_balls);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO_balls);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_balls);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * ball_number * (ball_mesh_resolution_x+1) * (ball_mesh_resolution_y+1) * 6, balls_mesh.vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_balls);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * ball_number * 6 * ball_mesh_resolution_x* ball_mesh_resolution_y, balls_mesh.indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal vector attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    float current_t = 0.f;

    double last_time = glfwGetTime();;
    // render loop
    glEnable(GL_DEPTH_TEST);
    while (!glfwWindowShouldClose(window))
    {
        // show fps
        double current_time = glfwGetTime();
        double fps = 1 / (current_time - last_time);
        std::stringstream ss;
        ss << "C++ cloth simulation " <<  fps << " FPS";
        glfwSetWindowTitle(window, ss.str().c_str());
        last_time = current_time;

        // input
        processInput(window);
        
        glClearColor(0.f, 0.f, 0.f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (current_t > 1.5)
        {
            cloth.initialize();
            balls.initialize();
            balls_mesh.update_vertices(balls);
            glBindVertexArray(VAO_balls);
            glBindBuffer(GL_ARRAY_BUFFER, VBO_balls);
            glBufferData(GL_ARRAY_BUFFER, sizeof(float) * ball_number * (ball_mesh_resolution_x + 1) * (ball_mesh_resolution_y + 1) * 6, balls_mesh.vertices, GL_STATIC_DRAW);
            current_t = 0.f;
        }

        for (int i = 0; i < substeps; ++i)
        {
            substep(cloth, balls, dt);
            current_t += dt;
        }

        mesh.update_vertices(cloth);
        
        // ---render cloth---
        glUseProgram(shaderProgram);

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
        glm::mat4 projection = glm::mat4(1.0f);
        projection = glm::perspective(glm::radians(45.0f), static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT), 0.1f, 100.0f);
        view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),  
            glm::vec3(0.0f, 0.0f, 0.0f),           
            glm::vec3(0.0f, 1.0f, 0.0f));

        int model_loc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));
        int view_loc = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
        int project_loc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(project_loc, 1, GL_FALSE, glm::value_ptr(projection));

        int light_color_loc = glGetUniformLocation(shaderProgram, "lightColor");
        glUniform3f(light_color_loc, 1.0f, 1.0f, 1.0f);
        int light_pos_loc = glGetUniformLocation(shaderProgram, "lightPos");
        glUniform3f(light_pos_loc, 0.0f, 1.0f, 2.0f);
        int view_pos_loc = glGetUniformLocation(shaderProgram, "viewPos");
        glUniform3f(view_pos_loc, 0.0f, 0.0f, 3.0f);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 9 * n * n, mesh.vertices, GL_STREAM_DRAW);

        glDrawElements(GL_TRIANGLES, (n-1)*(n-1)*6, GL_UNSIGNED_INT, 0);

        // ---render balls---
        glUseProgram(ball_shaderProgram);

        model_loc = glGetUniformLocation(ball_shaderProgram, "model");
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));
        view_loc = glGetUniformLocation(ball_shaderProgram, "view");
        glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
        project_loc = glGetUniformLocation(ball_shaderProgram, "projection");
        glUniformMatrix4fv(project_loc, 1, GL_FALSE, glm::value_ptr(projection));

        int object_color_loc = glGetUniformLocation(ball_shaderProgram, "objectColor");
        glUniform3f(object_color_loc, 0.7f, 0.0f, 0.0f);
        light_color_loc = glGetUniformLocation(ball_shaderProgram, "lightColor");
        glUniform3f(light_color_loc, 1.0f, 1.0f, 1.0f);
        light_pos_loc = glGetUniformLocation(ball_shaderProgram, "lightPos");
        glUniform3f(light_pos_loc, 0.0f, 1.0f, 2.0f);
        view_pos_loc = glGetUniformLocation(ball_shaderProgram, "viewPos");
        glUniform3f(view_pos_loc, 0.0f, 0.0f, 3.0f);
        
        glBindVertexArray(VAO_balls);
        //glBindBuffer(GL_ARRAY_BUFFER, VBO_balls);
        //glBufferData(GL_ARRAY_BUFFER, sizeof(float) * ball_number * (ball_mesh_resolution_x + 1) * (ball_mesh_resolution_y + 1) * 3, balls_mesh.vertices, GL_STATIC_DRAW);
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_TRIANGLES, ball_number * 6 * ball_mesh_resolution_x * ball_mesh_resolution_y, GL_UNSIGNED_INT, 0);
 
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
