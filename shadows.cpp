#include <iostream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <boost/filesystem.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>

#include "ShaderManager.h"
#include "TriangleBatch.h"
#include "Batch.h"
#include "GeometryFactory.h"
#include "MatrixStack.h"
#include "Frustum.h"
#include "TransformPipeline.h"
#include "Math3D.h"
#include "UniformManager.h"
#include "TextureManager.h"
#include "ModelLoader.h"

using namespace gliby;
using namespace Math3D;
using namespace std;

// window
GLFWwindow* window;
int window_w, window_h;
// shader stuff
ShaderManager* shaderManager;
GLuint shader;
// transformation stuff
Frame cameraFrame;
Frustum viewFrustum;
TransformPipeline transformPipeline;
MatrixStack modelViewMatrix;
MatrixStack projectionMatrix;
// texture
TextureManager* textureManager;
// uniform locations
UniformManager* uniformManager;

void setupContext(void){
    // general state
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DITHER);
    //glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    //glEnable(GL_DEPTH_CLAMP);

    // transform pipeline
    transformPipeline.setMatrixStacks(modelViewMatrix,projectionMatrix);
    viewFrustum.setPerspective(35.0f, float(window_w)/float(window_h), 1.0f, 5000.0f);
    projectionMatrix.loadMatrix(viewFrustum.getProjectionMatrix());
    modelViewMatrix.loadIdentity();

    // camera setup
    cameraFrame.setOrigin(0.0f, 1.0f, 0.0f);
    cameraFrame.lookAt(0.0f, 0.0f, 0.0f);

    // shader setup
    const char* searchPath[] = {"./shaders/","/home/ego/projects/personal/gliby/shaders/"};
    shaderManager = new ShaderManager(sizeof(searchPath)/sizeof(char*),searchPath);
    ShaderAttribute attrs[] = {{0,"vVertex"},{2,"vNormal"},{3,"vTexCoord"}};
    shader = shaderManager->buildShaderPair("diffuse_specular.vp","diffuse_specular.fp",sizeof(attrs)/sizeof(ShaderAttribute),attrs);
    const char* uniforms[] = {"mvpMatrix","mvMatrix","normalMatrix","lightPosition","ambientColor","diffuseColor","textureUnit","specularColor","shinyness"};
    uniformManager = new UniformManager(shader,sizeof(uniforms)/sizeof(char*),uniforms);

    // setup texture loader
    textureManager = new TextureManager();

    // setup geometry

    // setup textures
    /*const char* textures[] = {"textures/spnza_bricks_a_diff.tga"};
    textureManager.loadTextures(sizeof(textures)/sizeof(char*),textures,GL_TEXTURE_2D,GL_TEXTURE0);*/
}

void render(void){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // setup matrix
    Matrix44f mCamera;
    cameraFrame.getCameraMatrix(mCamera);
    modelViewMatrix.pushMatrix();
    modelViewMatrix.multMatrix(mCamera);

    // render
    glUseProgram(shader);
    glUniformMatrix4fv(uniformManager->get("mvpMatrix"),1,GL_FALSE,transformPipeline.getModelViewProjectionMatrix());
    glUniformMatrix4fv(uniformManager->get("mvMatrix"),1,GL_FALSE,transformPipeline.getModelViewMatrix());
    glUniformMatrix3fv(uniformManager->get("normalMatrix"),1,GL_FALSE,transformPipeline.getNormalMatrix());
    GLfloat lightPosition[] = {2.0f,2.0f,2.0f};
    glUniform3fv(uniformManager->get("lightPosition"),1,lightPosition);
    GLfloat ambientColor[] = {0.2f, 0.2f, 0.2f, 1.0f};
    glUniform4fv(uniformManager->get("ambientColor"),1,ambientColor);
    GLfloat diffuseColor[] = {0.7f, 0.7f, 0.7f, 1.0f};
    glUniform4fv(uniformManager->get("diffuseColor"),1,diffuseColor);
    GLfloat specularColor[] = {0.2f, 0.2f, 0.2f, 1.0f};
    glUniform4fv(uniformManager->get("specularColor"),1,specularColor);
    glUniform1i(uniformManager->get("textureUnit"),0);
    glUniform1f(uniformManager->get("shinyness"),0.1f);

    modelViewMatrix.popMatrix();
}

void keyCallback(GLFWwindow* window, int key, int scancode, int state, int mods){
    if(key == GLFW_KEY_ESCAPE && state == GLFW_RELEASE){
        glfwSetWindowShouldClose(window, true);
    }
    if(key == GLFW_KEY_SPACE && state == GLFW_RELEASE){
        // pass
    }
}

void resizeCallback(GLFWwindow* window, int width, int height){
    window_w = width;
    window_h = height;
    glViewport(0,0,window_w,window_h);
    viewFrustum.setPerspective(45.0f, float(window_w)/float(window_h),0.1f,5000.0f);
    projectionMatrix.loadMatrix(viewFrustum.getProjectionMatrix());
}

int main(int argc, char **argv){
    // force vsync on
    putenv((char*) "__GL_SYNC_TO_VBLANK=1");

    // init glfw
    if(!glfwInit()){
        std::cerr << "GLFW init failed" << std::endl;
        return -1;
    }

    // swap interval
    glfwSwapInterval(1);
    // set window open hints
    glfwWindowHint(GLFW_SAMPLES, 8);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // create glfw window
    window = glfwCreateWindow(800,600,"gltest",NULL,NULL);
    window_w = 800; window_h = 600;
    if(!window){
        std::cerr << "GLFW window creation failed" << std::endl;
        glfwTerminate();
        return -1;
    }
    // make the window's context current
    glfwMakeContextCurrent(window);

    // event handlers
    glfwSetKeyCallback(window,keyCallback);
    glfwSetWindowSizeCallback(window,resizeCallback);

    // init glew
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if(err != GLEW_OK){
        std::cerr << "Glew error: " << glewGetErrorString(err) << std::endl;
    }

    // setup context
    setupContext();

    // main loop
    while(!glfwWindowShouldClose(window)){
        render();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}
