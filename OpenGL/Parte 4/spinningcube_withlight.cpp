// Copyright (C) 2021 Emilio J. Padrón
// Released as Free Software under the X11 License
// https://spdx.org/licenses/X11.html
//
// Strongly inspired by spinnycube.cpp in OpenGL Superbible
// https://github.com/openglsuperbible

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

// GLM library to deal with matrix operations
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::perspective
#include <glm/gtc/type_ptr.hpp>

#include "textfile.h"

// Images textures library
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" 

int gl_width = 640;
int gl_height = 480;

void glfw_window_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void render(double);

GLuint shader_program = 0; // shader program to set render pipeline
GLuint vao = 0; // Vertext Array Object to set input data
GLint model_location, view_location, proj_location, normal_matrix_location; // Uniforms for transformation matrices
GLint light_position_location, light_ambient_location, light_diffuse_location, light_specular_location; // Uniforms for light
GLint second_light_position_location, second_light_ambient_location, second_light_diffuse_location, second_light_specular_location; // Uniforms for second light 
GLint material_diffuse_location, material_specular_location, material_shininess_location; // Uniforms for material
GLint cam_pos_location;
GLuint texture[1]; 


// Shader names
const char *vertexFileName = "spinningcube_withlight_vs.glsl";
const char *fragmentFileName = "spinningcube_withlight_fs.glsl";

// Camera
glm::vec3 camera_pos(0.0f, 0.0f, 3.0f);

// Lighting
glm::vec3 light_pos(1.5f, 0.0f, 1.0f);
glm::vec3 light_ambient(0.2f, 0.2f, 0.2f);
glm::vec3 light_diffuse(0.5f, 0.5f, 0.5f);
glm::vec3 light_specular(1.0f, 0.0f, 0.0f);

// Second lighting
glm::vec3 second_light_pos(-1.5f, 0.5f, 0.5f);
glm::vec3 second_light_ambient(0.2f, 0.2f, 0.2f);
glm::vec3 second_light_diffuse(0.5f, 0.5f, 0.5f);
glm::vec3 second_light_specular(0.0f, 1.0f, 0.0f); 

// Material
const GLfloat material_shininess = 32.0f;

int main() {
  // start GL context and O/S window using the GLFW helper library
  if (!glfwInit()) {
    fprintf(stderr, "ERROR: could not start GLFW3\n");
    return 1;
  }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow* window = glfwCreateWindow(gl_width, gl_height, "My spinning cube", NULL, NULL);
  if (!window) {
    fprintf(stderr, "ERROR: could not open window with GLFW3\n");
    glfwTerminate();
    return 1;
  }
  glfwSetWindowSizeCallback(window, glfw_window_size_callback);
  glfwMakeContextCurrent(window);

  // start GLEW extension handler
  // glewExperimental = GL_TRUE;
  glewInit();

  // get version info
  const GLubyte* vendor = glGetString(GL_VENDOR); // get vendor string
  const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
  const GLubyte* glversion = glGetString(GL_VERSION); // version as a string
  const GLubyte* glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION); // version as a string
  printf("Vendor: %s\n", vendor);
  printf("Renderer: %s\n", renderer);
  printf("OpenGL version supported %s\n", glversion);
  printf("GLSL version supported %s\n", glslversion);
  printf("Starting viewport: (width: %d, height: %d)\n", gl_width, gl_height);

  // Enable Depth test: only draw onto a pixel if fragment closer to viewer
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS); // set a smaller value as "closer"

  // Vertex Shader
  char* vertex_shader = textFileRead(vertexFileName);

  // Fragment Shader
  char* fragment_shader = textFileRead(fragmentFileName);

  // Shaders compilation
  GLuint vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vertex_shader, NULL);
  free(vertex_shader);
  glCompileShader(vs);

  int  success;
  char infoLog[512];
  glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vs, 512, NULL, infoLog);
    printf("ERROR: Vertex Shader compilation failed!\n%s\n", infoLog);

    return(1);
  }

  GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &fragment_shader, NULL);
  free(fragment_shader);
  glCompileShader(fs);

  glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fs, 512, NULL, infoLog);
    printf("ERROR: Fragment Shader compilation failed!\n%s\n", infoLog);

    return(1);
  }

  // Create program, attach shaders to it and link it
  shader_program = glCreateProgram();
  glAttachShader(shader_program, fs);
  glAttachShader(shader_program, vs);
  glLinkProgram(shader_program);

  glValidateProgram(shader_program);
  glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
  if(!success) {
    glGetProgramInfoLog(shader_program, 512, NULL, infoLog);
    printf("ERROR: Shader Program linking failed!\n%s\n", infoLog);

    return(1);
  }

  // Release shader objects
  glDeleteShader(vs);
  glDeleteShader(fs);

  // Vertex Array Object
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  // Cube to be rendered
  //
  //          0        3
  //       7        4 <-- top-right-near
  // bottom
  // left
  // far ---> 1        2
  //       6        5
  //
  const GLfloat vertex_positions[] = {
    -0.25f, -0.25f, -0.25f,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f,// 1
    -0.25f,  0.25f, -0.25f,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,// 0
     0.25f, -0.25f, -0.25f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,// 2

     0.25f,  0.25f, -0.25f,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f,// 3
     0.25f, -0.25f, -0.25f,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,// 2
    -0.25f,  0.25f, -0.25f,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,// 0

     0.25f, -0.25f, -0.25f,  1.0f, 0.0f, 0.0f,   1.0f, 0.0f,// 2
     0.25f,  0.25f, -0.25f,  1.0f, 0.0f, 0.0f,   1.0f, 1.0f,// 3
     0.25f, -0.25f,  0.25f,  1.0f, 0.0f, 0.0f,   0.0f, 0.0f,// 5

     0.25f,  0.25f,  0.25f,  1.0f, 0.0f, 0.0f,   0.0f, 1.0f,// 4
     0.25f, -0.25f,  0.25f,  1.0f, 0.0f, 0.0f,   0.0f, 0.0f,// 5
     0.25f,  0.25f, -0.25f,  1.0f, 0.0f, 0.0f,   1.0f, 1.0f,// 3

     0.25f, -0.25f,  0.25f,  0.0f, 0.0f, 1.0f,   1.0f, 0.0f,// 5
     0.25f,  0.25f,  0.25f,  0.0f, 0.0f, 1.0f,   1.0f, 1.0f,// 4
    -0.25f, -0.25f,  0.25f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f,// 6

    -0.25f,  0.25f,  0.25f,  0.0f, 0.0f, 1.0f,   0.0f, 1.0f,// 7
    -0.25f, -0.25f,  0.25f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f,// 6
     0.25f,  0.25f,  0.25f,  0.0f, 0.0f, 1.0f,   1.0f, 1.0f,// 4

    -0.25f, -0.25f,  0.25f,  -1.0f, 0.0f, 0.0f,  1.0f, 0.0f,// 6
    -0.25f,  0.25f,  0.25f,  -1.0f, 0.0f, 0.0f,  1.0f, 1.0f,// 7
    -0.25f, -0.25f, -0.25f,  -1.0f, 0.0f, 0.0f,  0.0f, 0.0f,// 1

    -0.25f,  0.25f, -0.25f,  -1.0f, 0.0f, 0.0f,  0.0f, 1.0f,// 0
    -0.25f, -0.25f, -0.25f,  -1.0f, 0.0f, 0.0f,  0.0f, 0.0f,// 1
    -0.25f,  0.25f,  0.25f,  -1.0f, 0.0f, 0.0f,  1.0f, 1.0f,// 7

     0.25f, -0.25f, -0.25f,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f,// 2
     0.25f, -0.25f,  0.25f,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,// 5
    -0.25f, -0.25f, -0.25f,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,// 1

    -0.25f, -0.25f,  0.25f,  0.0f, -1.0f, 0.0f,  0.0f, 1.0f,// 6
    -0.25f, -0.25f, -0.25f,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,// 1
     0.25f, -0.25f,  0.25f,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,// 5

     0.25f,  0.25f,  0.25f,  0.0f, 1.0f, 0.0f,   1.0f, 0.0f,// 4
     0.25f,  0.25f, -0.25f,  0.0f, 1.0f, 0.0f,   1.0f, 1.0f,// 3
    -0.25f,  0.25f,  0.25f,  0.0f, 1.0f, 0.0f,   0.0f, 0.0f,// 7

    -0.25f,  0.25f, -0.25f,  0.0f, 1.0f, 0.0f,   0.0f, 1.0f,// 0
    -0.25f,  0.25f,  0.25f,  0.0f, 1.0f, 0.0f,   0.0f, 0.0f,// 7
     0.25f,  0.25f, -0.25f,  0.0f, 1.0f, 0.0f,   1.0f, 1.0f,// 3
     
    // Tetrahedron
    2.0f,  0.5f, -0.2887f,   0.0f, 0.0f, -1.0f,  1.0f, 0.0f,
    1.5f, -0.5f, -0.2887f,   0.0f, 0.0f, -1.0f,  0.0f, 1.0f,
    2.5f, -0.5f, -0.2887f,   0.0f, 0.0f, -1.0f,  0.0f, 0.0f,

    2.0f,  0.5f, -0.2887f,   0.8165f, 0.0f, 0.5774f,  1.0f, 0.0f,
    2.5f, -0.5f, -0.2887f,   0.8165f, 0.0f, 0.5774f,  0.0f, 1.0f,
    2.0f,  0.0f,  0.5774f,   0.8165f, 0.0f, 0.5774f,  0.0f, 0.0f,

    2.5f, -0.5f, -0.2887f,   -0.8165f, 0.0f, 0.5774f,  1.0f, 0.0f,
    1.5f, -0.5f, -0.2887f,   -0.8165f, 0.0f, 0.5774f,  0.0f, 1.0f,
    2.0f,  0.0f,  0.5774f,   -0.8165f, 0.0f, 0.5774f,  0.0f, 0.0f,

    1.5f, -0.5f, -0.2887f,   0.0f, 0.0f, -1.0f,  1.0f, 0.0f,
    2.0f,  0.5f, -0.2887f,   0.0f, 0.0f, -1.0f,  0.0f, 1.0f,
    2.0f,  0.0f,  0.5774f,   0.0f, 0.0f, -1.0f,  0.0f, 0.0f
  };

// Vertex Buffer Object (for vertex coordinates)
  GLuint vbo = 0;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_positions), vertex_positions, GL_STATIC_DRAW);

  // Vertex attributes
  // 0: vertex position (x, y, z)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) (0 * sizeof(float)));
  glEnableVertexAttribArray(0);

  // 1: vertex normals (x, y, z)
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) (3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  
  // 2: text coordenates (x,y)
 glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *) (6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  // Unbind vbo (it was conveniently registered by VertexAttribPointer)
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // Unbind vao
  glBindVertexArray(0);

  // Uniforms
  // - Model matrix
  // - View matrix
  // - Projection matrix
  // - Normal matrix: normal vectors from local to world coordinates
  // - Camera position
  // - Light data
  // - Material data
  model_location = glGetUniformLocation(shader_program, "model");
  view_location = glGetUniformLocation(shader_program, "view");
  proj_location = glGetUniformLocation(shader_program, "projection");
  normal_matrix_location = glGetUniformLocation(shader_program, "normal_to_world");
  
  light_position_location = glGetUniformLocation(shader_program, "light.position");
  light_ambient_location = glGetUniformLocation(shader_program, "light.ambient");
  light_diffuse_location = glGetUniformLocation(shader_program, "light.diffuse");
  light_specular_location = glGetUniformLocation(shader_program, "light.specular");

  second_light_position_location = glGetUniformLocation(shader_program, "second_light.position");
  second_light_ambient_location = glGetUniformLocation(shader_program, "second_light.ambient");
  second_light_diffuse_location = glGetUniformLocation(shader_program, "second_light.diffuse");
  second_light_specular_location = glGetUniformLocation(shader_program, "second_light.specular"); 

  material_diffuse_location = glGetUniformLocation(shader_program, "material.diffuse");
  material_specular_location = glGetUniformLocation(shader_program, "material.specular");
  material_shininess_location = glGetUniformLocation(shader_program, "material.shininess");

  cam_pos_location = glGetUniformLocation(shader_program, "view_pos");
  
  // Create texture object
  glGenTextures(1, texture);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glUniform1i(material_diffuse_location, 0);

  int width, height, nrChannels;
  stbi_set_flip_vertically_on_load(1);
  unsigned char *data = stbi_load("box.jpg", &width, &height, &nrChannels, 0);
  // Image from https://learnopengl.com/index.php?p=Lighting/Lighting-maps

  if (data) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
  } else {
    printf("Failed to load the texture\n");
  }
  stbi_image_free(data);

// Render loop
  while(!glfwWindowShouldClose(window)) {

    processInput(window);

    render(glfwGetTime());

    glfwSwapBuffers(window);

    glfwPollEvents();
  }

  glfwTerminate();

  return 0;
}

void render(double currentTime) {
  float f = (float)currentTime * 0.3f;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glViewport(0, 0, gl_width, gl_height);

  glUseProgram(shader_program);
  glBindVertexArray(vao);
  
  // Texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture[0]);

  glm::mat4 model_matrix, view_matrix, proj_matrix, normal_matrix;

  model_matrix = glm::mat4(1.f);
  view_matrix = glm::lookAt(                 camera_pos,  // pos
                            glm::vec3(0.0f, 0.0f, 0.0f),  // target
                            glm::vec3(0.0f, 1.0f, 0.0f)); // up

  
  glUniformMatrix4fv(view_location, 1, GL_FALSE, glm::value_ptr(view_matrix));

	
  // Moving cube
  model_matrix = glm::translate(glm::mat4(1.f), glm::vec3(0.0f, 0.0f, -1.5f));
  model_matrix = glm::translate(model_matrix, glm::vec3(sinf(2.1f * f) * 0.5f, cosf(1.7f * f) * 0.5f, sinf(1.3f * f) * cosf(1.5f * f) * 2.0f));
  model_matrix = glm::rotate(model_matrix, glm::radians((float)currentTime * 25.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  model_matrix = glm::rotate(model_matrix, glm::radians((float)currentTime * 41.0f), glm::vec3(1.0f, 0.0f, 0.0f));
  glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(model_matrix));
  //
  // Projection
  proj_matrix = glm::perspective(glm::radians(50.0f), (float) gl_width / (float) gl_height, 0.1f, 1000.0f);
  glUniformMatrix4fv(proj_location, 1, GL_FALSE, glm::value_ptr(proj_matrix));
  //
  // Normal matrix: normal vectors to world coordinates
  normal_matrix = glm::transpose(glm::inverse(glm::mat3(model_matrix)));
  glUniformMatrix3fv(normal_matrix_location, 1, GL_FALSE, glm::value_ptr(normal_matrix));

  glUniform3fv(light_position_location, 1, glm::value_ptr(light_pos));
  glUniform3fv(light_ambient_location, 1, glm::value_ptr(light_ambient));
  glUniform3fv(light_diffuse_location, 1, glm::value_ptr(light_diffuse));
  glUniform3fv(light_specular_location, 1, glm::value_ptr(light_specular));
  
  glUniform3fv(second_light_position_location, 1, glm::value_ptr(second_light_pos)); 
  glUniform3fv(second_light_ambient_location, 1, glm::value_ptr(second_light_ambient));
  glUniform3fv(second_light_diffuse_location, 1, glm::value_ptr(second_light_diffuse));
  glUniform3fv(second_light_specular_location, 1, glm::value_ptr(second_light_specular));

  glUniform1f(material_shininess_location, material_shininess);
  
  glUniform3fv(cam_pos_location, 1, glm::value_ptr(camera_pos));

  glDrawArrays(GL_TRIANGLES, 0, 72);

}

void processInput(GLFWwindow *window) {
  if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, 1);
}

// Callback function to track window size and update viewport
void glfw_window_size_callback(GLFWwindow* window, int width, int height) {
  gl_width = width;
  gl_height = height;
  printf("New viewport: (width: %d, height: %d)\n", width, height);
}
